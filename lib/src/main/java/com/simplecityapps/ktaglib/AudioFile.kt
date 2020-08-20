package com.simplecityapps.ktaglib

/**
 * A ready to use data model with some basic fields for representing
 * various metadata fields of an audio file. If this data class is
 * insufficient to meet your needs, directly use [KTagLib.getMetadata]
 * to obtain a HashMap of all properties and write a mapping function
 * to convert it to your model. For an example, see the source of
 * [KTagLib.getAudioFile].
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
)
