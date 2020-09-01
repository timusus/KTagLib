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
	    implementation("com.github.timusus:KTagLib:release-SNAPSHOT") // or kTagLib:1.1
	}


See the sample app for an example of reading tags, using the Storage Access Framework.


## Usage ##

#### Read Tags ####

Read the tags from a file descriptor:

`KTagLib.getMetadata(fileDescriptor: Int)`

This returns a HashMap representing the tags (metadata) of the audio file located at `fileDescriptor`.

#### Retrieve Artwork ####

`getArtwork(fd: Int)`

Returns a `ByteArray` (or null) representing the image data of the largest image found.

#### Write Tags ####

    fun writeMetadata(
        fileDescriptor: Int,
        properties : HashMap<String, String>
    ): Boolean

Attempts to write the tags supplied as a HashMap to the file located at `fileDescriptor`.

Note: All tags passed to properties are replaced in the file. Therefore, only pass fields you want to change. To clear a tag, pass an empty string.

Returns true if the tags are successfully updated.
