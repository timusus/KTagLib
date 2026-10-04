# KTagLib

Kotlin bindings for [TagLib](https://github.com/taglib/taglib)

Gradle:

	dependencies {
	    implementation("com.simplecityapps:ktaglib:2.2.0")
	}

KTagLib is published to Maven Central, so no extra repository declaration is needed beyond `mavenCentral()`.

Versions up to 1.6.1 are also available on JitPack as `com.github.timusus:ktaglib:1.6.1` for projects that haven't migrated yet, but new releases are Maven Central only.


See the sample app for an example of reading tags, using the Storage Access Framework.


## Usage ##

#### File descriptors ####

Every function takes a file descriptor that **stays owned by the caller**: KTagLib works on its own duplicate of it and closes only that duplicate. Pass `pfd.fd` and let `use {}` close the `ParcelFileDescriptor`:

```kotlin
contentResolver.openFileDescriptor(uri, "r")?.use { pfd ->
    KTagLib().getMetadata(pfd.fd, filename)
}
```

Don't pass `pfd.detachFd()`. Up to 2.2.0 the library took ownership of the descriptor and closed it, so callers detached it; now nothing would close the original, and it would leak.

#### Read Tags ####

Read the tags from a file descriptor:

`KTagLib().getMetadata(fileDescriptor: Int, filename: String? = null)`

This returns a Metadata object, containing the tags and audio properties of the audio file located at `fileDescriptor`, or null only if the file can't be opened or its type isn't recognised. A file with no tags returns an empty property map.

The optional `filename` parameter helps with file type detection. Providing the filename (with extension) is recommended for better compatibility, especially for files with large metadata blocks.

#### Retrieve Artwork ####

`KTagLib().getArtwork(fileDescriptor: Int, filename: String? = null)`

Returns a `ByteArray` (or null) representing the image data of the largest image found.

The optional `filename` parameter helps with file type detection.

#### Write Tags ####

    fun writeMetadata(
        fileDescriptor: Int,
        properties: Map<String, List<String>>,
        filename: String? = null
    ): Boolean

Writes the tags supplied via the map to the file located at `fileDescriptor`, in every supported format:

- a key with values replaces that field wholesale;
- a key with an empty list (`emptyList()`) removes that field;
- fields whose keys aren't in the map are left untouched.

Returns true if the tags are successfully updated.

The optional `filename` parameter helps with file type detection.
