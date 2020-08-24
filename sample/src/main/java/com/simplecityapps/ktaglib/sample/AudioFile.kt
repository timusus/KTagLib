package com.simplecityapps.ktaglib.sample

import android.util.Log
import com.simplecityapps.ktaglib.KTagLib

/**
 * A ready to use data model with some basic fields for representing
 * various metadata fields of an audio file. If this data class is
 * insufficient to meet your needs, directly use [KTagLib.getMetadata]
 * to obtain a HashMap of all properties and write a mapping function
 * to convert it to your model as done here.
 */
data class AudioFile(
        val path: String,
        var size: Long,
        var lastModified: Long,
        val title: String?,
        val albumArtist: String?,
        val artist: String?,
        val album: String?,
        val track: Int?,
        val trackTotal: Int?,
        val disc: Int?,
        val discTotal: Int?,
        val duration: Int?,
        val date: String?,
        var genre: String?
) {
    companion object {
        /**
         * Returns an [AudioFile] with fields populated by invoking [KTagLib.getMetadata] on
         * [fileDescriptor].
         *
         * @param fileDescriptor associated with the file whose properties are to be retrieved
         * @param filePath path to the file
         * @param fileName name of the file
         * @return AudioFile with fields populated from properties of the desired file
         */
        @JvmStatic
        fun getAudioFile(fileDescriptor: Int, filePath: String, fileName: String): AudioFile {
            val properties = KTagLib.getMetadata(fileDescriptor)
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
}
