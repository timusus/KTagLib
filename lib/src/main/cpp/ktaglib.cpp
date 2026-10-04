
#include <jni.h>
#include <cerrno>
#include <climits>
#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
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
#include <ape/apeproperties.h>
#include <asf/asfproperties.h>
#include <dsdiff/dsdiffproperties.h>
#include <dsf/dsfproperties.h>
#include <flac/flacproperties.h>
#include <mpc/mpcproperties.h>
#include <mpeg/mpegproperties.h>
#include <ogg/opus/opusproperties.h>
#include <ogg/speex/speexproperties.h>
#include <ogg/vorbis/vorbisproperties.h>
#include <matroska/matroskaproperties.h>
#include <mp4/mp4properties.h>
#include <riff/aiff/aiffproperties.h>
#include <riff/wav/wavproperties.h>
#include <shorten/shortenproperties.h>
#include <trueaudio/trueaudioproperties.h>
#include <wavpack/wavpackproperties.h>

// Configure verbose logging for detailed diagnostics
// Set to 1 to enable detailed property/value logging (useful for debugging)
// Set to 0 for production to reduce logging overhead
#define KTAGLIB_ENABLE_VERBOSE_LOGGING 0

// Custom IOStream wrapper that delegates to FileStream but provides a filename hint
// This allows FileRef to use extension-based detection while still using file descriptors for I/O
class FileStreamWithName : public TagLib::IOStream {
public:
    FileStreamWithName(int fileDescriptor, const TagLib::String &fileName, bool readOnly = true)
        : m_name(fileName), m_stream(fileDescriptor, readOnly) {
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
    // m_name is declared (and so constructed) first: once m_stream has fdopen()ed the descriptor
    // it owns it, so nothing that can throw may run after it in the constructor.
    TagLib::String m_name;
    TagLib::FileStream m_stream;
};

// Thrown when a JNI call leaves a Java exception pending. The exception is cleared first, so the
// entry point's catch handler can return its failure value with nothing pending.
class JniException : public std::runtime_error {
public:
    explicit JniException(const char *what) : std::runtime_error(what) {}
};

static void checkJni(JNIEnv *env, const char *what) {
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        throw JniException(what);
    }
}

// Deletes a JNI local reference when it goes out of scope, including on an exception path.
template<typename T>
class LocalRef {
public:
    LocalRef(JNIEnv *env, T ref) : m_env(env), m_ref(ref) {}

    ~LocalRef() {
        if (m_ref != nullptr) m_env->DeleteLocalRef(m_ref);
    }

    LocalRef(const LocalRef &) = delete;
    LocalRef &operator=(const LocalRef &) = delete;

    T get() const { return m_ref; }

    T release() {
        T ref = m_ref;
        m_ref = nullptr;
        return ref;
    }

    void reset(T ref) {
        if (m_ref != nullptr) m_env->DeleteLocalRef(m_ref);
        m_ref = ref;
    }

private:
    JNIEnv *m_env;
    T m_ref;
};

// Closes a file descriptor when it goes out of scope, unless ownership was release()d.
class UniqueFd {
public:
    explicit UniqueFd(int fd) : m_fd(fd) {}

    ~UniqueFd() {
        if (m_fd >= 0) close(m_fd);
    }

    UniqueFd(const UniqueFd &) = delete;
    UniqueFd &operator=(const UniqueFd &) = delete;

    int get() const { return m_fd; }

    int release() {
        int fd = m_fd;
        m_fd = -1;
        return fd;
    }

private:
    int m_fd;
};

// Runs a JNI entry point body, turning any C++ exception (std::bad_alloc on a malformed file,
// a JniException, ...) into the entry point's failure value, so it never unwinds into the VM and
// aborts the process. No Java exception is left pending on return.
template<typename R, typename Body>
static R guarded(JNIEnv *env, const char *function, R failure, Body &&body) {
    try {
        return body();
    } catch (const std::exception &e) {
        __android_log_print(ANDROID_LOG_ERROR, "kTagLib", "%s failed: %s", function, e.what());
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "kTagLib", "%s failed with an unknown exception", function);
    }
    env->ExceptionClear();
    return failure;
}

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

