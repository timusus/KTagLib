
#include <jni.h>
#include <sys/stat.h>
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
#include <android/log.h>

class DebugListener : public TagLib::DebugListener {
    void printMessage(const TagLib::String &msg) override {
        __android_log_print(ANDROID_LOG_VERBOSE, "kTagLib", "%s", msg.toCString(true));
    }
};

DebugListener listener;

jclass globalSongClass;
jclass globalHashMapClass;
jmethodID hashMapInit;
jmethodID addProperty;
jmethodID songInit;

jclass globalIntClass;
jmethodID intInit;

jmethodID intGetValue;

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jclass hashMapClass = env->FindClass("java/util/HashMap");
    globalHashMapClass = reinterpret_cast<jclass>(env->NewGlobalRef(hashMapClass));
    env->DeleteLocalRef(hashMapClass);
    hashMapInit = env->GetMethodID(globalHashMapClass, "<init>", "()V");
    addProperty = env->GetMethodID(globalHashMapClass, "put",
            "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    jclass songClass = env->FindClass("com/simplecityapps/ktaglib/AudioFile");
    globalSongClass = reinterpret_cast<jclass>(env->NewGlobalRef(songClass));
    env->DeleteLocalRef(songClass);
    songInit = env->GetMethodID(
            globalSongClass,
            "<init>",
            "(Ljava/lang/String;JJLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/Integer;Ljava/lang/Integer;Ljava/lang/Integer;Ljava/lang/Integer;Ljava/lang/Integer;Ljava/lang/String;Ljava/lang/String;)V"
    );

    jclass intClass = env->FindClass("java/lang/Integer");
    globalIntClass = reinterpret_cast<jclass>(env->NewGlobalRef(intClass));
    env->DeleteLocalRef(intClass);
    intInit = env->GetMethodID(globalIntClass, "<init>", "(I)V");

    intGetValue = env->GetMethodID(globalIntClass, "intValue", "()I");

    TagLib::setDebugListener(&listener);

    return JNI_VERSION_1_6;
}

extern "C" void JNI_OnUnload(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);

    env->DeleteGlobalRef(globalHashMapClass);
    env->DeleteGlobalRef(globalSongClass);
    env->DeleteGlobalRef(globalIntClass);

    TagLib::setDebugListener(nullptr);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_simplecityapps_ktaglib_KTagLib_updateTags(
        JNIEnv *env,
        jobject instance,
        jint fd_,
        jstring title_,
        jstring artist_,
        jstring album_,
        jstring albumArtist_,
        jstring date_,
        jobject track_,
        jobject trackTotal_,
        jobject disc_,
        jobject discTotal_,
        jstring genre_
) {

    unique_fd uniqueFd = unique_fd(fd_);

    TagLib::IOStream *stream = new TagLib::FileStream(uniqueFd.get(), false);
    TagLib::FileRef fileRef(stream);

    bool saved = false;

    if (!fileRef.isNull()) {
        TagLib::Tag *tag = fileRef.tag();
        if (tag) {

            TagLib::PropertyMap properties = fileRef.properties();

            if (title_) {
                const char *title = env->GetStringUTFChars(title_, 0);
                TagLib::StringList stringList = TagLib::StringList();
                stringList.append(title);
                properties.replace("TITLE", stringList);
                env->ReleaseStringUTFChars(title_, title);
            }

            if (artist_) {
                const char *artist = env->GetStringUTFChars(artist_, 0);
                TagLib::StringList stringList = TagLib::StringList();
                stringList.append(artist);
                properties.replace("ARTIST", stringList);
                env->ReleaseStringUTFChars(artist_, artist);
            }

            if (album_) {
                const char *album = env->GetStringUTFChars(album_, 0);
                TagLib::StringList stringList = TagLib::StringList();
                stringList.append(album);
                properties.replace("ALBUM", stringList);
                env->ReleaseStringUTFChars(album_, album);
            }

            if (albumArtist_) {
                const char *albumArtist = env->GetStringUTFChars(albumArtist_, 0);
                TagLib::StringList stringList = TagLib::StringList();
                stringList.append(albumArtist);
                properties.replace("ALBUMARTIST", stringList);
                env->ReleaseStringUTFChars(albumArtist_, albumArtist);
            }

            if (date_) {
                const char *date = env->GetStringUTFChars(date_, 0);
                TagLib::StringList stringList = TagLib::StringList();
                stringList.append(date);
                properties.replace("DATE", stringList);
                env->ReleaseStringUTFChars(date_, date);
            }

            if (track_) {
                int track = env->CallIntMethod(track_, intGetValue);
                std::string trackString = std::to_string(track);

                if (trackTotal_) {
                    int trackTotal = env->CallIntMethod(trackTotal_, intGetValue);
                    trackString += "/";
                    trackString += std::to_string(trackTotal);
                }

                TagLib::StringList stringList = TagLib::StringList();
                stringList.append(trackString);
                properties.replace("TRACKNUMBER", stringList);
            }

            if (disc_) {
                int disc = env->CallIntMethod(disc_, intGetValue);
                std::string discString = std::to_string(disc);

                if (discTotal_) {
                    int discTotal = env->CallIntMethod(discTotal_, intGetValue);
                    discString += "/";
                    discString += std::to_string(discTotal);
                }

                TagLib::StringList stringList = TagLib::StringList();
                stringList.append(discString);
                properties.replace("DISCNUMBER", stringList);
            }

            if (genre_) {
                const char *genre = env->GetStringUTFChars(genre_, 0);
                TagLib::StringList stringList = TagLib::StringList();
                stringList.append(genre);
                properties.replace("GENRE", stringList);
                env->ReleaseStringUTFChars(genre_, genre);
            }

            if (!properties.isEmpty()) {
                tag->setProperties(properties);
            }
        }

        saved = fileRef.save();
    }

    uniqueFd.release();
    return saved;
}

extern "C" JNIEXPORT jbyteArray JNICALL Java_com_simplecityapps_ktaglib_KTagLib_getArtwork(JNIEnv *env, jobject instance, jint fd_) {

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

extern "C"
JNIEXPORT jobject JNICALL
Java_com_simplecityapps_ktaglib_TaglibUtils_getMetadata(JNIEnv *env, jobject thiz, jint file_descriptor, jstring file_path, jstring file_name) {
    unique_fd uniqueFd = unique_fd(file_descriptor);

    TagLib::IOStream *stream = new TagLib::FileStream(uniqueFd.get(), true);
    TagLib::FileRef fileRef(stream);

    jobject properties = env->NewObject(globalHashMapClass, hashMapInit);

    if (fileRef.isValid()) {
        TagLib::PropertyMap taglibProperties = fileRef.properties();
        for (auto & taglibProperty : taglibProperties)
            if (!taglibProperty.second.isEmpty()) {
                jstring key = env->NewStringUTF(taglibProperty.first.toCString(true));
                jstring value = env->NewStringUTF(taglibProperty.second.front().toCString(true));
                env->CallObjectMethod(properties, addProperty, key, value);
            }
    }

    uniqueFd.release();

    return properties;
}