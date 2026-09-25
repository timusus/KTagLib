package com.simplecityapps.ktaglib

import android.os.ParcelFileDescriptor
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File

/**
 * Writes non-ASCII tag values and reads them back, for each supported container.
 *
 * Covers timusus/Shuttle2#388: values used to be passed to TagLib as Latin-1, so `écrit` was saved
 * as `Ã©crit`, and characters outside the BMP (emoji) were mangled by JNI's modified UTF-8.
 */
@RunWith(AndroidJUnit4::class)
class UnicodeRoundTripTest {

    private val kTagLib = KTagLib()

    private val values = mapOf(
        "TITLE" to "écrit",
        "ARTIST" to "日本語",
        "ALBUM" to "Ελληνικά",
        "GENRE" to "Rock 🎸 😀",
    )

    @Test
    fun mp3() = assertRoundTrip("silence.mp3")

    @Test
    fun flac() = assertRoundTrip("silence.flac")

    @Test
    fun m4a() = assertRoundTrip("silence.m4a")

    @Test
    fun oggVorbis() = assertRoundTrip("silence.ogg")

    @Test
    fun oggOpus() = assertRoundTrip("silence.opus")

    private fun assertRoundTrip(asset: String) {
        val file = copyAsset(asset)

        val properties = HashMap<String, ArrayList<String?>>()
        values.forEach { (key, value) -> properties[key] = arrayListOf(value) }

        val written = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_WRITE).use { pfd ->
            kTagLib.writeMetadata(pfd.detachFd(), properties, file.name)
        }
        assertTrue("writeMetadata failed for $asset", written)

        val metadata = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
            kTagLib.getMetadata(pfd.detachFd(), file.name)
        }
        assertNotNull("getMetadata returned null for $asset", metadata)

        values.forEach { (key, value) ->
            assertEquals("$key in $asset", listOf(value), metadata!!.propertyMap[key])
        }
    }

    private fun copyAsset(name: String): File {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val file = File(instrumentation.targetContext.cacheDir, name)
        instrumentation.context.assets.open(name).use { input ->
            file.outputStream().use { output -> input.copyTo(output) }
        }
        return file
    }
}