// Looked up on the java.util.Map / java.util.List interfaces so writeMetadata accepts any
// implementation (Kotlin's read-only maps and lists included), not only HashMap / ArrayList.
jclass globalMapClass;
jmethodID getEntrySet;
jclass globalListClass;
jmethodID getListElement;
jmethodID getListSize;

jclass globalSetClass;
jclass globalIteratorClass;
jmethodID getIterator;
jmethodID iteratorHasNext;
jmethodID iteratorNextEntry;

jclass globalArrayListClass;
jmethodID arrayListInit;
jmethodID addListElement;


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
// Returns the bits per sample of formats that store one, or 0. TagLib exposes it only on the
// format-specific Properties subclasses, not on TagLib::AudioProperties.
static int bitsPerSample(const TagLib::AudioProperties *properties) {
    if (auto p = dynamic_cast<const TagLib::FLAC::Properties *>(properties)) return p->bitsPerSample();
    if (auto p = dynamic_cast<const TagLib::MP4::Properties *>(properties)) return p->bitsPerSample();
    if (auto p = dynamic_cast<const TagLib::RIFF::WAV::Properties *>(properties)) return p->bitsPerSample();
    if (auto p = dynamic_cast<const TagLib::RIFF::AIFF::Properties *>(properties)) return p->bitsPerSample();
    if (auto p = dynamic_cast<const TagLib::APE::Properties *>(properties)) return p->bitsPerSample();
    if (auto p = dynamic_cast<const TagLib::WavPack::Properties *>(properties)) return p->bitsPerSample();
    if (auto p = dynamic_cast<const TagLib::TrueAudio::Properties *>(properties)) return p->bitsPerSample();
    if (auto p = dynamic_cast<const TagLib::ASF::Properties *>(properties)) return p->bitsPerSample();
    if (auto p = dynamic_cast<const TagLib::Matroska::Properties *>(properties)) return p->bitsPerSample();
    if (auto p = dynamic_cast<const TagLib::DSF::Properties *>(properties)) return p->bitsPerSample();
    if (auto p = dynamic_cast<const TagLib::DSDIFF::Properties *>(properties)) return p->bitsPerSample();
    if (auto p = dynamic_cast<const TagLib::Shorten::Properties *>(properties)) return p->bitsPerSample();
    return 0;
}

