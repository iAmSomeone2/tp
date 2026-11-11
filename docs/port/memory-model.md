# Memory model: survey

> The design built on this survey is in [memory-design.md](memory-design.md).

> **Status: survey only.** This page records how the engine manages memory today and which parts of it depend on 32-bit sizes or on the console's hardware. It is the basis for the memory-model design. Nothing has been changed. Figures are for the retail GameCube USA build (`GZ2E01`, `DEBUG` off) unless marked otherwise.

The game runs in a single contiguous block of memory (the *arena*), which boot code divides into a fixed tree of JKR heaps with hand-tuned sizes. Everything after that allocates from those heaps. Either it names a heap explicitly, or it goes through the global `operator new` into whichever heap is *current*. Lifetimes are expressed by which heap something lives in, and whole heaps are freed or destroyed at well-defined points (actor deletion, room unload, stage change, menu close).

## Summary

| Finding | Why it matters |
|---|---|
| **A fixed heap split, tuned for 24 MiB.** Boot carves the arena into archive (8.87 MiB), game (4.30 MiB), J2D (500 KiB), command (4 KiB) and debug-print heaps, plus the system heap (the rest), inside which the Zelda heap takes all but 64 KiB. | All sizes are constants measured for the console. They become per-platform configuration. |
| **Hundreds of sizes measured on the 32-bit console.** 350 of 467 actor heaps pass a literal size estimate. About 10 other heaps have literal sizes. Room memory blocks are sized from a table *in the stage files*. | On 64-bit hosts objects are larger, so these sizes are too small. Actor heaps recover (slowly, with fragmentation); the others fail or crash. |
| **An ambient allocator for the whole program.** The global `operator new` allocates from the JKR *current heap*. About 2,000 `new` expressions name no heap; 266 name one explicitly. The current heap is switched at 190 sites in 46 files. | Since `operator new` is global, every C++ allocation once a heap is current lands in a game heap: the standard library's, gtest's and any future C++ library's. Destroying a game heap then also frees memory those libraries still use. |
| **The current heap is per thread only on the console.** An OS thread-switch callback (`JKRThreadSwitch`) saves and restores it per `JKRThread`. | A host OS has no such callback, so `JKRHeap::sCurrentHeap` becomes one global that the main, DVD and audio threads race on. |
| **4-byte alignment by default.** Both heap types round sizes to 4 bytes and align to 4 unless asked otherwise. The expanding heap's block header grows from 16 to 24 bytes on 64-bit. | On 64-bit hosts `new` is expected to return 16-byte-aligned memory (`__STDCPP_DEFAULT_NEW_ALIGNMENT__`), and pointers need 8. That's undefined behaviour today, usually harmless on x86-64 and ARM64, but it will trip sanitizers and SIMD types. |
| **Lifetimes are already tiered.** Heaps map onto process, scene, stage, room, actor and menu lifetimes; resources are reference-counted (`dRes_info_c::mCount`). | This is the basis for explicit, checked lifetimes (arenas) in the new model. |
| **Little gameplay depends on memory state.** Checks of free sizes are debug-only. What remains: "take all free space, build, shrink" allocations, a lock flag on the shared 2D heap, and NULL returns on exhaustion. | The model can change budgets and placement freely, as long as these patterns keep working. |
| **ARAM is a second 16 MiB pool.** 10 MiB goes to audio; the rest holds the 2D menu archives and Link's animation archive, which are copied into main RAM when needed. | ARAM addresses stay `u32` (an earlier decision). Hosts and the Vita can back it with ordinary memory, and the "copy from ARAM when needed" step could go away. |

## Boot: the arena and the root heaps

`mDoMch_Create` (`src/m_Do/m_Do_machine.cpp:741`) runs once:

1. The arena is whatever `OSGetArenaLo/Hi` report. A dev kit with more than 24 MiB is capped to 24 MiB.
2. `JFWSystem::firstInit` creates the **root heap** over the whole arena (`JKRExpHeap::createRoot`) and the **system heap** inside it. The system heap gets the arena minus 0xDAB400 bytes (13.67 MiB), which is exactly what the next step needs.
3. The fixed heaps are created from the root heap:

