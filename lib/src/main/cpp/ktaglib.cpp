
#include <jni.h>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <android/log.h>

#include <fileref.h>
#include <flacfile.h>
#include <opusfile.h>
#include <xiphcomment.h>
#include <tstring.h>
#include <tstringlist.h>
#include <toolkit/tiostream.h>
#include <toolkit/tfilestream.h>
#include <toolkit/tmap.h>
#include <toolkit/tpropertymap.h>
#include <toolkit/tdebuglistener.h>

// Configure verbose logging for detailed diagnostics
// Set to 1 to enable detailed property/value logging (useful for debugging)
// Set to 0 for production to reduce logging overhead
#define KTAGLIB_ENABLE_VERBOSE_LOGGING 0

// Custom IOStream wrapper that delegates to FileStream but provides a filename hint
// This allows FileRef to use extension-based detection while still using file descriptors for I/O
class FileStreamWithName : public TagLib::IOStream {
public:
    FileStreamWithName(int fileDescriptor, const TagLib::String &fileName, bool readOnly = true)
        : m_stream(fileDescriptor, readOnly), m_name(fileName) {
    }

    TagLib::FileName name() const override {
        return m_name.toCString(true);
    }

    TagLib::ByteVector readBlock(size_t length) override {
        return m_stream.readBlock(length);
    }

    void writeBlock(const TagLib::ByteVector &data) override {
        m_stream.writeBlock(data);
    }

    void insert(const TagLib::ByteVector &data, TagLib::offset_t start = 0, size_t replace = 0) override {
        m_stream.insert(data, start, replace);
    }

    void removeBlock(TagLib::offset_t start = 0, size_t length = 0) override {
        m_stream.removeBlock(start, length);
    }

    bool readOnly() const override {
        return m_stream.readOnly();
    }

    bool isOpen() const override {
        return m_stream.isOpen();
    }

    void seek(TagLib::offset_t offset, Position p = Beginning) override {
        m_stream.seek(offset, p);
    }

    void clear() override {
        m_stream.clear();
    }

    TagLib::offset_t tell() const override {
        return m_stream.tell();
    }

    TagLib::offset_t length() override {
        return m_stream.length();
    }

    void truncate(TagLib::offset_t length) override {
        m_stream.truncate(length);
    }

private:
    TagLib::FileStream m_stream;
    TagLib::String m_name;
};

jclass globalMetadataClass;
jmethodID metadataInit;

jclass globalAudioPropertiesClass;
jmethodID audioPropertiesInit;

jclass globalHashMapClass;
jmethodID hashMapInit;

jclass globalMapEntryClass;
jmethodID getPropertyKey;
jmethodID getPropertyValue;
jmethodID addProperty;
jmethodID getEntrySet;

jclass globalSetClass;
jclass globalIteratorClass;
jmethodID getIterator;
jmethodID iteratorHasNext;
jmethodID iteratorNextEntry;

jclass globalArrayListClass;
jmethodID arrayListInit;
jmethodID addListElement;
jmethodID getListElement;
jmethodID getListSize;


class DebugListener : public TagLib::DebugListener {
    void printMessage(const TagLib::String &msg) override {
        __android_log_print(ANDROID_LOG_VERBOSE, "kTagLib", "%s", msg.toCString(true));
    }
};

DebugListener listener;

// Returns the data of the largest picture in a FLAC::Picture list, or an empty ByteVector if the list
// is empty.
static TagLib::ByteVector largestPicture(const TagLib::List<TagLib::FLAC::Picture *> &picList) {
    TagLib::ByteVector largest;
    size_t largestSize = 0;
    for (auto picture : picList) {
        const TagLib::ByteVector &data = picture->data();
        if (data.size() > largestSize) {
            largest = data;
            largestSize = data.size();
        }
    }
    return largest;
}

