# 1. Project overview

## What this repository is

A **matching decompilation** of *The Legend of Zelda: Twilight Princess* (TP). "Matching" means the C++ in `src/` and `libs/`, once compiled with the original Metrowerks CodeWarrior compilers and linked, reproduces the original game executables **byte for byte** (verified against the SHA-1 in `config/<version>/build.sha1`).

* It contains **no game assets and no assembly**. You provide a disc image (placed under `orig/<version>/`); the build extracts the original binaries from it.
* It is **not a port**. It produces the original GameCube/Wii binaries, not a PC build.
* GameCube versions are fully matching at the binary level, but not every TU is linked yet (unfinished TUs fall back to the original machine code). Wii and Shield versions are still being brought up. See `README.md` and <https://decomp.dev/zeldaret/tp> for live progress.

## Game versions

`configure.py` defines these version codes. They are also the names of directories under `config/`, `orig/` and `assets/`, and of the CI build matrix (`.github/workflows/build.yml`).

| Code | Platform | Region / note | Built in CI |
|------|----------|---------------|:---:|
| `GZ2E01` | GameCube | USA (**default**) | ✔ |
| `GZ2P01` | GameCube | PAL | ✔ |
| `GZ2J01` | GameCube | Japan | ✔ |
| `RZDE01_00` | Wii | USA, revision 0 | ✔ |
| `RZDE01_02` | Wii | USA, revision 2 | ✔ |
| `RZDP01` | Wii | PAL | ✔ |
| `RZDJ01` | Wii | Japan | ✔ |
| `DZDE01` | Wii | USA kiosk demo | ✔ |
| `Shield` | Nvidia Shield | China, retail | ✔ |
| `ShieldD` | Nvidia Shield | China, **debug** build (has asserts, RTTI, HostIO, unoptimised) | ✔ |
| `RZDK01`, `DZDP01`, `ShieldP` | Wii KOR / Wii PAL demo / Shield production | declared but disabled | – |

The ID scheme is Nintendo's game ID: `GZ2` = Zelda Twilight Princess on GameCube (`Z2` also appears in `Z2AudioLib`), `RZD` = Wii, `DZD` = Wii demo; the 4th letter is the region (`E` USA, `P` PAL, `J` Japan, `K` Korea).

The main executable's internal name is **`framework`** (`Rframework` on Revolution/Wii/Shield, shipped as `sys/main.dol` on discs and `.alf` on Shield).

### Why the debug (`ShieldD`) version matters

The Shield debug build was compiled with assertions, symbol info and unoptimised code, so it reveals real function names, source file names, `JUT_ASSERT` conditions, argument names and struct layouts that stripped retail builds do not. Much of the naming in this repo comes from it. `JUT_ASSERT(353, FALSE);` (see `libs/JSystem/include/JSystem/JUtility/JUTAssert.h`) expands to a real check and `OSPanic(__FILE__, 353, …)` only when `DEBUG` is on; in retail builds it expands to nothing. The literal line numbers are kept so the debug build can match, and they also tell you the original file's line count around that code.

## One source tree, many binaries

All versions are built from the same files using conditional compilation (see `include/global.h`):

```c
VERSION_GCN_USA … VERSION_WII_USA_R0 … VERSION_SHIELD_DEBUG   // numeric IDs, 0..12
PLATFORM_GCN / PLATFORM_WII / PLATFORM_SHIELD                 // ranges of VERSION
REGION_USA / REGION_PAL / REGION_JPN / REGION_KOR / REGION_CHN
DEBUG                                                         // 1 for ShieldD (or --debug), else 0
```

Typical patterns you will run into (there are ~500 `VERSION` comparisons and ~2,900 `#if DEBUG` blocks):

```cpp
#if PLATFORM_WII || PLATFORM_SHIELD
    mReCPd::read();               // Wii remote / pointer input
#endif
#if DEBUG
    mDoMain_HIO.entryHIO("メイン"); // developer tooling compiled only into debug builds
#endif
#if VERSION != VERSION_WII_USA_R0 && VERSION != VERSION_WII_PAL
    /* 0x00F */ fpcNm_WARNING_SCENE_e,   // enum values shift between versions!
#endif
```

