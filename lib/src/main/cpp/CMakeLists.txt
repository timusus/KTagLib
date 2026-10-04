# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.10.2)

# Project name
project("ktaglib")

# Release builds (lib/build.gradle passes -DCMAKE_BUILD_TYPE=Release for the release variant):
# -O3, link-time optimise ktaglib and TagLib together, and hide every symbol but the JNI entry
# points (JNIEXPORT) so the linker can drop unused TagLib code. Set before add_subdirectory so
# TagLib is compiled with the same flags. Debug builds are unaffected.
# -O3 costs about 6-7% in stripped .so size over -O2 (both with LTO).
# -g keeps debug info in the unstripped .so for crash symbolication; AGP strips the shipped one.
set(KTAGLIB_OPT "-O3")
set(KTAGLIB_LTO "-flto")
set(KTAGLIB_RELEASE_FLAGS "${KTAGLIB_OPT} -g -DNDEBUG ${KTAGLIB_LTO} -ffunction-sections -fdata-sections -fvisibility=hidden -fvisibility-inlines-hidden")
set(CMAKE_C_FLAGS_RELEASE "${KTAGLIB_RELEASE_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${KTAGLIB_RELEASE_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${KTAGLIB_OPT} ${KTAGLIB_LTO} -Wl,--gc-sections -Wl,--exclude-libs,ALL")

add_subdirectory("../../../../taglib" taglib)

add_library(ktaglib SHARED ktaglib.cpp)
set_property(TARGET ktaglib PROPERTY CXX_STANDARD 17)

# Set 16KB page alignment for Android 15+ compatibility
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,max-page-size=16384")

find_library(log-lib log)
target_link_libraries(ktaglib
        ${log-lib}
        tag)

target_include_directories(ktaglib PUBLIC
        "../../../../taglib/taglib"
        "../../../../taglib/taglib/flac"
        "../../../../taglib/taglib/ogg"
        "../../../../taglib/taglib/ogg/opus"
        "../../../../taglib/taglib/toolkit")