// Converts a Java string to a TagLib::String through its UTF-16 code units.
//
// GetStringUTFChars returns *modified* UTF-8, which encodes characters outside the BMP (emoji, for
// example) as a pair of 3-byte surrogates that standard UTF-8 decoders reject, and a bare
// TagLib::String(const char *) is decoded as Latin-1. Either way non-ASCII text gets corrupted on
// write, so hand TagLib the UTF-16 code units, which it stores as-is.
static TagLib::String toTagLibString(JNIEnv *env, jstring str) {
    if (str == nullptr) {
        return TagLib::String();
    }
    const jsize length = env->GetStringLength(str);
    const jchar *chars = env->GetStringChars(str, nullptr);
    if (chars == nullptr) {
        return TagLib::String();
    }
    TagLib::ByteVector bytes(static_cast<unsigned int>(length) * 2, 0);
    for (jsize i = 0; i < length; i++) {
        bytes[i * 2] = static_cast<char>(chars[i] & 0xFF);
        bytes[i * 2 + 1] = static_cast<char>((chars[i] >> 8) & 0xFF);
    }
    env->ReleaseStringChars(str, chars);
    return TagLib::String(bytes, TagLib::String::UTF16LE);
}

// Converts a TagLib::String to a Java string through its UTF-16 code units.
//
// NewStringUTF expects modified UTF-8, so the standard 4-byte UTF-8 that toCString(true) produces
// for characters outside the BMP is not valid input for it (CheckJNI aborts on it).
static jstring toJString(JNIEnv *env, const TagLib::String &str) {
    const TagLib::ByteVector bytes = str.data(TagLib::String::UTF16LE);
    const size_t length = bytes.size() / 2;
    std::vector<jchar> chars(length);
    for (size_t i = 0; i < length; i++) {
        chars[i] = static_cast<jchar>(
                static_cast<unsigned char>(bytes[i * 2]) |
                (static_cast<unsigned char>(bytes[i * 2 + 1]) << 8));
    }
    return env->NewString(chars.data(), static_cast<jsize>(length));
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jclass metadataClass = env->FindClass("com/simplecityapps/ktaglib/Metadata");
    globalMetadataClass = reinterpret_cast<jclass>(env->NewGlobalRef(metadataClass));
    env->DeleteLocalRef(metadataClass);
    metadataInit = env->GetMethodID(globalMetadataClass, "<init>", "(Ljava/util/Map;Lcom/simplecityapps/ktaglib/AudioProperties;)V");

    jclass audioPropertiesClass = env->FindClass("com/simplecityapps/ktaglib/AudioProperties");
    globalAudioPropertiesClass = reinterpret_cast<jclass>(env->NewGlobalRef(audioPropertiesClass));
    env->DeleteLocalRef(audioPropertiesClass);
    audioPropertiesInit = env->GetMethodID(globalAudioPropertiesClass, "<init>", "(IIII)V");

    jclass setClass = env->FindClass("java/util/Set");
    globalSetClass = reinterpret_cast<jclass>(env->NewGlobalRef(setClass));
    env->DeleteLocalRef(setClass);

    jclass iteratorClass = env->FindClass("java/util/Iterator");
    globalIteratorClass = reinterpret_cast<jclass>(env->NewGlobalRef(iteratorClass));
    env->DeleteLocalRef(iteratorClass);

    getIterator = env->GetMethodID(globalSetClass, "iterator", "()Ljava/util/Iterator;");
    iteratorHasNext = env->GetMethodID(globalIteratorClass, "hasNext", "()Z");
    iteratorNextEntry = env->GetMethodID(globalIteratorClass, "next", "()Ljava/lang/Object;");

    jclass hashMapClass = env->FindClass("java/util/HashMap");
    globalHashMapClass = reinterpret_cast<jclass>(env->NewGlobalRef(hashMapClass));
    env->DeleteLocalRef(hashMapClass);
    hashMapInit = env->GetMethodID(globalHashMapClass, "<init>", "()V");
    addProperty = env->GetMethodID(globalHashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    getEntrySet = env->GetMethodID(globalHashMapClass, "entrySet", "()Ljava/util/Set;");

    jclass mapEntryClass = env->FindClass("java/util/Map$Entry");
    globalMapEntryClass = reinterpret_cast<jclass>(env->NewGlobalRef(mapEntryClass));
    env->DeleteLocalRef(mapEntryClass);
    getPropertyKey = env->GetMethodID(globalMapEntryClass, "getKey", "()Ljava/lang/Object;");
    getPropertyValue = env->GetMethodID(globalMapEntryClass, "getValue", "()Ljava/lang/Object;");

    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    globalArrayListClass = reinterpret_cast<jclass>(env->NewGlobalRef(arrayListClass));
    env->DeleteLocalRef(arrayListClass);
    arrayListInit = env->GetMethodID(globalArrayListClass, "<init>", "(I)V");
    addListElement = env->GetMethodID(globalArrayListClass, "add", "(Ljava/lang/Object;)Z");
    getListElement = env->GetMethodID(globalArrayListClass, "get", "(I)Ljava/lang/Object;");
    getListSize = env->GetMethodID(globalArrayListClass, "size", "()I");

    TagLib::setDebugListener(&listener);;

    return JNI_VERSION_1_6;
}

extern "C" void JNI_OnUnload(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);

    env->DeleteGlobalRef(globalMetadataClass);
    env->DeleteGlobalRef(globalAudioPropertiesClass);
    env->DeleteGlobalRef(globalHashMapClass);
    env->DeleteGlobalRef(globalMapEntryClass);
    env->DeleteGlobalRef(globalIteratorClass);
    env->DeleteGlobalRef(globalArrayListClass);

    TagLib::setDebugListener(nullptr);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_simplecityapps_ktaglib_KTagLib_getMetadata(JNIEnv *env, jclass clazz, jint file_descriptor, jstring filename) {

#if KTAGLIB_ENABLE_VERBOSE_LOGGING
    // Log function entry with file descriptor
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "getMetadata: Opening file descriptor %d", file_descriptor);
#endif

    // Create stream - use custom wrapper if filename provided, otherwise use standard FileStream
    std::unique_ptr<TagLib::IOStream> stream;
    if (filename != nullptr) {
        const TagLib::String filenameStr = toTagLibString(env, filename);
#if KTAGLIB_ENABLE_VERBOSE_LOGGING
        __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "Filename hint provided: %s", filenameStr.toCString(true));
#endif
        stream = std::make_unique<FileStreamWithName>(file_descriptor, filenameStr, true);
    } else {
#if KTAGLIB_ENABLE_VERBOSE_LOGGING
        __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "No filename hint provided");
