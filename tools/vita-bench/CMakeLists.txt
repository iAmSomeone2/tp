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
