# Memory model: design

> **Status: design, nothing built yet.** Decisions recorded here were made with the developer on 2026-11-10. The survey this builds on is [memory-model.md](memory-model.md); its requirements are referred to as R1–R8. Open questions are listed at the end.

The goal is memory management that is strict by default, in the spirit of Rust: every allocation names the heap it comes from, every heap has an owner that decides its lifetime, and the host program's own allocations never mix with the game's. Budgets become per-platform configuration instead of constants tuned for a 24 MiB console. The game keeps running at every step along the way.

## Decisions

| Topic | Decision |
|---|---|
| Heap interface | **`std::pmr::memory_resource`** (C++17), the C++ counterpart of Rust's `Allocator` trait. All heaps, old and new, are memory resources. |
| Existing heaps | **`JKRHeap` becomes a `memory_resource`** instead of hiding behind a new layer. Every `JKRHeap*` then works with `std::pmr` containers as it is, and the engine's own APIs (`new (heap, align)`, `JKRAllocFromHeap`) keep working. The JKR heaps are later rebuilt on the new heap classes. |
| Heap selection | **Explicit everywhere.** Each `new`/`delete` in engine code names its heap. The implicit "current heap" goes away module by module, in the order JSystem → `f_*`/`m_Do` → `d_*` → actors, and a checker counts what is left. |
| Solid heaps | **Chunked arenas** replace "take all free space, build, then shrink". An arena grows in chunks taken from its parent, so the parent is never blocked. |
| Object lifetimes in file data | **`start_lifetime_as`** (C++23, P2590) for reading baked file data as structs, through a small `nf::` wrapper. The wrapper uses the standard function where the library has it and a C++20 fallback otherwise (the Vita's GCC 15.2 library and libc++ lack it), so no toolchain is required to have it. |
| Language standard | **C++23 for the engine.** `nf_memory` requires it, and since JKernel links `nf_memory`, the whole engine compiles as C++23 (see CLAUDE.md). The analysis scripts' `-std=c++20` default follows. |
| macOS toolchain | **Apple clang stays supported.** It lacks `std::start_lifetime_as` and gets the fallback. |

## Toolchain facts this relies on

Checked on 2026-11-10:

| | GCC 16.2 + libstdc++ (Linux) | clang 22 + libstdc++ (Linux) | `arm-vita-eabi-g++` 15.2 (Vita) | Apple clang + libc++ (macOS) |
|---|---|---|---|---|
| `<memory_resource>` | yes | yes | yes (`__cpp_lib_memory_resource` 201603) | yes (cppreference) |
| `std::start_lifetime_as` | yes (`__cpp_lib_start_lifetime_as` 202207, `-std=c++23`) | yes (same library) | **no** | **no** (cppreference lists libc++ without it) |

`start_lifetime_as` is a library feature, so support follows the standard library, not the compiler: clang on Linux uses libstdc++ by default and has it, while cppreference's "Clang" column means LLVM's libc++, which Apple clang ships. The libc++ result was not tested here (Fedora's `libcxx-devel` headers aren't installed). On a Mac, `echo '#include <version>' | clang++ -std=c++23 -dM -E -x c++ - | grep lifetime_as` prints the macro if a later Xcode adds it.

The engine is built with `-fno-exceptions` (`cmake/TPConfig.cmake:79`). `memory_resource::allocate` must not return null, so a failed `std::pmr` allocation ends the program, which matches Rust's default `handle_alloc_error`. The engine's own allocation calls (`JKRHeap::alloc`, `new (heap, align)`) keep returning NULL on failure, because the game handles that (see *Failure policy*).

## Architecture

### The `nf_memory` module

New code lives in `include/nightfall/memory/`, `src/nightfall/memory/`, tests in `tests/nf_memory/`, target `nf_memory` (alias `nf::memory`), namespace `nf`. It has no dependency on the engine. JKernel links it.

| Piece | Rust counterpart | Purpose |
|---|---|---|
| `nf::ResourceDelete`, `nf::Box<T>` = `std::unique_ptr<T, ResourceDelete>` | `Box<T, A>` | An owning pointer that remembers which resource it came from and gives the memory back to it. This replaces the `HeapBox` of the RARC design. |
| `nf::BoxArray<T>` | `Box<[T], A>` | Owning, fixed-length array from a resource. It replaces `HeapArray`. |
| `nf::make_box<T>(resource, args…)` | `Box::new_in` | Allocates and constructs in one step, so a raw pointer is never exposed. |
| `nf::Arena` | a bump arena with a lifetime (`bumpalo`) | Chunked bump allocator, described below. |
| `nf::GeneralHeap` | a general allocator | Free-list allocator with individual frees, allocation from either end, and group IDs, i.e. what `JKRExpHeap` does today. |
| `nf::Budget` | none | Wrapper that counts bytes against a limit and reports the peak. |
| `nf::start_lifetime_as<T>(p)`, `nf::start_lifetime_as_array<T>(p, n)` | `bytemuck::from_bytes` | Calls `std::start_lifetime_as` when `__cpp_lib_start_lifetime_as` is defined. Otherwise it falls back to the C++20 way of creating objects in place, `std::memmove(p, p, size)` followed by `std::launder`. Restricted to trivially copyable `T` with a `static_assert`. |

All types are move-only where they own memory, constructors that can fail are replaced by factory functions returning `std::optional` (or `std::expected`, C++23), and the public API is `[[nodiscard]]` wherever dropping a result would leak.

### `JKRHeap` as a resource

```cpp
class JKRHeap : public JKRDisposer, public std::pmr::memory_resource {
    // ...existing interface unchanged...
private:
    void* do_allocate(std::size_t n, std::size_t a) override;       // alloc(); a failure ends the program
    void  do_deallocate(void* p, std::size_t, std::size_t) override { free(p); }
    bool  do_is_equal(const memory_resource& o) const noexcept override { return this == &o; }
};
```

`JKRHeap` gains a base class, which changes its layout. That is fine, since matching the original binary is not a goal. The memory heaps hand out keeps whatever alignment is asked for, with the minimum below.

### The current heap

* **Step 1: per thread.** `JKRHeap::sCurrentHeap` becomes `thread_local`. The heap-swapping part of `JKRThreadSwitch::callback` is removed, because neither the hosts nor the Vita have the console's thread-switch hook. This fixes the race (R2).
* **Then gone.** Once a module no longer uses it (see *Order of work*), its "make current … restore" pairs are deleted. When no engine module uses it, `getCurrentHeap`/`becomeCurrentHeap` are removed.

### Alignment

The minimum alignment of every heap is `alignof(std::max_align_t)`, which is 16 on 64-bit hosts, and stays 4 on the Vita, as on the console (R7). Requests for more (SIMD, GPU buffers) are honoured as asked.

### Separating the program from the game heaps

* **One reservation.** At boot, the platform layer takes one block of memory for all game heaps: `mmap` or `malloc` on the hosts, a kernel memory block on the Vita. Its size comes from the platform's budget configuration. The root heap is created over it, as with today's arena.
* **Global `operator new`/`delete` go to the host allocator** once the engine allocates explicitly everywhere (R1). The standard library, gtest and any other library then never touch a game heap.
* **`delete` is routed by address.** `operator delete(p)` checks whether `p` lies inside the reservation; if so, it hands `p` to the game heap that owns it, otherwise to the host allocator. A range check replaces today's heap-tree walk (`findFromRoot`) on every `delete`. During the transition this also keeps `delete` correct for objects that were created with an explicit heap.

### Arenas instead of "take all, then shrink"

`nf::Arena` takes chunks from its parent resource as it needs them, frees them all at once, and can return the unused end of its last chunk ("shrink"). It is not contiguous across chunks; none of the current solid-heap users need contiguity. This:

* Stops one open heap from blocking its parent (R6). Today a size-0 solid heap takes all of the parent's free space until it is shrunk.
* **Turns sizes measured on the console into hints** (R4). An actor's literal estimate becomes the size of its first chunk. The actor heap's retry logic (`fopAcM_entrySolidHeap_`'s fallback to the largest free block) goes away, and with it the need for heap callbacks to be safe to run twice.
* Turns the `MEMA` room-block sizes from the stage files into hints for the room arenas' first chunk, instead of hard limits.

`JKRSolidHeap` is then rebuilt on `nf::Arena` and `JKRExpHeap` on `nf::GeneralHeap`, keeping their engine-facing interfaces (`freeAll`, `adjustSize`, group IDs).

### Lifetime owners

Each tier from the survey gets an owner object holding its arena or heap. Destroying the owner frees the memory, so lifetimes become RAII instead of manual `freeAll`/`destroy` calls:

| Tier | Owner (where it lives) |
|---|---|
| Process | Boot (`mDoMch_Create`), for the system and Zelda heaps |
| Scene | The scene process; it holds the scene's 2D, message and particle heaps |
| Stage | `dStage_roomControl_c`: the room arenas |
| Room | The room's arena; `freeAll` when the room is unloaded |
| Actor | The actor; its arena is freed with the actor |
| Resource | `dRes_info_c` (already reference-counted): the archive and its data arena |
| Menu | The menu object; its sub-heaps of the 2D heap |

**Debug checks:**
* **Fill freed memory:** freed chunks get a fill pattern.
* **Generation counter:** each arena counts how many times it has been reset, so debug-only handles can detect use after a tier has ended.

This is where Rust's lifetimes are approximated at run time, since C++ cannot check them at compile time. The checks build on the JKR debug-fill flags that already exist.

### Budgets

* **Per platform:** the root reservation, the size of each fixed heap, the ARAM size and the arena chunk sizes come from a per-platform configuration table instead of the constants in `mDoMch_Create` (R3).
* **Counting:** `nf::Budget` counts the bytes used per tier, so the game's memory use can be read per tier on every platform.
* **Console-budget mode** (hosts, optional): enforces the Vita's limits, so a regression shows up on the dev PC instead of on the device. With 512 MiB on the Vita and the developer's guaranteed 256 MiB budget, the Vita's own limits are generous; the main purpose is catching leaks and unbounded growth.

### Resident set and cache tier

> **Status: decided 2026-11-11 from Vita measurements** ([vita-storage.md](vita-storage.md)); the budget is still open.

TP uses ARAM as a cache: archives are loaded once and copied into main RAM when used. The port keeps that *policy* but not the mechanism, and extends it to everything loaded from storage, because Vita storage is slow: 8–10 MiB/s, and the OS keeps no read cache, so anything read again comes from storage again ([vita-storage.md](vita-storage.md)). Reading `AlAnm` (3.8 MiB) a second time would cost about 400 ms.

* **Cache tier.** A budgeted cache of pack entries, keyed by index entry, with least-recently-used eviction. It sits in front of archive mounting in the DVD thread (`mDoDvdThd_*`): a mount checks the cache before reading storage. It is built on `nf_memory` (`Budget` plus a general heap). Returning to a recent area then needs no storage access.
* **Resident set.** What the logo scene mounts to ARAM today (about 25 archives, 6.8 MiB decompressed on GZ2E01, see [memory-model.md](memory-model.md#aram)) becomes pinned cache entries that are never evicted. Graphics stop using the ARAM pool: these archives live in main memory and callers keep using `readResource`/`getResource` (option B from the ARAM discussion). The few places that know an archive is an ARAM archive change: `dLib_getExpandSizeFromAramArchive` (`d_lib.cpp:277`, 4 callers), `d_s_play.cpp:921`, and the mount helpers in `d_s_logo.cpp`/`m_Do_dvd_thread.cpp`.
* **Compressed entries** (LZ4HC, ratio 1.70 on the benchmark corpus) are cached as read and decompressed once into the destination heap, at about 62 MiB/s on one core. That replaces today's copy out of ARAM, so it adds no copy, and the cache holds 1.7× as much per MiB.
* **Reading in place later (option C).** Once formats are baked, consumers can point into the loaded archive instead of copying, starting with Link's animations (`daPy_anmHeap_c`). This works for entries loaded uncompressed into a heap.
* **Audio** keeps the `u32`-addressed ARAM pool, emulated as a main-memory block with `memcpy` transfers, until the audio work replaces it.

### Failure policy

| Allocation through | On failure |
|---|---|
| Engine API: `JKRHeap::alloc`, `new (heap, align)`, `JKRAllocFromHeap` | Returns NULL and calls the error handler, as today. Game code handles NULL (or asserts). |
| `std::pmr` (`polymorphic_allocator`, `nf::make_box`, containers) | Ends the program (`-fno-exceptions`). New code treats running out of memory as fatal, like Rust. |

### `start_lifetime_as` and file data

Heaps hand out raw memory, and `new`/`std::construct_at` start the lifetimes of objects in it, so heaps don't need `start_lifetime_as`. Baked files are different:

* **Today** the engine reads loaded files by casting bytes to structs. That's undefined behaviour in C++ and only works because of `-fno-strict-aliasing`.
* **Baked structs qualify:** they are plain, pointer-free and trivially copyable ([bake.md](bake.md)), exactly what `start_lifetime_as` is for.
* **`nf::formats::at<T>(base, offset)`** from the bake design is built on `nf::start_lifetime_as<T>`, with a bounds check and an alignment check (`alignof(T)`) in debug builds. Reading a file then starts object lifetimes in the loaded bytes without copying them.

What this buys, and what it doesn't:

* **Gained:** reads of file data are defined behaviour by the letter of the standard. No current compiler or sanitizer exploits or detects the cast version, so there's no codegen difference; the gain is correctness that doesn't rest on habit, at the cost of a ten-line wrapper that the checks need anyway.
* **Not covered: strict aliasing.** Already off (`-fno-strict-aliasing`, `cmake/TPConfig.cmake:80`). The bake also removes in-place rewriting (relocation, endian swaps), so file memory is only ever read as its format type.
* **Not covered: alignment.** On the Vita, misaligned `LDRD`/`LDM`/`VLDR` fault, and the compiler may emit them for struct reads. That's why `at<T>` checks alignment; the bake's 32-byte file alignment plus naturally aligned fields keep it satisfied.
* **The fallback's limits:** `memmove` writes, so it must not be used on read-only memory (fine while files are loaded into heaps; it would matter only if baked files were mapped read-only). At `-O0` the self-move is a real n-byte copy, so call it once per file or table, not per field. Optimized builds normally remove it.
* **The plain alternative,** a bare `reinterpret_cast`, works in practice under `-fno-strict-aliasing`. It was passed over because `at<T>` is the one place bytes become typed and has to exist for the checks anyway.

## Order of work

| Step | Work | Done when |
|---|---|---|
| **0. Foundation** | `nf_memory` with `Box`/`BoxArray`/`make_box`, the `start_lifetime_as` wrapper and `Budget`. `JKRHeap` derives from `memory_resource`. `thread_local` current heap, switch-callback heap swap removed. Minimum alignment 16 on 64-bit hosts. | Unit tests: `std::pmr::vector` on a `JKRExpHeap`; `Box` returns memory to the right heap; two threads with different current heaps don't interfere; every default allocation on a 64-bit host is 16-byte aligned; `start_lifetime_as` wrapper on both library paths. All existing tests pass. |
| **1. Checker** | `tools/utilities/implicit_alloc_check.py` (`clang-query`): every `new` without a heap argument, every `delete`, and every current-heap switch in engine code, per module. It writes the generated page `docs/port/implicit-allocations.md`. | The page exists, and its count is the progress measure for step 2. |
| **2. Explicit selection** | Module by module (JSystem → `f_*`/`m_Do`/`SSystem`/`Z2AudioLib` → `d_*` → actors): inside each "make current … restore" pair, `new T` becomes `new (heap) T` with the heap that pair made current, so behaviour doesn't change. This is partly a clang-based rewrite, and the rest is done by hand. The module's switch pairs are then removed. | The checker reports 0 for the module, its tests pass, and step 2a passes for it. |
| **2a. Rewrite verification** | Its own check for each module's rewrite, before the switch pairs go. The current heap still exists during step 2, so in debug builds every rewritten `new (heap) T` asserts that `heap` is the current heap. A wrong rewrite then fails at the first allocation it makes, in tests or a play session. The hand-edited parts get a diff review. | No assertion fires across the module's tests and a play session through the module's code paths. |
| **3. Separation** | One reservation for all game heaps. Global `operator new`/`delete` go to the host allocator; `delete` is routed by address. | The checker reports 0 for the whole engine. A test shows that standard-library allocations made while a game heap is in use stay outside the reservation. |
| **4. New heap classes** | `nf::Arena` (chunked) and `nf::GeneralHeap`; `JKRSolidHeap`/`JKRExpHeap` rebuilt on them. The "take all, then shrink" sites move to growing arenas, and the actor heap's estimate becomes a first-chunk hint without the retry. Can run in parallel with step 2. | Unit tests for both classes; existing `JKRExpHeap` tests (including the 4 GiB boundary test) pass on the new implementation. |
| **5. Owners and budgets** | Lifetime owners per tier, debug fill and generation checks, the per-platform budget table, console-budget mode on the hosts. | A tier's memory is freed by destroying its owner only; a debug build catches use of a freed arena in a test. |
| **6. ARAM and the cache tier** | Graphics archives move from the ARAM pool to pinned cache entries in main memory (R8), then the cache tier (budget, eviction) in front of archive mounting. Audio keeps the emulated pool. | The logo scene's archives load into main memory and every menu and Link animation still loads; revisiting a stage reads nothing from storage. |

## Interplay with the bake

* **`ResourceSlot`** (from the RARC design, now part of the `.nfa` reader) uses `nf::Box` with the archive's resource. That is the same ownership model, built on this module.
* **Preload flags:** the `.nfa` container keeps RARC's per-file preload flags (main RAM, ARAM, DVD). With graphics out of the ARAM pool, "preload to ARAM" is read as "preload to main memory".
* **Pack files:** the cache tier caches pack entries from the bake's index ([bake.md](bake.md#pack-files-and-the-index)).
* **Reading baked data** goes through `nf::formats::at<T>`, built on `nf::start_lifetime_as` (above).

## Open questions

Settled on 2026-11-10: C++23 for the engine, Apple's libc++ (has `<memory_resource>`, lacks `start_lifetime_as`, covered by the fallback), and the rewrite tool's verification (step 2a). Still open:

1. **The rewrite tool's reach:** how much of step 2 a clang-based rewrite can do. Allocations inside nested switch pairs or across function calls need a human; step 2a catches mistakes either way.
2. **Host reservation size.**
   * *Fixed size* from the budget table: one allocation at boot, the same code shape as the Vita (which has no overcommit and gets a fixed block either way), and running past the budget shows up immediately. On Linux and macOS an untouched anonymous mapping takes no physical memory, so a generous fixed size already costs only what is used.
   * *Reserve and commit:* reserve a large address range (`mmap` with `PROT_NONE`) and commit pages as heaps grow. Heaps can never run out on the hosts, at the cost of platform-specific commit code and of hiding budget overruns unless console-budget mode is on.
   * Either way the range is fixed at boot, so routing `delete` by address works the same.
3. **ARAM and the cache tier** (step 6): graphics move to main memory as pinned cache entries (agreed on 2026-11-11). Entries are LZ4HC-compressed (measured, [vita-storage.md](vita-storage.md)). Still open: the cache budget, sized once the extended memory mode's budget is confirmed (153 MiB of user memory was still free beside a 96 MiB heap in the benchmark).
