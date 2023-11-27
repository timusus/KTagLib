
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
#include <android/log.h>

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
        __android_log_print(ANDROID_LOG_VERBOSE, "kTagLib", "%s", msg.toCString(false));
    }
};

DebugListener listener;

jstring convertToJString(JNIEnv *env, const TagLib::String &taglibString) {
    const char* utf8 = taglibString.toCString(false);
    jbyteArray bytes = env->NewByteArray(strlen(utf8));
    env->SetByteArrayRegion(bytes, 0, strlen(utf8), reinterpret_cast<const jbyte*>(utf8));

    jclass stringClass = env->FindClass("java/lang/String");
    jmethodID ctor = env->GetMethodID(stringClass, "<init>", "([BLjava/lang/String;)V");
    jstring encoding = env->NewStringUTF("UTF-8");

    jstring result = (jstring) env->NewObject(stringClass, ctor, bytes, encoding);

    env->DeleteLocalRef(bytes);
    env->DeleteLocalRef(encoding);
    env->DeleteLocalRef(stringClass);

    return result;
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
Java_com_simplecityapps_ktaglib_KTagLib_getMetadata(JNIEnv *env, jclass clazz, jint file_descriptor) {

    auto stream = std::make_unique<TagLib::FileStream>(file_descriptor, true);
    TagLib::FileRef fileRef(stream.get());

    if (fileRef.isValid()) {
        jobject jPropertyMap = env->NewObject(globalHashMapClass, hashMapInit);

        auto taglibProperties = fileRef.properties();
        for (auto &taglibProperty : taglibProperties) {
            jstring key = env->NewStringUTF(taglibProperty.first.toCString(false));
            jobject values = env->NewObject(globalArrayListClass, arrayListInit, (jint) 0);
            for (auto &value : taglibProperty.second) {
                env->CallBooleanMethod(values, addListElement, env->NewStringUTF(value.toCString(false)));
            }
            env->CallObjectMethod(jPropertyMap, addProperty, key, values);
        }

        jobject jAudioProperties = nullptr;
        auto audioProperties = fileRef.audioProperties();
        if (audioProperties != nullptr) {
            jAudioProperties = env->NewObject(
                    globalAudioPropertiesClass,
                    audioPropertiesInit,
                    (jint) audioProperties->lengthInMilliseconds(),
                    (jint) audioProperties->bitrate(),
                    (jint) audioProperties->sampleRate(),
                    (jint) audioProperties->channels()
            );
        }
        return env->NewObject(globalMetadataClass, metadataInit, jPropertyMap, jAudioProperties);
    }

    return nullptr;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_simplecityapps_ktaglib_KTagLib_writeMetadata(JNIEnv *env, jclass clazz, jint file_descriptor, jobject properties) {

    auto stream = std::make_unique<TagLib::FileStream>(file_descriptor, false);
    TagLib::FileRef fileRef(stream.get());

    jboolean isSuccessful = false;

    if (fileRef.isValid()) {
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
Java_com_simplecityapps_ktaglib_KTagLib_getArtwork(JNIEnv *env, jclass clazz, jint file_descriptor) {

    auto stream = std::make_unique<TagLib::FileStream>(file_descriptor, true);
    TagLib::FileRef fileRef(stream.get());

    jbyteArray result = nullptr;

    if (fileRef.isValid()) {
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
                TagLib::PictureMap pictureMap = tag->pictures();
                if (!pictureMap.isEmpty()) {
                    // Finds the largest picture by byte size
                    size_t picSize = 0;
                    for (auto const &pair: pictureMap) {
                        for (auto const &i: pair.second) {
                            size_t size = i.data().size();
                            if (size > picSize) {
                                byteVector = i.data();
                            }
                            picSize = size;
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