// Returns the lowercase name of the audio codec (not the container), or nullptr if it is unknown.
static const char *codecName(const TagLib::AudioProperties *properties) {
    if (auto p = dynamic_cast<const TagLib::MP4::Properties *>(properties)) {
        switch (p->codec()) {
            case TagLib::MP4::Properties::AAC: return "aac";
            case TagLib::MP4::Properties::ALAC: return "alac";
            case TagLib::MP4::Properties::AC3: return "ac3";
            case TagLib::MP4::Properties::EAC3: return "eac3";
            case TagLib::MP4::Properties::FLAC: return "flac";
            case TagLib::MP4::Properties::DTS: return "dts";
            case TagLib::MP4::Properties::Opus: return "opus";
            default: return nullptr;
        }
    }
    if (dynamic_cast<const TagLib::FLAC::Properties *>(properties)) return "flac";
    if (dynamic_cast<const TagLib::RIFF::WAV::Properties *>(properties)) return "wav";
    if (dynamic_cast<const TagLib::RIFF::AIFF::Properties *>(properties)) return "aiff";
    if (dynamic_cast<const TagLib::APE::Properties *>(properties)) return "ape";
    if (dynamic_cast<const TagLib::WavPack::Properties *>(properties)) return "wavpack";
    if (dynamic_cast<const TagLib::DSF::Properties *>(properties)) return "dsd";
    if (dynamic_cast<const TagLib::DSDIFF::Properties *>(properties)) return "dsd";
    if (auto p = dynamic_cast<const TagLib::MPEG::Properties *>(properties)) {
        return p->layer() == 1 ? "mp1" : p->layer() == 2 ? "mp2" : "mp3";
    }
    if (dynamic_cast<const TagLib::Vorbis::Properties *>(properties)) return "vorbis";
    if (dynamic_cast<const TagLib::Ogg::Opus::Properties *>(properties)) return "opus";
    if (dynamic_cast<const TagLib::Ogg::Speex::Properties *>(properties)) return "speex";
    if (auto p = dynamic_cast<const TagLib::ASF::Properties *>(properties)) {
        switch (p->codec()) {
            case TagLib::ASF::Properties::WMA1:
            case TagLib::ASF::Properties::WMA2: return "wma";
            case TagLib::ASF::Properties::WMA9Pro: return "wmapro";
            case TagLib::ASF::Properties::WMA9Lossless: return "wmalossless";
            default: return nullptr;
        }
    }
    if (dynamic_cast<const TagLib::TrueAudio::Properties *>(properties)) return "tta";
    if (dynamic_cast<const TagLib::Shorten::Properties *>(properties)) return "shorten";
    if (dynamic_cast<const TagLib::MPC::Properties *>(properties)) return "musepack";
    if (auto p = dynamic_cast<const TagLib::Matroska::Properties *>(properties)) {
        const std::string id = p->codecName().to8Bit(false);
        auto startsWith = [&id](const char *prefix) { return id.rfind(prefix, 0) == 0; };
        if (id == "A_FLAC") return "flac";
        if (startsWith("A_AAC")) return "aac";
        if (id == "A_ALAC") return "alac";
        if (id == "A_OPUS") return "opus";
        if (startsWith("A_VORBIS")) return "vorbis";
        if (id == "A_MPEG/L3") return "mp3";
        return nullptr;
    }
    return nullptr;
}

// GetStringUTFChars returns *modified* UTF-8, which encodes characters outside the BMP (emoji, for
// example) as a pair of 3-byte surrogates that standard UTF-8 decoders reject, and a bare
// TagLib::String(const char *) is decoded as Latin-1. Either way non-ASCII text gets corrupted on
// write, so hand TagLib the UTF-16 code units, which it stores as-is.
static TagLib::String toTagLibString(JNIEnv *env, jstring str) {
    if (str == nullptr) {
        return TagLib::String();
    }
    const jsize length = env->GetStringLength(str);
    checkJni(env, "GetStringLength");
    const jchar *chars = env->GetStringChars(str, nullptr);
    if (chars == nullptr) {
        checkJni(env, "GetStringChars");
        throw JniException("GetStringChars returned null");
    }
    // Release the chars even if the ByteVector allocation throws.
    std::unique_ptr<const jchar, std::function<void(const jchar *)>> charsGuard(
            chars, [env, str](const jchar *c) { env->ReleaseStringChars(str, c); });
    TagLib::ByteVector bytes(static_cast<unsigned int>(length) * 2, 0);
    for (jsize i = 0; i < length; i++) {
        bytes[i * 2] = static_cast<char>(chars[i] & 0xFF);
        bytes[i * 2 + 1] = static_cast<char>((chars[i] >> 8) & 0xFF);
    }
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
    jstring result = env->NewString(chars.data(), static_cast<jsize>(length));
    checkJni(env, "NewString");
    if (result == nullptr) throw JniException("NewString returned null");
    return result;
}

// Creates a java.util.ArrayList holding the given values.
static jobject toJStringList(JNIEnv *env, const TagLib::StringList &values) {
    LocalRef<jobject> list(env, env->NewObject(globalArrayListClass, arrayListInit, (jint) values.size()));
    checkJni(env, "new ArrayList");
    for (const auto &value : values) {
#if KTAGLIB_ENABLE_VERBOSE_LOGGING
        __android_log_print(ANDROID_LOG_VERBOSE, "kTagLib", "  Value: '%s'", value.toCString(true));
#endif
        LocalRef<jstring> jValue(env, toJString(env, value));
        env->CallBooleanMethod(list.get(), addListElement, jValue.get());
        checkJni(env, "ArrayList.add");
    }
    return list.release();
}

