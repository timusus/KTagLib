
#include <jni.h>
#include <string>
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
#define KTAGLIB_ENABLE_VERBOSE_LOGGING 1

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

static const char *convertJStringToCString(JNIEnv *env, jstring str) {
    return env->GetStringUTFChars(str, JNI_FALSE);
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

    // Log function entry with file descriptor
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "getMetadata: Opening file descriptor %d", file_descriptor);

    // Convert filename from Java string (may be null)
    const char* filenameStr = nullptr;
    if (filename != nullptr) {
        filenameStr = env->GetStringUTFChars(filename, nullptr);
        __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "Filename hint provided: %s", filenameStr);
    } else {
        __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "No filename hint provided");
    }

    // Create stream - use custom wrapper if filename provided, otherwise use standard FileStream
    std::unique_ptr<TagLib::IOStream> stream;
    if (filenameStr != nullptr) {
        stream = std::make_unique<FileStreamWithName>(file_descriptor, TagLib::String(filenameStr), true);
        env->ReleaseStringUTFChars(filename, filenameStr);
    } else {
        stream = std::make_unique<TagLib::FileStream>(file_descriptor, true);
    }

    // Log stream creation and name (if available)
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "Stream created: %s", stream->name());

    TagLib::FileRef fileRef(stream.get());

    // Check if FileRef was created successfully (file type detection)
    if (fileRef.isNull()) {
        __android_log_print(ANDROID_LOG_WARN, "kTagLib",
            "FileRef is null - file type not recognized or file corrupt");
        return nullptr;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "FileRef created successfully - file type recognized");

    // Check if tag data exists
    if (!fileRef.tag()) {
        __android_log_print(ANDROID_LOG_WARN, "kTagLib",
            "Tag is null - no metadata found in file");
        return nullptr;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "Tag found in file");

    jobject jPropertyMap = env->NewObject(globalHashMapClass, hashMapInit);

    auto taglibProperties = fileRef.properties();

    // Log property count
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib",
        "Extracted %lu properties from file", (unsigned long)taglibProperties.size());

    if (taglibProperties.isEmpty()) {
        __android_log_print(ANDROID_LOG_WARN, "kTagLib",
            "Property map is empty - file has tag but no readable fields");
    }

    for (auto &taglibProperty : taglibProperties) {
        // Convert property key once and reuse
        const char* keyStr = taglibProperty.first.toCString(true);
        jstring key = env->NewStringUTF(keyStr);

#if KTAGLIB_ENABLE_VERBOSE_LOGGING
        // Log each property key and value count
        __android_log_print(ANDROID_LOG_VERBOSE, "kTagLib",
            "Property: %s = [%lu values]",
            keyStr,
            (unsigned long)taglibProperty.second.size());
#endif

        jobject values = env->NewObject(globalArrayListClass, arrayListInit, (jint) 0);
        for (auto &value : taglibProperty.second) {
#if KTAGLIB_ENABLE_VERBOSE_LOGGING
            // Log individual values
            __android_log_print(ANDROID_LOG_VERBOSE, "kTagLib",
                "  Value: '%s'", value.toCString(true));
#endif
            env->CallBooleanMethod(values, addListElement, env->NewStringUTF(value.toCString(true)));
        }
        env->CallObjectMethod(jPropertyMap, addProperty, key, values);
    }

    jobject jAudioProperties = nullptr;
    auto audioProperties = fileRef.audioProperties();
    if (audioProperties != nullptr) {
        // Log audio properties
        __android_log_print(ANDROID_LOG_DEBUG, "kTagLib",
            "Audio properties: duration=%dms, bitrate=%dkbps, sampleRate=%dHz, channels=%d",
            audioProperties->lengthInMilliseconds(),
            audioProperties->bitrate(),
            audioProperties->sampleRate(),
            audioProperties->channels());

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

    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "Successfully created Metadata object");
    return env->NewObject(globalMetadataClass, metadataInit, jPropertyMap, jAudioProperties);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_simplecityapps_ktaglib_KTagLib_writeMetadata(JNIEnv *env, jclass clazz, jint file_descriptor, jobject properties, jstring filename) {

    // Convert filename from Java string (may be null)
    const char* filenameStr = nullptr;
    if (filename != nullptr) {
        filenameStr = env->GetStringUTFChars(filename, nullptr);
    }

    // Create stream - use custom wrapper if filename provided, otherwise use standard FileStream
    std::unique_ptr<TagLib::IOStream> stream;
    if (filenameStr != nullptr) {
        stream = std::make_unique<FileStreamWithName>(file_descriptor, TagLib::String(filenameStr), false);
        env->ReleaseStringUTFChars(filename, filenameStr);
    } else {
        stream = std::make_unique<TagLib::FileStream>(file_descriptor, false);
    }

    TagLib::FileRef fileRef(stream.get());

    jboolean isSuccessful = false;

    if (!fileRef.isNull() && fileRef.tag()) {
        TagLib::PropertyMap taglibProperties = fileRef.properties();
        jobject entrySet = env->CallObjectMethod(properties, getEntrySet);
        jobject iterator = env->CallObjectMethod(entrySet, getIterator);

        while (env->CallBooleanMethod(iterator, iteratorHasNext)) {
            jobject entry = env->CallObjectMethod(iterator, iteratorNextEntry);
            auto key = (jstring) env->CallObjectMethod(entry, getPropertyKey);
            jobject values = env->CallObjectMethod(entry, getPropertyValue);
            jint len = env->CallIntMethod(values, getListSize);
            TagLib::StringList stringList;
            for (jint i = 0; i < len; i++) {
                auto element = (jstring) env->CallObjectMethod(values, getListElement, i);
                stringList.append(TagLib::String(convertJStringToCString(env, element)));
            }
            taglibProperties.replace(
                    TagLib::String(convertJStringToCString(env, key)),
                    stringList
            );
        }

        fileRef.setProperties(taglibProperties);
        isSuccessful = fileRef.save();
    }

    return isSuccessful;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_simplecityapps_ktaglib_KTagLib_getArtwork(JNIEnv *env, jclass clazz, jint file_descriptor, jstring filename) {

    // Convert filename from Java string (may be null)
    const char* filenameStr = nullptr;
    if (filename != nullptr) {
        filenameStr = env->GetStringUTFChars(filename, nullptr);
    }

    // Create stream - use custom wrapper if filename provided, otherwise use standard FileStream
    std::unique_ptr<TagLib::IOStream> stream;
    if (filenameStr != nullptr) {
        stream = std::make_unique<FileStreamWithName>(file_descriptor, TagLib::String(filenameStr), true);
        env->ReleaseStringUTFChars(filename, filenameStr);
    } else {
        stream = std::make_unique<TagLib::FileStream>(file_descriptor, true);
    }

    TagLib::FileRef fileRef(stream.get());

    jbyteArray result = nullptr;

    if (!fileRef.isNull()) {
        TagLib::ByteVector byteVector;

        if (auto *flacFile = dynamic_cast<TagLib::FLAC::File *>(fileRef.file())) {
            const TagLib::List<TagLib::FLAC::Picture *> &picList = flacFile->pictureList();
            if (!picList.isEmpty()) {
                size_t picSize = 0;
                for (auto i : picList) {
                    size_t size = i->data().size();
                    if (size > picSize) {
                        byteVector = i->data();
                    }
                    picSize = size;
                }
            }
        } else if (auto *opusFile = dynamic_cast<TagLib::Ogg::Opus::File *>(fileRef.file())) {
            TagLib::Ogg::XiphComment *tag = opusFile->tag();
            if (tag != nullptr) {
                const TagLib::List<TagLib::FLAC::Picture *> &picList = tag->pictureList();
                if (!picList.isEmpty()) {
                    size_t picSize = 0;
                    for (auto i : picList) {
                        size_t size = i->data().size();
                        if (size > picSize) {
                            byteVector = i->data();
                        }
                        picSize = size;
                    }
                }
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