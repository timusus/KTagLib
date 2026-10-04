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
 * Shuttle2 #523: TagLib's Matroska property map omits the Segment Info title, so KTagLib adds it
 * as TITLE when the file has no other title.
 */
@RunWith(AndroidJUnit4::class)
class MatroskaSegmentTitleTest {

    private val kTagLib = KTagLib()

    // segment-title.mka has only a Segment Info title ("Segment Only Title"), no track or tag-level TITLE
    @Test
    fun matroskaSegmentTitleFallsBackToTitle() {
        val metadata = read("segment-title.mka")
        assertEquals(listOf("Segment Only Title"), metadata.propertyMap["TITLE"])
        assertEquals("flac", metadata.audioProperties!!.codec)
    }

    private fun read(name: String): Metadata {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val file = File(instrumentation.targetContext.cacheDir, name)
        instrumentation.context.assets.open(name).use { input ->
            file.outputStream().use { output -> input.copyTo(output) }
        }
        val metadata = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
            kTagLib.getMetadata(pfd.detachFd(), file.name)
        }
        assertNotNull("getMetadata returned null for $name", metadata)
        return metadata!!
    }
}