// Opens a TagLib stream over a duplicate of the file descriptor, with the filename (if any) as a
// type hint. Returns nullptr if the descriptor can't be duplicated or opened.
//
// The caller keeps ownership of fileDescriptor; only the duplicate is closed here. The stream takes
// ownership of the duplicate once it has fdopen()ed it (~FileStream fclose()s it). Until then, and
// if fdopen fails, the duplicate is closed by UniqueFd, so it is closed on every path.
static std::unique_ptr<TagLib::IOStream> openStream(JNIEnv *env, jint fileDescriptor, jstring filename, bool readOnly) {
    // Converted before duplicating, so a failure here has nothing to close.
    TagLib::String name;
    if (filename != nullptr) {
        name = toTagLibString(env, filename);
#if KTAGLIB_ENABLE_VERBOSE_LOGGING
        __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "Filename hint provided: %s", name.toCString(true));
#endif
    }
    UniqueFd fd(fcntl(fileDescriptor, F_DUPFD_CLOEXEC, 0));
    if (fd.get() < 0) {
        __android_log_print(ANDROID_LOG_WARN, "kTagLib", "Could not duplicate file descriptor %d: %s",
                            fileDescriptor, strerror(errno));
        return nullptr;
    }
    std::unique_ptr<TagLib::IOStream> stream;
    if (filename != nullptr) {
        stream = std::make_unique<FileStreamWithName>(fd.get(), name, readOnly);
    } else {
        stream = std::make_unique<TagLib::FileStream>(fd.get(), readOnly);
    }
    if (!stream->isOpen()) {
        __android_log_print(ANDROID_LOG_WARN, "kTagLib", "Could not open file descriptor %d", fileDescriptor);
        return nullptr;
    }
    fd.release();
    return stream;
}

// Looks up a class and keeps a global reference to it, or returns nullptr (with any pending
// exception cleared) if it can't be found.
static jclass findGlobalClass(JNIEnv *env, const char *name) {
    jclass localClass = env->FindClass(name);
    if (localClass == nullptr || env->ExceptionCheck()) {
        env->ExceptionClear();
        __android_log_print(ANDROID_LOG_ERROR, "kTagLib", "JNI_OnLoad: class %s not found", name);
        return nullptr;
    }
    auto globalClass = reinterpret_cast<jclass>(env->NewGlobalRef(localClass));
    env->DeleteLocalRef(localClass);
    return globalClass;
}

