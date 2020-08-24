
#include <jni.h>
#include <sys/stat.h>
#include <fileref.h>
#include <flacfile.h>
#include <opusfile.h>
#include <xiphcomment.h>
#include <toolkit/tiostream.h>
#include <toolkit/tfilestream.h>
#include <toolkit/tpicture.h>
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
jmethodID songInit;

jclass globalIntClass;
jmethodID intInit;

jmethodID intGetValue;

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

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

    env->DeleteGlobalRef(globalSongClass);
    env->DeleteGlobalRef(globalIntClass);

    TagLib::setDebugListener(nullptr);
}

extern "C" JNIEXPORT jobject JNICALL Java_com_simplecityapps_ktaglib_KTagLib_getAudioFile(JNIEnv *env, jobject instance, jint fd_, jstring path, jstring fileName) {
    unique_fd uniqueFd = unique_fd(fd_);

    jobject audioFile = nullptr;

    std::unique_ptr<TagLib::IOStream> stream = std::make_unique<TagLib::FileStream>( uniqueFd.get( ), false);
    TagLib::FileRef fileRef(stream.get());

    if (!fileRef.isNull()) {

        jstring title = fileName;

        jstring artist = nullptr;
        jstring albumArtist = nullptr;
        jstring album = nullptr;
        jstring genre = nullptr;

        jobject track = nullptr;
        jobject trackTotal = nullptr;
        jobject disc = nullptr;
        jobject discTotal = nullptr;
        jobject duration = nullptr;
        jobject date = nullptr;

        if (fileRef.audioProperties()) {
            TagLib::AudioProperties *audioProperties = fileRef.audioProperties();
            duration = env->NewObject(globalIntClass, intInit, audioProperties->lengthInMilliseconds());
        }

        TagLib::Tag *tag = fileRef.tag();
        if (tag) {
            TagLib::PropertyMap properties = tag->properties();

            if (properties.contains("TITLE")) {
                const TagLib::StringList &stringList = properties["TITLE"];
                if (!stringList.isEmpty()) {
                    title = env->NewStringUTF(stringList.front().toCString(true));
                }
            }

            if (properties.contains("ARTIST")) {
                const TagLib::StringList &stringList = properties["ARTIST"];
                if (!stringList.isEmpty()) {
                    artist = env->NewStringUTF(stringList.front().toCString(true));
                }
            }

            if (properties.contains("ALBUMARTIST")) {
                const TagLib::StringList &stringList = properties["ALBUMARTIST"];
                if (!stringList.isEmpty()) {
                    albumArtist = env->NewStringUTF(stringList.front().toCString(true));
                }
            }

            if (properties.contains("ALBUM")) {
                const TagLib::StringList &stringList = properties["ALBUM"];
                if (!stringList.isEmpty()) {
                    album = env->NewStringUTF(stringList.front().toCString(true));
                }
            }

            if (properties.contains("TRACKNUMBER")) {
                const TagLib::StringList &stringList = properties["TRACKNUMBER"];
                if (!stringList.isEmpty()) {
                    track = env->NewObject(globalIntClass, intInit, stringList.front().toInt());
                }
            }

            if (properties.contains("DISCNUMBER")) {
                const TagLib::StringList &stringList = properties["DISCNUMBER"];
                if (!stringList.isEmpty()) {
                    disc = env->NewObject(globalIntClass, intInit, stringList.front().toInt());
                }
            }

            if (properties.contains("DATE")) {
                const TagLib::StringList &stringList = properties["DATE"];
                if (!stringList.isEmpty()) {
                    date = env->NewStringUTF(stringList.front().toCString(true));
                }
            }

            if (properties.contains("GENRE")) {
                const TagLib::StringList &stringList = properties["GENRE"];
                if (!stringList.isEmpty()) {
                    genre = env->NewStringUTF(stringList.front().toCString(true));
                }
            }
        }

        struct stat statbuf{};
        fstat(uniqueFd.get(), &statbuf);
        long long lastModified = statbuf.st_mtime * 1000L;
        long long size = statbuf.st_size;

        audioFile = env->NewObject(
                globalSongClass,
                songInit,
                path,
                size,
                lastModified,
                title,
                albumArtist,
                artist,
                album,
                track,
                trackTotal,
                disc,
                discTotal,
                duration,
                date,
                genre
        );
    }

    uniqueFd.release();
    return audioFile;
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

    std::unique_ptr<TagLib::IOStream> stream = std::make_unique<TagLib::FileStream>( uniqueFd.get( ), false);
    TagLib::FileRef fileRef(stream.get());

    bool saved = false;

    if (!fileRef.isNull()) {
        TagLib::Tag *tag = fileRef.tag();
        if (tag) {

            TagLib::PropertyMap properties = fileRef.properties();

            if (title_) {
                const char *title = env->GetStringUTFChars(title_, 0);
                properties.replace("TITLE",  TagLib::String(title));
                env->ReleaseStringUTFChars(title_, title);
            }

            if (artist_) {
                const char *artist = env->GetStringUTFChars(artist_, 0);
                properties.replace("ARTIST", TagLib::String(artist));
                env->ReleaseStringUTFChars(artist_, artist);
            }

            if (album_) {
                const char *album = env->GetStringUTFChars(album_, 0);
                properties.replace("ALBUM", TagLib::String(album));
                env->ReleaseStringUTFChars(album_, album);
            }

            if (albumArtist_) {
                const char *albumArtist = env->GetStringUTFChars(albumArtist_, 0);
                properties.replace("ALBUMARTIST", TagLib::String(albumArtist));
                env->ReleaseStringUTFChars(albumArtist_, albumArtist);
            }

            if (date_) {
                const char *date = env->GetStringUTFChars(date_, 0);
                properties.replace("DATE", TagLib::String(date));
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

                properties.replace("TRACKNUMBER",  TagLib::String(trackString));
            }

            if (disc_) {
                int disc = env->CallIntMethod(disc_, intGetValue);
                std::string discString = std::to_string(disc);

                if (discTotal_) {
                    int discTotal = env->CallIntMethod(discTotal_, intGetValue);
                    discString += "/";
                    discString += std::to_string(discTotal);
                }

                properties.replace("DISCNUMBER", TagLib::String(discString));
            }

            if (genre_) {
                const char *genre = env->GetStringUTFChars(genre_, 0);
                properties.replace("GENRE", TagLib::String(genre));
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

    std::unique_ptr<TagLib::IOStream> stream = std::make_unique<TagLib::FileStream>( uniqueFd.get( ), false);
    TagLib::FileRef fileRef(stream.get());

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
