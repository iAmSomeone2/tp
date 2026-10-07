# Roadmap, risks and open decisions

A proposed order of attack, built on the findings in the other pages. It is sequenced so that **each phase produces something you can run and check**, and so that the expensive unknowns (graphics fidelity, audio, endianness of assets) are reached only after the platform is stable enough to debug them. Sizes are relative (S/M/L/XL), not calendar estimates.

## Guiding principles

1. **Keep the SDK-shaped interface, replace the implementation** (`platform/` layer). Game code stays recognisable and upstream changes still merge.
2. **Make everything measurable.** The repo now has three re-runnable scans (`port_survey.py`, `clang_sweep.py`, `dup_symbol_check.py`); extend them as milestones (e.g., "TUs that parse cleanly", "GX functions implemented").
3. **Correctness before speed** for GPU and audio (ubershader, straightforward mixer), then optimise with real profiles.
4. **Iterate fast.** Consider a *host bring-up build* (Linux/macOS with SDL) sharing the same `platform/` API so you can debug with normal tools. Caveat: a 64-bit host breaks assumptions that hold on the Vita (pointer-sized fields inside file formats such as `ResTIMG::imageOffset`); use a 32-bit host build, or fix those spots. A Vita emulator (Vita3K) can shorten the on-device loop; verify which GXM features it supports.
5. **Do not touch assets in the repo.** As upstream, the repository contains no game data; the port reads files the user extracted from their own disc.

## Phases

