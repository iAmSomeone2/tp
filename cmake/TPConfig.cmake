# Project-wide configuration: game version, options and the `tp::config` / `tp::warnings`
# interface targets that every library in the project links.
include_guard(GLOBAL)

include(CheckCompilerFlag)

# --- Game version -------------------------------------------------------------------------
# GameCube is the core platform. The index of each entry must equal the matching
# VERSION_GCN_* value in include/global.h, because it is passed to the compiler as VERSION.
set(TP_SUPPORTED_VERSIONS GZ2E01 GZ2P01 GZ2J01)
set(TP_VERSION GZ2E01 CACHE STRING "Game version to build: GZ2E01 (USA), GZ2P01 (PAL), GZ2J01 (JPN)")
set_property(CACHE TP_VERSION PROPERTY STRINGS ${TP_SUPPORTED_VERSIONS})

list(FIND TP_SUPPORTED_VERSIONS "${TP_VERSION}" TP_VERSION_NUM)
if(TP_VERSION_NUM EQUAL -1)
    list(JOIN TP_SUPPORTED_VERSIONS ", " _tp_versions)
    message(FATAL_ERROR "TP_VERSION '${TP_VERSION}' is not supported; choose one of: ${_tp_versions}")
endif()

# --- Options ------------------------------------------------------------------------------
option(TP_BUILD_TESTS
    "Build the unit tests in tests/ (needs GoogleTest: an installed copy, or a download at configure time)" OFF)
option(TP_ENABLE_WARNINGS
    "Enable -Wall -Wextra. Off by default: the sources produce a very large number of warnings." OFF)

set(TP_ASSET_DIR "" CACHE PATH
    "Directory holding committed asset headers (default: assets/<TP_VERSION>)")
set(TP_GENERATED_INCLUDE_DIR "" CACHE PATH
    "Directory holding headers generated from a disc image (default: build/<TP_VERSION>/include)")

if(NOT TP_ASSET_DIR)
    set(TP_ASSET_DIR "${PROJECT_SOURCE_DIR}/assets/${TP_VERSION}")
endif()
if(NOT TP_GENERATED_INCLUDE_DIR)
    set(TP_GENERATED_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/build/${TP_VERSION}/include")
endif()

if(NOT EXISTS "${TP_GENERATED_INCLUDE_DIR}/assets")
    message(STATUS
        "TP: no generated asset headers in ${TP_GENERATED_INCLUDE_DIR}; translation units that "
        "include \"assets/*.h\" will not compile until they exist (they come from a disc image).")
endif()

if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- tp::config ---------------------------------------------------------------------------
# Everything that must be identical across all code in the project, including consumers of
# its headers: language level, VERSION, include search order and the data-layout/ABI
# switches that reproduce what configure.py asks the Metrowerks compiler for.
add_library(tp_config INTERFACE)
add_library(tp::config ALIAS tp_config)

target_compile_features(tp_config INTERFACE cxx_std_20)

target_compile_definitions(tp_config INTERFACE VERSION=${TP_VERSION_NUM})

# Same order as the -i flags in configure.py (minus the Metrowerks library directories, which
# a host build replaces with the host's standard library). JSystem and Dolphin SDK headers are
# added by the tp::JSystem_headers and tp::dolphin targets that tp_add_library() links.
target_include_directories(tp_config INTERFACE
    "${PROJECT_SOURCE_DIR}/include"
    "${TP_GENERATED_INCLUDE_DIR}"
    "${TP_ASSET_DIR}"
    "${PROJECT_SOURCE_DIR}/src"
)

# -enum int, -char signed, -fp_contract off, -RTTI off, -Cpp_exceptions off. Strict aliasing
# is disabled because the code type-puns heavily (e.g. `*(u32*)&x`).
set(_tp_abi_flags
    -fno-short-enums
    -fsigned-char
    -ffp-contract=off
    -fno-rtti
    -fno-exceptions
    -fno-strict-aliasing
)
foreach(_flag IN LISTS _tp_abi_flags)
    string(MAKE_C_IDENTIFIER "TP_HAVE${_flag}" _var)
    check_compiler_flag(CXX "${_flag}" ${_var})
    if(${_var})
        target_compile_options(tp_config INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:${_flag}>")
    else()
        message(WARNING "TP: the C++ compiler does not support ${_flag}")
    endif()
endforeach()

# --- tp::warnings -------------------------------------------------------------------------
add_library(tp_warnings INTERFACE)
add_library(tp::warnings ALIAS tp_warnings)
if(TP_ENABLE_WARNINGS)
    target_compile_options(tp_warnings INTERFACE -Wall -Wextra)
else()
    target_compile_options(tp_warnings INTERFACE -w)
endif()

message(STATUS "TP: building ${TP_VERSION} (VERSION=${TP_VERSION_NUM}) with "
               "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
