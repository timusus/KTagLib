# KTagLib

Kotlin bindings for [TagLib](https://github.com/taglib/taglib)

Gradle:

Step 1. Add the Jitpack repository to the root build.gradle

	allprojects {
		repositories {
			...
			maven { url 'https://jitpack.io' }
		}
	}

Step 2. Add the dependency

	dependencies {
	    implementation("com.github.timusus:KTagLib:release-SNAPSHOT") // or kTagLib:1.6.0
	}


See the sample app for an example of reading tags, using the Storage Access Framework.


## Usage ##

#### Read Tags ####

Read the tags from a file descriptor:

`KTagLib().getMetadata(fileDescriptor: Int, filename: String? = null)`

This returns a Metadata object, containing the tags and audio properties of the audio file located at `fileDescriptor`, or null if none are found.

The optional `filename` parameter helps with file type detection. Providing the filename (with extension) is recommended for better compatibility, especially for files with large metadata blocks.

#### Retrieve Artwork ####

`KTagLib().getArtwork(fileDescriptor: Int, filename: String? = null)`

Returns a `ByteArray` (or null) representing the image data of the largest image found.

The optional `filename` parameter helps with file type detection.

#### Write Tags ####

    fun writeMetadata(
        fileDescriptor: Int,
        properties: HashMap<String, ArrayList<String>>,
        filename: String? = null
    ): Boolean

Attempts to write the tags supplied via the map to the file located at `fileDescriptor`. Existing tags with the same key are replaced.

Returns true if the tags are successfully updated.

The optional `filename` parameter helps with file type detection.
