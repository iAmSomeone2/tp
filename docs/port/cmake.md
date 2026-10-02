# CMake build

A host-agnostic CMake build of the GameCube engine, independent of `configure.py` (which keeps driving the Metrowerks matching build, untouched). It compiles the game and JSystem sources with a stock Clang or GCC and is the base for the port.

Requires CMake 4.0+ and Ninja. No Metrowerks compiler, disc image or decomp-toolkit is needed to configure or compile.

```sh
cmake --preset default            # Ninja, GameCube USA (GZ2E01), RelWithDebInfo -> build/cmake/default
cmake --build --preset default    # add `-- -k 0` to keep going past failing files
```

`debug` and `release` presets exist as well. Pick another compiler the usual way (`CXX=g++-14 cmake --preset default`, or `-DCMAKE_CXX_COMPILER=…`), and keep one build directory per compiler. The build directory lives under the already-ignored `build/`, next to (not inside) the Metrowerks outputs.

## Current status

Measured with Apple clang 21, `debug` preset, GameCube USA:

* **1,280 translation units** are compiled (1,281 for PAL/JPN).
* **1,215 compile; 65 do not.** The 65 are exactly the files an independent syntax sweep of the same sources predicts (same set, none extra, none missing), so the CMake setup itself adds no failures. Each is covered by the punch list in [compiler-fixes.md](compiler-fixes.md): mainly the missing `DEG_TO_RAD`/`RAD_TO_DEG` macros, the `JUTConsole.h` template narrowing, `switch` jumps past initialisation, and five files that need generated `assets/*.h`.
* **Nothing is linked yet.** There is no executable target: the SDK implementation is missing and the libraries have not been checked for duplicate symbols ([duplicate-symbols.md](duplicate-symbols.md)).
* **GCC is untested** (not installed on the machine this was written on). The flags are all standard GCC options and are probed with `check_compiler_flag`, but expect the GCC-only diagnostics listed in `compiler-fixes.md`.

## What is built, and what is not

Targets mirror the libraries in `configure.py` and are static libraries named `tp_<name>` (alias `tp::<name>`).

| Target | Directory | Contents |
|---|---|---|
| `tp_machine` | `src/m_Do` | machine glue (`m_Do_*`) |
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

## Known gaps and decisions for later

* **Linking and a platform layer.** Needs an SDK implementation for `tp::dolphin`, an entry point (`m_Do_main.cpp` has `void main`), and a check of cross-library duplicate symbols. `tp::engine` already uses a rescanned link group where the linker supports one, because the libraries reference each other freely.
* **Precompiled headers.** The Metrowerks build uses `d/dolzel.pch` and friends; on stock compilers those are ordinary includes. CMake `target_precompile_headers` on `dolzel` would speed up builds but has not been tried.
* **Generated asset headers.** Five TUs (`d_a_grass`, `d_a_mant`, `d_a_player`, `d_error_msg`, `m_Do_ext`) need `assets/*.h` extracted from a disc image by the existing decomp-toolkit flow. CMake only exposes the include path; it does not generate them.
* **`u32` is `unsigned long`** (`libs/dolphin/include/dolphin/types.h`), so it is 64-bit on LP64 hosts. See `compiler-fixes.md` before depending on this for anything but compile checks.
* **Per-REL libraries.** All actors share one library. Keeping the REL boundaries would mean ~750 targets and is not useful once RELs are linked statically.
