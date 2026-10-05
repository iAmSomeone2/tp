# Getting the tree to compile on stock Clang and GCC: punch list

Hand-written work list for the `task/std-compiler-fixes` branch. Each item gives the file and line where the compiler stopped and a suggested edit; `[x]` marks the ones that have been applied. The raw per-file output is in [compile-sweep.md](compile-sweep.md).

**Targets.** The GameCube retail configuration (USA, PAL, JPN) built with stock Clang and GCC on **64-bit Linux (LP64)** is a real target, alongside macOS on Apple silicon and, later, the 32-bit Vita. So LP64 problems (64-bit pointers, 64-bit `long`) are requirements here, not nice-to-haves. **ShieldD (`DEBUG=1`) and the Wii/Shield versions are not targets**; their items in section D are kept for reference and are not planned.

Measured with Apple clang 21 (`-std=c++20 -fsyntax-only`, no Metrowerks/MSL headers). **GCC was not available**, so GCC-only diagnostics are not covered; see [What this does not cover](#what-this-does-not-cover).

## Where things stand

For the GameCube file set (what the CMake build compiles: 1,280 files for USA):

* **Before the integer-typedef change, 1,275 of 1,280 compiled.** Every source item in sections A–C was done; the five that failed (`d_a_grass`, `d_a_mant`, `d_a_player`, `d_error_msg`, `m_Do_ext`) need generated `assets/*.h` from a disc image and no source change.
* **After `libs/dolphin/include/dolphin/types.h` was switched to `<cstdint>` types, 313 compiled and 967 failed** (85 error sites, section F). That was the expected fallout of making `u32`/`s32` 32-bit on a 64-bit host, not a regression elsewhere.
* **Now: 1,235 compile and 45 fail.** The two headers that took out nearly everything (`functionvalue.h`, `data.h`) are fixed. 72 error sites remain: 69 pointer casts in 39 `.cpp` files, and 3 type mismatches in 2 files. Five of the 45 failing files (`d_a_grass`, `d_a_mant`, `d_a_player`, `d_error_msg`, `m_Do_ext`) also need generated `assets/*.h`, which needs no source change; `m_Do_ext` is also among the files with a cast.

History, as failing files in the CMake build: 65 at the first build, 32 after the math macros, 15 after the narrowing and `case`-label fixes, 5, then 967 once the typedefs changed, then 45.

The generated reports (`compile-sweep.md`, `duplicate-symbols.md`) were **not** regenerated for this state: with the typedefs half-migrated they would show a thousand failures and no useful actor symbol data. Regenerate them once section F is done.

Not yet covered: GCC (not installed where this was measured).

## Sweep caveats (read before trusting a failure)

`clang_sweep.py` compiles every TU in one configuration, whether or not `configure.py` builds that file there, and it forces `-D__GEKKO__`. Several "failures" are therefore artifacts, not bugs:

* **`__GEKKO__` is forced on.** `m_Do_printf.cpp:24` (`asm void OSSwitchFiberEx`) sits under `#ifdef __GEKKO__` and only fails because the sweep defines it. A real x86/ARM host build does not.
* **DEBUG-only code compiled with `DEBUG` off.** `JORServer.cpp`, `JAHioNode.cpp`, `Z2DebugSys.cpp`, `d_event_debug.cpp` (the `listenPropertyEvent` / `getJORServer` / `field_0x0?_debug` errors) use members that only exist `#if DEBUG`. They compile cleanly under ShieldD (`VERSION=12 DEBUG=1`).
* **Files not built for that version.** `f_ap_game.cpp`, `d_s_title.cpp`, `m_Do_MemCard.cpp` are `MatchingFor(ALL_GCN)`; `d_cursor_mng.cpp` is Wii/DZDE01; `JHIMccBuf.cpp` is ShieldD. Their errors in other configs mean nothing.
* **Wii/Shield TUs need `-I libs/revolution/include` instead of `libs/dolphin`, plus `-D__REVOLUTION_SDK__`, `-DWIDESCREEN_SUPPORT=1` and (RZDE01_00, ShieldD) `-DENABLE_REGHIO=1`.** Without those, hundreds of spurious errors appear (`isWide`, `m_fullFrameBuffer*`, `mChildReg`…).
* **ShieldD needs `-DDEBUG=1` and all of the above.** In that configuration 1,292 of 1,383 TUs parse after correcting the flags; Wii USA R0 gives 1,301 and Shield 1,302 (all three measured at C++17, before the `DEG_TO_RAD` macros landed).
* The 19 TUs that fail on `'revolution/…' file not found` (`Z2AudioCS/*`, `Z2SoundPlayer.cpp`, `JHIMccBuf.cpp`, `JHIRMcc.cpp`, `d_home_button.cpp`, `d_cursor_mng.cpp`, `m_Re_controller_pad.cpp`) and the 5 that need generated `assets/*.h` (`d_a_grass`, `d_a_mant`, `d_a_player`, `d_error_msg`, `m_Do_ext`) need no source change; the first group belongs to other versions and the second needs a disc image.

Possible follow-up for the tool itself (`tools/utilities/clang_sweep.py`): add a `--config` that picks the right SDK include path and defines per version, and drop `-D__GEKKO__`. Not done.

## Punch list

Ordered by payoff. `[ ]` = not done, `[x]` = done. Sections A–C are the GameCube configuration; D is not planned.

### A. Header-level (all done)

* [x] **1. `DEG_TO_RAD` / `RAD_TO_DEG`.** *Done, differently from the suggestion below: they are defined in `include/nightfall/compat/globals.hpp` (included by `global.h`) with `std::numbers::pi_v<float>`, which is why the project baseline is now C++20.* Original suggestion: These macros are defined only in the bundled MSL `libs/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Include/cmath:17-18`. Stock `<cmath>` has no equivalent. Define them in `include/global.h` inside the existing `#ifndef __MWERKS__` block next to `#include <cmath>` (global.h:~187), e.g. `#define DEG_TO_RAD(d) ((d) * (3.14159265358979323846f / 180.0f))` and the inverse. Write the literal out rather than using `M_PI`: MSL's `M_PI` is a `float` literal, and the host's may be absent under strict `-std=`. Fixed 35 TUs when trialled (the remaining two used it alongside other errors).
* [x] **2. `JUTConsole.h:161`** `typedef JGadget::TLinkList<JUTConsole, -sizeof(JKRDisposer)> ConsoleList;` negates a `size_t`, producing 18446744073709551568, which cannot narrow to the `int` template parameter. Change to `-(int)sizeof(JKRDisposer)`. Fixed 19 TUs (`JFW*`, `JKR*`, `JUT*`, `DynamicLink`, `d_resorce`, `d_s_logo`, `d_s_play`, `m_Do_*`). It is a straight error on any host, including 32-bit, because the unsigned wrap-around does not fit `int`.
* [x] **3. `d_event_debug.h` enum.** `LBL_EVENT_MANAGER_TESTING = (1 << 31) + 1`, `BTN_FORCED_TERMINATION = (1 << 31) + 4` are negative `int`s; `d_event_debug.cpp:153,173,175` then `switch` on a `u32` with them as `case` labels (narrowing error). Use `1u << 31` in the header. (`1 << 31` itself is accepted since C++14.)
* [x] **4. `d_jpreviewer.cpp:19`** `case -1:` in a `switch` over `u32 type`. Change to `case 0xFFFFFFFF:`.

### B. `switch` jumps past initialisation (4 files, 38 sites; clang has no flag to relax this) (all done)

MWCC accepts a `case` label after a declaration-with-initialiser in the same scope; standard C++ does not. Wrap each offending case body in `{ }` or hoist the declaration above the `switch`. GCC rejects these too (`-fpermissive` would only downgrade it).

* [x] `src/Z2AudioLib/Z2Creature.cpp:638`
* [x] `src/d/actor/d_a_e_gb.cpp:417`
* [x] `src/d/actor/d_a_kago.cpp:2627, 2670, 2702, 2742, 2743, 2769`
* [x] `src/d/actor/d_a_mg_rod.cpp:4740, 4787, 4822, 4855, 4895, 4926, 5014, 5021, 5113, 5125, 5136, 5160, 5179, 5190, 5211, 5220, 5234, 5336, 5372, 5399, 5409, 5433, 5442, 5474, 5478, 5498, 5499, 5654, 5657, 5670` (line numbers are the `case` labels the compiler reports, not the declarations)

### C. Per-site errors in GCN / all configs

* [x] **5. `<cstdarg>` missing** (`va_start`/`va_end` undeclared): `libs/JSystem/src/JAudio2/JASReport.cpp:66,69` and `libs/JSystem/src/JUtility/JUTDbPrint.cpp:103,105,112,114`. Add `#include <cstdarg>`. MSL provides it transitively; the host does not.
* [x] **6. `JAHioUtil.cpp:19`** `va_start(msg, args)` has the arguments swapped; it should be `va_start(args, msg)`. MSL's macro hides the mistake; stock `va_start` does not. This is a genuine bug that only the stock compiler catches. (Also fails in ShieldD.)
* [x] **7. `std-streambuf.cpp:38`** `std::copy<char>(param_0, var_r27, pCurrent_put_)` supplies one explicit template argument; the standard `std::copy` has two or three type parameters, so `<char>` binds `InputIt`. Drop the `<char>`.
* [x] **8. `stricmp` / `strnicmp`** are MSL `extras.h` extensions: `src/d/d_resorce.cpp:838` (and `:865` in ShieldD), `src/d/d_s_room.cpp:92`. Options: a small inline shim in a host-only header (`strcasecmp` / `strncasecmp` from `<strings.h>` on POSIX, `_stricmp` / `_strnicmp` on MSVC), or call the POSIX names at those three sites.
* [x] **9. `main` must return `int`:** `src/m_Do/m_Do_main.cpp:979` `void main(int argc, const char* argv[])`. Both Clang and GCC reject this. Use `#ifdef __MWERKS__ void #else int #endif` to keep the matching build, and `return 0` on the host path. Note the signature `const char* argv[]` also differs from the standard `char* argv[]`; a hosted entry point should be a thin wrapper that forwards to this function.
* [x] **10. `m_Do_main.cpp:1138`** (GCN, `#ifndef __MWERKS__` block) specialises `JASGlobalInstance<JAUSectionHeap>::sInstance` but `JAUSectionHeap` is not declared in that TU. Include its header or forward-declare it there. (The ShieldD build has it via another include path.)
* [x] **11. Pointer → `int` casts** (GameCube sites done; the ShieldD-only ones below are not planned). These are errors on LP64 hosts, so they are required for 64-bit Linux, although they are harmless on 32-bit ARM. Replace `(int)` with `(intptr_t)` (or `uintptr_t`; `<cstdint>`), typed so the matching build is unchanged, or `reinterpret_cast<u32>` as `d_event_debug.cpp` already does:
  * `libs/JSystem/include/JSystem/JSupport/JSUList.h:233` `operator int() { return (int)mTree; }` is used by `Z2WaveArcLoader.cpp:117` (`(int)i != (int)rootheap->getEndChild()`, and the same line fails on its own cast). Consider replacing the `operator int` with `operator==` / `!=` between iterators.
  * `src/d/actor/d_a_npc_tkj2.cpp:106` `int userArea = (int)i_this;`
  * Only in ShieldD (`DEBUG`), not planned: `d_a_npc_bouS.cpp:236`, `d_a_npc_ykm.cpp:249`, `d_a_npc_ykw.cpp:189`, `d_a_tag_lantern.cpp:27`, `d_cam_param.cpp:438` (`switch ((int)event->id)`) and `:654` (`(char*)((int)mTypeTable + i * 0x44)`), `d_s_play.cpp:294,382`, `d_vibration.cpp:740` (all `switch ((int)event->id)`, as are the four actor sites above), and for Wii/Shield `m_Do_Reset.cpp:135` (`(int)block.userData`).

### D. ShieldD (`DEBUG=1`) only (not planned)

ShieldD is not a target of this fork, so these were deliberately skipped. They are kept in case the debug-only code is ever revived.

* [ ] **12. `d_s_play.cpp:192,195,196,200`** `static const char l_nodeName[][20]` initialisers with Japanese text are too long: 22 / 25 bytes against 20. MWCC compiles with `-enc SJIS` (2 bytes per character); clang/GCC read the UTF-8 source (3 bytes per character). Enlarge the array dimension (e.g. `[32]`) for non-MWCC. This is the visible instance of a wider issue; see below.
* [ ] **13. `JAISe.cpp:161`** `*(u32*)&getID()` takes the address of a temporary; store `getID()` in a local first. **`JAISeq.cpp:111`** passes a non-trivial `JAISoundID` through `OS_REPORT`'s variadic `%08x`; convert it to `u32` explicitly (clang notes this would abort at run time).
* [ ] **14. `m_Do_main.cpp:881`** `JHIComPortManager<JHICmnMem>* JHIComPortManager<JHICmnMem>::instance;` is missing `template<>`. **`:902`** `char* var_r27 = strchr(argv[i] + 7, ',');` assigns a `const char*` (the C++ `strchr` overload keeps `const`); make it `const char*` and `const_cast` for the `*var_r27 = '\0'` write.
* [ ] **15. `s16` tables initialised from unsigned constants.** `{0xC800, 0x00FF}`, `{0x8000, 0x00A0}`, etc. narrow `int` → `s16` in a braced initialiser, which is an error in C++11 and later. Sites: `d_a_npc_ashB.cpp:305-330`, `d_a_npc_moir.cpp:543-568`, `d_a_npc_wrestler.cpp:1174-1204`, `d_a_obj_gra2.cpp:1216`, and the big `saveBitLabels` table at `d_save.cpp:2054-2120` (also Shield). Either cast each value `(s16)0xC800`, or change the array element type to `u16`. A blanket alternative for the whole class is `-Wno-c++11-narrowing` (Clang) / `-Wno-narrowing` (GCC), but it hides future real narrowing.
  * `d_s_menu.cpp:628` `GXColor sp24 = {0x14, 0x78, 0x14, 0xDC - alpha};` narrows `int` → `u8`; cast the last element `(u8)(0xDC - alpha)`.

### F. Fallout from the 32-bit `u32`/`s32` typedefs (in progress)

72 error sites as of the last build (`cmake --build --preset debug -- -k 0`), down from 85. No headers are involved any more, so every remaining failure is in a single `.cpp` file.

* [ ] **16. Pointer-to-integer casts: 69 sites in 39 files** (`cast from pointer to smaller type 'u32'`). Cast through `uintptr_t` (or `intptr_t`) instead of `u32`/`s32`/`int`. By file (site count): d_a_movie_player (9), J2DAnmLoader (4), J3DAnmLoader (4), JASCalc (4), JASAramStream (3), JKRDvdAramRipper (3), d_a_mg_fish (3), m_Do_ext (3), JKRAramArchive (2), JSUMemoryStream (2), DynamicLink (2), d_a_npc_tks (2), d_bg_w_kcol (2), and one each in JAIStream, JAIStreamMgr, JASAiCtrl, JASDSPInterface, JASSeqParser, JKRArchivePub, JKRDecomp, JKRMemArchive, object-actor, JUTCacheFont, JUTException, Z2SoundObject, d_a_bd, d_a_e_hm, d_a_e_hz, d_a_e_ww, d_a_npc_aru, d_a_npc_henna, d_a_npc_hoz, d_a_npc_pachi_besu, d_a_npc_pachi_maro, d_a_npc_pachi_taro, d_a_npc_raca, d_a_npc_zelRo, d_a_obj_ornament_cloth and d_resorce. The two headers that used to dominate (`JStudio/functionvalue.h`, `JMessage/data.h`) are fixed.
* [ ] **17. Type mismatches left: 3 sites in 2 files**
  * `JMessage/resource.cpp:66`: a `const u32*` is initialised from a `uintptr_t*`. Use one type for the pointer and what it points at.
  * `d/actor/d_a_alink_effect.inc:1259, 1294` (compiled into `d_a_alink.cpp`): a `u32*` parameter is given a `uintptr_t*`. Same fix; an out-parameter must keep the type the callee declares.
* [x] **Done in this round:** `JGadget/search.h` (`TExpandStride_` and the 64-bit difference type), `JAudio2/JASHeapCtrl.cpp`/`.h` (`uintptr_t` in `initRootHeap`, `setupAramHeap`, `sAramBase`, and the `alloc` loop) and the `OSRoundUp32B`/`OSPhysicalToCached`-style macros in `dolphin/os.h`, which now go through `uintptr_t`.
  * **`JASHeap::alloc` / `initRootHeap` were checked for behavioural equivalence.** The new code was compiled into a scratch harness and run beside a verbatim copy of the original `alloc` using 32-bit arithmetic, on 900,000 random `alloc`/`allocTail`/`free` steps over 3,000 scenarios (57,000 of the allocations went through the fragmented gap-search path). Results, base offsets, sizes and free totals matched at every step, and a deliberately broken variant of the original was detected, so the harness can see differences. The same comparison now lives in `tests/JSystem/JAudio2/JASHeapCtrl_test.cpp`.
* Beyond compile errors: casts that now compile may still be wrong where a `u32` was being used to hold a pointer (it truncates on LP64). Grep for `(u32)` applied to pointers that the compiler could not flag, e.g. through macros such as `POINTER_ADD`, and for `%08x`/`%u` format strings that print pointers.

* [x] **18. Header-defined globals and the global allocator** (found while making the tests link the real code).
  * `libs/dolphin/include/dolphin/os/OSExec.h` defined `__OSExecParams` and `__OSAppLoaderOffset` in every translation unit on non-Metrowerks compilers (the `AT_ADDRESS` macro expands to nothing), so any link of two engine objects failed with duplicate symbols (758 definitions across the actors). They are `extern` there now and defined once in `src/nightfall/platform/os_globals.cpp`; the Metrowerks expansion is byte-for-byte unchanged. This also cut the duplicate-symbol report from 20 symbols to 18.
  * `dolphin/os.h` turned `__OSBusClock`/`__OSCoreClock` into reads of a fixed console address, which faults on a host and crashes any global initialiser that uses `OS_TIMER_CLOCK` (`JUTGamePad` does). They are variables now, defined with the console's values by the same file.
  * `JKRHeap.cpp`'s global `operator new`/`delete` assumed a JKR heap always exists, so the first allocation in a host process (even inside the standard library, during static initialisation) returned null. They fall back to the standard allocator while no JKR heap is current (non-Metrowerks builds only).

### E. Needs your call (no obvious source edit)

* **`u32`/`s32` typedefs: decided and applied.** On 64-bit Linux the old `unsigned long`/`long` were 8 bytes, which changes struct layouts, `sizeof`, wrap-around and every overlay of a struct onto loaded game data. `types.h` now defines `s8`…`u64` from `<cstdint>`, so `u32` is 32-bit everywhere. What remains is the fallout in section F. Separately, any struct holding a real **pointer** is still wider on LP64 (see [endianness.md](endianness.md) and the file-format notes); the README's statement that pointer size matches source and target holds for the Vita only, not for 64-bit Linux.
* **Source text encoding.** Japanese string literals are UTF-8 on stock compilers but Shift-JIS on MWCC. Item 12 is only the visible failure (fixed-size arrays); everywhere else the strings compile but have different byte contents and lengths.
* **Multi-character constants** wider than four characters are already handled by `MULTI_CHAR` in `global.h`; shorter ones (`'JMSG'`) give `int` values that differ between MWCC and GCC/Clang only in endianness and width-warnings. No error, but worth knowing.

## What this does not cover

* **GCC.** Not installed on the machine this was measured on, and it is a required target. GCC differences to watch when you build on Linux: it errors on some things clang only warns about (e.g. `-Wnarrowing` on constants, missing `typename`/`template` disambiguators in dependent contexts, `-fpermissive`-class conversions), and diagnoses different pieces of MWCC-style template code. Run the same configuration (`-std=gnu++17 -fsyntax-only -w`) and add anything new to this list. `clang_sweep.py` already accepts `--cxx g++ --std gnu++17`, but its `-fdeclspec` and `-ferror-limit=0` flags are clang-only and will need removing for GCC.
* **Linking.** Syntax-only. Duplicate symbols are tracked separately in [duplicate-symbols.md](duplicate-symbols.md).
* **`.c` files and `libs/revolution`, `libs/dolphin`, MSL sources** are outside the sweep (only `src/` and `libs/JSystem/src` `.cpp`).
* **Warnings** (including `-Wc++11-narrowing` where it is a warning, and `-Wnon-pod-varargs`) were ignored per the task scope, except where Clang promotes them to errors, as above.
