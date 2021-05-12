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
	    implementation("com.github.timusus:KTagLib:release-SNAPSHOT") // or kTagLib:1.2
	}


See the sample app for an example of reading tags, using the Storage Access Framework.


## Usage ##

#### Read Tags ####

Read the tags from a file descriptor:

`KTagLib().getMetadata(fileDescriptor: Int)`

This returns a Metadata object, containing the tags and audio properties of the audio file located at `fileDescriptor`, or null if none are found

#### Retrieve Artwork ####

`KTagLib().getArtwork(fileDescriptor: Int)`

Returns a `ByteArray` (or null) representing the image data of the largest image found.

#### Write Tags ####

    fun writeMetadata(
        fileDescriptor: Int,
        properties : HashMap<String, ArrayList<String>>
    ): Boolean

Attempts to write the tags supplied via the map to the file located at `fileDescriptor`. Existing tags with the same key are replaced.

Returns true if the tags are successfully updated.