| Heap | Size (retail) | Holds | Created by |
|---|---:|---|---|
| Archive | 0x8DF400 (8.87 MiB) | Mounted resource archives (`dRes_control_c`), room memory blocks | `mDoExt_createArchiveHeap` |
| Game | 0x44E000 (4.30 MiB) | Actor heaps, scene-lifetime objects, particle scene heap | `mDoExt_createGameHeap` |
| J2D | 0x7D000 (500 KiB) | 2D/menu resources loaded from ARAM archives | `mDoExt_createJ2dHeap` |
| Command | 0x1000 (4 KiB) | DVD thread command objects (`mDoDvdThd_*`, allocated at the tail) | `mDoExt_createCommandHeap` |
| Debug print | 0x1800 (6 KiB) | `JUTDbPrint` | `mDoExt_createDbPrintHeap` |
| Assert | small | `JUTAssertion` | `mDoExt_createAssertHeap` |
| Dynamic link | 0x180000 (`DEBUG` only) | REL modules | `DynamicModuleControlBase::createHeap` |

4. The **Zelda heap** takes the system heap's free space minus 64 KiB (`mDoExt_createZeldaHeap`) and becomes the current heap. It holds process-lifetime state (managers, global objects) and every process object, actors included: `fpcBs_Create` and the create requests allocate them with `cMl::memalignB(-4, …)`, i.e. from the tail of the Zelda heap (`cMl::init(mDoExt_getZeldaHeap())`). An actor's *heap* (models, buffers) is separate and lives in the game heap.
5. ARAM is set up by `JKRAram::create`: 0xA00000 (10 MiB) audio buffer, the rest graphics (`setAramGraphBufSize(-1)`). The GX FIFO is 0xA0000.

The sizes are built from dozens of `+=`/`-=` constants, with a separate correction for retail builds (the "fakematch" comment in the code). They are the result of tuning on the console, not derived from anything.

## Heap types

