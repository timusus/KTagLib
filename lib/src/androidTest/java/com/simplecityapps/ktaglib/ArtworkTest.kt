package com.simplecityapps.ktaglib

import android.os.ParcelFileDescriptor
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertNotNull
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File

/**
 * Covers timusus/KTagLib#7: getArtwork must return the largest embedded picture. The fixtures embed
 * three pictures in non-monotonic size order (largest, smallest, middle) so a "track the last size
 * seen" bug returns the wrong one.
 */
@RunWith(AndroidJUnit4::class)
class ArtworkTest {

    private val kTagLib = KTagLib()

    @Test
    fun flacReturnsLargestPicture() = assertLargestPictureReturned("artwork.flac")

    @Test
    fun opusReturnsLargestPicture() = assertLargestPictureReturned("artwork.opus")

    private fun assertLargestPictureReturned(asset: String) {
        val file = copyAsset(asset)

        val artwork = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
            kTagLib.getArtwork(pfd.detachFd(), file.name)
        }
        assertNotNull("getArtwork returned null for $asset", artwork)
        assertArrayEquals("largest picture in $asset", LARGEST_PICTURE, artwork)
    }

    private fun copyAsset(name: String): File {
        val instrumentation = InstrumentationRegistry.getInstrumentation()
        val file = File(instrumentation.targetContext.cacheDir, name)
        instrumentation.context.assets.open(name).use { input ->
            file.outputStream().use { output -> input.copyTo(output) }
        }
        return file
    }

    companion object {
        // The largest of the three pictures embedded in artwork.flac / artwork.opus (embedded in the
        // order: largest, smallest, middle), read once and compared by content.
        private val LARGEST_PICTURE: ByteArray by lazy {
            InstrumentationRegistry.getInstrumentation().context.assets.open("pic_large.png").use { it.readBytes() }
        }
    }
}
