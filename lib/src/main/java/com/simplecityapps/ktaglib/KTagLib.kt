package com.simplecityapps.ktaglib

class KTagLib {

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
