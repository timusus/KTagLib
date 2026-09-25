package com.simplecityapps.ktaglib

class KTagLib {

    init {
        System.loadLibrary("ktaglib")
    }

    /**
     * Returns a [HashMap] containing all fields in the tag, audio properties of the file and some system properties of the file associated with the file descriptor.
     *
     * @param fileDescriptor associated with the file whose properties are to be retrieved
     * @param filename optional filename hint to help with file type detection (recommended for better compatibility)
     * @return [HashMap] of metadata and other file properties
     */
    external fun getMetadata(fileDescriptor: Int, filename: String? = null): Metadata?

    /**
     * Returns true if the tags are successfully written to the file associated with the file descriptor.
     *
     * Each key in [properties] names a tag field (a TagLib property name such as `TITLE` or
     * `ARTIST`, case-insensitive), and the write follows these rules for every supported format:
     * - a key with a non-empty list replaces that field wholesale: its existing values are
     *   discarded and the list's values are written in order;
     * - a key with an empty list removes that field from the tag;
     * - fields whose keys are not in [properties] are left untouched.
     *
     * Note: [fileDescriptor] should have write access otherwise the fields cannot be written.
     *
     * @param fileDescriptor associated with the file to which metadata is to be written
     * @param properties the fields to replace or remove, keyed by property name
     * @param filename optional filename hint to help with file type detection (recommended for better compatibility)
     * @return true if metadata written successfully, false otherwise
     */
    external fun writeMetadata(fileDescriptor: Int, properties: Map<String, List<String>>, filename: String? = null): Boolean

    /**
     * Returns a [ByteArray] representing the artwork for the file located at File Descriptor [fileDescriptor], or null if no artwork can be found.
     *
     * @param fileDescriptor File descriptor
     * @param filename optional filename hint to help with file type detection (recommended for better compatibility)
     */
    external fun getArtwork(fileDescriptor: Int, filename: String? = null): ByteArray?
}