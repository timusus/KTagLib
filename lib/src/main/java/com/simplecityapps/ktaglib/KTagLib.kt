package com.simplecityapps.ktaglib

object KTagLib {

    init {
        System.loadLibrary("ktaglib")
    }

    /**
     * Returns a [HashMap] containing all fields in the tag, audio properties of the file and some system properties of the file associated with the file descriptor.
     *
     * @param fileDescriptor associated with the file whose properties are to be retrieved
     * @return [HashMap] of metadata and other file properties
     */
    @JvmStatic
    external fun getMetadata(fileDescriptor: Int): HashMap<String, String>

    /**
     * Returns true if the tags are successfully written to the file associated with the file descriptor.
     *
     * The values of fields in [properties] will overwrite the existing values in the tag.
     *
     * Note: [fileDescriptor] should have write access otherwise the fields cannot be written.
     * Note: Taglib supports inserting values in existing fields operations but this implementation does not.
     *
     * @param fileDescriptor associated with the file to which metadata is to be written
     * @param properties the metadata fields and their values to be written
     * @return true if metadata written successfully, false otherwise
     */
    @JvmStatic
    external fun writeMetadata(fileDescriptor: Int, properties: HashMap<String, String>): Boolean

    /**
     * Returns a [ByteArray] representing the artwork for the file located at File Descriptor [fileDescriptor], or null if no artwork can be found.
     *
     * @param fileDescriptor File descriptor
     */
    @JvmStatic
    external fun getArtwork(fileDescriptor: Int): ByteArray?
}