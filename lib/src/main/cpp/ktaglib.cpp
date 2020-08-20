
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
#include <toolkit/tpicture.h>
#include <toolkit/tmap.h>
#include <toolkit/tpicturemap.h>
#include <toolkit/tdebuglistener.h>

#include "unique_fd.h"


class DebugListener : public TagLib::DebugListener {
    void printMessage(const TagLib::String &msg) override {
        __android_log_print(ANDROID_LOG_VERBOSE, "kTagLib", "%s", msg.toCString(true));
    }
};

DebugListener listener;

jclass globalHashMapClass;
jclass globalSetClass;
jclass globalIteratorClass;
jclass globalMapEntryClass;

jmethodID hashMapInit;
jmethodID addProperty;
jmethodID getEntrySet;
jmethodID getIterator;
jmethodID iteratorHasNext;
jmethodID iteratorNextEntry;
jmethodID getPropertyKey;
jmethodID getPropertyValue;

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jclass hashMapClass = env->FindClass("java/util/HashMap");
    globalHashMapClass = reinterpret_cast<jclass>(env->NewGlobalRef(hashMapClass));
    env->DeleteLocalRef(hashMapClass);

    jclass iteratorClass = env->FindClass("java/util/Iterator");
    globalIteratorClass = reinterpret_cast<jclass>(env->NewGlobalRef(iteratorClass));
    env->DeleteLocalRef(iteratorClass);

    jclass mapEntryClass = env->FindClass("java/util/Map$Entry");
    globalMapEntryClass = reinterpret_cast<jclass>(env->NewGlobalRef(mapEntryClass));
    env->DeleteLocalRef(mapEntryClass);

    jclass setClass = env->FindClass("java/util/Set");
    globalSetClass = reinterpret_cast<jclass>(env->NewGlobalRef(setClass));
    env->DeleteLocalRef(setClass);

    hashMapInit = env->GetMethodID(globalHashMapClass, "<init>", "()V");
    addProperty = env->GetMethodID(globalHashMapClass, "put",
            "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    getEntrySet = env->GetMethodID(globalHashMapClass, "entrySet",
            "()Ljava/util/Set;");

    getIterator = env->GetMethodID(globalSetClass, "iterator",
            "()Ljava/util/Iterator;");

    iteratorHasNext = env->GetMethodID(globalIteratorClass, "hasNext", "()Z");
    iteratorNextEntry = env->GetMethodID(globalIteratorClass, "next",
            "()Ljava/lang/Object;");

    getPropertyKey = env->GetMethodID(globalMapEntryClass, "getKey",
            "()Ljava/lang/Object;");
    getPropertyValue = env->GetMethodID(globalMapEntryClass, "getValue",
            "()Ljava/lang/Object;");

    TagLib::setDebugListener(&listener);

    return JNI_VERSION_1_6;
}