// Looks up a method, or returns nullptr (with any pending exception cleared) if it can't be found.
static jmethodID findMethod(JNIEnv *env, jclass clazz, const char *name, const char *signature) {
    if (clazz == nullptr) return nullptr;
    jmethodID method = env->GetMethodID(clazz, name, signature);
    if (method == nullptr || env->ExceptionCheck()) {
        env->ExceptionClear();
        __android_log_print(ANDROID_LOG_ERROR, "kTagLib", "JNI_OnLoad: method %s%s not found", name, signature);
        return nullptr;
    }
    return method;
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    // A missing class or method means a mismatched Kotlin/native build; fail the load (System.loadLibrary
    // throws UnsatisfiedLinkError) rather than crash on a null ID later.
    return guarded(env, "JNI_OnLoad", (jint) JNI_ERR, [&]() -> jint {
        globalMetadataClass = findGlobalClass(env, "com/simplecityapps/ktaglib/Metadata");
        metadataInit = findMethod(env, globalMetadataClass, "<init>", "(Ljava/util/Map;Lcom/simplecityapps/ktaglib/AudioProperties;)V");

        globalAudioPropertiesClass = findGlobalClass(env, "com/simplecityapps/ktaglib/AudioProperties");
        audioPropertiesInit = findMethod(env, globalAudioPropertiesClass, "<init>", "(IIIIILjava/lang/String;)V");

        globalSetClass = findGlobalClass(env, "java/util/Set");
        globalIteratorClass = findGlobalClass(env, "java/util/Iterator");
        getIterator = findMethod(env, globalSetClass, "iterator", "()Ljava/util/Iterator;");
        iteratorHasNext = findMethod(env, globalIteratorClass, "hasNext", "()Z");
        iteratorNextEntry = findMethod(env, globalIteratorClass, "next", "()Ljava/lang/Object;");

        globalHashMapClass = findGlobalClass(env, "java/util/HashMap");
        hashMapInit = findMethod(env, globalHashMapClass, "<init>", "()V");
        addProperty = findMethod(env, globalHashMapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

        globalMapClass = findGlobalClass(env, "java/util/Map");
        getEntrySet = findMethod(env, globalMapClass, "entrySet", "()Ljava/util/Set;");

        globalMapEntryClass = findGlobalClass(env, "java/util/Map$Entry");
        getPropertyKey = findMethod(env, globalMapEntryClass, "getKey", "()Ljava/lang/Object;");
        getPropertyValue = findMethod(env, globalMapEntryClass, "getValue", "()Ljava/lang/Object;");

        globalArrayListClass = findGlobalClass(env, "java/util/ArrayList");
        arrayListInit = findMethod(env, globalArrayListClass, "<init>", "(I)V");
        addListElement = findMethod(env, globalArrayListClass, "add", "(Ljava/lang/Object;)Z");

        globalListClass = findGlobalClass(env, "java/util/List");
        getListElement = findMethod(env, globalListClass, "get", "(I)Ljava/lang/Object;");
        getListSize = findMethod(env, globalListClass, "size", "()I");

        const void *required[] = {
                metadataInit, audioPropertiesInit, getIterator, iteratorHasNext, iteratorNextEntry,
                hashMapInit, addProperty, getEntrySet, getPropertyKey, getPropertyValue,
                arrayListInit, addListElement, getListElement, getListSize,
        };
        for (const void *id : required) {
            if (id == nullptr) return JNI_ERR;
        }

        TagLib::setDebugListener(&listener);

        return JNI_VERSION_1_6;
    });
}

extern "C" void JNI_OnUnload(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return;
    }

    for (jclass globalClass : {globalMetadataClass, globalAudioPropertiesClass, globalHashMapClass,
                               globalMapEntryClass, globalIteratorClass, globalArrayListClass,
                               globalMapClass, globalListClass, globalSetClass}) {
        if (globalClass != nullptr) env->DeleteGlobalRef(globalClass);
    }

    TagLib::setDebugListener(nullptr);
}

// Creates a com.simplecityapps.ktaglib.AudioProperties from TagLib's audio properties.
static jobject toJAudioProperties(JNIEnv *env, const TagLib::AudioProperties *audioProperties) {
#if KTAGLIB_ENABLE_VERBOSE_LOGGING
    // Log audio properties
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib",
        "Audio properties: duration=%dms, bitrate=%dkbps, sampleRate=%dHz, channels=%d, bitsPerSample=%d, codec=%s",
        audioProperties->lengthInMilliseconds(),
        audioProperties->bitrate(),
        audioProperties->sampleRate(),
        audioProperties->channels(),
        bitsPerSample(audioProperties),
        codecName(audioProperties) != nullptr ? codecName(audioProperties) : "(null)");
#endif

    const char *codec = codecName(audioProperties);
    LocalRef<jstring> jCodec(env, codec != nullptr ? env->NewStringUTF(codec) : nullptr);
    checkJni(env, "NewStringUTF");
    jobject jAudioProperties = env->NewObject(
            globalAudioPropertiesClass,
            audioPropertiesInit,
            (jint) audioProperties->lengthInMilliseconds(),
            (jint) audioProperties->bitrate(),
            (jint) audioProperties->sampleRate(),
            (jint) audioProperties->channels(),
            (jint) bitsPerSample(audioProperties),
            jCodec.get()
    );
    checkJni(env, "new AudioProperties");
    return jAudioProperties;
}

