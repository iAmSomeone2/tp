# CMake build

A host-agnostic CMake build of the GameCube engine, independent of `configure.py` (which keeps driving the Metrowerks matching build, untouched). It compiles the game and JSystem sources with a stock Clang or GCC and is the base for the port.

Requires CMake 4.0+ and Ninja. No Metrowerks compiler, disc image or decomp-toolkit is needed to configure or compile.

```sh
cmake --preset default            # Ninja, GameCube USA (GZ2E01), RelWithDebInfo -> build/cmake/default
cmake --build --preset default    # add `-- -k 0` to keep going past failing files
```

`debug` and `release` presets exist as well. Pick another compiler the usual way (`CXX=g++-14 cmake --preset default`, or `-DCMAKE_CXX_COMPILER=…`), and keep one build directory per compiler. The build directory lives under the already-ignored `build/`, next to (not inside) the Metrowerks outputs.

## Current status

Measured with Apple clang 21, `debug` preset (C++20), GameCube USA:

* **1,280 translation units** are compiled (1,281 for PAL/JPN).
* **1,274 compile; 6 do not.** Four need generated `assets/*.h` from a disc image (`d_a_grass`, `d_a_mant`, `d_a_player`, `m_Do_ext`; `d_error_msg` needed them too until its disc-error screen was removed, so it should now compile without a disc image, but the count here has not been re-measured) and need no source change. The sixth, `d_a_movie_player`, has six `(u32)&(h->maxCode)` sites that exist only in the PowerPC-assembly path and are deliberately left alone. History: 1,215, 1,248, 1,265, then 1,275 of 1,280 until `libs/dolphin/include/dolphin/types.h` was switched to `<cstdint>` types (which made `u32`/`s32` 32-bit on 64-bit hosts) dropped it to 313, then back up as the pointer casts and type mismatches of section F were fixed ([compiler-fixes.md](compiler-fixes.md)).
* **The executable `tp` is defined but cannot link yet.** `m_Do_main.cpp` is its entry point and `tp::engine` supplies everything else, but the SDK implementation is missing. The unit tests do link the real libraries, leniently (see Tests below), and the actor duplicate-symbol check is in [duplicate-symbols.md](duplicate-symbols.md).
* **GCC 16.2 (Fedora 44, `default` preset, GameCube USA, disc image present): 1,279 of 1,280 library TUs compile**; only `d_a_movie_player` fails, as with clang. Getting there needed two include-case fixes (`JASDSPInterface.h`, `J2DOrthoGraph.h`; the earlier macOS filesystem ignored case) and `std::isinf` in `JUTException.cpp` (libstdc++'s `<cmath>` has no global `isinf`). With GCC 16, CMake also scans every C++20 TU for modules, which roughly doubles the build graph; the top-level `CMakeLists.txt` turns that off (`CMAKE_CXX_SCAN_FOR_MODULES OFF`) until something uses modules. All 75 unit tests pass (see Tests below for the GNU ld link-order fix).

## What is built, and what is not

Targets mirror the libraries in `configure.py` and are static libraries named `tp_<name>` (alias `tp::<name>`).

| Target | Directory | Contents |
|---|---|---|
| `tp_machine` | `src/m_Do` | machine glue (`m_Do_*`), without `m_Do_main.cpp` |
| `tp` | `src/m_Do` | the game **executable**: `m_Do_main.cpp` (which holds `main`) plus `tp::engine` |
| `tp_c` | `src/c` | dynamic-link name table, damage-reaction data |
| `tp_framework`, `tp_f_pc_profile_lst`, `tp_DynamicLink` | `src` | process framework (`f_ap`/`f_op`/`f_pc`), profile table, REL name linking |
| `tp_dolzel` | `src/d` | game logic, including the 9 actors that live in the DOL |
| `tp_actors` | `src/d/actor` | the 756 actors that were separate RELs, as one static library |
| `tp_SSystem`, `tp_Z2AudioLib` | `src/SSystem`, `src/Z2AudioLib` | collision/math/lists, sound engine |
| `tp_JKernel`, `tp_J3DGraphBase`, … (19) | `libs/JSystem` | one library per JSystem module |
| `tp::engine` | (top level) | interface library linking all of the above |
| `tp::dolphin` | `libs/dolphin` | SDK **headers only** |

Deliberately **not** built, because a host or Vita build replaces them rather than compiling them:

* the Dolphin SDK implementation (`libs/dolphin/src`: OS, DVD, EXI, GX, card, …). It is PowerPC/Metrowerks code (the `.c` files include `<cstddef>`, use `asm`, MWCC pragmas). The port supplies its own implementation library for the `tp::dolphin` headers;
* the Metrowerks libraries (`MSL_C`, `Runtime.PPCEABI.H`, `TRK_MINNOW_DOLPHIN`): the host's C and C++ standard library replaces MSL, and TRK is a debugger stub;
* the REL loader (`src/REL`) and the debugger stubs (`amcstubs`, `odemuexi2`, `odenotstub`). The actors are linked statically instead;
* everything that exists only in Wii/Shield versions or only under `DEBUG` (HostIO, `Z2AudioCS`, `d_home_button`, widescreen, …). Per the project direction these are not part of the core and can be pulled in later.

## Layout

```
CMakeLists.txt             project, module path, subdirectories
CMakePresets.json          default / debug / release (Ninja)
cmake/
  TPPreProject.cmake       before project(): refuse in-source builds and non-Ninja generators
  TPChecks.cmake           refuse non-GCC/Clang toolchains
  TPConfig.cmake           TP_VERSION + options, the tp::config and tp::warnings targets
  TPLibrary.cmake          tp_add_library(), the tp::engine target, maintenance targets
libs/dolphin/              tp::dolphin (headers)
libs/JSystem/              tp::JSystem_headers + one library per module
src/                       framework, profile table, DynamicLink; subdirectories below
src/{m_Do,c,SSystem,Z2AudioLib,d,d/actor}/
src/nightfall/             nf_platform: host definitions of SDK globals (platform layer)
```

Each directory with sources has a hand-written `CMakeLists.txt` and a **generated** `sources.cmake` that defines explicit source lists (no globbing).

## Configuration

| Cache variable | Default | Meaning |
|---|---|---|
| `TP_VERSION` | `GZ2E01` | `GZ2E01` (USA), `GZ2P01` (PAL), `GZ2J01` (JPN). Sets `VERSION` to the matching `VERSION_GCN_*` value (0/1/2). PAL/JPN add one source file (`JUTFontData_Ascfont_fix12.cpp`) over USA; everything else is selected by `#if VERSION` in the code, as before. |
| `TP_BUILD_TESTS` | `OFF` | build `tests/` (GoogleTest, see `tests/CMakeLists.txt`) and enable `ctest`. Uses an installed GoogleTest if found, otherwise downloads a pinned release at configure time. Run with `ctest --test-dir build/cmake/<preset>`. |
| `TP_ENABLE_WARNINGS` | `OFF` | `-Wall -Wextra`. Off adds `-w`, because the sources are very noisy at this stage. |
| `TP_ASSET_DIR` | `assets/<TP_VERSION>` | committed asset headers |
| `TP_GENERATED_INCLUDE_DIR` | `build/<TP_VERSION>/include` | headers generated from a disc image (`assets/*.h`); the same place the Metrowerks flow writes them |
| `TP_GENERATE_ASSETS` | `AUTO` | add the `tp_generate_assets` target (see "Generated asset headers"): `AUTO` when a disc image is found in `orig/<TP_VERSION>`, `ON` to require one, `OFF` to skip |
| `TP_DTK_PATH` | (empty) | decomp-toolkit binary for that target; empty downloads the release pinned in `configure.py` to `build/tools/` |

`DEBUG`, widescreen (`WIDESCREEN_SUPPORT`) and `ENABLE_REGHIO` are never defined: this is the GameCube retail configuration. A CMake `Debug` build type means "unoptimised", not the game's `DEBUG` macro.

### How `configure.py`'s flags map

All game and JSystem libraries share the same effective flags in `configure.py`, so there is one `tp::config` interface target that every library links publicly (consumers of the headers must match it).

| `configure.py` (Metrowerks) | CMake |
|---|---|
| `-DVERSION=<n>` | `VERSION=<n>` |
| `-i include / build/<v>/include / assets/<v> / src / libs/JSystem/include / libs/dolphin/include{,/dolphin}` | same order, via `tp::config`, `tp::JSystem_headers`, `tp::dolphin` |
| `-i …/MSL/…`, `Runtime`, `MetroTRK` | dropped: host standard library |
| `-D__GEKKO__` | dropped: it is not needed by the sources and made `asm` in `m_Do_printf.cpp` fail |
| `-enum int` | `-fno-short-enums` |
| (default `char` is signed) | `-fsigned-char` (ARM defaults to unsigned) |
| `-fp_contract off` | `-ffp-contract=off` |
| `-RTTI off`, `-Cpp_exceptions off` | `-fno-rtti`, `-fno-exceptions` |
| (type punning such as `*(u32*)&x`) | `-fno-strict-aliasing` |
| language level | C++20 minimum via `tp::config` (legacy code includes `nightfall/compat/globals.hpp` through `global.h`, which needs `std::numbers`), no compiler extensions. A target that links something requiring a newer standard is compiled at that standard. |
| `-O*`, `-inline`, `-sym`, `-str`, `-schedule`, `-use_lmw_stmw`, `-sdata`, `-func_align`, … | dropped: codegen tuning for the PowerPC compiler |
| `-W` flags | `TP_BUILD_TESTS` | `OFF` | build `tests/` (GoogleTest, see `tests/CMakeLists.txt`) and enable `ctest`. Uses an installed GoogleTest if found, otherwise downloads a pinned release at configure time. Run with `ctest --test-dir build/cmake/<preset>`. |
| `TP_ENABLE_WARNINGS` |

The same flag set was checked against the existing sweep and produces no new failures.

## Keeping source lists in sync

`configure.py` plus `config/<version>/splits.txt` stay the authority for which files exist. `tools/utilities/gen_cmake_sources.py` combines them for the three GameCube versions and rewrites the `sources.cmake` files; run it whenever files are added, removed or move between libraries:

```sh
python3 tools/utilities/gen_cmake_sources.py          # rewrite
python3 tools/utilities/gen_cmake_sources.py --check  # exit 1 if stale
cmake --build build/cmake/default --target tp_update_sources   # same, via CMake
cmake --build build/cmake/default --target tp_check_sources
```

Objects are included by *existence in the GameCube splits*, regardless of their matching status (`Matching`, `NonMatching`, `Equivalent`): a host build has no reason to skip files that do not byte-match. Which library an object belongs to, and where that library lives in the CMake tree, is the `GROUPS` table at the top of the script.

## Generated asset headers

Four translation units (`d_a_grass`, `d_a_mant`, `d_a_player`, `m_Do_ext`) include `assets/*.h`: textures and display lists read out of the user's own disc image, so they are not in the repository. The `tp_generate_assets` target makes them with `tools/utilities/gen_asset_headers.py`, which runs what `configure.py`/ninja runs:

1. `dtk dol split config/<ver>/config.yml build/<ver>` (decomp-toolkit, downloaded to `build/tools/` on first use at the tag `configure.py` pins) reads the DOL and RELs from `orig/<ver>` and writes `build/<ver>/include/assets/*.h`;
2. `tools/converters/matDL_dis.py` converts the assets marked `custom_type: matDL`, which dtk leaves without a header.

The output is the directory `TP_GENERATED_INCLUDE_DIR` already points at, shared with the Metrowerks build. `tp_machine`, `tp_dolzel` and `tp_actors` depend on the target, so a normal build generates the headers first; it reruns only when the disc image, `config/<ver>/config.yml` or the scripts change. With `TP_GENERATE_ASSETS=AUTO` (default) the target exists only if `orig/<TP_VERSION>` holds a disc image; without one, those five files fail to compile as before. The script also runs standalone (`python3 tools/utilities/gen_asset_headers.py --version GZ2E01 [--dtk <binary>]`). The split also writes assembly and `config.json` into `build/<ver>`, the same tree the ninja build uses.

## Known gaps and decisions for later

* **Linking and a platform layer.** Needs an SDK implementation for `tp::dolphin` and a check of cross-library duplicate symbols. `tp::engine` already uses a rescanned link group where the linker supports one, because the libraries reference each other freely.
* **Precompiled headers.** The Metrowerks build uses `d/dolzel.pch` and friends; on stock compilers those are ordinary includes. CMake `target_precompile_headers` on `dolzel` would speed up builds but has not been tried.
* **`u32`/`s32` are now fixed-width** (`<cstdint>`) on every compiler, so they are 32-bit on 64-bit Linux. The pointer-cast fallout is fixed apart from `d_a_movie_player`; see section F of `compiler-fixes.md`. Casts that compile but still truncate (a `u32` holding a pointer) are the remaining risk.
* **Per-REL libraries.** All actors share one library. Keeping the REL boundaries would mean ~750 targets and is not useful once RELs are linked statically.

## Tests

Unit tests use GoogleTest and are off by default (`-DTP_BUILD_TESTS=ON`, then `ctest --test-dir <build dir>`). They mirror the source tree, leaving out the `src`/`include` levels: tests for `libs/JSystem/src/JMessage/resource.cpp` live in `tests/JSystem/JMessage/resource_test.cpp`, and each directory has its own `CMakeLists.txt` that calls `tp_add_test()` (defined in `tests/CMakeLists.txt`). Shared stand-ins for SDK functions and data are in `tests/support/`.

* **Linking the real code.** Every test links the real engine libraries (the JSystem tests use the group `tp_test_jsystem_libs`: JAudio2, JKernel, JMessage, JSupport, JUtility, JGadget), not copies of the sources. They only link what they need, not `tp::engine`, so they build even where unrelated engine files cannot (for example the ones that need generated assets). Earlier versions `#include`d the `.cpp` files to avoid duplicate symbols from header-defined globals; that stopped being necessary when the Dolphin headers stopped defining them (see "Platform layer" below).
* **Lenient linking.** Because the SDK implementation is not built, tests link with `LENIENT_LINK`: symbols they never call stay unresolved (`-undefined dynamic_lookup` on macOS, `--unresolved-symbols=ignore-all` on Linux). A call that does reach one crashes, so tests stub whatever they execute. References that the loader resolves at start-up (data, and functions whose address is taken) cannot be left unresolved, so they get stand-ins in `tests/support/load_time_stubs.hpp`. Windows has no equivalent here, so those tests are skipped there.
* **Current tests (75):**
  * `JMessage`: message-ID lookup, including a guard against walking the 32-bit ID table as 64-bit words.
  * `JAudio2`: `JASHeap` allocation (with a comparison against the original algorithm) and the ARAM chunk manager at 32-bit and 64-bit addresses.
  * `JUtility`: the `JUTCacheFont` page list.
  * `JKernel`: `JKRArchive::check_mount_already` (including mount keys that differ only above bit 31), the `JKRDecomp` thread loop with its callback, and the type contract of the async-load callback. The callback call inside `JKRDvdAramRipper::loadToAram_Async` is not executed, because it needs the DVD/ARAM stack; the test file says so.
* Each group was checked against deliberately broken versions of the code to confirm that it fails.
* **GNU ld link order.** GNU ld searches each archive once, in order, so `nf_platform` has to come after the libraries that use its globals. Under `--unresolved-symbols=ignore-all`, if it comes too early, `__OSBusClock` resolves to address 0 and every test segfaults in `JUTGamePad.cpp`'s global initialiser. `tp::dolphin` therefore links `nf_platform` (`libs/dolphin/CMakeLists.txt`), which puts it after every engine library. ld64 on macOS is not order-sensitive, which is why this did not show up there. `tp::engine` and the JSystem test group wrap the engine libraries in a `RESCAN` link group (`--start-group`). Both check `CMAKE_CXX_LINK_GROUP_USING_RESCAN_SUPPORTED` or `CMAKE_LINK_GROUP_USING_RESCAN_SUPPORTED`, because on Linux CMake sets only the second. `nf_platform` is not a member of the group: `tp::dolphin` already links it after the group, and membership would make a dependency cycle that CMake rejects. With GCC 16.2, `tp` then fails to link on 373 undefined symbols, none of which are defined anywhere in the engine: the Dolphin SDK (about 300), HostIO/JOR and debug-view code that the build omits, PowerPC intrinsics (`PPCMfmsr`, `__cntlzw`), and a few functions that no built file defines (for example `JSUOutputStream::write` and `GFSetFog`).

## Platform layer

The first pieces of the host platform layer exist as `nf_platform` (`src/nightfall/platform/`, alias `nf::platform`, linked through `tp::dolphin` by everything that uses the SDK headers):

* `os_globals.cpp` defines the Dolphin SDK globals that the headers only declare outside the Metrowerks build: `__OSExecParams`, `__OSAppLoaderOffset` and the console's clocks `__OSBusClock` (162 MHz) and `__OSCoreClock` (486 MHz). With Metrowerks these are variables pinned to low memory (`type name : (address)`), unchanged. Everywhere else the headers used to *define* two of them in every translation unit (duplicate symbols) and to turn the clocks into reads of a fixed address (a crash in any global initialiser that uses `OS_TIMER_CLOCK`).
* `libs/JSystem/src/JKernel/JKRHeap.cpp`: the global `operator new`/`new[]` and `operator delete`/`delete[]` fall back to the standard allocator while no JKR heap is current, and delete frees any pointer no JKR heap owns. On the console a heap always exists first, so nothing changes there (the change is `#ifndef __MWERKS__`); on a host the process would otherwise crash on its first `new`, even inside the standard library.