extern "C" void JNI_OnUnload(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);

    env->DeleteGlobalRef(globalHashMapClass);
    env->DeleteGlobalRef(globalMapEntryClass);
    env->DeleteGlobalRef(globalIteratorClass);
    env->DeleteGlobalRef(globalSetClass);

    TagLib::setDebugListener(nullptr);
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_com_simplecityapps_ktaglib_KTagLib_getArtwork(JNIEnv *env, jclass clazz, jint fd_) {

    unique_fd uniqueFd = unique_fd(fd_);

    TagLib::IOStream *stream = new TagLib::FileStream(uniqueFd.get(), true);
    TagLib::FileRef fileRef(stream);

    jbyteArray result = nullptr;

    if (!fileRef.isNull()) {
        TagLib::ByteVector byteVector;
        if (auto *flacFile = dynamic_cast<TagLib::FLAC::File *>(fileRef.file())) {
            const TagLib::List<TagLib::FLAC::Picture *> &picList = flacFile->pictureList();
            if (!picList.isEmpty()) {
                byteVector = picList[0]->data();
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
        } else {
            TagLib::Tag *tag = fileRef.tag();
            TagLib::PictureMap pictureMap = tag->pictures();
            TagLib::Picture picture;
            // Finds the largest picture by byte size
            size_t picSize = 0;
            for (auto const &x: pictureMap) {
                for (auto const &y: x.second) {
                    size_t size = y.data().size();
                    if (size > picSize) {
                        picture = y;
                    }
                    picSize = size;
                }
            }
            byteVector = picture.data();
        }
        size_t len = byteVector.size();
        if (len > 0) {
            jbyteArray arr = env->NewByteArray(len);
            char *data = byteVector.data();
            env->SetByteArrayRegion(arr, 0, len, reinterpret_cast<jbyte *>(data));
            result = arr;
        }
    }

    uniqueFd.release();
    return result;
}

static const char* convertJStringToCString(JNIEnv *env, jstring str) {
    jboolean is_copy;
    const char *CString = env->GetStringUTFChars(str, &is_copy);
    return CString;
}

static void addIntegerProperty(JNIEnv *env, jobject properties, const char* key, long long value) {
    std::string string_value = std::to_string(value);
    jstring jKey = env->NewStringUTF(key);
    jstring jValue = env->NewStringUTF(string_value.c_str());
    env->CallObjectMethod(properties, addProperty, jKey, jValue);
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_simplecityapps_ktaglib_KTagLib_getMetadata(JNIEnv *env, jclass clazz, jint file_descriptor) {
    unique_fd uniqueFd = unique_fd(file_descriptor);

    TagLib::IOStream *stream = new TagLib::FileStream(uniqueFd.get(), true);
    TagLib::FileRef fileRef(stream);

    jobject properties = env->NewObject(globalHashMapClass, hashMapInit);

    if (fileRef.isValid()) {
        auto taglibProperties = fileRef.properties();
        for (auto & taglibProperty : taglibProperties)
            if (!taglibProperty.second.isEmpty()) {
                jstring key = env->NewStringUTF(taglibProperty.first.toCString(true));
                jstring value = env->NewStringUTF(taglibProperty.second.front().toCString(true));
                env->CallObjectMethod(properties, addProperty, key, value);
            }

        auto audioProperties = fileRef.audioProperties();
        addIntegerProperty(env, properties, "BITRATE", audioProperties->bitrate());
        addIntegerProperty(env, properties, "CHANNELS", audioProperties->channels());
        addIntegerProperty(env, properties, "DURATION", audioProperties->lengthInMilliseconds());
        addIntegerProperty(env, properties, "SAMPLERATE", audioProperties->sampleRate());

        struct stat statbuf{};
        fstat(uniqueFd.get(), &statbuf);
        addIntegerProperty(env, properties, "LAST_MODIFIED", statbuf.st_mtime * 1000L);
        addIntegerProperty(env, properties, "SIZE", statbuf.st_size);
    }

    uniqueFd.release();

    return properties;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_simplecityapps_ktaglib_KTagLib_writeMetadata(JNIEnv *env, jclass clazz, jint file_descriptor, jobject properties) {
    unique_fd uniqueFd = unique_fd(file_descriptor);

    TagLib::IOStream *stream = new TagLib::FileStream(uniqueFd.get(), false);
    TagLib::FileRef fileRef(stream);

    jboolean isSuccessful = false;

    if (fileRef.isValid()) {
        TagLib::PropertyMap taglibProperties = fileRef.properties();
        jobject entrySet = env->CallObjectMethod(properties, getEntrySet);
        jobject iterator = env->CallObjectMethod(entrySet, getIterator);

        while (env->CallBooleanMethod(iterator, iteratorHasNext)) {
            jobject entry = env->CallObjectMethod(iterator, iteratorNextEntry);
            auto key = (jstring) env->CallObjectMethod(entry, getPropertyKey);
            auto value = (jstring) env->CallObjectMethod(entry, getPropertyValue);
            taglibProperties.replace(
                    TagLib::String(convertJStringToCString(env, key)),
                    TagLib::String(convertJStringToCString(env, value)));
        }

        fileRef.setProperties(taglibProperties);
        isSuccessful = fileRef.save();
    }

    uniqueFd.release();

    return isSuccessful;
}