#endif
        stream = std::make_unique<TagLib::FileStream>(file_descriptor, true);
    }

#if KTAGLIB_ENABLE_VERBOSE_LOGGING
    // Log stream creation and name (if available)
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "Stream created: %s", stream->name());
#endif

    TagLib::FileRef fileRef(stream.get());

    // Check if FileRef was created successfully (file type detection)
    if (fileRef.isNull()) {
        __android_log_print(ANDROID_LOG_WARN, "kTagLib",
            "FileRef is null - file type not recognized or file corrupt");
        return nullptr;
    }
#if KTAGLIB_ENABLE_VERBOSE_LOGGING
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "FileRef created successfully - file type recognized");
#endif

    jobject jPropertyMap = env->NewObject(globalHashMapClass, hashMapInit);

    // A recognized file without a tag (e.g. a bare WAV with no ID3) still has audio properties
    // worth returning, so only skip property extraction here rather than the whole result -
    // TagLib::File::properties() dereferences tag() unconditionally, so it is not safe to call
    // when the tag is null.
    if (!fileRef.tag()) {
        __android_log_print(ANDROID_LOG_WARN, "kTagLib",
            "Tag is null - file has no readable tag, returning audio properties only");
    } else {
#if KTAGLIB_ENABLE_VERBOSE_LOGGING
        __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "Tag found in file");
