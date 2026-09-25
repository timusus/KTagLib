package com.simplecityapps.ktaglib

import com.simplecityapps.ktaglib.TagLibCorpus.artwork
import com.simplecityapps.ktaglib.TagLibCorpus.metadata
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.runners.Parameterized

/**
 * Crash sweep (timusus/KTagLib#13): every file in TagLib's sample corpus, including the deliberately
 * broken ones (segfault.*, invalid-frames*, truncated and zero-length files), goes through
 * getMetadata and getArtwork. Only "doesn't crash or throw" is asserted; null results are fine.
 *
 * A native crash aborts the whole instrumentation run rather than failing one case, so if that
 * happens the last test started (in logcat / the runner output) and the tombstone name the file.
 */
@RunWith(Parameterized::class)
class TagLibCorpusTest(private val name: String) {

    companion object {
        @JvmStatic
        @Parameterized.Parameters(name = "{0}")
        fun files() = TagLibCorpus.files()
    }

    private val kTagLib = KTagLib()

    @Test
    fun readsWithoutCrashing() {
        val file = TagLibCorpus.copy(name)
        kTagLib.metadata(file)
        kTagLib.artwork(file)
    }
}
