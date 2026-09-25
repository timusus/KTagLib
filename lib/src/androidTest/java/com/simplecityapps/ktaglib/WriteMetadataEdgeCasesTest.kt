package com.simplecityapps.ktaglib

import android.os.ParcelFileDescriptor
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File

/**
 * Covers timusus/KTagLib#10: writeMetadata must skip a null property key (rather than writing it
 * under an empty-string key), and an empty value list for a key must remove that field from the
 * tag rather than leaving it untouched or writing an empty placeholder value.
 */
@RunWith(AndroidJUnit4::class)
class WriteMetadataEdgeCasesTest {

    private val kTagLib = KTagLib()

    @Test
    fun emptyListRemovesFieldFromFlacTag() {
        val file = copyAsset("silence.flac")

        write(file, mapOf("TITLE" to arrayListOf("Some Title")))
        assertEquals(listOf("Some Title"), read(file).propertyMap["TITLE"])

        write(file, mapOf("TITLE" to arrayListOf()))
        assertFalse("TITLE should be removed after writing an empty list", read(file).propertyMap.containsKey("TITLE"))
    }

    @Test
    fun nullValuesWithinAListAreIgnored() {
        val file = copyAsset("silence.flac")

        write(file, mapOf("GENRE" to arrayListOf("Rock", null, "Pop")))

        assertEquals(listOf("Rock", "Pop"), read(file).propertyMap["GENRE"])
    }

    @Test
    fun nullKeyIsSkippedAndOtherPropertiesAreStillWritten() {
        val file = copyAsset("silence.flac")

        @Suppress("UNCHECKED_CAST")
        val properties = HashMap<Any?, Any?>().apply {
            put("TITLE", arrayListOf("Kept Title"))
            put(null, arrayListOf("Should be skipped"))
        } as HashMap<String, ArrayList<String?>>

        val written = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_WRITE).use { pfd ->
            kTagLib.writeMetadata(pfd.detachFd(), properties, file.name)
        }
        assertTrue("writeMetadata failed", written)

        assertEquals(listOf("Kept Title"), read(file).propertyMap["TITLE"])
    }

    private fun write(file: File, properties: Map<String, ArrayList<String?>>) {
        val written = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_WRITE).use { pfd ->
            kTagLib.writeMetadata(pfd.detachFd(), HashMap(properties), file.name)
        }
        assertTrue("writeMetadata failed", written)
    }

    private fun read(file: File): Metadata {
        val metadata = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
            kTagLib.getMetadata(pfd.detachFd(), file.name)
        }
        return requireNotNull(metadata) { "getMetadata returned null for ${file.name}" }
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
