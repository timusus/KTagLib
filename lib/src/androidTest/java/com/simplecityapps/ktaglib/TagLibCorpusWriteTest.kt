package com.simplecityapps.ktaglib

import com.simplecityapps.ktaglib.TagLibCorpus.metadata
import com.simplecityapps.ktaglib.TagLibCorpus.write
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.Parameterized

/**
 * Non-ASCII TITLE/ARTIST round trip for the writable formats in TagLib's corpus that
 * WriteMetadataFormatsTest (mp3, flac, m4a, ogg, opus, wav) doesn't cover.
 *
 * Not covered, because TagLib can't store these values:
 * - Shorten (2sec-silence.shn): read-only in TagLib, save() always fails.
 * - Tracker modules (test.mod, test.s3m, test.it, test.xm): the title is a fixed-width 8-bit
 *   field (non-ASCII text comes back mangled) and there is no ARTIST field.
 */
@RunWith(Parameterized::class)
class TagLibCorpusWriteTest(private val name: String) {

    companion object {
        @JvmStatic
        @Parameterized.Parameters(name = "{0}")
        fun files() = listOf(
            "empty.aiff",
            "mac-399.ape",
            "click.wv",
            "click.mpc",
            "empty.tta",
            "empty.spx",
            "empty_flac.oga",
            "silence-1.wma",
            "empty10ms.dsf",
            "empty10ms.dff",
            "empty1s.aac",
        )

        private const val TITLE = "Tïtlé 日本語 🎵"
        private const val ARTIST = "Ärtist Ωμέγα"
    }

    private val kTagLib = KTagLib()

    @Test
    fun nonAsciiTitleAndArtistRoundTrip() {
        val file = TagLibCorpus.copy(name)

        assertTrue("writeMetadata failed for $name", kTagLib.write(file, mapOf("TITLE" to listOf(TITLE), "ARTIST" to listOf(ARTIST))))

        val properties = requireNotNull(kTagLib.metadata(file)) { "getMetadata returned null for $name" }.propertyMap
        assertEquals("TITLE in $name", listOf(TITLE), properties["TITLE"])
        assertEquals("ARTIST in $name", listOf(ARTIST), properties["ARTIST"])
    }
}
