cmake_minimum_required(VERSION 4.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(TPPreProject)
endif()

project(Nightfall
    DESCRIPTION "Portable modernization of The Legend of Zelda: Twilight Princess (GameCube) engine."
    LANGUAGES CXX
)

# This build is independent of configure.py/ninja (the Metrowerks matching build). It
# compiles the game and JSystem sources with a stock Clang or GCC; see docs/port/cmake.md.

include(TPChecks)
include(TPConfig)
include(TPLibrary)

add_subdirectory(libs)
add_subdirectory(src)

tp_add_engine_target()
tp_add_maintenance_targets()

if(TP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
