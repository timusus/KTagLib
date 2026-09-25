# Changelog

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
