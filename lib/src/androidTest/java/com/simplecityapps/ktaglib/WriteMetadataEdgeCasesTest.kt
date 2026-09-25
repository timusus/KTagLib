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
 * Edge cases of writeMetadata's input handling. The Kotlin signature rules out null keys and values,
 * but Java callers can still pass them: a null key or a null value list is skipped (timusus/KTagLib#10)
 * and a null element within a list is dropped. Any Map/List implementation is accepted, not only
 * HashMap/ArrayList.
 */
@RunWith(AndroidJUnit4::class)
class WriteMetadataEdgeCasesTest {

    private val kTagLib = KTagLib()

    @Test
    fun anyMapAndListImplementationIsAccepted() {
        val file = copyAsset("silence.flac")

        write(file, sortedMapOf("TITLE" to java.util.LinkedList(listOf("Linked")), "ARTIST" to listOf("Single")))

        val properties = read(file).propertyMap
        assertEquals(listOf("Linked"), properties["TITLE"])
        assertEquals(listOf("Single"), properties["ARTIST"])
    }

    @Test
    fun nullValuesWithinAListAreIgnored() {
        val file = copyAsset("silence.flac")

        write(file, javaMap("GENRE" to arrayListOf("Rock", null, "Pop")))

        assertEquals(listOf("Rock", "Pop"), read(file).propertyMap["GENRE"])
    }

    @Test
    fun nullKeyIsSkippedAndOtherPropertiesAreStillWritten() {
        val file = copyAsset("silence.flac")

        write(file, javaMap("TITLE" to arrayListOf("Kept Title"), null to arrayListOf("Should be skipped")))

        val properties = read(file).propertyMap
        assertEquals(listOf("Kept Title"), properties["TITLE"])
        assertFalse("an empty key should not be written", properties.containsKey(""))
    }

    @Test
    fun nullValueListIsSkippedAndLeavesTheFieldUntouched() {
        val file = copyAsset("silence.flac")
        write(file, mapOf("TITLE" to listOf("Original")))

        write(file, javaMap("TITLE" to null, "ARTIST" to arrayListOf("Written")))

        val properties = read(file).propertyMap
        assertEquals(listOf("Original"), properties["TITLE"])
        assertEquals(listOf("Written"), properties["ARTIST"])
    }

    // Builds the kind of map a Java caller could pass, with nulls the Kotlin types forbid.
    @Suppress("UNCHECKED_CAST")
    private fun javaMap(vararg entries: Pair<String?, List<String?>?>): Map<String, List<String>> = HashMap<String?, List<String?>?>().apply { entries.forEach { (key, value) -> put(key, value) } } as Map<String, List<String>>

    private fun write(file: File, properties: Map<String, List<String>>) {
        val written = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_WRITE).use { pfd ->
            kTagLib.writeMetadata(pfd.detachFd(), properties, file.name)
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
