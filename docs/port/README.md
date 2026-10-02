# PS Vita port: planning notes

> **Status: analysis only.** Nothing has been ported. These pages size the work and propose an order of attack for this fork, based on reading and scanning the source tree. Numbers come from regex scans (`tools/utilities/port_survey.py`) and a compiler sweep (`tools/utilities/clang_sweep.py`); both can be re-run as the tree changes. Read [../03-engine-architecture.md](../03-engine-architecture.md) first if you haven't.

## Target versus source hardware

| | GameCube (the original code's home) | Wii | PS Vita (target) |
|---|---|---|---|
| CPU | Gekko, 1 core PowerPC 750-class, ~485 MHz, big-endian, 32-bit, paired-single FPU | Broadway, ~729 MHz, same family | Quad Cortex-A9 (ARMv7, 32-bit), little-endian, VFP/NEON |
| RAM | ~24 MB main + 16 MB ARAM (audio DRAM) + 3 MB embedded framebuffer/texture cache | ~88 MB in two pools | 512 MiB + 128 MiB VRAM |
| GPU | Flipper: fixed-function "GX" pipeline (TEV stages) | Hollywood, same GX | PowerVR SGX543MP4+, programmable shaders through GXM, tile-based |
| Audio | Custom DSP running Nintendo microcode, mixes voices | same | CPU-side mixing to an audio output port |
| Storage | DVD, memory card | DVD/NAND | Storage card / flash |

The console figures are approximate and only here for context. The important asymmetry is that **the Vita has vastly more memory and a far more capable GPU, but the game's code assumes a 32-bit big-endian PowerPC and a fixed-function GX GPU.** The pointer size matches (32-bit on both), which is a real advantage: file formats and structs that embed pointer-sized integers keep their layout.

## Headline findings

1. **The game logic is unusually portable.** A syntax-only sweep with stock clang 21 in C++17 mode, using *none* of the Metrowerks headers, parses **1,293 of 1,383 translation units (93.5%)** unmodified ([compile-sweep.md](compile-sweep.md)). The remaining failures come from a few mechanical causes (a missing math macro, `switch` jumps, narrowing, Wii-only or debug-only headers). Upstream has already merged a run of GCC/Clang compatibility fixes.
2. **Hardware access is concentrated in a small surface.** Outside the Nintendo SDK itself, only **42 files under `src/`** call the GPU directly; the rest of the game reaches it through JSystem (J3D/J2D). There are ~1,000 `OS*` calls (a third are logging), 70 `DVD*` calls in 23 files, 67 `CARD*` calls in 3 files, ~10 pad calls in 1 file. See [sdk-surface.md](sdk-surface.md).
3. **Endianness is the largest hidden risk, but it is localised.** The tree has *no* byte-swap helper; everything is read in native big-endian order. However, of ~2,420 resource fetches in the code, ~86% return J3D models/animations that go through central loaders and another ~7% are collision meshes (`cBgD_t`) and textures; only a few dozen fetches hand raw structs to game code. Byte order therefore has to be handled in a few dozen loader and parser files, not across 900k lines. See [endianness.md](endianness.md).
4. **Graphics is the biggest single job.** The game emits ~4,800 GX/GD calls, and J3D goes further: it writes raw GX hardware register commands (`BP`/`XF`) into display lists and calls prebuilt vertex command streams stored in the model files. A GX-to-GXM layer therefore has to handle a command stream, not just an API. See [graphics.md](graphics.md).
5. **Audio needs a software mixer.** JAudio2 drives a proprietary DSP microcode task through mailboxes. The sequencer, bank and wave-table code above it is portable; the DSP voice mixer has to be replaced.
6. **Code loading (RELs) can simply be linked statically.** With 512 MiB there is no reason to keep ~750 relocatable modules. See [toolchain.md](toolchain.md#actors-rels-become-a-static-link).

## Pages

| Page | Contents |
|------|----------|
| [toolchain.md](toolchain.md) | Compiler and language portability: the sweep result, typedefs, `char` signedness, enum size, MWCC-isms (`asm`, multi-char constants, pragmas), REL → static link. |
| [sdk-surface.md](sdk-surface.md) | Every SDK family the code touches (OS, DVD, PAD, CARD, VI, audio HW, MTX…): how much, where, and what replaces it. |
| [endianness.md](endianness.md) | Where big-endian data is parsed, per format, and strategies (convert on load vs. cook once). |
| [graphics.md](graphics.md) | GX usage, what J3D/J2D/JPA emit, options for a GXM back end, textures, render targets, screen size. |
| [roadmap.md](roadmap.md) | Phased plan, milestones, risks, and open decisions. |
| [survey-data.md](survey-data.md) | *Generated.* Full tables behind the numbers in these pages. |
| [compile-sweep.md](compile-sweep.md) | *Generated.* Per-file results of the modern-compiler sweep. |

## Decisions that shape everything (recommendations in bold)

| Decision | Options |
|---|---|
| Which game version is the code base? | **GameCube USA (`GZ2E01`)**: the only fully matching version, so the code is best understood; port Wii-only features (widescreen, pointer) selectively. Wii/Shield code has more modern features but is less complete. |
| Keep matching the original binaries? | **No.** This fork no longer needs byte-identical output. Treat the Metrowerks-specific hacks (weak function order, `dummy()` functions, PCH include order) as deletable. Keep the upstream `configure.py` flow only if you want to keep pulling upstream changes. |
| Endianness strategy | **Convert data at load time into native structs** (per-format fix-up passes), optionally caching the result. Avoid byte-swapping through accessors everywhere. |
| GPU strategy | **Write a GX-compatible layer (state tracking + command-stream interpreter) first**, so J3D/J2D/JPA and the game's ~40 direct-GX files run unmodified, then optimise the hot paths natively. |
| Screen | Native Vita resolution is 960×544 (about 16:9). Decide between 4:3 pillarbox first (least work) and adopting the Wii/Shield `WIDESCREEN_SUPPORT` code paths. |
| Save/asset compatibility | Read the game's data from a user-supplied dump, as upstream does (no assets are in this repo). Decide whether save files must interoperate with original saves (not required). |

See [roadmap.md](roadmap.md) for the phased order and risks.
