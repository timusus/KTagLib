package com.simplecityapps.ktaglib

import com.simplecityapps.ktaglib.TagLibCorpus.artwork
import com.simplecityapps.ktaglib.TagLibCorpus.write
import org.junit.Assert.assertArrayEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.Parameterized

/**
 * writeMetadata must not drop embedded pictures: for every corpus file (plus the artwork.flac /
 * artwork.opus fixtures) that has artwork, write TITLE and check getArtwork returns the same bytes.
 * Files without artwork, or that TagLib can't save, pass trivially. They return early rather than
 * using JUnit assumptions, because AGP 9's connected test runner reports AssumptionViolatedException
 * as a failure instead of a skip.
 */
@RunWith(Parameterized::class)
class TagLibArtworkSurvivalTest(private val name: String, private val dir: String) {

    companion object {
        @JvmStatic
        @Parameterized.Parameters(name = "{0}")
        fun files(): List<Array<String>> =
            TagLibCorpus.files().map { arrayOf(it, TagLibCorpus.ASSET_DIR) } +
                listOf("artwork.flac", "artwork.opus").map { arrayOf(it, "") }
    }

    private val kTagLib = KTagLib()

    @Test
    fun writingTitleKeepsArtwork() {
        val file = TagLibCorpus.copy(name, dir)
        val before = kTagLib.artwork(file) ?: return
        if (!kTagLib.write(file, mapOf("TITLE" to listOf("New Title")))) return

        assertArrayEquals("artwork of $name after writing TITLE", before, kTagLib.artwork(file))
    }
}
