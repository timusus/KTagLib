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
 *
 * @see <a href="https://taglib.org/api/classTagLib_1_1AudioProperties.html">TagLib AudioProperties</a>
 */
@Keep
data class AudioProperties @JvmOverloads constructor(
    val duration: Int,
    val bitrate: Int,
    val sampleRate: Int,
    val channelCount: Int,
    val bitsPerSample: Int = 0
) {
    /**
     * The 2.0 `copy`, kept so that code compiled against 2.0 still links. Hidden from source, where the generated
     * `copy` (which also takes [bitsPerSample]) is used instead.
     */
    @Deprecated("Kept for binary compatibility", level = DeprecationLevel.HIDDEN)
    fun copy(
        duration: Int = this.duration,
        bitrate: Int = this.bitrate,
        sampleRate: Int = this.sampleRate,
        channelCount: Int = this.channelCount
    ): AudioProperties = AudioProperties(duration, bitrate, sampleRate, channelCount, bitsPerSample)
}
