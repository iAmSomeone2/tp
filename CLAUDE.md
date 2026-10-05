# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

A fork of the [zeldaret/tp](https://github.com/zeldaret/tp) matching decompilation of *The Legend of Zelda: Twilight Princess*. Upstream's goal is byte-identical rebuilds of the GameCube/Wii/Shield binaries with the original Metrowerks compilers. **This fork is meant to diverge** and modernize the engine as a base for a native PS Vita port, so matching constraints are not sacred here (but note which changes would break the upstream matching build). The repo contains no game data; builds need the user's own disc image in `orig/<version>/`.

## Working agreement

The user does the development work themselves. **Edit source files only when explicitly asked.** Only assume the user wants you to make edits if you can determine their intention beyond a reasonable doubt; otherwise, ask! Keeping `docs/` (and the generator scripts under `tools/utilities/`) up to date is always fine. Commit and push only when asked.

## New code: Nightfall (`nf`)

New modules of this fork are called Nightfall and kept apart from the decompiled `tp`/`d_*` code. The C++ namespace is `nf`. Layout: public headers in `include/nightfall/<module>/` (e.g. `nightfall/compat/globals.hpp`), sources in `src/nightfall/`, tests in `tests/nf_<module>/`. CMake targets are `nf_<module>` with alias `nf::<module>`.

New code should be modern C++ and need not match the legacy style. The whole project is built as C++20 (`tp::config` requires `cxx_std_20`, because `global.h` includes `nightfall/compat/globals.hpp`, which uses `std::numbers`), and a target may require something newer. Legacy code will be updated as needed too, and those updates will often raise the standard further. CMake compiles a target at the highest standard requested by anything it links, so linking an `nf` library that needs a newer standard into a legacy library raises that library's standard.

## Commands

There are two independent builds. `configure.py` + `ninja` is the Metrowerks **matching** build (upstream's). The **CMake** build is this fork's host build with stock Clang/GCC and is the basis for the port; see the CMake section below. Building the matching build requires `orig/<version>/` to contain a disc image; without it only the analysis scripts below work. There is no test suite and no linter beyond optional `clang-format` (`.clang-format`) and `.flake8` for the Python tools.

```sh
python configure.py [--version GZ2E01]   # default GZ2E01 (GCN USA); also GZ2P01 GZ2J01 RZDE01_00 RZDE01_02 RZDP01 RZDJ01 DZDE01 Shield ShieldD
ninja                                    # build and verify against config/<ver>/build.sha1
ninja all_source                         # compile every source file, including ones not yet linked
python configure.py progress             # print decompilation progress
python tools/decompctx.py src/d/<file>.cpp   # single-file context for a decomp.me scratch
```

Useful `configure.py` flags: `--map`, `--non-matching`, `--debug`, `--warn all|off|error`, `--reghio`. Diff objects with objdiff (loads the generated `objdiff.json`).

### CMake build (host Clang/GCC, GameCube only)

Needs CMake 4.0+ and Ninja; no disc image or Metrowerks tools. Targets GameCube USA by default; Wii/Shield-only features (widescreen, `DEBUG`, HostIO, `Z2AudioCS`, ...) are deliberately not part of it. Details in `docs/port/cmake.md`.

```sh
cmake --preset default                    # also: debug, release; output in build/cmake/<preset>
cmake --build --preset default -- -k 0    # -k 0 keeps going past files that still fail
cmake -S . -B build/cmake/pal -G Ninja -DTP_VERSION=GZ2P01   # GZ2E01 (default) / GZ2P01 / GZ2J01
cmake --preset default -DTP_BUILD_TESTS=ON && ctest --test-dir build/cmake/default   # unit tests (GoogleTest)
python3 tools/utilities/gen_cmake_sources.py [--check]       # regenerate / verify the sources.cmake lists
```

Status: 313 of 1,280 TUs compile with clang. The typedefs in `types.h` were just made fixed-width (32-bit `u32`/`s32`, needed for 64-bit Linux), and the 85 remaining error sites, mostly pointer casts, are section F of `docs/port/compiler-fixes.md` (1,275 compiled before that change). Nothing links yet (no SDK implementation or entry point). GCC is untested.

Tests (GoogleTest, `-DTP_BUILD_TESTS=ON`) mirror the source tree without the `src`/`include` levels, e.g. `tests/JSystem/JMessage/` for `libs/JSystem/src/JMessage/`; add new ones with `tp_add_test()` in that directory's `CMakeLists.txt`. They link the real engine libraries, leniently, since the SDK implementation is not built (stand-ins for what they execute are in `tests/support/`). `src/nightfall/platform/` (`nf_platform`) holds the host definitions of SDK globals that the Dolphin headers only declare outside Metrowerks. See `docs/port/cmake.md`.

Port-analysis scripts (host clang, no disc image needed); each takes a minute or two and rewrites a generated page under `docs/port/`:

```sh
python3 tools/utilities/clang_sweep.py --report docs/port/compile-sweep.md        # how many TUs parse with stock clang
python3 tools/utilities/dup_symbol_check.py --report docs/port/duplicate-symbols.md  # symbols that would collide in a static link
python3 tools/utilities/port_survey.py                                            # writes docs/port/survey-data.md
python3 tools/utilities/gen_actor_index.py                                        # writes docs/actor-index.md
```

When their numbers change, also update prose that quotes them (e.g. in `docs/port/README.md` and `docs/port/toolchain.md`).

## Architecture (big picture)

Read `docs/README.md` first; it indexes a full orientation guide. The essentials:

- **Layers:** `m_Do_*` (machine glue) → `f_pc`/`f_op`/`f_ap` (the "framework": process scheduler and typed processes) → `d_*` ("dolzel" game logic, `d_a_*` = actors) on top of `SSystem` (`c_*` math/collision/lists), `libs/JSystem` (Nintendo middleware: J3D/J2D/JKernel/JAudio2/JStudio…), `Z2AudioLib`, and the Dolphin/Revolution SDKs under `libs/`.
- **Everything alive is a process** created from a `g_profile_*` descriptor (name in `include/f_pc/f_pc_name.h`, listed in `src/f_pc/f_pc_profile_lst.cpp`). Each frame `fpcM_Management` runs delete → create (multi-frame `cPhs_*` phases) → execute (16 ordered "lines") → draw handlers. Scenes own a layer of child processes; changing stage deletes the old scene's layer. See `docs/03-engine-architecture.md`.
- **Actors are ~750 separate REL modules** (`ActorRel(...)` in `configure.py`, name table in `src/c/c_dylink.cpp`) loaded on demand; the player (`d_a_alink`), NPC/object base classes and core systems are in the DOL. Placement data comes from stage/room files on the disc; `l_objectName[]` in `src/d/d_stage.cpp` maps editor names to process names.
- **`configure.py` is the project manifest**: per-library compiler flags and every `Object(Matching|NonMatching|Equivalent, "file.cpp")`. `config/<ver>/{splits,symbols}.txt` define translation-unit boundaries and names for decomp-toolkit. The CMake build derives its source lists from these (via `gen_cmake_sources.py`), so it stays in sync only if you regenerate.
- **One source tree builds all versions** via `VERSION`/`PLATFORM_*`/`DEBUG` conditionals (`include/global.h`). Enum values and struct offsets can differ per version.
- **Global state** lives in `g_dComIfG_gameInfo` (`dComIfG_inf_c`): `dComIfGs_*` = save data, `dComIfGp_*` = play state, `dComIfGd_*` = draw lists.

## Gotchas

- `src/d/actor/*.inc` files (e.g. `d_a_alink_*.inc`, `d_grass.inc`) are `#include`d into one big TU, not compiled separately; `d_a_npc2.cpp` and `d_a_npc4.cpp` are included into `d_a_npc.cpp`.
- Many oddities exist only to match the original binary: `dummy()` functions, PCH include order (`d/dolzel.h`, `d/dolzel_rel.h`), weak-function ordering, `AUDIO_INSTANCES;`, `IS_REF_NULL`. `JUT_ASSERT(line, …)` line numbers are literal; don't renumber them.
- Misspellings in names (`cPhs_COMPLEATE_e`, `d_resorce`, `d_tresure`) are original and canonical.
- Community names in `@brief` comments and `field_0x…` members are guesses; confirm against sound IDs (`Z2SE_*`), resource names and debug strings.
- Switching file formats to little-endian, GX → GXM, DSP audio replacement and static-linking the RELs are the major port topics; the plan and measured numbers are in `docs/port/`.
- **CMake source lists are generated.** `sources.cmake` files (in `src/`, `src/*/`, `src/d/actor/`, `libs/JSystem/`) are written by `tools/utilities/gen_cmake_sources.py`; don't hand-edit them. Re-run it after adding, removing or moving a source file or changing a library in `configure.py`. The CMake build compiles every file present in the GameCube splits regardless of matching status.
- **The CMake build omits the Dolphin SDK implementation, Metrowerks libs (MSL, Runtime, TRK) and the REL loader** on purpose; only SDK headers (`tp::dolphin`) are used and the host's standard library replaces MSL. It does not define `__GEKKO__`, `DEBUG`, `WIDESCREEN_SUPPORT` or `ENABLE_REGHIO`.
- `.gitignore` ignores root-level `*.txt`; `CMakeLists.txt` is explicitly re-included.
