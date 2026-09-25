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
     * The values of fields in [properties] will overwrite the existing values in the tag.
     *
     * A null key in [properties] is skipped (and logged as a warning); a null element within a
     * field's value list is simply ignored, so `["a", null, "b"]` is written as `["a", "b"]`. An
     * empty value list for a key removes that field from the tag - confirmed for Xiph comments
     * (FLAC/Vorbis/Opus), where an empty list, on write, explicitly removes all values for the field.
     *
     * Note: [fileDescriptor] should have write access otherwise the fields cannot be written.
     * Note: Taglib supports inserting values in existing fields operations but this implementation does not.
     *
     * @param fileDescriptor associated with the file to which metadata is to be written
     * @param properties the metadata fields and their values to be written
     * @param filename optional filename hint to help with file type detection (recommended for better compatibility)
     * @return true if metadata written successfully, false otherwise
     */
    external fun writeMetadata(fileDescriptor: Int, properties: HashMap<String, ArrayList<String?>>, filename: String? = null): Boolean

    /**
     * Returns a [ByteArray] representing the artwork for the file located at File Descriptor [fileDescriptor], or null if no artwork can be found.
     *
     * @param fileDescriptor File descriptor
     * @param filename optional filename hint to help with file type detection (recommended for better compatibility)
     */
    external fun getArtwork(fileDescriptor: Int, filename: String? = null): ByteArray?
}