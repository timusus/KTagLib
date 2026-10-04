package com.simplecityapps.ktaglib

import androidx.annotation.Keep

/**
 * A set of audio properties associated with a file
 *
 * @param duration the audio duration, in milliseconds
 * @param bitrate the bitrate in kb/s. For variable bitrate formats, this is either the average or nominal bitrate
 * @param sampleRate the sample rate in hz
 * @param channelCount the number of audio channels
 * @param bitsPerSample the number of bits per audio sample (bit depth) as stored in the file, or 0 if the format
 * doesn't store one (MP3, Ogg Vorbis, Opus, Musepack) or it isn't known. Lossy formats that do store one (MP4 AAC,
 * for example) report the container's nominal value, typically 16.
 * @param codec the audio codec (not the container, so an `.m4a` is `"aac"` or `"alac"`), as one of the lowercase names
 * below, or null if the codec is unknown or can't be detected:
 * `"aac"`, `"alac"`, `"ac3"`, `"eac3"`, `"dts"` (MP4 only, the latter three), `"flac"`, `"wav"`, `"aiff"`, `"ape"`,
 * `"wavpack"`, `"dsd"` (DSF and DSDIFF), `"mp1"`, `"mp2"`, `"mp3"`, `"vorbis"`, `"opus"`, `"speex"`, `"musepack"`,
 * `"tta"`, `"shorten"`, `"wma"`, `"wmapro"`, `"wmalossless"`. Matroska files report `"flac"`, `"aac"`, `"alac"`,
 * `"opus"`, `"vorbis"` or `"mp3"` when their track codec ID maps to one of those, otherwise null.
 *
 * @see <a href="https://taglib.org/api/classTagLib_1_1AudioProperties.html">TagLib AudioProperties</a>
 */
@Keep
data class AudioProperties @JvmOverloads constructor(
    val duration: Int,
    val bitrate: Int,
    val sampleRate: Int,
    val channelCount: Int,
    val bitsPerSample: Int = 0,
    val codec: String? = null
) {
    /**
     * The 2.1 default-argument constructor bridge, kept so that Kotlin code compiled against 2.1 (which calls
     * it when omitting [bitsPerSample]) still links. [mask] bit 4 means [bitsPerSample] was omitted.
     */
    @Deprecated("Kept for binary compatibility", level = DeprecationLevel.HIDDEN)
    @Suppress("UNUSED_PARAMETER")
    constructor(
        duration: Int,
        bitrate: Int,
        sampleRate: Int,
        channelCount: Int,
        bitsPerSample: Int,
        mask: Int,
        marker: kotlin.jvm.internal.DefaultConstructorMarker?
    ) : this(duration, bitrate, sampleRate, channelCount, if (mask and 16 != 0) 0 else bitsPerSample, null)

    /**
     * The 2.1 `copy`, kept so that code compiled against 2.1 still links. Hidden from source, where the generated
     * `copy` (which also takes [codec]) is used instead.
     */
    @Deprecated("Kept for binary compatibility", level = DeprecationLevel.HIDDEN)
    fun copy(
        duration: Int = this.duration,
        bitrate: Int = this.bitrate,
        sampleRate: Int = this.sampleRate,
        channelCount: Int = this.channelCount,
        bitsPerSample: Int = this.bitsPerSample
    ): AudioProperties = AudioProperties(duration, bitrate, sampleRate, channelCount, bitsPerSample, codec)

    /**
     * The 2.0 `copy`, kept so that code compiled against 2.0 still links. Hidden from source, where the generated
     * `copy` (which also takes [bitsPerSample] and [codec]) is used instead.
     */
    @Deprecated("Kept for binary compatibility", level = DeprecationLevel.HIDDEN)
    fun copy(
        duration: Int = this.duration,
        bitrate: Int = this.bitrate,
        sampleRate: Int = this.sampleRate,
        channelCount: Int = this.channelCount
    ): AudioProperties = AudioProperties(duration, bitrate, sampleRate, channelCount, bitsPerSample, codec)
}
