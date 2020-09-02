package com.simplecityapps.ktaglib.sample

import android.content.Context
import com.bumptech.glide.load.Options
import com.bumptech.glide.load.model.ModelLoader
import com.bumptech.glide.load.model.ModelLoaderFactory
import com.bumptech.glide.load.model.MultiModelLoaderFactory
import com.bumptech.glide.signature.ObjectKey
import java.io.ByteArrayInputStream

class AlbumArtLoader(private val context: Context) : ModelLoader<AudioFile, ByteArrayInputStream> {

    override fun buildLoadData(
        model: AudioFile,
        width: Int,
        height: Int,
        options: Options
    ): ModelLoader.LoadData<ByteArrayInputStream>? {
        return ModelLoader.LoadData(ObjectKey(model.path), AlbumArtFetcher(context, model))
    }

    override fun handles(model: AudioFile): Boolean = model.path.isNotBlank()

    class Factory(private val context: Context) : ModelLoaderFactory<AudioFile, ByteArrayInputStream> {

        override fun build(multiFactory: MultiModelLoaderFactory): ModelLoader<AudioFile, ByteArrayInputStream> {
            return AlbumArtLoader(context)
        }

        override fun teardown() {}
    }
}