# KTagLib

Kotlin bindings for [TagLib](https://github.com/taglib/taglib)

Gradle:
`implementation("com.github.timusus:KTagLib:master-SNAPSHOT")`

See the sample app for an example of reading tags, using the Storage Access Framework.


## Usage ##

Instantiate KTagLib:

`val tagLib = KTagLib()`

#### Read Tags ####

Read the tags from a file descriptor:

`tagLib.getAudioFile(fd: Int, path: String, name: String)`

This returns an AudioFile object, representing the id3/Vorbis tags (metadata) of the file located at `FileDescriptor` fd.

Note: Only the `FileDescriptor` (fd) is used to retrieve the tags. The path and name are just metadata.


#### Retrieve Artwork ####

`getArtwork(fd: Int)`

Returns a `ByteArray` (or null) representing the image data of the largest image found.


#### Write Tags ####

    fun updateTags(
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

Attempts to write the supplied tags to the file located at `FileDescriptor` 'fd'.

Note: Null fields are ignored. To clear a tag, pass an empty string.

Returns true if the tags are successfully updated.