#endif

        auto taglibProperties = fileRef.properties();

#if KTAGLIB_ENABLE_VERBOSE_LOGGING
        // Log property count
        __android_log_print(ANDROID_LOG_DEBUG, "kTagLib",
            "Extracted %lu properties from file", (unsigned long)taglibProperties.size());
#endif

        if (taglibProperties.isEmpty()) {
            __android_log_print(ANDROID_LOG_WARN, "kTagLib",
                "Property map is empty - file has tag but no readable fields");
        }

        for (auto &taglibProperty : taglibProperties) {
            // Convert property key once and reuse
            jstring key = toJString(env, taglibProperty.first);

#if KTAGLIB_ENABLE_VERBOSE_LOGGING
            // Log each property key and value count
            __android_log_print(ANDROID_LOG_VERBOSE, "kTagLib",
                "Property: %s = [%lu values]",
                taglibProperty.first.toCString(true),
                (unsigned long)taglibProperty.second.size());
#endif

            jobject values = env->NewObject(globalArrayListClass, arrayListInit, (jint) 0);
            for (auto &value : taglibProperty.second) {
#if KTAGLIB_ENABLE_VERBOSE_LOGGING
                // Log individual values
                __android_log_print(ANDROID_LOG_VERBOSE, "kTagLib",
                    "  Value: '%s'", value.toCString(true));
#endif
                jstring jValue = toJString(env, value);
                env->CallBooleanMethod(values, addListElement, jValue);
                env->DeleteLocalRef(jValue);
            }
            jobject previous = env->CallObjectMethod(jPropertyMap, addProperty, key, values);
            env->DeleteLocalRef(previous);
            env->DeleteLocalRef(values);
            env->DeleteLocalRef(key);
        }
    }

    jobject jAudioProperties = nullptr;
    auto audioProperties = fileRef.audioProperties();
    if (audioProperties != nullptr) {
#if KTAGLIB_ENABLE_VERBOSE_LOGGING
        // Log audio properties
        __android_log_print(ANDROID_LOG_DEBUG, "kTagLib",
            "Audio properties: duration=%dms, bitrate=%dkbps, sampleRate=%dHz, channels=%d",
            audioProperties->lengthInMilliseconds(),
            audioProperties->bitrate(),
            audioProperties->sampleRate(),
            audioProperties->channels());
#endif

        jAudioProperties = env->NewObject(
                globalAudioPropertiesClass,
                audioPropertiesInit,
                (jint) audioProperties->lengthInMilliseconds(),
                (jint) audioProperties->bitrate(),
                (jint) audioProperties->sampleRate(),
                (jint) audioProperties->channels()
        );
    } else {
        __android_log_print(ANDROID_LOG_WARN, "kTagLib", "Audio properties not available");
    }

