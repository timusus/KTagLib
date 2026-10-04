package com.simplecityapps.ktaglib

import android.os.ParcelFileDescriptor
import android.util.Log
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.Parameterized
import java.io.File
import kotlin.random.Random

/**
 * timusus/KTagLib#16: a truncated or garbage file must not crash the process or throw - the native
 * entry points catch C++ exceptions and return null / false instead.
 *
 * Empty files and random bytes are rejected outright for the formats with a magic number TagLib
 * checks (FLAC, MP4, Ogg Vorbis, Opus). TagLib's MPEG and RIFF/WAV parsers accept any bytes when
 * the filename hint picks them: getMetadata returns a (property-less) Metadata, and writeMetadata
 * succeeds by adding a tag to the file. So for .mp3 and .wav, and for every truncated file (one
 * whose header survives is still parseable), only "no crash, no exception" is asserted. Every
 * outcome is logged under [LOG_TAG].
 */
@RunWith(Parameterized::class)
class MalformedFileTest(private val asset: String) {

    companion object {
        private const val LOG_TAG = "MalformedFileTest"

        @JvmStatic
        @Parameterized.Parameters(name = "{0}")
        fun assets() = listOf("silence.mp3", "silence.flac", "silence.m4a", "silence.ogg", "silence.opus", "untagged.wav")

        /** Extensions whose TagLib parser accepts any bytes, so empty and garbage files aren't rejected. */
        private val LENIENT = setOf("mp3", "wav")
    }

    private val kTagLib = KTagLib()
    private val instrumentation get() = InstrumentationRegistry.getInstrumentation()
    private val extension get() = asset.substringAfterLast('.')

    @Test
    fun emptyFileDoesNotCrash() {
        val outcome = exercise("empty.$extension", ByteArray(0))
        Log.i(LOG_TAG, "$asset empty: $outcome")
        assertNull("getArtwork of an empty .$extension", outcome.artwork)
        if (extension !in LENIENT) {
            assertNull("getMetadata of an empty .$extension", outcome.metadata)
            assertFalse("writeMetadata to an empty .$extension", outcome.written)
        }
    }

    @Test
    fun truncatedFilesDoNotCrash() {
        val original = instrumentation.context.assets.open(asset).use { it.readBytes() }
        for (length in listOf(16, 128, original.size / 2, original.size - 1)) {
            val outcome = exercise("truncated-$length.$extension", original.copyOf(length))
            Log.i(LOG_TAG, "$asset truncated $length/${original.size}: $outcome")
        }
    }

    @Test
    fun garbageFilesDoNotCrash() {
        val random = Random(asset.hashCode())
        for (length in listOf(16, 128, 4096, 65536)) {
            val outcome = exercise("garbage-$length.$extension", random.nextBytes(length))
            Log.i(LOG_TAG, "$asset garbage $length: $outcome")
            assertNull("getArtwork of $length garbage bytes as .$extension", outcome.artwork)
            if (extension !in LENIENT) {
                assertNull("getMetadata of $length garbage bytes as .$extension", outcome.metadata)
                assertFalse("writeMetadata to $length garbage bytes as .$extension", outcome.written)
            }
        }
    }

    private data class Outcome(val metadata: Metadata?, val artwork: ByteArray?, val written: Boolean) {
        override fun toString() = "metadata=${metadata != null}, artwork=${artwork != null}, written=$written"
    }

    /** Runs [bytes], saved under [name], through getMetadata, getArtwork and writeMetadata. */
    private fun exercise(name: String, bytes: ByteArray): Outcome {
        val file = File(File(instrumentation.targetContext.cacheDir, "malformed").apply { mkdirs() }, name)
        file.writeBytes(bytes)
        val metadata = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
            kTagLib.getMetadata(pfd.detachFd(), file.name)
        }
        val artwork = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
            kTagLib.getArtwork(pfd.detachFd(), file.name)
        }
        val written = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_WRITE).use { pfd ->
            kTagLib.writeMetadata(pfd.detachFd(), mapOf("TITLE" to listOf("Title")), file.name)
        }
        return Outcome(metadata, artwork, written)
    }
}
