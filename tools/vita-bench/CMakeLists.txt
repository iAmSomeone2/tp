cmake_minimum_required(VERSION 3.24)

# Vita storage and decompression benchmark. Not part of the engine build.
#   cmake -S tools/vita-bench -B build/vita-bench/app [-DNFBENCH_CORPUS=build/vita-bench/corpus]
#   cmake --build build/vita-bench/app
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Set VITASDK to the VitaSDK install path")
  endif()
endif()

project(nfbench C CXX)
include("${VITASDK}/share/vita.cmake" REQUIRED)

include(FetchContent)
# SOURCE_SUBDIR points at a directory without a CMakeLists.txt, so only the
# sources are fetched; the files needed are compiled directly below.
FetchContent_Declare(lz4 GIT_REPOSITORY https://github.com/lz4/lz4.git GIT_TAG v1.10.0
                     GIT_SHALLOW TRUE SOURCE_SUBDIR nf-sources-only)
FetchContent_Declare(zstd GIT_REPOSITORY https://github.com/facebook/zstd.git GIT_TAG v1.5.7
                     GIT_SHALLOW TRUE SOURCE_SUBDIR nf-sources-only)
FetchContent_MakeAvailable(lz4 zstd)

file(GLOB ZSTD_SOURCES ${zstd_SOURCE_DIR}/lib/common/*.c ${zstd_SOURCE_DIR}/lib/compress/*.c
                       ${zstd_SOURCE_DIR}/lib/decompress/*.c)

add_executable(nfbench
  src/main.cpp
  ${lz4_SOURCE_DIR}/lib/lz4.c
  ${lz4_SOURCE_DIR}/lib/lz4hc.c
  ${ZSTD_SOURCES}
  ${VITASDK}/share/gcc-arm-vita-eabi/samples/common/debugScreen.c)
target_include_directories(nfbench PRIVATE
  ${lz4_SOURCE_DIR}/lib ${zstd_SOURCE_DIR}/lib
  ${VITASDK}/share/gcc-arm-vita-eabi/samples/common)
target_compile_features(nfbench PRIVATE cxx_std_20)
target_compile_definitions(nfbench PRIVATE ZSTD_DISABLE_ASM=1)
target_compile_options(nfbench PRIVATE -O2)
target_link_libraries(nfbench SceDisplay_stub SceCtrl_stub ScePower_stub pthread)

set(VPK_FILES)
if(NFBENCH_CORPUS)
  file(GLOB_RECURSE corpus_files RELATIVE ${NFBENCH_CORPUS} ${NFBENCH_CORPUS}/*)
  foreach(f IN LISTS corpus_files)
    list(APPEND VPK_FILES FILE ${NFBENCH_CORPUS}/${f} corpus/${f})
  endforeach()
endif()

# ATTRIBUTE2=12 in param.sfo turns on the extended memory mode, which raises the
# app's user memory budget. Off gives the default budget, for comparison.
# vita_create_vpk is a macro that appends to these in this scope (title ID,
# bundled files), so memcheck below starts again from the values before it.
set(base_sfo_flags "${VITA_MKSFOEX_FLAGS}")
set(base_pack_flags "${VITA_PACK_VPK_FLAGS}")
option(NFBENCH_EXTENDED_MEM "Request the extended memory mode" ON)
if(NFBENCH_EXTENDED_MEM)
  set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE2=12")
endif()

vita_create_self(nfbench.self nfbench)
vita_create_vpk(nfbench.vpk NFBN00001 nfbench.self
  VERSION 01.00
  NAME "Nightfall storage bench"
  ${VPK_FILES})

# memcheck: free memory per type, with the default budget and with the
# extended memory mode. Separate title IDs, so both can be installed.
foreach(variant IN ITEMS default extended)
  set(VITA_PACK_VPK_FLAGS "${base_pack_flags}")
  if(variant STREQUAL "default")
    set(titleid NFBN00002)
    set(extended 0)
    set(VITA_MKSFOEX_FLAGS "${base_sfo_flags}")
  else()
    set(titleid NFBN00003)
    set(extended 1)
    set(VITA_MKSFOEX_FLAGS "${base_sfo_flags} -d ATTRIBUTE2=12")
  endif()
  add_executable(memcheck_${variant} src/memcheck.cpp)
  target_compile_definitions(memcheck_${variant} PRIVATE NFMEM_TITLEID="${titleid}" NFMEM_EXTENDED=${extended})
  target_compile_features(memcheck_${variant} PRIVATE cxx_std_20)
  vita_create_self(memcheck_${variant}.self memcheck_${variant})
  vita_create_vpk(memcheck_${variant}.vpk ${titleid} memcheck_${variant}.self
    VERSION 01.00
    NAME "Nightfall memcheck (${variant})")
endforeach()

# abiprobe (src/abiprobe.cpp): thread_local on std and Sce threads, and which loads
# fault at which alignment, for the heaps' minimum alignment.
set(VITA_PACK_VPK_FLAGS "${base_pack_flags}")
set(VITA_MKSFOEX_FLAGS "${base_sfo_flags}")
add_executable(abiprobe src/abiprobe.cpp)
target_compile_features(abiprobe PRIVATE cxx_std_20)
target_compile_options(abiprobe PRIVATE -O2)
target_link_libraries(abiprobe pthread)
vita_create_self(abiprobe.self abiprobe)
vita_create_vpk(abiprobe.vpk NFBN00007 abiprobe.self
  VERSION 01.00
  NAME "Nightfall ABI probe")

# Stream decode cost while playing through SDL3 audio (src/streambench.cpp): Opus
# with libopus in float and in fixed point, and the disc's AFC ADPCM for
# comparison. The samples (make_stream_samples.py) hold disc audio, so they are
# passed in rather than kept in the repo.
#   -DNFBENCH_SAMPLES=$PWD/build/vita-bench/stream
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)
set(SDL_VIDEO OFF)
set(SDL_RENDER OFF)
set(SDL_GPU OFF)
set(SDL_CAMERA OFF)
set(SDL_JOYSTICK OFF)
set(SDL_HAPTIC OFF)
set(SDL_SENSOR OFF)
FetchContent_Declare(SDL3 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-3.4.16
                     GIT_SHALLOW TRUE)
FetchContent_Declare(opus GIT_REPOSITORY https://github.com/xiph/opus.git GIT_TAG v1.6.1
                     GIT_SHALLOW TRUE SOURCE_SUBDIR nf-sources-only)
FetchContent_MakeAvailable(SDL3 opus)

# libopus's own CMake build, twice from one checkout. NEON is presumed: there is no
# run-time CPU detection on the Vita.
include(ExternalProject)
foreach(variant IN ITEMS float fixed)
  if(variant STREQUAL "float")
    set(fixed OFF)
  else()
    set(fixed ON)
  endif()
  set(prefix ${CMAKE_BINARY_DIR}/opus-${variant})
  ExternalProject_Add(opus_${variant}_build
    SOURCE_DIR ${opus_SOURCE_DIR}
    INSTALL_DIR ${prefix}
    CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_BUILD_TYPE=Release
               -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DOPUS_FIXED_POINT=${fixed}
               -DOPUS_PRESUME_NEON=ON -DOPUS_BUILD_TESTING=OFF -DOPUS_BUILD_PROGRAMS=OFF
    BUILD_BYPRODUCTS ${prefix}/lib/libopus.a)
  file(MAKE_DIRECTORY ${prefix}/include/opus)
  add_library(opus_${variant} STATIC IMPORTED)
  set_target_properties(opus_${variant} PROPERTIES IMPORTED_LOCATION ${prefix}/lib/libopus.a
                        INTERFACE_INCLUDE_DIRECTORIES ${prefix}/include/opus)
  add_dependencies(opus_${variant} opus_${variant}_build)
endforeach()

# name: title ID, sample file, codec definitions, extra libraries/sources.
set(opusbench_float_args NFBN00004 opus.bin "NFOPUS_FIXED=0" opus_float)
set(opusbench_fixed_args NFBN00005 opus.bin "NFOPUS_FIXED=1" opus_fixed src/opus_fixed_xcorr.c)
set(afcbench_args        NFBN00006 afc.bin  "NFBENCH_AFC=1")
foreach(name IN ITEMS opusbench_float opusbench_fixed afcbench)
  list(POP_FRONT ${name}_args titleid sample defs)
  set(extra_sources ${${name}_args})
  list(FILTER extra_sources INCLUDE REGEX "\\.c$")
  list(FILTER ${name}_args EXCLUDE REGEX "\\.c$")
  add_executable(${name} src/streambench.cpp ${extra_sources}
    ${VITASDK}/share/gcc-arm-vita-eabi/samples/common/debugScreen.c)
  target_include_directories(${name} PRIVATE ${VITASDK}/share/gcc-arm-vita-eabi/samples/common)
  target_compile_definitions(${name} PRIVATE ${defs})
  target_compile_features(${name} PRIVATE cxx_std_20)
  target_compile_options(${name} PRIVATE -O2)
  target_link_libraries(${name} SDL3::SDL3-static ${${name}_args}
    SceDisplay_stub ScePower_stub SceAudio_stub SceAudioIn_stub pthread)

  set(VITA_PACK_VPK_FLAGS "${base_pack_flags}")
  set(VITA_MKSFOEX_FLAGS "${base_sfo_flags}")
  set(sample_files)
  if(NFBENCH_SAMPLES)
    set(sample_files FILE ${NFBENCH_SAMPLES}/${sample} sample.bin)
  endif()
  vita_create_self(${name}.self ${name})
  vita_create_vpk(${name}.vpk ${titleid} ${name}.self
    VERSION 01.00
    NAME "Nightfall stream bench (${name})"
    ${sample_files})
endforeach()
