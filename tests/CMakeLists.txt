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

# tp_add_test(<name> SOURCES <file>... [LIBRARIES <target>...])
#
# Creates the test executable `<name>` from SOURCES (relative to the calling directory), links it
# against GoogleTest + GoogleMock with a main(), and registers each test case with CTest. The
# project configuration (tp::config), the JSystem and Dolphin SDK headers are always available;
# list the engine libraries under test in LIBRARIES, e.g. `LIBRARIES tp::SSystem`.
function(tp_add_test name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;LIBRARIES")
    if (NOT ARG_SOURCES)
        message(FATAL_ERROR "tp_add_test(${name}): no SOURCES given")
    endif ()

    add_executable(${name} ${ARG_SOURCES})
    set_target_properties(${name} PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(${name} PRIVATE
            tp::config
            tp::JSystem_headers
            tp::dolphin
            tp::warnings
            ${ARG_LIBRARIES}
            GTest::gtest_main
            GTest::gmock
    )

    # Discover test cases when ctest runs rather than by running the executable at build time,
    # so a test that cannot start on the build machine does not break the build.
    gtest_discover_tests(${name} DISCOVERY_MODE PRE_TEST)
endfunction()

# Test executables are declared below, one per library under test, e.g.:
#
#   tp_add_test(test_SSystem
#       SOURCES SSystem/c_math_test.cpp
#       LIBRARIES tp::SSystem
#   )

add_executable(nf_compat_test nf_compat/nf_compat_test.cpp)
set_target_properties(nf_compat_test PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(nf_compat_test PRIVATE cxx_std_20)
target_include_directories(nf_compat_test PRIVATE ../include)
target_link_libraries(nf_compat_test PRIVATE GTest::gtest_main)
gtest_discover_tests(nf_compat_test DISCOVERY_MODE PRE_TEST)
