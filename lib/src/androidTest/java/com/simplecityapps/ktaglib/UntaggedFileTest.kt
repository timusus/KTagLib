package com.simplecityapps.ktaglib

import android.os.ParcelFileDescriptor
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File

/**
 * Covers timusus/KTagLib#9: getMetadata must return audio properties for a recognized file even
 * when it has no tag data, rather than returning null.
 *
 * untagged.wav has no LIST/INFO or ID3 chunk (generated with ffmpeg -map_metadata -1
 * -fflags +bitexact), but TagLib's RIFF::WAV::File::tag() always returns a non-null (empty)
 * combined tag rather than null - every TagLib 2.x File subclass does the same, so
 * FileRef::tag() cannot actually be observed as null for a recognized file. This test exercises
 * the empty-tag fallback path instead.
 */
@RunWith(AndroidJUnit4::class)
class UntaggedFileTest {

    private val kTagLib = KTagLib()

    @Test
    fun untaggedFileReturnsAudioPropertiesWithEmptyPropertyMap() {
        val file = copyAsset("untagged.wav")

        val metadata = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
            kTagLib.getMetadata(pfd.detachFd(), file.name)
        }

        assertNotNull("getMetadata returned null for untagged file", metadata)
        assertTrue("propertyMap should be empty for untagged file", metadata!!.propertyMap.isEmpty())
        assertNotNull("audioProperties should be present for untagged file", metadata.audioProperties)
        assertTrue("duration should be positive", metadata.audioProperties!!.duration > 0)
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
