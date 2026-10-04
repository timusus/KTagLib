package com.simplecityapps.ktaglib

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class AudioPropertiesTest {
    @Test
    fun olderConstructorsDefaultCodecToNull() {
        assertNull(AudioProperties(1, 2, 3, 4).codec) // 2.0 shape
        assertNull(AudioProperties(1, 2, 3, 4, 16).codec) // 2.1 shape
        assertEquals(0, AudioProperties(1, 2, 3, 4).bitsPerSample)
    }
}
