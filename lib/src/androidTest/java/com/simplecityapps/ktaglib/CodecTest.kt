package com.simplecityapps.ktaglib

import android.os.ParcelFileDescriptor
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File

/**
 * AudioProperties.codec for small generated fixtures (0.5 s of a 440 Hz sine, made with ffmpeg and
 * `-map_metadata -1`).
 */
@RunWith(AndroidJUnit4::class)
class CodecTest {

    private val kTagLib = KTagLib()

    @Test
    fun alacReportsCodecAndBitDepth() {
        val properties = read("alac24.m4a").audioProperties!!
        assertEquals("alac", properties.codec)
        assertEquals(24, properties.bitsPerSample) // generated as 24-bit ALAC (s32p, 8 kHz)
    }

    @Test
    fun aacReportsCodec() {
        assertEquals("aac", read("aac.m4a").audioProperties!!.codec)
    }

    @Test
    fun flacReportsCodec() {
        assertEquals("flac", read("codec.flac").audioProperties!!.codec)
    }

    @Test
    fun mp3ReportsCodec() {
        assertEquals("mp3", read("codec.mp3").audioProperties!!.codec)
    }

    private fun read(name: String): Metadata {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val file = File(instrumentation.targetContext.cacheDir, name)
        instrumentation.context.assets.open(name).use { input ->
            file.outputStream().use { output -> input.copyTo(output) }
        }
        val metadata = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
            kTagLib.getMetadata(pfd.fd, file.name)
        }
        assertNotNull("getMetadata returned null for $name", metadata)
        return metadata!!
    }
}