| # | Phase | Size | Exit criteria |
|---|---|:---:|---|
| 0 | **Groundwork** | S | CMake/VitaSDK project builds a "hello" that links a few libraries (`SSystem`, `JKernel`). `platform/` skeleton exists (empty shims for `OS`, `DVD`, `PAD`, `VI`, `CARD`, `AR`). Compiler flags from [toolchain.md](toolchain.md) applied. |
| 1 | **Everything compiles and links** | M | `clang_sweep.py` reports 0 failing TUs for the GameCube configuration (6 of 1,280 remain: five that need generated assets and `d_a_movie_player`'s PPC-asm casts). Actors link statically; `dup_symbol_check.py` reports 0 duplicates (18 left). Enable the layout `STATIC_ASSERT`s. Link with stub SDK (functions that abort with a message). |
| 2 | **Boot to the framework loop** | M | `main` → `main01` → `mDoMch_Create` → `fapGm_Create` → `fpcM_Management` runs a frame with a stub GPU. Heaps allocated from `malloc`; `OS` threads/queues/mutexes work; logging works. The process manager schedules `LOGO_SCENE` (no drawing yet). |
| 3 | **File I/O and archives** | M | `DVD*` shim over the extracted data directory; `JKRArchive` opens `.arc`; Yaz0 decompression verified. *Requires the archive header fix-ups from [endianness.md](endianness.md).* |
| 4 | **Data fix-ups (endianness)** | L | Loaders for J3D models/animations, `ResTIMG`, `.dzs/.dzr/.dzb`, BMG, events, JPC, STB byte-swap correctly. Golden-dump tests pass for many files. Loading a full stage (`F_SP103`, Ordon Village) completes with no asserts. |
| 5 | **GX layer + 2D** | XL | GX API subset implemented over GXM; ubershader TEV; immediate-mode batching; textures converted. J2D screens render: the Nintendo logo and title screen appear. |
| 6 | **3D world** | XL | J3D models decode and draw (option C in [graphics.md](graphics.md)): stage geometry, Link with skeletal animation, lighting and fog, sky. Draw-order passes match the original. Needs host versions of the SDK and JSystem matrix math first ([math.md](math.md)). Walk around Ordon Village with input working. |
| 7 | **Input, save, misc I/O** | S–M | Vita controls mapped ([sdk-surface.md](sdk-surface.md#input-pad-si)); file-backed memory card; the message box, HUD and menu run; game can be saved/loaded. |
| 8 | **Audio** | L | Software mixer behind `JASDSPChannel`; sequenced music, sound effects and streaming work; Z2 3D positioning correct. Silence is acceptable until here; *do not block phases 5–7 on it.* |
| 9 | **Effects and special cases** | L | Particles, shadows, bloom, depth-of-field, indirect-texture water/warp, mirrors, cloth, rain; THP video replacement (or skip cutscene movies initially). |
| 10 | **Performance, memory, polish** | L | Batching/precompiled shaders; NEON for hot math ([math.md](math.md)); texture/mesh cooking; profile the 30 Hz frame on device; touch UI; widescreen HUD decision; suspend/resume. |

### Milestones you can announce

* **M1** – Compiles and links for the Vita target with stub platform.
* **M2** – Boots into the framework loop; logs process creation for the logo scene.
* **M3** – Logo and title screen render.
* **M4** – Ordon Village explorable (no audio).
* **M5** – Audio.
* **M6** – First dungeon playable end to end.
* **M7** – Full game; performance target reached.

## Stretch goal: frame rates above 30 Hz

Not part of the port proper; consider it once M7 is reached. Size: L.

**Approach: fixed-rate simulation, interpolated rendering.** The game logic stays at its fixed 30 Hz tick (`d_s_play.cpp` sets `mDoGph_gInf_c::setTickRate(OS_TIMER_CLOCK / 30)`; the logo scene uses 60 Hz). Rendering runs at the display rate and blends between the two most recent ticks by a fraction `alpha` in [0, 1): actor base matrices, joint animation (the frame value, or the computed joint matrices), the camera (eye, center, field of view) and 2D element positions. Gameplay, physics and timings are unchanged; motion is smoother. The PC ports of Ocarina of Time and Majora's Mask (Ship of Harkinian, 2 Ship 2 Harkinian) took a 20 Hz game to high frame rates this way.

**Rejected: converting to delta time.** The game logic assumes a fixed step throughout. Per-tick velocity (`speedF`) appears on 3,974 lines of actor code, and the per-tick chase/ease helpers `cLib_chaseF`/`cLib_addCalc*` on about 7,400 lines; `addCalc` applies a fixed fraction per tick, so a correct delta-time version needs `pow()` and changes the tuning at every call site. Add frame-count timers, animation frame counters, collision tuned to the step size and the `d_a_alink` state machine, and every one of ~750 actors becomes a bug hunt.

**Work involved:**

* Split the frame loop: `fpcM_Management`'s execute step on simulation ticks only, the draw step on every display frame, with `alpha` available to the draw path.
* Make draw handlers idempotent. They would run several times per tick, so any draw code that advances a counter, draws from the RNG or changes state has to move into execute. This needs an audit of the actors.
* Snap instead of blending across discontinuities: camera cuts, warps, respawns, room changes, and actors created or deleted between ticks. The SoH approach tags each recorded matrix with a stable ID and only blends matching IDs.
* Particles (JPA) and many J2D animations step once per tick; interpolate their output or step them per frame.
* CPU cost: recomputing J3D joint matrices per display frame roughly doubles that work (see [math.md](math.md); GPU skinning helps most).
* Input latency stays at the 30 Hz tick; only the presentation gets smoother.

**On the Vita** the display is 60 Hz, so the target there is interpolated 60 Hz, and only if a stable 30 Hz leaves headroom. A desktop host build is where higher refresh rates pay off.

**Do now:** nothing to implement, but design the GX → GXM layer so this stays cheap later; see [graphics.md](graphics.md#designing-for-frame-interpolation).

## Concrete first tasks

Small, independent, and useful whichever way the rest goes:

1. ~~Fix the `switch`-scope errors~~ (done, along with the shared `DEG_TO_RAD`/`RAD_TO_DEG` macros, the integer typedefs and the pointer casts). The sweep now covers only the files the GameCube build compiles; what is left for it is the generated asset headers and `d_a_movie_player`.
2. `static`-ify `l_HIO`, `hio_set`, `l_arcName`, etc. (the 18 remaining duplicates; `__OSExecParams`/`__OSAppLoaderOffset` are already `extern`).
3. Write `platform/endian.h` and convert the choke points (`JSUInputStream`, `JASSeqReader`, `JKR*Archive`) behind a `PLATFORM_LITTLE_ENDIAN` switch, keeping the original big-endian build unaffected.
4. Implement the `DVD*` file shim and `JUTGamePad` over `sceCtrl` (both are tiny and unblock everything).
5. Build a **reference asset dumper** (Python with big-endian `struct`) for RARC and J3D to serve as the oracle for phase 4.
6. Delete decomp-only hacks as you meet them (`dummy()` functions, `IS_REF_NULL`, PCH include-order tricks), on a branch that keeps a clean revert path if you ever want to rebuild the matching binaries.
7. Decide the base version and screen-size question (below) before writing much graphics code.

## Risks

| Risk | Why it matters | Mitigation |
|---|---|---|
| **GX fidelity** | TEV combinations, indirect texturing, fog, lighting and texgen have many corner cases; small deviations show as wrong colours. | Ubershader first; keep a per-material debug view; compare against references (screenshots/emulator captures of the same scene). |
| **J3D display-list decoding** | The shape display lists are stored in the model files and consumed by hardware. A decoder bug corrupts geometry silently. | Decode once at load with structural checks (vertex counts, index ranges); test on many models; visualise. |
| **Audio behaviour** | The DSP microcode defines exact mixing, filtering and envelope behaviour that JAudio2 assumes. | Implement the `TChannel` contract precisely; test with isolated sounds; add effects buses later. |
| **CPU performance** | Cortex-A9 cores are not the Gekko; the game is single-threaded by design, so the four cores do not help by default. | Profile early on device; keep the 30 Hz tick; look at CPU-side skinning cost, `MTX*` math, collision (`dBgS`, `dCcS`), and particle counts. Moving audio mixing, file I/O and GPU command building off the main thread are the natural parallelisation points. |
| **Alignment / undefined-behaviour code** | Code written and matched for one compiler may rely on unaligned loads, strict-aliasing violations or null-`this` tricks. | Compile with `-fno-strict-aliasing -fno-delete-null-pointer-checks`; use sanitizers on the host build; convert parsers to `memcpy` loads. |
| **Upstream drift** | The fork is expected to diverge; conflicts grow with time. | Keep the platform layer additive; rebase or cherry-pick upstream fixes on a cadence; keep the original build on a branch for matching-based validation. |
| **Memory on device** | Homebrew-accessible memory may be less than the hardware total (system reserves, allocator limits). | Measure the actual budget early; the ARAM/REL/heap machinery can be simplified but heap *sizes* in `mDoMch_Create` should be re-tuned to the measured budget. |
| **Assets and legal hygiene** | Distribution of engine-only code is the safe model; shipping any game data is not. | Follow upstream: no assets in the repo; require user-supplied data; keep converters generic. |
| **Wii/Shield-only features** | Widescreen, pointer, HOME menu live behind `PLATFORM_WII`/`WIDESCREEN_SUPPORT`; GameCube code lacks them. | Decide what to back-port; keep the `#if` structure so behaviour is switchable. |

## Open decisions (need your input)

1. **Base version**: GameCube USA (recommended; fully matching, best-understood) vs. a Wii or Shield base (widescreen and newer code, but less decompiled).
2. **Aspect ratio**: pillar-boxed 4:3 first, then widescreen (port the Wii `WIDESCREEN_SUPPORT` paths), or widescreen from the start?
3. **Shader strategy** (graphics.md): ubershader first with offline precompilation later, or runtime generation from the beginning?
4. **Endianness**: convert on load first (recommended), then cook assets; or cook up front?
5. **Save compatibility**: native little-endian saves only, or importing original GameCube saves?
6. **Audio ambition**: faithful DSP behaviour (effects, filters, Dolby pan) or a simpler mixer to start?
7. **Controls**: where do Z, the analog triggers and the C-stick-dependent actions go (rear touch, chords, or a remappable layout)?
8. **Host bring-up build**: worth building a desktop target alongside the Vita target for faster iteration? (The `platform/` layer makes it cheap.)
9. **Upstream relationship**: track upstream (cherry-pick fixes) or fully diverge?
