package com.simplecityapps.ktaglib

import com.simplecityapps.ktaglib.TagLibCorpus.artwork
import com.simplecityapps.ktaglib.TagLibCorpus.write
import org.junit.Assert.assertArrayEquals
import org.junit.Assume.assumeTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.Parameterized

/**
 * writeMetadata must not drop embedded pictures: for every corpus file (plus the artwork.flac /
 * artwork.opus fixtures) that has artwork, write TITLE and check getArtwork returns the same bytes.
 * Files without artwork, or that TagLib can't save, are skipped via JUnit assumptions.
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
        val before = kTagLib.artwork(file)
        assumeTrue("$name has no artwork", before != null)
        assumeTrue("TagLib can't save $name", kTagLib.write(file, mapOf("TITLE" to listOf("New Title"))))

        assertArrayEquals("artwork of $name after writing TITLE", before, kTagLib.artwork(file))
    }
}
