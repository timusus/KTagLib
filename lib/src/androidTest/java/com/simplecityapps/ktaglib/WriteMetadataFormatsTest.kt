package com.simplecityapps.ktaglib

import android.os.ParcelFileDescriptor
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.Parameterized
import java.io.File

/**
 * The writeMetadata contract, checked for every supported container: a key with values replaces
 * the field wholesale, a key with an empty list removes it, and keys not passed are untouched.
 */
@RunWith(Parameterized::class)
class WriteMetadataFormatsTest(private val asset: String) {

    companion object {
        @JvmStatic
        @Parameterized.Parameters(name = "{0}")
        fun assets() = listOf("silence.mp3", "silence.flac", "silence.m4a", "silence.ogg", "silence.opus", "untagged.wav")
    }

    private val kTagLib = KTagLib()

    @Test
    fun emptyListRemovesTheField() {
        val file = copyAsset(asset)

        write(file, mapOf("TITLE" to listOf("Some Title"), "ARTIST" to listOf("Kept Artist")))
        assertEquals(listOf("Some Title"), read(file)["TITLE"])

        write(file, mapOf("TITLE" to emptyList()))

        val properties = read(file)
        assertFalse("TITLE should be removed from $asset, was ${properties["TITLE"]}", properties.containsKey("TITLE"))
        assertEquals("ARTIST in $asset", listOf("Kept Artist"), properties["ARTIST"])
    }

    @Test
    fun valuesReplaceTheFieldWholesale() {
        val file = copyAsset(asset)

        write(file, mapOf("GENRE" to listOf("Rock", "Pop"), "ARTIST" to listOf("Kept Artist")))
        assertEquals(listOf("Rock", "Pop"), read(file)["GENRE"])

        write(file, mapOf("GENRE" to listOf("Jazz")))

        val properties = read(file)
        assertEquals("GENRE in $asset", listOf("Jazz"), properties["GENRE"])
        assertEquals("ARTIST in $asset", listOf("Kept Artist"), properties["ARTIST"])
    }

    private fun write(file: File, properties: Map<String, List<String>>) {
        val written = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_WRITE).use { pfd ->
            kTagLib.writeMetadata(pfd.detachFd(), properties, file.name)
        }
        assertTrue("writeMetadata failed for ${file.name}", written)
    }

    private fun read(file: File): Map<String, List<String>> {
        val metadata = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
            kTagLib.getMetadata(pfd.detachFd(), file.name)
        }
        return requireNotNull(metadata) { "getMetadata returned null for ${file.name}" }.propertyMap
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