static jobject getMetadata(JNIEnv *env, jint file_descriptor, jstring filename) {

#if KTAGLIB_ENABLE_VERBOSE_LOGGING
    // Log function entry with file descriptor
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "getMetadata: Opening file descriptor %d", file_descriptor);
#endif

    std::unique_ptr<TagLib::IOStream> stream = openStream(env, file_descriptor, filename, true);
    if (!stream) {
        return nullptr;
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

    LocalRef<jobject> jPropertyMap(env, env->NewObject(globalHashMapClass, hashMapInit));
    checkJni(env, "new HashMap");
    bool hasTitle = false;

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
#if KTAGLIB_ENABLE_VERBOSE_LOGGING
            // Log each property key and value count
            __android_log_print(ANDROID_LOG_VERBOSE, "kTagLib",
                "Property: %s = [%lu values]",
                taglibProperty.first.toCString(true),
                (unsigned long)taglibProperty.second.size());
#endif

            if (taglibProperty.first == "TITLE" && !taglibProperty.second.isEmpty()
                && !taglibProperty.second.front().isEmpty()) {
                hasTitle = true;
            }

            LocalRef<jstring> key(env, toJString(env, taglibProperty.first));
            LocalRef<jobject> values(env, toJStringList(env, taglibProperty.second));
            LocalRef<jobject> previous(env, env->CallObjectMethod(jPropertyMap.get(), addProperty, key.get(), values.get()));
            checkJni(env, "HashMap.put");
        }
    }

    auto audioProperties = fileRef.audioProperties();

    // TagLib's Matroska property map omits the Segment Info title, so offer it as TITLE when the
    // file has no track or tag-level title.
    if (!hasTitle) {
        if (auto mkvProperties = dynamic_cast<const TagLib::Matroska::Properties *>(audioProperties)) {
            const TagLib::String segmentTitle = mkvProperties->title();
            if (!segmentTitle.isEmpty()) {
                LocalRef<jstring> key(env, toJString(env, TagLib::String("TITLE")));
                LocalRef<jobject> values(env, toJStringList(env, TagLib::StringList(segmentTitle)));
                LocalRef<jobject> previous(env, env->CallObjectMethod(jPropertyMap.get(), addProperty, key.get(), values.get()));
                checkJni(env, "HashMap.put");
            }
        }
    }
    LocalRef<jobject> jAudioProperties(env, nullptr);
    if (audioProperties != nullptr) {
        jAudioProperties.reset(toJAudioProperties(env, audioProperties));
    } else {
        __android_log_print(ANDROID_LOG_WARN, "kTagLib", "Audio properties not available");
    }

#if KTAGLIB_ENABLE_VERBOSE_LOGGING
    __android_log_print(ANDROID_LOG_DEBUG, "kTagLib", "Successfully created Metadata object");
#endif
    jobject metadata = env->NewObject(globalMetadataClass, metadataInit, jPropertyMap.get(), jAudioProperties.get());
    checkJni(env, "new Metadata");
    return metadata;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_simplecityapps_ktaglib_KTagLib_getMetadata(JNIEnv *env, jclass clazz, jint file_descriptor, jstring filename) {
    return guarded(env, "getMetadata", (jobject) nullptr, [&] { return getMetadata(env, file_descriptor, filename); });
}

