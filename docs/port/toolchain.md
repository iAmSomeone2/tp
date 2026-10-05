# Toolchain and language portability

The code was written for Metrowerks CodeWarrior (`mwcceppc`) targeting 32-bit big-endian PowerPC EABI. A Vita build uses GCC or Clang for 32-bit little-endian ARMv7 (VitaSDK). This page covers how far the source is from that, what compiler flags the original semantics need, and what to change.

## How far is the tree from a stock compiler?

`tools/utilities/clang_sweep.py` runs `clang++ -std=c++20 -fsyntax-only` over every translation unit under `src/` and `libs/JSystem/src`, **without any Metrowerks or MSL headers** (GameCube USA configuration, `DEBUG` off, host pointer size). Result, from [compile-sweep.md](compile-sweep.md):

* **1,346 of 1,383 TUs (97.3%) parse cleanly.** (This was 1,293 before the `DEG_TO_RAD`/`RAD_TO_DEG` macros moved into `include/nightfall/compat/globals.hpp`, which `global.h` includes on non-Metrowerks compilers and which needs C++20, and the narrowing, `case`-label and `main` fixes that followed.)
* The 37 remaining failures fall into a handful of mechanical categories:

| Cause | Files | Fix |
|---|---|---|
| ~~`DEG_TO_RAD` / `RAD_TO_DEG` missing (provided by MSL `<cmath>`)~~ | ~~37~~ 0 | Fixed: defined in `include/nightfall/compat/globals.hpp` using `std::numbers::pi_v<float>`. |
| `switch` jumps past initialisation (`cannot jump from switch statement to this case label`) | 4 (38 errors, mostly `d_a_mg_rod.cpp`) | Wrap the case bodies in braces. |
| ~~Narrowing in template arguments/case labels, e.g. `-offsetof(...)` in `JUTConsole.h`~~ | ~~21~~ 0 | Fixed with explicit casts. |
| Wii/Shield-only SDK headers (`revolution/…`) | 14 | Not part of a GameCube configuration: `Z2AudioCS` (8 files, Wii-remote speaker), `m_Re` (remote pad), `d_cursor_mng`, `d_home_button`, `Z2SoundPlayer`, and 2 HostIO/MCC files. Exclude, or reimplement if you want touch/gyro-driven equivalents. |
| Debug-only HostIO classes (`JORReflexible`, `getJORServer`, …) | ~8 | Compile out (`DEBUG` off) or drop HostIO entirely. |
| `va_start`/`va_end`, `stricmp`/`strnicmp`, `JAUSectionHeap` declaration, `asm` in `m_Do_printf.cpp` (the last only because the sweep forces `-D__GEKKO__`) | 6 | Include `<cstdarg>`, provide the two string helpers, include or forward-declare `JAUSectionHeap`. (`main`'s return type is fixed.) |
| Generated asset headers missing (`assets/…`) | 5 | Produced from your disc image by the build (see [../01-project-overview.md](../01-project-overview.md)). |
| Pointer cast to a smaller integer | 2 | Only fails on 64-bit hosts; fine on 32-bit ARM. Use `uintptr_t`. |

A deeper check confirms the picture: compiling all actor TUs to **object code** (`-c -O0`) succeeds for 758 of 765 files (the same failure list), so the game-object layer is very close to building.

This is not evidence that the game *works*: it is a syntax/codegen pass on a 64-bit host. It does say the remaining language-level work is small compared with the platform work (graphics, audio, I/O, data).

Upstream has already merged compatibility fixes for GCC/Clang (`<string>` → `<cstring>`, `std::isnan`, `MULTI_CHAR(...)` macro for >4-char literals, `NULL` → `nullptr`, `uintptr_t` for pointer/integer casts). Pulling those in is the cheapest way to stay ahead of the compiler.

## Compiler flags to reproduce Metrowerks semantics

The original build flags (`configure.py`) encode assumptions the source relies on. Set the equivalents explicitly rather than trusting toolchain defaults.

| Original (MWCC) | Meaning | GCC/Clang equivalent |
|---|---|---|
| `-char signed` (game/JSystem); `-char unsigned` only for the Dolphin SDK | Plain `char` is signed in game code. ARM EABI defaults to **unsigned**. | `-fsigned-char` |
| `-enum int` | Enums are `int`-sized. Some ARM bare-metal toolchains default to `-fshort-enums`. | `-fno-short-enums` |
| `-Cpp_exceptions off`, `-RTTI off` (retail) | No exceptions/RTTI. | `-fno-exceptions -fno-rtti` |
| `-fp hardware`, `-fp_contract on` | Hardware FP; fused multiply-add allowed. | `-ffp-contract=fast` is *not* recommended; values differ slightly anyway (see below). |
| `-multibyte` (GCN) | Multi-character constants like `'J3D2'` and `'n_all'` are valid integer literals. | Works by default (`-Wno-multichar`); >4 chars need the `MULTI_CHAR` macro already in `global.h`. |
| `-str reuse,pool,readonly` | String pooling. | Default behaviour. |
| (no strict-aliasing model) | The code freely casts between pointer types (resource blobs, `f32`↔`u32` tricks). | `-fno-strict-aliasing` |
| Signed overflow wraps | Typical for embedded PPC code. | `-fwrapv` |
| `this`/reference null checks | The original tests `&ref == NULL` (`IS_REF_NULL`, 25 uses) and, in two places, `this == NULL`. Modern compilers may delete such checks. | `-fno-delete-null-pointer-checks`; `global.h` already maps `IS_REF_NULL` to `0` off-MWCC, review each use. |

## Type sizes and layout

* **Pointers and `long` are 4 bytes on both** the PowerPC EABI and 32-bit ARM. Structs annotated `/* 0x.. */` and file-embedded pointer-sized fields (e.g. `ResTIMG::imageOffset` is a `uintptr_t`) keep their layout. A 64-bit desktop build would break these; the Vita build does not. This is a genuine advantage of the target.
* `u32`/`s32` (and the other integer typedefs) are now defined from `<cstdint>` in `libs/dolphin/include/dolphin/types.h`, so they are 32-bit on 64-bit Linux as well as on ARM; before, `unsigned long`/`long` made them 64-bit on LP64. Fixing the pointer casts that fell out of this is in progress; see section F of [compiler-fixes.md](compiler-fixes.md).
* **Bit-fields** (62 in 7 files, mostly JAudio2 `JAISound.h`, `JASTrack.h`, `Z2SeqMgr.h`) allocate from the most significant bit on big-endian and from the least significant on little-endian. Fine for purely in-memory state, wrong if the struct overlays file/hardware data. See [endianness.md](endianness.md).
* **Pointer-to-member** types appear in ~610 places (336 files), mostly state-machine tables such as `typedef void (dFoo_c::*procFunc)()`. Their size differs between MWCC (12 bytes) and the Itanium ABI used on ARM (8 bytes). It only matters where code depends on absolute offsets into such classes; the many `/* 0x… */` offset comments and `STATIC_ASSERT(sizeof(X) == 0x..)` (788 of them) will not hold for classes containing member pointers.
* **Virtual tables and multiple inheritance** follow different ABIs; nothing in the game depends on vtable layout, but decomp-era hacks that reorder or force vtable/`weak` emission are irrelevant on a new toolchain.
* `STATIC_ASSERT` is only enabled for MWCC GCN-USA (see `include/global.h`). Enabling it for the Vita build turns those 788 checks into a **struct-layout regression suite** for anything that reads binary data; expect the member-pointer classes above to need their asserts relaxed.

## Metrowerks-isms in the source

| Feature | Amount | Plan |
|---|---|---|
| `asm { … }` functions and inline asm | 110 occurrences, 27 files; 38 in `d_a_movie_player.cpp` (THP video decode) | J3D/JMath/JGeometry asm blocks are already guarded by `#ifdef __MWERKS__` with C fallbacks (`#if DEBUG \|\| !defined(__MWERKS__)`). The THP decoder and `m_Do_printf.cpp` need real ports. |
| PPC intrinsics (`__dcbz`, `__cntlzw`, `__frsqrte`, `__fres`, `__abs`, `__rlwimi`, `__sync`) | 27 uses | Replace with C, `__builtin_clz`, or plain division/`sqrtf`. `__dcbz` (13) zeroes a cache line: use `memset`. |
| Data-cache/locked-cache operations (`DCStoreRange`, `DCInvalidateRange`, `DCFlushRange`, `LC*`) | ~150 calls | No-ops on a cache-coherent CPU (keep as empty functions during the port, but note DMA-to-GPU/audio buffers may need real cache maintenance on the Vita). |
| OS fast-cast helpers (`OSf32tos16`, `OSu8tof32`, …) | 33 uses, in J2D/J3D animation | Implemented with PPC quantised load/store registers; replace with C casts that saturate (a plain C float→int cast is undefined for out-of-range values). |
| `#pragma` (`push/pop`, `force_active`, `dont_inline`, `optimization_level`, `section`, `pack`) | ~140 | Delete the decomp-only ones; keep `pack` where a struct maps binary data. |
| `__declspec(section …)` / `weak` | 10 | Remove/`__attribute__((weak))`. |
| `-multibyte` constants | ~4,000 occurrences in ~146 files | Mostly J2D pane names such as `'n_all'` (up to 8 chars → `u64`). Works through `MULTI_CHAR`. Values are big-endian *by construction*, so they compare correctly with byte-swapped file data. |
| `AT_ADDRESS(...)` | header-level SDK globals | Empty off-MWCC. This is why `__OSExecParams` and `__OSAppLoaderOffset` are *defined* in every TU (see below). |
| Decomp compatibility hacks (`dummy()`/`dummy2()`/`dummyLiteral()` functions, PCH include order, `IS_REF_NULL`) | dozens | Delete once matching no longer matters. |

### Floating point

The Gekko FPU differs from ARM VFP in ways that can shift simulation results slightly: fused multiply-add (`fmadds` is compiled from `-fp_contract on`), estimate instructions (`fres`, `frsqrte`), paired-single math in hand-written asm, and denormal/rounding behaviour. Game logic is not required to be bit-identical to the original, but keep in mind that (a) trigonometric tables and `cM_` helpers are hand-rolled, so they will behave the same; (b) tuning constants in `d_a_alink_HIO_data.inc` assume the original math; and (c) some debugging "TEV/tolerance" checks compare floats exactly.

## Actors: RELs become a static link

The original loads ~750 actor RELs on demand through `DynamicModuleControl` (`src/DynamicLink.cpp`) and a name table in `src/c/c_dylink.cpp`. With 512 MiB RAM, link them all into one executable:

* `cDyl_*` and `fpcLd_*` become trivial (every process is "already linked"); `DMC[]` slots are `NULL` in the original for DOL-resident code, and the same logic can apply to everything.
* The profile list `g_fpcPfLst_ProfileList[]` is already a plain table of `&g_profile_*` (see `src/f_pc/f_pc_profile_lst.cpp`), so it works as-is once the profiles are in the same binary. Keep it ordered by `fpcNm_*_e`.
* **Symbol collisions.** Because each REL was its own link unit, actor TUs can define the same non-static global. `tools/utilities/dup_symbol_check.py` compiled 752 of 765 actor TUs and found only **20 duplicated strong symbols** ([duplicate-symbols.md](duplicate-symbols.md)):
  * `__OSExecParams` and `__OSAppLoaderOffset`: header-defined SDK globals emitted into every TU because `AT_ADDRESS` is empty off-MWCC. Make them `extern` in the header, define once.
  * `l_HIO` (24 files), `hio_set` (6), `l_arcName` (3), `l_evtList` (2), `target_info*`, `jv_offset`, `jc_data`, `c_start`: file-local names that should be `static`.
  * `dummy()`, `dummy2()`, `dummyLiteral()`, `dummyString()`, `dummy_lit_3931()`: decomp-only ordering hacks; delete.
  * `daB_DS_c::getHandPosL/R`, `daObj_SSBase_c::setSoldOut`, `useHeapInit`: real functions defined in more than one TU.
  The check covers actors only; DOL-resident TUs may add more.
* Static constructors: REL `_prolog` ran each module's constructors (`src/REL/executor.c`). In one executable the normal C++ static-initialisation order applies; watch for order-dependent globals (`JASGlobalInstance<T>` singletons, the `AUDIO_INSTANCES` macro that forces template statics into every TU, upstream fixed related definitions in #3108).
* Memory: the original frees a REL's code when its last user dies. That accounting (`DynamicModuleControl::dump`, heap `mDoExt_getArchiveHeap`) can be deleted.

## Build system

Upstream's `configure.py`/`ninja` flow runs the Metrowerks toolchain via `wibo` and compares hashes. A Vita build wants CMake (VitaSDK ships a toolchain file and packaging helpers). Suggested shape:

* one CMake target per current "library" in `configure.py` (`machine`, `framework`, `dolzel2`, `SSystem`, `JSystem/*`, `Z2AudioLib`), plus a new `platform/` layer (see [sdk-surface.md](sdk-surface.md));
* the ~800 actor TUs as one static library;
* generated asset headers (`assets/<ver>/…`, `build/<ver>/include`) come from the disc image; keep the extraction step from upstream or replace it with a one-time "asset cooking" tool (see [endianness.md](endianness.md));
* the precompiled headers (`dolzel.pch` etc.) are optional speed-ups; drop them at first.

Keep the upstream build working on a separate branch while the Vita build stabilises so that you can still pull upstream fixes and validate changes against the matching binaries.
