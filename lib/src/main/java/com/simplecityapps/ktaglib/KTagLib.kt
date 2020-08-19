package com.simplecityapps.ktaglib

class KTagLib {

    /**
     * Updates the file at File Descriptor [fd] with the supplied tags.
     *
     * Note: Null tags are ignored.
     *
     * @param fd File descriptor
     *
     * @return true if the tags were successfully updated.
     */
    external fun updateTags(
        fd: Int,
        title: String?,
        artist: String?,
        album: String?,
        albumArtist: String?,
        date: String?,
        track: Int?,
        trackTotal: Int?,
        disc: Int?,
        discTotal: Int?,
        genre: String?
    ): Boolean

    /**
     * Returns a [ByteArray] representing the artwork for the file located at File Descriptor [fd], or null if no artwork can be found.
     *
     * @param fd File descriptor
     */
    external fun getArtwork(fd: Int): ByteArray?


    companion object {
        init {
            System.loadLibrary("ktaglib")
        }
    }
}
