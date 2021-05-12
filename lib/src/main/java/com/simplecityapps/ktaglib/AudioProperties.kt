package com.simplecityapps.ktaglib

import androidx.annotation.Keep

/**
 * A set of audio properties associated with a file
 *
 * @param duration the audio duration, in milliseconds
 * @param bitrate the bitrate in kb/s. For variable bitrate formats, this is either the average or nominal bitrate
 * @param sampleRate the sample rate in hz
 * @param channelCount the number of audio channels
 *
 * @see <a href="https://taglib.org/api/classTagLib_1_1AudioProperties.html">TagLib AudioProperties</a>
 */
@Keep
data class AudioProperties(
    val duration: Int,
    val bitrate: Int,
    val sampleRate: Int,
    val channelCount: Int
)