# The GameCube SDK as the game sees it: its public headers only.
#
# The SDK implementation (libs/dolphin/src: OS, DVD, EXI, GX, ...) is PowerPC/Metrowerks specific
# and is deliberately not built. A host or Vita build provides its own implementation library
# for these headers and links it alongside tp::engine.
add_library(tp_dolphin INTERFACE)
add_library(tp::dolphin ALIAS tp_dolphin)

# types.h is included as <types.h> by SDK headers, hence the second directory.
target_include_directories(tp_dolphin INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/dolphin"
)

# Outside Metrowerks the headers only declare the SDK's fixed-address globals (__OSBusClock, ...);
# nf_platform defines them. Linking it here puts it after every library that uses them, which
# single-pass linkers such as GNU ld need.
target_link_libraries(tp_dolphin INTERFACE nf_platform)
