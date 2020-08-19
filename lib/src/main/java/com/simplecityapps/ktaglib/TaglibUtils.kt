package com.simplecityapps.ktaglib

import android.util.Log
import java.util.HashMap

object TaglibUtils {
    private external fun getMetadata(fileDescriptor: Int, filePath: String, fileName: String) : HashMap<String, String>

    external fun writeMetadata(fileDescriptor: Int, properties : HashMap<String, String>) : Boolean

    fun getAudioFile(fileDescriptor: Int, filePath: String, fileName: String) : AudioFile {
        val properties = getMetadata(fileDescriptor, filePath, fileName)
        Log.d("Metadata Properties", properties.toString())
        return AudioFile(
            filePath,
            properties["SIZE"]?.toLong() ?: 0,
            properties["LAST_MODIFIED"]?.toLong() ?: 0,
            properties["TITLE"],
            properties["ALBUMARTIST"],
            properties["ARTIST"],
            properties["ALBUM"],
            properties["TRACK"]?.toInt(),
            properties["TRACKTOTAL"]?.toInt(),
            properties["DISC"]?.toInt(),
            properties["DISCTOTAL"]?.toInt(),
            properties["DURATION"]?.toInt(),
            properties["DATE"],
            properties["GENRE"]
        )
    }
}