static jboolean writeMetadata(JNIEnv *env, jint file_descriptor, jobject properties, jstring filename) {

    std::unique_ptr<TagLib::IOStream> stream = openStream(env, file_descriptor, filename, false);
    if (!stream) {
        return JNI_FALSE;
    }
    // Only reachable from Java callers.
    if (properties == nullptr) {
        __android_log_print(ANDROID_LOG_WARN, "kTagLib", "writeMetadata: properties is null");
        return JNI_FALSE;
    }

    TagLib::FileRef fileRef(stream.get(), false);

    if (fileRef.isNull() || !fileRef.tag()) {
        return JNI_FALSE;
    }

    TagLib::PropertyMap taglibProperties = fileRef.properties();
    LocalRef<jobject> entrySet(env, env->CallObjectMethod(properties, getEntrySet));
    checkJni(env, "Map.entrySet");
    LocalRef<jobject> iterator(env, env->CallObjectMethod(entrySet.get(), getIterator));
    checkJni(env, "Set.iterator");

    while (true) {
        const jboolean hasNext = env->CallBooleanMethod(iterator.get(), iteratorHasNext);
        checkJni(env, "Iterator.hasNext");
        if (!hasNext) break;

        LocalRef<jobject> entry(env, env->CallObjectMethod(iterator.get(), iteratorNextEntry));
        checkJni(env, "Iterator.next");
        LocalRef<jstring> key(env, (jstring) env->CallObjectMethod(entry.get(), getPropertyKey));
        checkJni(env, "Map.Entry.getKey");
        LocalRef<jobject> values(env, env->CallObjectMethod(entry.get(), getPropertyValue));
        checkJni(env, "Map.Entry.getValue");

        // A null key has no valid tag field to write to, and a null value list has no
        // meaning - skip either (only reachable from Java callers) rather than guess.
        if (key.get() == nullptr || values.get() == nullptr) {
            __android_log_print(ANDROID_LOG_WARN, "kTagLib",
                "writeMetadata: skipping property with a null %s", key.get() == nullptr ? "key" : "value list");
            continue;
        }

        const jint len = env->CallIntMethod(values.get(), getListSize);
        checkJni(env, "List.size");
        TagLib::StringList stringList;
        for (jint i = 0; i < len; i++) {
            LocalRef<jstring> element(env, (jstring) env->CallObjectMethod(values.get(), getListElement, i));
            checkJni(env, "List.get");
            // Null elements (Java callers only) are dropped.
            if (element.get() != nullptr) {
                stringList.append(toTagLibString(env, element.get()));
            }
        }
        // An empty list removes the field. Erasing the key (rather than storing an empty
        // StringList) makes that uniform: setProperties removes every key absent from the map
        // in all formats, whereas an empty value list is handled per format (WAV keeps the
        // field).
        const TagLib::String tagKey = toTagLibString(env, key.get());
        if (stringList.isEmpty()) {
            taglibProperties.erase(tagKey);
        } else {
            taglibProperties.replace(tagKey, stringList);
        }
    }

    fileRef.setProperties(taglibProperties);
    return fileRef.save() ? JNI_TRUE : JNI_FALSE;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_simplecityapps_ktaglib_KTagLib_writeMetadata(JNIEnv *env, jclass clazz, jint file_descriptor, jobject properties, jstring filename) {
    return guarded(env, "writeMetadata", (jboolean) JNI_FALSE, [&] { return writeMetadata(env, file_descriptor, properties, filename); });
}

static jbyteArray getArtwork(JNIEnv *env, jint file_descriptor, jstring filename) {

    std::unique_ptr<TagLib::IOStream> stream = openStream(env, file_descriptor, filename, true);
    if (!stream) {
        return nullptr;
    }

    TagLib::FileRef fileRef(stream.get(), false);

    if (fileRef.isNull()) {
        return nullptr;
    }

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

    const size_t len = byteVector.size();
    if (len == 0 || len > INT_MAX) {
        return nullptr;
    }
    LocalRef<jbyteArray> arr(env, env->NewByteArray(static_cast<jsize>(len)));
    checkJni(env, "NewByteArray");
    if (arr.get() == nullptr) {
        return nullptr;
    }
    env->SetByteArrayRegion(arr.get(), 0, static_cast<jsize>(len), reinterpret_cast<const jbyte *>(byteVector.data()));
    checkJni(env, "SetByteArrayRegion");
    return arr.release();
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_simplecityapps_ktaglib_KTagLib_getArtwork(JNIEnv *env, jclass clazz, jint file_descriptor, jstring filename) {
    return guarded(env, "getArtwork", (jbyteArray) nullptr, [&] { return getArtwork(env, file_descriptor, filename); });
}
