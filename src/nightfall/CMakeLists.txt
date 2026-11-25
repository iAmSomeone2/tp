# Minimal compatibility fixes allowing engine code to be built on Clang or GCC

# Host definitions of globals that the SDK headers only declare (see platform/os_globals.cpp).
# Linked through tp::dolphin by everything that includes the SDK headers.
add_library(nf_platform STATIC platform/os_globals.cpp)
add_library(nf::platform ALIAS nf_platform)
set_target_properties(nf_platform PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(nf_platform
    PUBLIC tp::config tp::dolphin
    PRIVATE tp::warnings
)
# Not in TP_LIBRARIES: tp::dolphin links it, which already puts it after every engine library, and
# as a member of tp::engine's RESCAN group that dependency would form a cycle CMake rejects.

# Memory management (docs/port/memory-design.md).
# No engine dependency; tp::config gives it the project's include path, C++23 and ABI flags.
add_library(nf_memory STATIC memory/arena.cpp
        memory/block_pool.cpp)
add_library(nf::memory ALIAS nf_memory)
target_link_libraries(nf_memory PUBLIC tp::config PRIVATE tp::warnings)

add_subdirectory(movie)
