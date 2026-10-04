package com.simplecityapps.ktaglib

import android.os.ParcelFileDescriptor
import androidx.test.ext.junit.runners.AndroidJUnit4
import com.simplecityapps.ktaglib.TagLibCorpus.metadata
import com.simplecityapps.ktaglib.TagLibCorpus.write
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File

/**
 * timusus/KTagLib#16: getMetadata(fd, filename, keys) returns only the requested properties, and
 * everything else exactly as the unfiltered getMetadata does.
 */
@RunWith(AndroidJUnit4::class)
class MetadataKeysTest {

    private val kTagLib = KTagLib()

    @Test
    fun returnsOnlyRequestedKeys() {
        val file = TagLibCorpus.copy("silence.flac", dir = "")
        assertTrue(kTagLib.write(file, mapOf("TITLE" to listOf("Title"), "ARTIST" to listOf("Artist"), "ALBUM" to listOf("Album"))))

        val all = kTagLib.metadata(file)!!
        val filtered = read(file, setOf("title", "ALBUM", "NOT_PRESENT"))!!

        assertEquals(mapOf("TITLE" to listOf("Title"), "ALBUM" to listOf("Album")), filtered.propertyMap)
        assertEquals(all.audioProperties, filtered.audioProperties)
        assertTrue(all.propertyMap.keys.containsAll(listOf("TITLE", "ARTIST", "ALBUM")))
    }

    @Test
    fun emptyKeysReturnsEmptyPropertyMap() {
        val file = TagLibCorpus.copy("silence.flac", dir = "")
        val metadata = read(file, emptySet())!!
        assertTrue(metadata.propertyMap.isEmpty())
        assertEquals(kTagLib.metadata(file)!!.audioProperties, metadata.audioProperties)
    }

    @Test
    fun matroskaSegmentTitleFallbackFollowsKeys() {
        val file = TagLibCorpus.copy("segment-title.mka", dir = "")
        assertEquals(listOf("Segment Only Title"), read(file, setOf("TITLE"))!!.propertyMap["TITLE"])
        assertTrue(read(file, setOf("ARTIST"))!!.propertyMap.isEmpty())
    }

    private fun read(file: File, keys: Set<String>): Metadata? =
        ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
            kTagLib.getMetadata(pfd.fd, file.name, keys)
        }
}
