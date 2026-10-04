package com.simplecityapps.ktaglib

/**
 * Reads and writes audio metadata through TagLib.
 *
 * ## File descriptor ownership
 *
 * Every function takes a `fileDescriptor` that **the caller keeps owning**: native code works on
 * its own duplicate (`dup`) of it and closes only that duplicate, on every path, including
 * failures. Close the original yourself when the call returns, e.g. pass `pfd.fd` inside
 * `ParcelFileDescriptor.use {}`. Do not pass `pfd.detachFd()`: nothing would close the original,
 * and it would leak.
 *
 * The duplicate shares the original's file offset, so the offset is unspecified after a call;
 * use positioned reads or seek before reading from the descriptor again.
 *
 * The functions never throw for an unreadable or malformed file; they return null (or false).
 */
class KTagLib {

    init {
        System.loadLibrary("ktaglib")
    }

    /**
     * Returns a [HashMap] containing all fields in the tag, audio properties of the file and some system properties of the file associated with the file descriptor.
     *
     * @param fileDescriptor associated with the file whose properties are to be retrieved. Still
     * owned by the caller, who must close it (see [KTagLib]); pass `pfd.fd`, not `pfd.detachFd()`.
     * @param filename optional filename hint to help with file type detection (recommended for better compatibility)
     * @return [HashMap] of metadata and other file properties, or null if the file can't be read or
     * its type isn't recognized
     */
    external fun getMetadata(fileDescriptor: Int, filename: String? = null): Metadata?

    /**
     * Like [getMetadata], but [Metadata.propertyMap] holds only the properties whose key is in
     * [keys] (TagLib property names such as `TITLE` or `ARTIST`, matched case-insensitively; the
     * map's keys are uppercase). The others are skipped natively, before any Java strings are
     * created for them. Audio properties are returned as usual.
     *
     * @param fileDescriptor associated with the file whose properties are to be retrieved. Still
     * owned by the caller, who must close it (see [KTagLib]); pass `pfd.fd`, not `pfd.detachFd()`.
     * @param filename optional filename hint to help with file type detection (recommended for better compatibility)
     * @param keys the property keys to return
     * @return the metadata, or null if the file can't be read or its type isn't recognized
     */
    fun getMetadata(fileDescriptor: Int, filename: String?, keys: Set<String>): Metadata? =
        getMetadataForKeys(fileDescriptor, filename, keys.toTypedArray())

    private external fun getMetadataForKeys(fileDescriptor: Int, filename: String?, keys: Array<String>): Metadata?

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
     * @param fileDescriptor associated with the file to which metadata is to be written. Still
     * owned by the caller, who must close it (see [KTagLib]); pass `pfd.fd`, not `pfd.detachFd()`.
     * @param properties the fields to replace or remove, keyed by property name
     * @param filename optional filename hint to help with file type detection (recommended for better compatibility)
     * @return true if metadata written successfully, false otherwise
     */
    external fun writeMetadata(fileDescriptor: Int, properties: Map<String, List<String>>, filename: String? = null): Boolean

    /**
     * Returns a [ByteArray] representing the artwork for the file located at File Descriptor [fileDescriptor], or null if no artwork can be found.
     *
     * @param fileDescriptor File descriptor. Still owned by the caller, who must close it (see
     * [KTagLib]); pass `pfd.fd`, not `pfd.detachFd()`.
     * @param filename optional filename hint to help with file type detection (recommended for better compatibility)
     */
    external fun getArtwork(fileDescriptor: Int, filename: String? = null): ByteArray?
}