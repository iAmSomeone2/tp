# Unit tests, built with GoogleTest (and GoogleMock for faking SDK/platform seams).
#
# This directory is only meant to be added from the top-level CMakeLists.txt, which must also
# call enable_testing() (or include(CTest)) so that `ctest` finds the tests from the build root.
include(FetchContent)
include(GoogleTest)

# Prefer an installed GoogleTest (`find_package(GTest)`, e.g. from a distro or vcpkg package);
# otherwise download the pinned release. Pass -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=<dir> to use
# a local checkout instead, which also works offline.
FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
        URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
        EXCLUDE_FROM_ALL
        SYSTEM
        FIND_PACKAGE_ARGS NAMES GTest
)
set(INSTALL_GTEST OFF)
set(BUILD_GMOCK ON)
FetchContent_MakeAvailable(googletest)

# The engine libraries are built with -fno-rtti -fno-exceptions (see tp::config). GoogleTest's
# headers change with those settings, so a fetched copy must be built with the same compile
# options as the tests, or the test executables would disagree with libgtest about them. Only
# the options are taken from tp::config, not its include paths or definitions. A found
# (installed) GoogleTest is prebuilt and cannot be adjusted; it copes with either setting.
if (TARGET gtest)
    foreach (_gtest_target IN ITEMS gtest gtest_main gmock gmock_main)
        if (TARGET ${_gtest_target})
            target_compile_options(${_gtest_target} PUBLIC
                    "$<TARGET_PROPERTY:tp::config,INTERFACE_COMPILE_OPTIONS>")
        endif ()
    endforeach ()
endif ()

# tests/ is on every test's include path so tests can reach tests/support/ ("support/os_stubs.hpp").
#
# Tests link the real engine libraries. The Dolphin SDK implementation is not built on the host, so
# the linked code mentions SDK (and other) functions that a test never calls. LENIENT_LINK lets the
# link succeed with those symbols unresolved. A call that does reach one crashes, so tests stub
# whatever they execute (tests/support). References the loader resolves at start-up (data, and
# functions whose address is taken) cannot be left unresolved; tests/support/load_time_stubs.hpp
# covers those.
#
#   LENIENT_LINK is supported on macOS and on GNU-style ELF linkers. Elsewhere the target is not
#   created and a status message says so.
add_library(tp_test_lenient_link INTERFACE)
if (APPLE)
    target_link_options(tp_test_lenient_link INTERFACE LINKER:-undefined,dynamic_lookup)
    set(TP_TEST_LENIENT_LINK_SUPPORTED TRUE)
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD|NetBSD|OpenBSD")
    target_link_options(tp_test_lenient_link INTERFACE LINKER:--unresolved-symbols=ignore-all)
    set(TP_TEST_LENIENT_LINK_SUPPORTED TRUE)
else ()
    set(TP_TEST_LENIENT_LINK_SUPPORTED FALSE)
    message(STATUS "TP tests: lenient linking is not available on ${CMAKE_SYSTEM_NAME}; "
                   "tests that need it are skipped")
endif ()

# tp_add_test(<name> SOURCES <file>... [LIBRARIES <target>...] [INCLUDE_DIRS <dir>...] [LENIENT_LINK])
#
# Creates the test executable `<name>` from SOURCES (relative to the calling directory), links it
# against GoogleTest + GoogleMock with a main(), and registers each test case with CTest. The
# project configuration (tp::config), the JSystem and Dolphin SDK headers are always available;
# list the engine libraries under test in LIBRARIES, e.g. `LIBRARIES tp::SSystem`. INCLUDE_DIRS
# adds extra include directories; LENIENT_LINK is described above.
function(tp_add_test name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "LENIENT_LINK" "" "SOURCES;LIBRARIES;INCLUDE_DIRS")
    if (NOT ARG_SOURCES)
        message(FATAL_ERROR "tp_add_test(${name}): no SOURCES given")
    endif ()
    if (ARG_LENIENT_LINK AND NOT TP_TEST_LENIENT_LINK_SUPPORTED)
        message(STATUS "TP tests: skipping ${name} (needs lenient linking)")
        return()
    endif ()

    add_executable(${name} ${ARG_SOURCES})
    target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}" ${ARG_INCLUDE_DIRS})
    if (ARG_LENIENT_LINK)
        target_link_libraries(${name} PRIVATE tp_test_lenient_link)
    endif ()
    set_target_properties(${name} PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(${name} PRIVATE
            tp::config
            tp::JSystem_headers
            tp::dolphin
            tp::warnings
            nf::platform
            ${ARG_LIBRARIES}
            GTest::gtest_main
            GTest::gmock
    )

    # Discover test cases when ctest runs rather than by running the executable at build time,
    # so a test that cannot start on the build machine does not break the build.
    gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST)
endfunction()

# The tests mirror the source tree, minus the "src"/"include" levels: tests for
# libs/JSystem/src/JMessage/resource.cpp live in tests/JSystem/JMessage/. Each directory declares
# its own executables, e.g.:
#
#   tp_add_test(test_SSystem
#       SOURCES SSystem/c_math_test.cpp
#       LIBRARIES tp::SSystem
#   )
add_subdirectory(JSystem)

add_executable(nf_compat_test nightfall/nf_compat_test.cpp)
set_target_properties(nf_compat_test PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(nf_compat_test PRIVATE cxx_std_20)
target_include_directories(nf_compat_test PRIVATE ../include)
target_link_libraries(nf_compat_test PRIVATE GTest::gtest_main)
gtest_discover_tests(nf_compat_test DISCOVERY_MODE PRE_TEST)
