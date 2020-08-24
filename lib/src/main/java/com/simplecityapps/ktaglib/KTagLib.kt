package com.simplecityapps.ktaglib

import java.util.*

object KTagLib {

    init {
        System.loadLibrary("ktaglib")
    }

    /**
     * Returns a [HashMap] containing all fields in the tag, audio properties of the file and
     * some system properties of the file associated with the file descriptor.
     *
     * @param fileDescriptor associated with the file whose properties are to be retrieved
     * @return [HashMap] of metadata and other file properties
     */
    @JvmStatic
    external fun getMetadata(fileDescriptor: Int) : HashMap<String, String>

    /**
     * Returns true if successful in writing the tags to the file associated with the file
     * descriptor.
     *
     * [fileDescriptor] should have write access otherwise the fields cannot be written.
     * The values of fields in [properties] will overwrite the existing values in the tag.
     * It is recommended that only fields which are desired to be changed are passed
     * and with the complete values. Taglib supports inserting values in existing fields
     * operations but this implementation does not.
     *
     * @param fileDescriptor associated with the file to which metadata is to be written
     * @param properties the metadata fields and their values to be written
     * @return true if metadata written successfully, false otherwise
     */
    @JvmStatic
    external fun writeMetadata(fileDescriptor: Int, properties : HashMap<String, String>) : Boolean

    /**
     * Returns a [ByteArray] representing the artwork for the file located at File Descriptor
     * [fd], or null if no artwork can be found.
     *
     * @param fd File descriptor
     */
    @JvmStatic
    external fun getArtwork(fd: Int): ByteArray?
}
