package com.simplecityapps.ktaglib

import androidx.annotation.Keep

/**
 * The metadata associated with an audio file
 *
 * @param propertyMap a [Map<String, List<String] of audio metadata tags.
 * @param audioProperties [AudioProperties] associated with an audio file, or null if none are found
 *
 * @see <a href="https://taglib.org/api/classTagLib_1_1PropertyMap.html">TagLib PropertyMap</a>
 *
 */
@Keep
data class Metadata(val propertyMap: Map<String, List<String>>, val audioProperties: AudioProperties?)