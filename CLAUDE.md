# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

A fork of the [zeldaret/tp](https://github.com/zeldaret/tp) matching decompilation of *The Legend of Zelda: Twilight Princess*. Upstream's goal is byte-identical rebuilds of the GameCube/Wii/Shield binaries with the original Metrowerks compilers. **This fork is meant to diverge** and modernize the engine as a base for a native PS Vita port, so matching constraints are not sacred here (but note which changes would break the upstream matching build). The repo contains no game data; builds need the user's own disc image in `orig/<version>/`.

## Working agreement

The user does the development work themselves. **Edit source files only when explicitly asked.** Keeping `docs/` (and the generator scripts under `tools/utilities/`) up to date is always fine. Commit and push only when asked.

## Commands

Everything is driven by `configure.py` (generates `build.ninja` + `objdiff.json`) and `ninja`. Building requires `orig/<version>/` to contain a disc image; without it only the analysis scripts below work. There is no test suite and no linter beyond optional `clang-format` (`.clang-format`) and `.flake8` for the Python tools.

```sh
python configure.py [--version GZ2E01]   # default GZ2E01 (GCN USA); also GZ2P01 GZ2J01 RZDE01_00 RZDE01_02 RZDP01 RZDJ01 DZDE01 Shield ShieldD
ninja                                    # build and verify against config/<ver>/build.sha1
ninja all_source                         # compile every source file, including ones not yet linked
python configure.py progress             # print decompilation progress
python tools/decompctx.py src/d/<file>.cpp   # single-file context for a decomp.me scratch
```

Useful `configure.py` flags: `--map`, `--non-matching`, `--debug`, `--warn all|off|error`, `--reghio`. Diff objects with objdiff (loads the generated `objdiff.json`).

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
- **`configure.py` is the project manifest**: per-library compiler flags and every `Object(Matching|NonMatching|Equivalent, "file.cpp")`. `config/<ver>/{splits,symbols}.txt` define translation-unit boundaries and names for decomp-toolkit.
- **One source tree builds all versions** via `VERSION`/`PLATFORM_*`/`DEBUG` conditionals (`include/global.h`). Enum values and struct offsets can differ per version.
- **Global state** lives in `g_dComIfG_gameInfo` (`dComIfG_inf_c`): `dComIfGs_*` = save data, `dComIfGp_*` = play state, `dComIfGd_*` = draw lists.

## Gotchas

- `src/d/actor/*.inc` files (e.g. `d_a_alink_*.inc`, `d_grass.inc`) are `#include`d into one big TU, not compiled separately; `d_a_npc2.cpp` and `d_a_npc4.cpp` are included into `d_a_npc.cpp`.
- Many oddities exist only to match the original binary: `dummy()` functions, PCH include order (`d/dolzel.h`, `d/dolzel_rel.h`), weak-function ordering, `AUDIO_INSTANCES;`, `IS_REF_NULL`. `JUT_ASSERT(line, …)` line numbers are literal; don't renumber them.
- Misspellings in names (`cPhs_COMPLEATE_e`, `d_resorce`, `d_tresure`) are original and canonical.
- Community names in `@brief` comments and `field_0x…` members are guesses; confirm against sound IDs (`Z2SE_*`), resource names and debug strings.
- Switching file formats to little-endian, GX → GXM, DSP audio replacement and static-linking the RELs are the major port topics; the plan and measured numbers are in `docs/port/`.
