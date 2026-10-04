# Changelog

## 2.2.0

Source and binary compatible with 2.1.0 and 2.0.0.

### Added

- `AudioProperties.codec`: the lowercase name of the audio codec (not the container, so an `.m4a`
  is `"aac"` or `"alac"`), or null when it is unknown or can't be detected. Values: `"aac"`,
  `"alac"`, `"ac3"`, `"eac3"`, `"dts"`, `"flac"`, `"wav"`, `"aiff"`, `"ape"`, `"wavpack"`, `"dsd"`
  (DSF and DSDIFF), `"mp1"`, `"mp2"`, `"mp3"`, `"vorbis"`, `"opus"`, `"speex"`, `"musepack"`,
  `"tta"`, `"shorten"`, `"wma"`, `"wmapro"` and `"wmalossless"`. Matroska files report `"flac"`,
  `"aac"`, `"alac"`, `"opus"`, `"vorbis"` or `"mp3"` when the track's codec ID maps to one. It
  defaults to null in the constructor, and the 2.0 and 2.1 constructors and `copy` are kept for
  binary compatibility.
- Matroska files without a track or tag-level title now report the Segment Info title as `TITLE`,
  rather than leaving it to fall back to the file name.

## 2.1.0

Source and binary compatible with 2.0.0.

### Added

- `AudioProperties.bitsPerSample`: the bit depth stored by FLAC, WAV, AIFF, MP4, APE, WavPack,
  TrueAudio, ASF, Matroska, DSF, DSDIFF and Shorten files, or 0 for formats without one (MP3,
  Ogg Vorbis, Opus, Musepack). Lossy formats that store a value (MP4 AAC, for example) report the
  container's nominal value, typically 16. It defaults to 0 in the constructor, and the 2.0
  constructor and `copy` are kept for binary compatibility.

## 2.0.0

### Breaking

- Consumers need Kotlin 2.2 or newer: the library compiles at Kotlin API/language version 2.2 and
  depends on kotlin-stdlib 2.2.21. Minimum SDK stays 21.
- `writeMetadata` now takes `properties: Map<String, List<String>>` instead of
  `HashMap<String, ArrayList<String?>>`. Keys and values are non-null, and any `Map`/`List`
  implementation is accepted. The contract holds for every supported format (MP3, FLAC, MP4,
  Ogg Vorbis, Opus, WAV):
  - a key with values replaces that field wholesale;
  - a key with an empty list removes that field;
  - keys not in the map are left untouched.

#### Migrating from 1.x

- To clear a field, pass `emptyList()` where you used to pass `listOf(null)` (or `arrayListOf(null)`).
- Drop any other nulls from your value lists; they are no longer representable.
- A `HashMap`/`ArrayList` still works as an argument, but `mapOf(...)`/`listOf(...)` are enough.

### Fixed

- Non-ASCII tag values (accented characters, CJK, emoji) are written as proper Unicode instead
  of being mangled as Latin-1 or modified UTF-8 (timusus/Shuttle2#388).
- `getArtwork` returns the largest embedded picture for FLAC and Opus, not the last one (#7).
- `writeMetadata` releases the JNI local references it holds for the property map's entry set
  and iterator (#8).
- A null property key passed from Java is skipped instead of being written under an empty key (#10).

### Changed

- `getMetadata` returns null only when the file can't be opened or its type isn't recognised;
  a file without tags returns an empty property map along with its audio properties (#9). TagLib 2
  always provides a tag object, so this documents the contract rather than fixing an observed failure.
- TagLib 2.1.1 → 2.3.2: reads and writes Matroska (MKA, MKV) and WebM files, verifies values
  parsed from crafted or corrupt files more strictly across most formats, tolerates more malformed
  MP4 cover art and RIFF chunks, supports RF64/BW64 WAV, and fixes data races in shared caches.

### Performance

- `getMetadata` debug logging is gated behind the verbose logging flag (#12).
- `getArtwork` and `writeMetadata` no longer parse audio properties they don't use (#11).