#if KTAGLIB_ENABLE_VERBOSE_LOGGING
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "Successfully created Metadata object");
#endif
    return env->NewObject(globalMetadataClass, metadataInit, jPropertyMap, jAudioProperties);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_simplecityapps_ktaglib_KTagLib_writeMetadata(JNIEnv *env, jclass clazz, jint file_descriptor, jobject properties, jstring filename) {

    // Create stream - use custom wrapper if filename provided, otherwise use standard FileStream
    std::unique_ptr<TagLib::IOStream> stream;
    if (filename != nullptr) {
        stream = std::make_unique<FileStreamWithName>(file_descriptor, toTagLibString(env, filename), false);
    } else {
        stream = std::make_unique<TagLib::FileStream>(file_descriptor, false);
    }

    TagLib::FileRef fileRef(stream.get(), false);

    jboolean isSuccessful = false;

    if (!fileRef.isNull() && fileRef.tag()) {
        TagLib::PropertyMap taglibProperties = fileRef.properties();
        jobject entrySet = env->CallObjectMethod(properties, getEntrySet);
        jobject iterator = env->CallObjectMethod(entrySet, getIterator);

        while (env->CallBooleanMethod(iterator, iteratorHasNext)) {
            jobject entry = env->CallObjectMethod(iterator, iteratorNextEntry);
            auto key = (jstring) env->CallObjectMethod(entry, getPropertyKey);
            jobject values = env->CallObjectMethod(entry, getPropertyValue);

            // A null key has no valid tag field to write to - skip it instead of writing it
            // under an empty-string key.
            if (key == nullptr) {
                __android_log_print(ANDROID_LOG_WARN, "kTagLib",
                    "writeMetadata: skipping property with a null key");
                env->DeleteLocalRef(values);
                env->DeleteLocalRef(entry);
                continue;
            }

            jint len = env->CallIntMethod(values, getListSize);
            TagLib::StringList stringList;
            for (jint i = 0; i < len; i++) {
                auto element = (jstring) env->CallObjectMethod(values, getListElement, i);
                if (element != nullptr) {
                    stringList.append(toTagLibString(env, element));
                    env->DeleteLocalRef(element);
                }
            }
            taglibProperties.replace(toTagLibString(env, key), stringList);
            env->DeleteLocalRef(values);
            env->DeleteLocalRef(key);
            env->DeleteLocalRef(entry);
        }
        env->DeleteLocalRef(iterator);
        env->DeleteLocalRef(entrySet);

        fileRef.setProperties(taglibProperties);
        isSuccessful = fileRef.save();
    }

    return isSuccessful;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_simplecityapps_ktaglib_KTagLib_getArtwork(JNIEnv *env, jclass clazz, jint file_descriptor, jstring filename) {

    // Create stream - use custom wrapper if filename provided, otherwise use standard FileStream
    std::unique_ptr<TagLib::IOStream> stream;
    if (filename != nullptr) {
        stream = std::make_unique<FileStreamWithName>(file_descriptor, toTagLibString(env, filename), true);
    } else {
        stream = std::make_unique<TagLib::FileStream>(file_descriptor, true);
    }

    TagLib::FileRef fileRef(stream.get(), false);

    jbyteArray result = nullptr;

    if (!fileRef.isNull()) {
        TagLib::ByteVector byteVector;

        if (auto *flacFile = dynamic_cast<TagLib::FLAC::File *>(fileRef.file())) {
            byteVector = largestPicture(flacFile->pictureList());
        } else if (auto *opusFile = dynamic_cast<TagLib::Ogg::Opus::File *>(fileRef.file())) {
            TagLib::Ogg::XiphComment *tag = opusFile->tag();
            if (tag != nullptr) {
                byteVector = largestPicture(tag->pictureList());
            }
        } else {
            TagLib::Tag *tag = fileRef.tag();
            if (tag != nullptr) {
                TagLib::List<TagLib::VariantMap> pictureMap = tag->complexProperties("PICTURE");
                if (!pictureMap.isEmpty()) {
                    // Finds the largest picture by byte size
                    size_t picSize = 0;
                    for (auto const &property: pictureMap) {
                        for (auto const &[key, value]: property) {
                            if (value.type() == TagLib::Variant::ByteVector) {
                                auto i = value.value<TagLib::ByteVector>();
                                size_t size = i.size();
                                if (size > picSize) {
                                    byteVector = i;
                                    picSize = size;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!byteVector.isEmpty()) {
            size_t len = byteVector.size();
            if (len > 0) {
                jbyteArray arr = env->NewByteArray(len);
                char *data = byteVector.data();
                env->SetByteArrayRegion(arr, 0, len, reinterpret_cast<jbyte *>(data));
                result = arr;
            }
        }
    }

    return result;
}