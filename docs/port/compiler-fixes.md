# Getting the tree to compile on stock Clang and GCC: punch list

Hand-written work list for the `task/std-compiler-fixes` branch. Nothing here has been applied; every item is a suggested edit with the file and line where the compiler stopped. The raw per-file output is in [compile-sweep.md](compile-sweep.md).

Measured with Apple clang 21 (`-std=c++20 -fsyntax-only`, no Metrowerks/MSL headers). **GCC was not available**, so GCC-only diagnostics are not covered; see [What this does not cover](#what-this-does-not-cover).

## Where things stand

GameCube USA, `DEBUG` off (the `clang_sweep.py` default): **55 of 1,383 TUs fail.** The sweep reported 90 before item 1 landed (it cleared 35 on its own). The largest remaining group is the `JUTConsole.h` narrowing (item 2), which accounts for 19 of the 55.

| Cause | TUs | Fix scope |
|---|---|---|
| ~~`DEG_TO_RAD` / `RAD_TO_DEG` missing~~ (done) | ~~37~~ 0 | 1 header |
| Template-argument narrowing in `JUTConsole.h:161` | 19 | 1 line |
| `switch` jumping past an initialisation | 4 | 4 files, 38 sites |
| Everything else (items 4–15) | ~15 | per-site |
| Wii-only SDK headers / generated `assets/…` headers | 19 | no source fix (see below) |

I trialled items 1, 2 and the two `u32` `case` fixes (items 3 and 4) in the tree and re-ran the sweep: **90 → 38 failures**, and the remaining 38 were the items below plus the ~19 no-fix-needed TUs. Those trial edits were reverted at your request.

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

Ordered by payoff. `[ ]` = not done.

### A. Header-level (clears ~56 of the 90 failing TUs)

* [x] **1. `DEG_TO_RAD` / `RAD_TO_DEG`.** *Done, differently from the suggestion below: they are defined in `include/nightfall/compat/globals.hpp` (included by `global.h`) with `std::numbers::pi_v<float>`, which is why the project baseline is now C++20.* Original suggestion: These macros are defined only in the bundled MSL `libs/PowerPC_EABI_Support/MSL/MSL_C/MSL_Common/Include/cmath:17-18`. Stock `<cmath>` has no equivalent. Define them in `include/global.h` inside the existing `#ifndef __MWERKS__` block next to `#include <cmath>` (global.h:~187), e.g. `#define DEG_TO_RAD(d) ((d) * (3.14159265358979323846f / 180.0f))` and the inverse. Write the literal out rather than using `M_PI`: MSL's `M_PI` is a `float` literal, and the host's may be absent under strict `-std=`. Fixed 35 TUs when trialled (the remaining two used it alongside other errors).
* [ ] **2. `JUTConsole.h:161`** `typedef JGadget::TLinkList<JUTConsole, -sizeof(JKRDisposer)> ConsoleList;` negates a `size_t`, producing 18446744073709551568, which cannot narrow to the `int` template parameter. Change to `-(int)sizeof(JKRDisposer)`. Fixed 19 TUs (`JFW*`, `JKR*`, `JUT*`, `DynamicLink`, `d_resorce`, `d_s_logo`, `d_s_play`, `m_Do_*`). It is a straight error on any host, including 32-bit, because the unsigned wrap-around does not fit `int`.
* [ ] **3. `d_event_debug.h` enum.** `LBL_EVENT_MANAGER_TESTING = (1 << 31) + 1`, `BTN_FORCED_TERMINATION = (1 << 31) + 4` are negative `int`s; `d_event_debug.cpp:153,173,175` then `switch` on a `u32` with them as `case` labels (narrowing error). Use `1u << 31` in the header. (`1 << 31` itself is accepted since C++14.)
* [ ] **4. `d_jpreviewer.cpp:19`** `case -1:` in a `switch` over `u32 type`. Change to `case 0xFFFFFFFF:`.

### B. `switch` jumps past initialisation (4 files, 38 sites; clang has no flag to relax this)

MWCC accepts a `case` label after a declaration-with-initialiser in the same scope; standard C++ does not. Wrap each offending case body in `{ }` or hoist the declaration above the `switch`. GCC rejects these too (`-fpermissive` would only downgrade it).

* [ ] `src/Z2AudioLib/Z2Creature.cpp:638`
* [ ] `src/d/actor/d_a_e_gb.cpp:417`
* [ ] `src/d/actor/d_a_kago.cpp:2627, 2670, 2702, 2742, 2743, 2769`
* [ ] `src/d/actor/d_a_mg_rod.cpp:4740, 4787, 4822, 4855, 4895, 4926, 5014, 5021, 5113, 5125, 5136, 5160, 5179, 5190, 5211, 5220, 5234, 5336, 5372, 5399, 5409, 5433, 5442, 5474, 5478, 5498, 5499, 5654, 5657, 5670` (line numbers are the `case` labels the compiler reports, not the declarations)

### C. Per-site errors in GCN / all configs

* [ ] **5. `<cstdarg>` missing** (`va_start`/`va_end` undeclared): `libs/JSystem/src/JAudio2/JASReport.cpp:66,69` and `libs/JSystem/src/JUtility/JUTDbPrint.cpp:103,105,112,114`. Add `#include <cstdarg>`. MSL provides it transitively; the host does not.
* [ ] **6. `JAHioUtil.cpp:19`** `va_start(msg, args)` has the arguments swapped; it should be `va_start(args, msg)`. MSL's macro hides the mistake; stock `va_start` does not. This is a genuine bug that only the stock compiler catches. (Also fails in ShieldD.)
* [ ] **7. `std-streambuf.cpp:38`** `std::copy<char>(param_0, var_r27, pCurrent_put_)` supplies one explicit template argument; the standard `std::copy` has two or three type parameters, so `<char>` binds `InputIt`. Drop the `<char>`.
* [ ] **8. `stricmp` / `strnicmp`** are MSL `extras.h` extensions: `src/d/d_resorce.cpp:838` (and `:865` in ShieldD), `src/d/d_s_room.cpp:92`. Options: a small inline shim in a host-only header (`strcasecmp` / `strncasecmp` from `<strings.h>` on POSIX, `_stricmp` / `_strnicmp` on MSVC), or call the POSIX names at those three sites.
* [ ] **9. `main` must return `int`:** `src/m_Do/m_Do_main.cpp:979` `void main(int argc, const char* argv[])`. Both Clang and GCC reject this. Use `#ifdef __MWERKS__ void #else int #endif` to keep the matching build, and `return 0` on the host path. Note the signature `const char* argv[]` also differs from the standard `char* argv[]`; a hosted entry point should be a thin wrapper that forwards to this function.
* [ ] **10. `m_Do_main.cpp:1136`** (GCN, `#ifndef __MWERKS__` block) specialises `JASGlobalInstance<JAUSectionHeap>::sInstance` but `JAUSectionHeap` is not declared in that TU. Include its header or forward-declare it there. (The ShieldD build has it via another include path.)
* [ ] **11. Pointer → `int` casts** (error on LP64 hosts; fine on 32-bit ARM, so lower priority for the Vita but required for a Linux GCC/Clang build). Replace `(int)` with `(intptr_t)` (or `uintptr_t`; `<cstdint>`), typed so the matching build is unchanged, or `reinterpret_cast<u32>` as `d_event_debug.cpp` already does:
  * `libs/JSystem/include/JSystem/JSupport/JSUList.h:233` `operator int() { return (int)mTree; }` is used by `Z2WaveArcLoader.cpp:117` (`(int)i != (int)rootheap->getEndChild()`, and the same line fails on its own cast). Consider replacing the `operator int` with `operator==` / `!=` between iterators.
  * `src/d/actor/d_a_npc_tkj2.cpp:106` `int userArea = (int)i_this;`
  * Only in ShieldD (`DEBUG`): `d_a_npc_bouS.cpp:236`, `d_a_npc_ykm.cpp:249`, `d_a_npc_ykw.cpp:189`, `d_a_tag_lantern.cpp:27`, `d_cam_param.cpp:438` (`switch ((int)event->id)`) and `:654` (`(char*)((int)mTypeTable + i * 0x44)`), `d_s_play.cpp:294,382`, `d_vibration.cpp:740` (all `switch ((int)event->id)`, as are the four actor sites above), and for Wii/Shield `m_Do_Reset.cpp:135` (`(int)block.userData`).

### D. ShieldD (`DEBUG=1`) only

* [ ] **12. `d_s_play.cpp:192,195,196,200`** `static const char l_nodeName[][20]` initialisers with Japanese text are too long: 22 / 25 bytes against 20. MWCC compiles with `-enc SJIS` (2 bytes per character); clang/GCC read the UTF-8 source (3 bytes per character). Enlarge the array dimension (e.g. `[32]`) for non-MWCC. This is the visible instance of a wider issue; see below.
* [ ] **13. `JAISe.cpp:161`** `*(u32*)&getID()` takes the address of a temporary; store `getID()` in a local first. **`JAISeq.cpp:111`** passes a non-trivial `JAISoundID` through `OS_REPORT`'s variadic `%08x`; convert it to `u32` explicitly (clang notes this would abort at run time).
* [ ] **14. `m_Do_main.cpp:881`** `JHIComPortManager<JHICmnMem>* JHIComPortManager<JHICmnMem>::instance;` is missing `template<>`. **`:902`** `char* var_r27 = strchr(argv[i] + 7, ',');` assigns a `const char*` (the C++ `strchr` overload keeps `const`); make it `const char*` and `const_cast` for the `*var_r27 = '\0'` write.
* [ ] **15. `s16` tables initialised from unsigned constants.** `{0xC800, 0x00FF}`, `{0x8000, 0x00A0}`, etc. narrow `int` → `s16` in a braced initialiser, which is an error in C++11 and later. Sites: `d_a_npc_ashB.cpp:305-330`, `d_a_npc_moir.cpp:543-568`, `d_a_npc_wrestler.cpp:1174-1204`, `d_a_obj_gra2.cpp:1216`, and the big `saveBitLabels` table at `d_save.cpp:2054-2120` (also Shield). Either cast each value `(s16)0xC800`, or change the array element type to `u16`. A blanket alternative for the whole class is `-Wno-c++11-narrowing` (Clang) / `-Wno-narrowing` (GCC), but it hides future real narrowing.
  * `d_s_menu.cpp:628` `GXColor sp24 = {0x14, 0x78, 0x14, 0xDC - alpha};` narrows `int` → `u8`; cast the last element `(u8)(0xDC - alpha)`.

### E. Needs your call (no obvious source edit)

* **`u32`/`s32` are `unsigned long`/`long`** (`libs/dolphin/include/dolphin/types.h:7-8`). On LP64 hosts these are 64-bit, which is why many `(u32)ptr` casts compile at all; on 32-bit ARM they are 32-bit. Item 11's `(int)` fixes are independent of that, but changing `u32` to `unsigned int` on non-MWCC would turn every pointer-to-`u32` cast into an error on LP64. Decide whether Linux GCC/Clang builds are a compile-only check or a real target before touching it.
* **Source text encoding.** Japanese string literals are UTF-8 on stock compilers but Shift-JIS on MWCC. Item 12 is only the visible failure (fixed-size arrays); everywhere else the strings compile but have different byte contents and lengths.
* **Multi-character constants** wider than four characters are already handled by `MULTI_CHAR` in `global.h`; shorter ones (`'JMSG'`) give `int` values that differ between MWCC and GCC/Clang only in endianness and width-warnings. No error, but worth knowing.

## What this does not cover

* **GCC.** Not installed here. GCC differences to watch when you build on Linux: it errors on some things clang only warns about (e.g. `-Wnarrowing` on constants, missing `typename`/`template` disambiguators in dependent contexts, `-fpermissive`-class conversions), and diagnoses different pieces of MWCC-style template code. Run the same configuration (`-std=gnu++17 -fsyntax-only -w`) and add anything new to this list. `clang_sweep.py` already accepts `--cxx g++ --std gnu++17`, but its `-fdeclspec` and `-ferror-limit=0` flags are clang-only and will need removing for GCC.
* **Linking.** Syntax-only. Duplicate symbols are tracked separately in [duplicate-symbols.md](duplicate-symbols.md).
* **`.c` files and `libs/revolution`, `libs/dolphin`, MSL sources** are outside the sweep (only `src/` and `libs/JSystem/src` `.cpp`).
* **Warnings** (including `-Wc++11-narrowing` where it is a warning, and `-Wnon-pod-varargs`) were ignored per the task scope, except where Clang promotes them to errors, as above.
