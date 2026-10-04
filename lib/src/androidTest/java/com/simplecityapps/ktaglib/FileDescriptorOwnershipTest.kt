package com.simplecityapps.ktaglib

import android.os.ParcelFileDescriptor
import android.system.Os
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import kotlin.random.Random

/**
 * timusus/KTagLib#16: the caller keeps ownership of the file descriptor it passes in. Native code
 * works on (and closes) only its own duplicate, on success and failure paths alike.
 */
@RunWith(AndroidJUnit4::class)
class FileDescriptorOwnershipTest {

    private val kTagLib = KTagLib()

    private val flac: File get() = TagLibCorpus.copy("silence.flac", dir = "")

    private val garbage: File
        get() = File(flac.parentFile, "garbage.flac").apply { writeBytes(Random(16).nextBytes(4096)) }

    @Test
    fun callerFdStaysOpenAfterEachCall() {
        val file = flac
        ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_WRITE).use { pfd ->
            assertNotNull(kTagLib.getMetadata(pfd.fd, file.name))
            assertStillOpen(pfd)
            kTagLib.getArtwork(pfd.fd, file.name)
            assertStillOpen(pfd)
            assertTrue(kTagLib.writeMetadata(pfd.fd, mapOf("TITLE" to listOf("Title")), file.name))
            assertStillOpen(pfd)
            assertEquals(listOf("Title"), kTagLib.getMetadata(pfd.fd, file.name)?.propertyMap?.get("TITLE"))
            assertStillOpen(pfd)
        }
    }

    @Test
    fun callerFdStaysOpenAfterFailedCalls() {
        val file = garbage
        ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_WRITE).use { pfd ->
            assertNull(kTagLib.getMetadata(pfd.fd, file.name))
            assertStillOpen(pfd)
            assertNull(kTagLib.getArtwork(pfd.fd, file.name))
            assertStillOpen(pfd)
            assertFalse(kTagLib.writeMetadata(pfd.fd, mapOf("TITLE" to listOf("Title")), file.name))
            assertStillOpen(pfd)
        }
    }

    @Test
    fun invalidFdReturnsNull() {
        assertNull(kTagLib.getMetadata(-1, "silence.flac"))
        assertNull(kTagLib.getArtwork(-1, "silence.flac"))
        assertFalse(kTagLib.writeMetadata(-1, mapOf("TITLE" to listOf("Title")), "silence.flac"))
    }

    @Test
    fun repeatedCallsDoNotLeakFds() {
        val file = flac
        val garbage = garbage
        val before = openFdCount()
        repeat(2000) { i ->
            when (i % 5) {
                0 -> ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
                    assertNotNull(kTagLib.getMetadata(pfd.fd, file.name))
                }
                1 -> ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
                    kTagLib.getArtwork(pfd.fd, file.name)
                }
                // A read-only fd can't be written, so the save fails after the stream is open.
                2 -> ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
                    assertFalse(kTagLib.writeMetadata(pfd.fd, mapOf("TITLE" to listOf("Title")), file.name))
                }
                // Unrecognised file: the stream opens, FileRef is null.
                3 -> ParcelFileDescriptor.open(garbage, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
                    assertNull(kTagLib.getMetadata(pfd.fd, garbage.name))
                }
                // A write-only fd can't be fdopen()ed for reading, so the stream never opens.
                4 -> ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_WRITE_ONLY).use { pfd ->
                    assertNull(kTagLib.getMetadata(pfd.fd, file.name))
                }
            }
        }
        val after = openFdCount()
        // A little slack for fds other threads open in the meantime; a per-call leak adds hundreds.
        assertTrue("open fds grew from $before to $after over 2000 calls", after - before < 20)
    }

    private fun assertStillOpen(pfd: ParcelFileDescriptor) {
        Os.fstat(pfd.fileDescriptor)
        val magic = ByteArray(4)
        assertEquals(4, Os.pread(pfd.fileDescriptor, magic, 0, magic.size, 0))
    }

    private fun openFdCount() = requireNotNull(File("/proc/self/fd").list()).size
}