Consequences worth knowing:

* **Enum values and struct offsets can differ per version.** Do not assume `0x1B` means the same thing everywhere. Comments such as `/* 0x3 (0x6) */` show GCN and Wii/Shield values.
* Wii and Shield add features (pointer cursor `d_cursor_mng`, widescreen, HOME button `d_home_button`, extra attention types).
* Japanese comments/strings appear in debug builds (e.g. HostIO menu labels) because the developers' tooling was in Japanese; strings are Shift-JIS (`-enc SJIS`) on Wii/Shield.

## How the build works

```
orig/<ver>/  (disc image you provide)
     │  decomp-toolkit (dtk)   – extracts main.dol + RELs, splits them per config/<ver>/splits.txt,
     ▼                           names symbols per config/<ver>/symbols.txt
 original per-TU objects  ─┐
                           ├─►  linked together (Metrowerks linker)  ─►  build/<ver>/main.dol + *.rel
 your compiled src/ objects┘         which one wins per TU is decided by Object(Matching|NonMatching|…) in configure.py
     │
     ▼
 sha1 check vs config/<ver>/build.sha1   (and objdiff.json for the diffing GUI)
```

Key files:

| File | Purpose |
|------|---------|
| `configure.py` | **The project manifest.** ~3,000 lines. Declares compiler flags per library, the list of libraries and every `Object(status, "path.cpp")`, the RELs (`ActorRel(...)`), precompiled headers, progress categories. Run `python configure.py [--version X]` to generate `build.ninja` + `objdiff.json`. |
| `tools/project.py` | Generic dtk build-file generator shared between decomp projects (from `encounter/dtk-template`). Rarely edited. |
| `config/<ver>/config.yml` | dtk config: which binary, its hash, list of RELs (`modules:`), `force_active`, manual relocations. |
| `config/<ver>/splits.txt` | Says which address ranges of the original `.text/.data/.rodata/.bss…` belong to which source file. **This defines TU boundaries.** |
| `config/<ver>/symbols.txt` | Every known symbol (~26,700 lines for the GCN-USA DOL): name, address, size, scope. Names like `@1234` are compiler-generated locals. |
| `config/<ver>/rels/<rel>/{splits,symbols}.txt` | Same, for each REL. |
| `config/<ver>/build.sha1` | Expected hashes of the finished outputs. |
| `assets/<ver>/res/**/*.h` | Generated enum headers listing the file indices inside each game archive (see below). No actual assets. |
| `tools/` | `decompctx.py` (make a single-file context for <https://decomp.me>), `converters/res_arc.py` (generates the `assets/` headers from `.arc` archives), `utilities/*` (animation data beautifier, `greg_calc.py` for debug "register" HIO offsets, `weak_order_diff.py`), `rebuild-decomp-tp.py` (a leftover helper inherited from the Wind Waker decomp; not part of the build). |
| `include/d/dolzel*.pch`, `include/m_Do/machine.pch`, `libs/JSystem/include/JSystem/JSystem.pch` | Precompiled headers. Nearly every game `.cpp` begins with `#include "d/dolzel.h"` (DOL) or `"d/dolzel_rel.h"` (REL). The game was built with a PCH; matching the code often requires the same include set (`dolzel_base.pch` even pulls in a header that fixes weak `.bss` ordering). |

### Commands

```sh
python configure.py                 # default GZ2E01
python configure.py --version GZ2P01
python configure.py --debug         # non-matching debug-info build
ninja                               # build + verify hashes
ninja all_source                    # compile every source file, even non-linked ones
python configure.py progress        # print progress numbers
```

Useful `configure.py` options: `--map` (write linker maps), `--non-matching` (also link `Equivalent` objects), `--warn all|off|error`, `--reghio` (enable debug "register" HostIO in retail builds).

### Object status and "matching"

Inside `configure.py`, each source file is registered like this:

```python
Object(MatchingFor(ALL_GCN), "d/d_stage.cpp"),     # matches on the listed versions → linked
Object(NonMatching, "m_Do/m_Do_ext2.cpp"),         # not matching yet → original asm is used instead
Object(Equivalent, "d/d_particle.cpp"),            # behaves the same, but bytes differ (e.g. weak-function order); linked only with --non-matching
Object(Matching, "f_pc/f_pc_load.cpp"),            # matches on every version
```

`MatchingFor(...)` accepts version-group lists: `ALL_GCN`, `ALL_WII`, `ALL_DEMO`, `ALL_SHIELD`, or individual codes. A file that is "matching for GCN only" is often still *compiled* for the other versions (`ninja all_source`), it just isn't trusted to reproduce their bytes yet.

Comments after entries flag known blockers: `weak func order`, `debug weak literal order`, `RTTI`, `vtable order`. These are compiler-ordering quirks – the Metrowerks compiler emits inline/weak functions and string literals in an order that depends on exact usage across the whole TU (and header).

Progress is reported in four categories (`config.progress_categories`):

| Category | Meaning |
|----------|---------|
| `game` | Zelda-specific game code (`d_*`, actors, RELs, `c/`) |
| `core` | The Zelda engine core (`m_Do`, `f_*`, `DynamicLink`, `CaptureScreen`) |
| `sdk` | Nintendo Dolphin/Revolution SDK + REL glue |
| `third_party` | JSystem, SSystem, Z2 audio, MSL C library, Runtime, TRK, etc. |

### Compilers

Selected by `MWVersion()` in `configure.py` and run via `wibo` on macOS/Linux (downloaded automatically):

| Versions | Compiler | Notes |
|----------|----------|-------|
| GCN | `GC/2.7` | Base flags `-proc gekko -O4,p -inline auto -RTTI off -Cpp_exceptions off -fp hardware`; the game/framework layer overrides to `-O3,s -inline noauto -str reuse,pool,readonly`. |
| Wii (retail/demo) | `GC/3.0a3p1` (modified) | Patched to accept multi-char constants. Linker `GC/3.0a5`. |
| Shield | `Wii/1.0` | |
| Dolphin SDK libs | `GC/1.2.5n` | Each SDK library has its own flags. |

Flags differ by layer (`cflags_framework`, `cflags_jsystem`, `cflags_rel`, `cflags_dolphin`, `cflags_revolution_*`, `cflags_runtime`, `cflags_trk`): e.g. RELs get `-sdata 0 -sdata2 0` (no small-data sections), the SDK gets `-sym on`, `-fp_contract off`.

## Where the project's terminology comes from

* **DOL** – the main executable (`main.dol`). Contains the engine core, most `d_*` systems, the player, JSystem, SDK.
* **REL** – relocatable module loaded at runtime (Nintendo's "dynamic link" facility). TP ships ~750 of them, almost all one-actor-each, plus `f_pc_profile_lst` (the list of all process profiles).
* **TU** – translation unit.
* **dtk** – [decomp-toolkit](https://github.com/encounter/decomp-toolkit).
* **objdiff** – GUI that diffs your compiled object against the original, per function.
* **decomp.me scratch** – online per-function matching sandbox; `scratch_preset_id` 69 = DOL, 70 = REL.
* **HIO / HostIO** – Nintendo dev-kit "host I/O" debugging channel; classes derived from `JORReflexible` build tweakable-parameter panels on a PC. Only compiled in debug builds. Much of the "unused" data in retail (`*HIO` classes with `genMessage`) exists for this.
* **`fakematch` / `Equivalent` / `NonMatching`** – see above.

## Code-style conventions in the decomp

* Struct fields carry offsets: `/* 0x04 */ u32 mFoo;`. `STATIC_ASSERT(sizeof(X) == 0x24)` guards sizes.
* Names given by the team are member-prefixed (`mFoo`, `mpFoo` pointer, `field_0x1c` unknown), parameters are `i_` (in) / `o_` (out) prefixed, and unknown locals are `var_r30`, `sp8` style.
* Doxygen comments (`@ingroup actors-enemies`, `@brief …`) group classes for the generated site (`Doxyfile`, `docs/mainpage.h`).
* `.inc` files under `src/d/actor/` (e.g. `d_a_alink_*.inc`) are pieces of one huge TU that are `#include`d into the main `.cpp`; they are not separate objects.
* `clang-format` is optional (`.clang-format`).
