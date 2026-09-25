package com.simplecityapps.ktaglib

import androidx.test.ext.junit.runners.AndroidJUnit4
import com.simplecityapps.ktaglib.TagLibCorpus.artwork
import com.simplecityapps.ktaglib.TagLibCorpus.metadata
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Values TagLib's own C++ tests assert for its sample files, read back through KTagLib. Each
 * expectation cites the TagLib test (taglib/tests/test_*.cpp) and function it comes from.
 */
@RunWith(AndroidJUnit4::class)
class TagLibGoldenTest {

    private val kTagLib = KTagLib()

    // test_mpeg.cpp, TestMPEG::testAudioPropertiesXingHeaderCBR
    @Test
    fun mp3XingCbrProperties() {
        assertAudio("lame_cbr.mp3", duration = 1887164, bitrate = 64, sampleRate = 44100, channels = 1)
    }

    // test_mpeg.cpp, TestMPEG::testExtendedHeader
    @Test
    fun mp3ExtendedHeaderTags() {
        val tags = read("extended-header.mp3").propertyMap
        assertEquals(listOf("Druids"), tags["TITLE"])
        assertEquals(listOf("Excelsis"), tags["ARTIST"])
        assertEquals(listOf("Vo Chrieger U Drache"), tags["ALBUM"])
        assertEquals(listOf("Folk/Power Metal"), tags["GENRE"])
    }

    // test_flac.cpp, TestFLAC::testAudioProperties
    @Test
    fun flacProperties() {
        assertAudio("sinewave.flac", duration = 3550, bitrate = 145, sampleRate = 44100, channels = 2)
    }

    // test_flac.cpp, TestFLAC::testReadPicture: one 150-byte PNG picture
    @Test
    fun flacPicture() {
        val file = TagLibCorpus.copy("silence-44-s.flac")
        val artwork = nonNull(kTagLib.artwork(file), "artwork of silence-44-s.flac")
        assertEquals(150, artwork.size)
    }

    // test_mp4.cpp, TestMP4::testPropertiesAAC (audio), TestMP4::testProperties (ARTIST) and
    // TestMP4::testCovrRead (two covers of 79 and 287 bytes; KTagLib returns the largest)
    @Test
    fun m4aPropertiesTagsAndCover() {
        assertAudio("has-tags.m4a", duration = 3708, bitrate = 3, sampleRate = 44100, channels = 2)
        assertEquals(listOf("Test Artist"), read("has-tags.m4a").propertyMap["ARTIST"])
        val artwork = nonNull(kTagLib.artwork(TagLibCorpus.copy("has-tags.m4a")), "artwork of has-tags.m4a")
        assertEquals(287, artwork.size)
    }

    // test_ogg.cpp, TestOGG::testAudioProperties
    @Test
    fun oggVorbisProperties() {
        assertAudio("empty.ogg", duration = 3685, bitrate = 1, sampleRate = 44100, channels = 2)
    }

    // test_opus.cpp, TestOpus::testAudioProperties and TestOpus::testReadComments
    @Test
    fun opusPropertiesAndComments() {
        val metadata = assertAudio("correctness_gain_silent_output.opus", duration = 7737, bitrate = 36, sampleRate = 48000, channels = 1)
        assertEquals(listOf("Xiph.Org Opus testvectormaker"), metadata.propertyMap["ENCODER"])
        assertFalse(metadata.propertyMap.containsKey("ARTIST"))
    }

    // test_wav.cpp, TestWAV::testPCMProperties
    @Test
    fun wavProperties() {
        assertAudio("empty.wav", duration = 3675, bitrate = 32, sampleRate = 1000, channels = 2)
    }

    // test_aiff.cpp, TestAIFF::testAiffProperties
    @Test
    fun aiffProperties() {
        assertAudio("empty.aiff", duration = 67, bitrate = 706, sampleRate = 44100, channels = 1)
    }

    // test_ape.cpp, TestAPE::testProperties399
    @Test
    fun apeProperties() {
        assertAudio("mac-399.ape", duration = 3550, bitrate = 192, sampleRate = 44100, channels = 2)
    }

    // test_wavpack.cpp, TestWavPack::testTaggedProperties
    @Test
    fun wavPackProperties() {
        assertAudio("tagged.wv", duration = 3550, bitrate = 172, sampleRate = 44100, channels = 2)
    }

    // test_mpc.cpp, TestMPC::testPropertiesSV7
    @Test
    fun mpcProperties() {
        assertAudio("click.mpc", duration = 40, bitrate = 318, sampleRate = 44100, channels = 2)
    }

    private fun read(name: String): Metadata = nonNull(kTagLib.metadata(TagLibCorpus.copy(name)), "metadata of $name")

    private fun assertAudio(name: String, duration: Int, bitrate: Int, sampleRate: Int, channels: Int): Metadata {
        val metadata = read(name)
        assertEquals("audio properties of $name", AudioProperties(duration, bitrate, sampleRate, channels), metadata.audioProperties)
        return metadata
    }

    private fun <T> nonNull(value: T?, what: String): T {
        assertNotNull(what, value)
        return value!!
    }
}