| Type | Behaviour | Used for |
|---|---|---|
| `JKRExpHeap` | General allocator: free list, allocations from the head or the tail (negative alignment), `free`, `freeAll`, group IDs. Each block has a `CMemBlock` header (two pointers, a size and flags: 16 bytes on the console, 24 on 64-bit hosts). | Root, system, Zelda, archive, game, J2D, command; room memory blocks; menu and message heaps |
| `JKRSolidHeap` | Bump allocator from the head or tail. No individual `free`; only `freeAll`, `freeTail` and `adjustSize` (shrink to what's used). | Per-actor heaps, per-resource data heaps, effect and model heaps |
| `JKRAssertHeap` | Heap for assertion output. | Assertions |
| `JKRAramHeap` | Allocator over ARAM addresses (`u32`). | Audio buffers, ARAM-mounted archives |

`JKRUnitHeap` exists in JKernel but the game doesn't use it.

## Lifetimes (as the code expresses them today)

| Tier | Where it lives | Ends when |
|---|---|---|
| Process | Zelda and system heaps | Never (or on soft reset) |
| Scene | Game heap objects created by the scene; the play scene's 2D heap (0xBB800) and message heap (0xA800) (`d_s_play.cpp:1421`); the particle scene heap (`d_particle.cpp:1222`) | The scene process is deleted |
| Stage | Room memory blocks (`dStage_roomControl_c::mMemoryBlock[19]`, in the archive heap) | `destroyMemoryBlock()` on a stage change (`d_stage.cpp:2738`) |
| Room | Contents of a room's memory block | `freeAll()` on that block when the room is unloaded (`d_s_room.cpp:350`), unless the room is kept |
| Actor | One solid heap per actor (`fopAcM_entrySolidHeap`, models, animation buffers) | The actor is deleted (the heap is destroyed with it) |
| Resource | `dRes_info_c`: one mounted archive plus a solid data heap with the J3D data built from it | Its reference count (`mCount`) drops to 0 (`dRes_control_c::deleteRes`) |
| Menu/UI | Sub-heaps of the 2D heap, `freeAll()`ed when a menu closes (`d_meter2.cpp`, `d_menu_collect.cpp`, `d_gameover.cpp`, …) | The menu closes |
| Transient | Command heap; tail allocations while loading archives (12 sites, mostly `m_Do_dvd_thread.cpp`) | The command or the load completes |

Pointers into these heaps are handed around freely (e.g. `getResource` returns a raw `void*`). Nothing checks that a pointer is not used after its heap is freed, but the tiers above are where such pointers become invalid.

## Sizes measured on the 32-bit console

| What | How many | On a 64-bit host |
|---|---:|---|
| Actor heaps with a literal estimate (`fopAcM_entrySolidHeap(actor, cb, 0x…)`) | 350 of 467 calls (16 pass 0, "measure") | **Recovers.** `fopAcM_entrySolidHeap_` (`f_op_actor_mng.cpp:391`) tries the estimate. If the actor doesn't fit, it builds the actor again in the game heap's largest free block and shrinks that heap to fit. Slower, and it fragments the game heap. The actor's heap callback must be safe to run more than once. |
| Fixed-size solid heaps with no fallback | 8 (`d_attention.cpp:175` 0x9000, `d_k_wmark.cpp:36` 0x880, `d_k_wpillar.cpp:39` 0x2200, `d_simple_model.cpp:130` 0x2000, `d_kankyo.cpp:1174` 0x600, `d_file_select.cpp:5183` 0x25800, `d_menu_collect.cpp:2224` 0x25800, and the debug-only model viewer `f_ap_game.cpp:598` 0x4B000) | **Fails.** Allocations inside them return NULL. |
| Fixed-size expanding heaps | Play-scene 2D heap 0xBB800, message heap 0xA800; `DEBUG`-only editor heaps 0x100000 | Too small by however much their objects grew |
| Room memory blocks | Up to 19 per stage, each sized from the stage file's `MEMA` chunk (`dStage_memaInfoInit`, `d_stage.cpp:2217`) plus 0x380 | **Sized by data from the disc.** The bake could rewrite the table, but the engine should stop relying on it. |
| Root heap split | 6 constants (above) | Needs per-platform values |

## The ambient allocator

* `operator new`/`new[]` without arguments call `JKRHeap::alloc(size, 4, NULL)`, which allocates from `JKRHeap::sCurrentHeap` (`JKRHeap.cpp:493`). `operator new(size, heap, align)` names a heap.
* There are about 2,000 `new` expressions without a heap argument and 266 with placement arguments (most of them a heap and an alignment). The current heap is switched at 190 sites in 46 files, usually by a "make this heap current, build things, restore" pair (`mDoExt_setCurrentHeap`, `becomeCurrentHeap`, `mDoExt_create*HeapToCurrent`).
* **This fork's addition** (`JKRHeap.cpp:473`): when no JKR heap is current (static initialisation, unit tests), `new` uses the standard allocator. `delete` asks `JKRHeap::findFromRoot` whether a JKR heap owns the pointer and calls `std::free` if none does. That is a walk of the heap tree on every `delete`. A pointer from a heap that has already been destroyed goes to `std::free`.
* **The operators are global.** After boot, every C++ allocation goes to the current game heap: `std::vector`, `std::string`, gtest and any future C++ library. SDL3 is a C library using `malloc`, so it is unaffected. When a game heap is freed all at once, any library memory in it is freed too.
* **Threads:** `JKRThreadSwitch::callback` (`JKRThread.cpp:174–218`) saves the current heap of the outgoing `JKRThread` and installs the incoming thread's heap, on every OS context switch. Host operating systems offer no such hook, so the per-thread behaviour has to become explicit, e.g. `thread_local`.

## Alignment

* `JKRExpHeap::do_alloc` and `JKRSolidHeap::do_alloc` treat alignment up to 4 as 4 and round every size up to 4 bytes (`JKRExpHeap.cpp:181–240`, `JKRSolidHeap.cpp:98`). With the 24-byte block header on 64-bit, a default allocation is only guaranteed to be 4-byte aligned.
* The C++ ABI on x86-64 and AArch64 expects `operator new` to return 16-byte-aligned memory, and any object containing a pointer needs 8. On the Vita (ILP32, 4-byte pointers) 4 is mostly enough, as on the console.
* No engine type is declared over-aligned today (no `alignas` in the engine headers), so the misalignment goes unnoticed. It is still undefined behaviour, and the planned NEON/SIMD math ([math.md](math.md)) will need 16-byte alignment.

## Behaviour that depends on memory state

* **Debug-only:** the leak check `heapSizeCheck` (`d_s_play.cpp:972`, `#if DEBUG`), Z2's debug and ARAM status screens, the out-of-memory report `myMemoryErrorRoutine` (`m_Do_machine.cpp:341`, counts failures per heap and dumps the heap). Many retail sites call `getTotalFreeSize()` and ignore the result; these are leftovers of stripped debug output.
* **"Take all free space, then shrink":** solid heaps created with size 0 or −1 take the parent's entire free space (`d_resorce.cpp:593/608`, `d_bg_parts.cpp:758`, `d_ky_thunder.cpp:24`, `d_com_inf_game.cpp:990`, `d_particle.cpp:1197/1222`, `m_Do_graphic.cpp:2119`, the actor fallback). The same goes for `Z2FxLineMgr.cpp:362` and `d_menu_dmap.cpp:1630`, which size a heap from `getFreeSize()`. While such a heap is open, its parent has no free space, so nothing else may allocate from the parent until it is shrunk. This is one reason loading is strictly sequential.
* **Shared 2D heap lock:** `dComIfGp_setHeapLockFlag` / `getSubHeapLockFlag` (24 uses) record which menu currently owns the shared 2D heap and its sub-heaps.
* **Allocation failure:** allocators return NULL and call the error handler, which reports and counts the failure. Solid-heap failures are expected, because the actor fallback probes with them. Most callers then assert or dereference NULL.

## ARAM

* **Audio (10 MiB):** the JAudio2 ARAM heap (`JASKernel::setupAramHeap`), wave banks, the single stream buffer.
* **Graphics (the rest, about 6 MiB):** archives mounted with `MOUNT_ARAM` by the logo scene (`d_s_logo.cpp:1443-1501`). Measured on GZ2E01:

  | Archive(s) | Files | Stored in ARAM | All files decompressed |
  |---|---|---|---|
  | `AlAnm.arc` (Link's animations) | 1,039 | 2.65 MiB | 3.80 MiB |
  | `Field0.arc` (field map data) | 113 | 289 KiB | 342 KiB |
  | Menu layouts (`fmapres`, `dmapres`, `clctres`, `itemicon`, `ringres`, `playerName`, `itmInfRes`, `button`, `cardicon`) | 409 | 1.05 MiB | 1.99 MiB |
  | Message layouts (`msgcom`, `msgres00`–`06`) | 104 | 167 KiB | 687 KiB |
  | **Total** | 1,766 | **4.15 MiB** | **6.80 MiB** |

  Each archive is decompressed as a whole into ARAM, but most member files stay Yaz0-compressed there and are decompressed when copied out.
* **How graphics data leaves ARAM:** callers use the generic `JKRArchive` interface and copy a file into a buffer they own (`readResource`/`JKRReadIdxResource`): the J2D heap for menus, Link's per-slot animation buffers (`daPy_anmHeap_c::loadData`, `d_a_player.cpp:293`), `d_stage.cpp:477` and `d_com_inf_game.cpp:2454` for `Field0.arc`. Only a few places know the archive is in ARAM: `dLib_getExpandSizeFromAramArchive` (`d_lib.cpp:277`; called from `d_pane_class.cpp`, `d_stage.cpp`, `d_menu_fmap.cpp`), `d_s_play.cpp:921` (casts any archive to `JKRAramArchive`), and the `#if DEBUG` memory report in `f_ap_game.cpp`.
* **Font caches:** `JUTCacheFont` keeps glyph sheets in ARAM, but only the Japanese build's main font uses it (`m_Do_ext.cpp:3605`, `REGION_JPN`). Other fonts are held whole in main RAM.
* **Compressed mounts:** `MOUNT_COMP` archives can keep part of their data in ARAM; the only one is `RELS.arc`, which static linking removes.
* ARAM addresses are `u32` offsets into an emulated buffer ([compiler-fixes.md](compiler-fixes.md), item 19). The design moves graphics out of the pool into main memory ([memory-design.md](memory-design.md#resident-set-and-cache-tier)).

## Requirements this puts on the new model

These follow from the findings; the design will decide how to meet them.

1. **Separate game heaps from the host's allocations.** The standard library and third-party C++ code must not allocate from game heaps.
2. **Make the current heap explicit and per thread.**
3. **Budgets are configuration.** Root split, fixed heaps, ARAM size and actor estimates become per-platform values. On the hosts they can be generous, plus an optional "console budget" mode that enforces the Vita's limits to catch regressions.
4. **Measured sizes become hints, not limits.** This applies to actor estimates, `MEMA` room blocks and the fixed heaps.
5. **Keep the lifetime tiers, and make them checkable.** Each tier becomes an arena with an owner, so a debug build can detect use after its heap was freed.
6. **Keep "take all, then shrink" working**, or replace it with something that doesn't lock the parent.
7. **Alignment:** at least 16 bytes for `new` on 64-bit hosts; explicit alignment for SIMD types.
8. **ARAM:** keep the `u32`-addressed pool for audio, and decide whether graphics archives stay in it.

## How this was measured

`grep` over `src/`, `include/` and `libs/JSystem/` for heap creation and destruction (`mDoExt_create*Heap*`, `JKR*Heap::create`, `freeAll`, `destroy*Heap`), current-heap switches, `new` expressions, free-size queries and negative alignments, followed by reading each relevant site. The `new` counts are approximate: the patterns also match comments. Heap sizes were computed from the constants in `mDoMch_Create`.
