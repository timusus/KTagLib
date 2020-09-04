package com.simplecityapps.ktaglib.sample

import com.simplecityapps.ktaglib.KTagLib
import java.util.*

/**
 * A data model with some basic fields for representing various metadata fields of an audio file.
 * If this data class is insufficient to meet your needs, directly use [KTagLib.getMetadata] to obtain a HashMap of all properties and write a mapping function to convert it to your model as done here.
 */
data class AudioFile(
    val path: String,
    var size: Long,
    var lastModified: Date,
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
         * @param fileDescriptor associated with the file whose properties are to be retrieved
         * @param filePath path to the file
         * @param fileName name of the file
         * @return an AudioFile populated with various metadata retrieved from the tags of the file located at fileDescriptor, or null if metadata can't be retrieved
         */
        @JvmStatic
        fun getAudioFile(fileDescriptor: Int, filePath: String, fileName: String, lastModified: Long, size: Long): AudioFile? {
            return KTagLib.getMetadata(fileDescriptor)?.let { metadata ->
                AudioFile(
                    filePath,
                    size,
                    Date(lastModified),
                    metadata.propertyMap["TITLE"]?.firstOrNull() ?: fileName,
                    metadata.propertyMap["ALBUMARTIST"]?.firstOrNull(),
                    metadata.propertyMap["ARTIST"]?.firstOrNull(),
                    metadata.propertyMap["ALBUM"]?.firstOrNull(),
                    metadata.propertyMap["TRACKNUMBER"]?.firstOrNull()?.substringBefore('/')?.toIntOrNull(),
                    metadata.propertyMap["TRACKNUMBER"]?.firstOrNull()?.substringAfter('/', "")?.toIntOrNull(),
                    metadata.propertyMap["DISCNUMBER"]?.firstOrNull()?.substringBefore('/')?.toIntOrNull(),
                    metadata.propertyMap["DISCNUMBER"]?.firstOrNull()?.substringAfter('/', "")?.toIntOrNull(),
                    metadata.audioProperties?.duration,
                    metadata.propertyMap["DATE"]?.firstOrNull(),
                    metadata.propertyMap["GENRE"]?.firstOrNull()
                )
            }
        }
    }
}
