package com.simplecityapps.ktaglib

import android.os.ParcelFileDescriptor
import androidx.test.platform.app.InstrumentationRegistry
import java.io.File

/**
 * TagLib's own sample files (taglib/tests/data), packaged as androidTest assets under
 * [ASSET_DIR] by the copyTagLibTestData Gradle task, plus fd-based helpers around [KTagLib].
 */
object TagLibCorpus {

    const val ASSET_DIR = "taglib-data"

    private val instrumentation get() = InstrumentationRegistry.getInstrumentation()

    /** Every file name in the corpus, sorted. */
    fun files(): List<String> = requireNotNull(instrumentation.context.assets.list(ASSET_DIR)) {
        "no $ASSET_DIR assets packaged"
    }.sorted()

    /**
     * Copies a corpus file (or, with [dir] = "", a top-level androidTest asset) into a fresh
     * directory in the cache dir, keeping its real file name so it works as the filename hint.
     */
    fun copy(name: String, dir: String = ASSET_DIR): File {
        val target = File(instrumentation.targetContext.cacheDir, "corpus").apply { mkdirs() }
        val file = File(target, name)
        val assetPath = if (dir.isEmpty()) name else "$dir/$name"
        instrumentation.context.assets.open(assetPath).use { input ->
            file.outputStream().use { output -> input.copyTo(output) }
        }
        return file
    }

    fun KTagLib.metadata(file: File): Metadata? = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
        getMetadata(pfd.detachFd(), file.name)
    }

    fun KTagLib.artwork(file: File): ByteArray? = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY).use { pfd ->
        getArtwork(pfd.detachFd(), file.name)
    }

    fun KTagLib.write(file: File, properties: Map<String, List<String>>): Boolean = ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_WRITE).use { pfd ->
        writeMetadata(pfd.detachFd(), properties, file.name)
    }
}
