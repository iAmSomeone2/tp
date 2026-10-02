# Checks that must run before project(): they only need the generator and the directories, and
# failing early avoids leaving cache files in the source tree or probing the compiler first.
include_guard(GLOBAL)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR
        "In-source builds are not supported (the source tree already uses build/ for the "
        "Metrowerks build). Configure with `cmake --preset default` or "
        "`cmake -S . -B build/cmake/default -G Ninja`. (CMake has already created CMakeCache.txt "
        "and CMakeFiles/ here; delete them.)")
endif()
