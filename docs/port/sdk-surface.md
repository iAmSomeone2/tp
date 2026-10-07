# SDK surface: what the game asks of the hardware layer

The game and its middleware (`src/`, `libs/JSystem/`) sit on Nintendo's Dolphin SDK (`libs/dolphin`, ~77k lines) or Revolution SDK (`libs/revolution`, ~116k lines). A port **does not port the SDK**; it provides the same *interface* (or a slimmer one) on top of the Vita's own libraries. This page lists what is actually called, where, and what to replace it with. All counts are from [survey-data.md](survey-data.md) (regenerate with `python3 tools/utilities/port_survey.py`).

Suggested approach: create a `platform/` layer that keeps the SDK function names (`OSReport`, `DVDOpen`, `PADRead`, `GXBegin`, …) but implements them for the Vita. Game code and JSystem then compile unchanged, and you can later shrink or rename the layer as you modernise. The alternative (rewriting call sites) touches ~7,000 call sites for no early benefit.

## Overview table

| SDK family | Calls | Distinct | Files | Where it concentrates | Effort | Replace with |
|---|---:|---:|---:|---|:---:|---|
| **GX** GPU (+ **GD** display-list builders) | 4,640 (+163) | 184 (+12) | 88 | `src/d` core 1,918; J2D 799; `m_Do` 775; actors 443; J3D | **Very high** | GXM back end; see [graphics.md](graphics.md) |
| **MTX/VEC/QUAT** math | 1,794 | 62 | 419 | actors 1,063; `d` 236; SSystem 187 | Low | Portable C/NEON (`MTXCopy` alone is 930 calls) |
| **OS** | 1,013 | 111 | 132 | `d` core 257, `m_Do` 224, actors 92, JKernel 83 | Medium | Threads, time, logging, arena shims |
| **VI** video | 100 | 17 | 14 | JUtility (`JUTVideo`) 54, `m_Do` 23 | Medium | Vblank/swap pacing, frame buffers |
| **DVD** disc | 70 | 17 | 23 | JKernel 24, actors 16 (movie player), `m_Do` 12, JAudio2 9 | Medium | File I/O over an extracted data directory |
| **CARD** memory card | 67 | 24 | 3 | `m_Do_MemCard*` 64 | Low | File-based save backend |
| **AI/AX/DSP** audio HW | 50 | 19 | 15 | JAudio2 49 | **High** | Software mixer + audio port thread |
| **AR/ARQ** ARAM | 9 | 6 | 3 | JKernel, JAudio2 | Low | Plain memory |
| **PAD/SI** GameCube pad | 10 | 9 | 2 | `JUTGamePad` | Low | Vita buttons/sticks/touch |
| **EXI/HIO/MCC** dev hardware | 14 | 10 | 2 | JHostIO | None | Delete (debug tooling) |
| **WPAD/KPAD** Wii remote | 45 | 27 | 6 | `m_Re`, `Z2AudioCS` | Optional | Only for Wii features (pointer/gyro) |
| **NAND/SC** Wii storage/settings | 44 | 19 | 8 | `m_Do_MemCard*`, `d_s_logo` | None | Wii-only; skip on a GameCube base |

Effort here is the work in the platform layer, not the cost of the game code above it.

## OS

1,013 calls to 111 functions. By purpose:

| Purpose | Calls | Notes / replacement |
|---|---:|---|
| Logging/panic (`OSReport` 296, `OSPanic`, warnings) | ~320 | `printf`-style to `sceClibPrintf`/a log file. Trivial. Keep `OSPanic` as abort with message. |
| Threads, mutexes, message queues | ~260 | `OSCreateThread` is called in only 6 files: `JKRThread` (base of all JSystem threads), the DVD thread (`m_Do_dvd_thread`), memory-card thread (`m_Do_MemCard`), DVD-error thread (`m_Do_DVDError`), `m_Do_main` (the main game thread) and 5× in `d_a_movie_player`. Map to pthreads/Vita threads with a priority table; implement `OSMessageQueue` with a mutex+condvar. `OSSuspendThread/ResumeThread/CancelThread` need care (cooperative flags rather than async suspend). |
| Interrupt masking (`OSDisableInterrupts`/`OSRestoreInterrupts`/`OSEnableInterrupts`) | ~90 | Used as *critical sections*. Replace with a global recursive mutex (or per-caller mutex once you know what each protects). Beware callbacks that the SDK ran in interrupt context (DVD, audio, VI retrace); they now run on threads. |
| Time (`OSGetTime` 70, `OSGetTick`, `OSTicksTo…`, stopwatches) | ~170 | Return 64-bit timebase ticks; define `OS_TIMER_CLOCK` to match your clock and convert from a monotonic clock. The game sets its logic tick as `OS_TIMER_CLOCK / 30` (`dScnPly_c`), so the constant must be consistent. Stopwatches are debug profilers. |
| Alarms | 9 | Timer callbacks; emulate with a timer thread. |
| Arena / memory (`OSGetArenaHi/Lo`, `OSAlloc`, `OSRoundUp32B`) | ~43 | `mDoMch_Create` carves the JKernel root heap from the arena; hand it one large `malloc`'d block. |
| Dynamic modules (`OSLink`, `OSUnlink`, `OSSetStringTable`) | 6 | REL loading; disappears with a static link ([toolchain.md](toolchain.md#actors-rels-become-a-static-link)). |
| Reset/HOME/settings (`OSGetResetCode`, language, sound mode, progressive mode) | ~27 | `mDoRst` implements soft-reset and shutdown. Map to app suspend/resume/exit; return constants for language/sound mode (or read the Vita system language). |
| Fast casts (`OSf32tos16`, …) and cache ops (`DC*`, `LC*`) | 33 + ~150 | See [toolchain.md](toolchain.md#metrowerks-isms-in-the-source). |

Also in this family: `JUTException` (crash handler that decodes PPC registers, ~44 OS calls) and `m_Do_machine_exception`: replace with a POSIX signal handler or drop.

## DVD (file I/O)

70 calls, 17 functions, 23 files. Everything the game loads goes through `JKR` (JKernel):

```
game code ──► JKRArchive / JKRFileLoader ──► JKRDvdFile / JKRDvdRipper / JKRFileCache ──► DVDOpen / DVDReadPrio / DVDClose
              (mount, getResource)            (read + optional Yaz0/Yay0 decompress)
```

* Top functions: `DVDReadPrio` (19), `DVDClose` (11), `DVDGetDriveStatus` (6, disc-error handling), `DVDConvertPathToEntrynum` (6), `DVDOpen` (4), `DVDFastOpen` (3, opens by "entry number" resolved earlier), `DVDOpenDir/ReadDir/CloseDir` (directory scans in `JKRFileCache`).
* Other users: `d_a_movie_player` (16: streams THP video), `JASAramStream`/`JASDvdThread` (audio streaming), `m_Do_Reset`, `m_Do_main`, `JUTDirectFile`.
* **Replacement**: implement the `DVD*` subset on `stdio`/`sceIo*` over a directory of extracted game files (game paths look like `/res/Object/Alink.arc`, `/Audiores/Z2Sound.baa`, `/res/ItemTable/item_table.bin`). Entry numbers can be indices into a table built at start-up. Keep the asynchronous behaviour: the game issues loads from a dedicated DVD thread (`mDoDvdThd_*` commands) and polls them, so a worker thread with the same command queue works as is.
* **Disc-error UI** (`dDvdErrorMsg_c`, `m_Do_DVDError`) can be disabled (`DVDGetDriveStatus` → always "ok") or repurposed for "data missing".
* You must supply the data extracted from the user's disc image. Upstream's toolchain (`dtk`) can read GameCube images; see [roadmap.md](roadmap.md) for an asset-cooking step.

## VI (video) and frame pacing

100 calls, 14 files. `JUTVideo` (30) registers pre/post-retrace callbacks and tracks the retrace count; `JFWDisplay` sequences a frame (begin render → draw → copy to XFB → swap → wait); `m_Do_graphic`/`m_Do_machine` configure modes; `JKRDvdRipper`/`JUTException`/`d_a_movie_player` also poll the retrace count.

* The external frame buffer (XFB) concept goes away: the game renders to an EFB and `GXCopyDisp` copies to XFB. On the Vita you render straight to a swapchain image.
* `VIWaitForRetrace`, `VIGetRetraceCount`, `VIFlush`, `VISetBlack`: implement on the Vita's vblank counter. The Vita display runs at 60 Hz; the game logic runs at 30 Hz, so present every second vblank (vsync interval 2).
* Interlace/progressive/DTV/trap-filter/dimming calls are no-ops.

## Memory card (`CARD*`) and save data

67 calls to 24 functions, confined to `m_Do_MemCard.cpp` and `m_Do_MemCardRWmng.cpp` (a memory-card thread and read/write manager) plus a header. The `CARDSet*IconFormat/Speed`/banner calls exist only to draw the GameCube memory-card icon.

* Replace with a small file-backed "virtual card": a directory of three save-slot files plus the option file, written atomically (`write tmp → rename`).
* The save struct is a raw memory image (`dSv_save_c`, 0x958 bytes per slot; see [../04-game-systems.md](../04-game-systems.md#save-data-and-flags)) with a checksum (`mDoMemCdRWm_SetCheckSumGameData`). If you write your own saves you can keep it little-endian native; only cross-compatibility with original GameCube saves would require swapping every field. See [endianness.md](endianness.md).
* Wii builds use `NAND*`/`SC*` instead; not relevant to a GameCube base.

## Audio hardware (`AI/AX/DSP/AR`)

Two layers: the game-side sound library `Z2AudioLib` (portable logic: what to play, when, at what volume, on which actor) and **JAudio2** (engine: sequence interpreter `JASTrack`, banks/waves, ADPCM, mixer control). JAudio2 talks to hardware in two places (49 calls):

* **AI (audio interface)**: `JASAiCtrl` sets up a DMA callback that is asked for the next block of PCM (`AIInitDMA`, sample-rate queries). Replace with an audio-port thread that requests blocks of stereo PCM and calls the same callback. `JASDriver` already supports both rates (`sDacRate` 32 kHz with 7 sub-frames, or 48 kHz with 10 sub-frames, `JASAiCtrl.cpp`), so the Vita's 48 kHz output is a natural fit.
* **DSP task** (`dsptask.cpp`, `osdsp*.cpp`, `dspproc.cpp`, `JASDSPInterface.cpp`, `JASDSPChannel.cpp`): the game uploads a small **proprietary DSP microcode** (`jdsp[7936]`, embedded in `dsptask.cpp`) and talks to it via mailboxes. The DSP mixes 64 voices (`DSP_CHANNELS`), decoding 4-bit DSP-ADPCM (`mBytesPerBlock`, `mSamplesPerBlock`), applying pitch/resampling, FIR/IIR filters (`fir_filter_params`, `iir_filter_params`), volume/pan/dolby/effects-send mixes per output bus (`OutputChannelConfig`), looping (`mLoopStartSample`, `mEndSample`) and streaming from ARAM (`mAramStreamPosition`).
  * **Replace the DSP with a software mixer** that implements the `JASDsp::TChannel` semantics: the *contract* is the `TChannel` struct (~0x180 bytes) and the sequence of frames JAudio2 writes. Everything above `JASDSPChannel` (sequencer, banks, `JAIStream`, `Z2*`) stays.
  * Effects: reverb/chorus buses (`Z2FxLineMgr`, `AXFX*` calls in the SDK) are separate; start without effects.
* **ARAM** (`ARAlloc`, `ARQPostRequest`, 9 calls): waves were preloaded into a separate 16 MB audio RAM. Make `JKRAram`/`JASHeapCtrl` allocate from ordinary memory; DMA "copies" become `memcpy`.
* **Streaming music/voices** use `JASAramStream` reading via DVD; keep the logic and back it with the file layer.
* The `bit-fields` in `JAISound.h`/`JASTrack.h` are pure in-memory state and are fine on little-endian; the data files (`.baa`, banks, sequences) are the endianness concern, see [endianness.md](endianness.md).

## Input (`PAD`, `SI`)

`PADRead` and friends are called only from `JUTGamePad.cpp` (9 calls); `mDoCPd_c` (`m_Do_controller_pad`) and the rest of the game use `JUTGamePad`. Implement the ~9 PAD functions over `sceCtrl*` and translate to the `PADStatus` structure (`PAD_BUTTON_*` bit masks, stick, C-stick, trigger analog).

| GameCube | Vita | Comment |
|---|---|---|
| Main stick / C-stick | Left / right stick | Direct. |
| A, B, X, Y | Cross/Circle/Square/Triangle (choose a layout) | The game's HUD button icons (`itemicon`, `button.h` layouts) show GameCube glyphs; new icons/layout tweaks will be needed for a faithful look. |
| L, R (analog triggers + click), Z | L, R (digital) + ? | The Vita has no analog triggers: report full deflection when pressed. `getAnalogL/R` is read in 21 places (`d_camera`, `d_attention` for lock-on, `m_Do`). **Z has no direct key**: rear touchpad, Select, or a chord. |
| Start | Start | |
| Rumble (`PADControlMotor`) | none | The Vita has no rumble motors in the handheld; stub out (the game also has an option to disable it). |
| (Wii) pointer, gyro, Nunchuk | Front/rear touch, gyro | Optional; `d_cursor_mng`, `m_Re_controller_pad` are the models to follow. |

The Vita screen also offers touch for menus and map selection; the item wheel (`dMenu_Ring_c`) and maps are natural candidates.

## Math library (`MTX`, `VEC`, `QUAT`)

1,794 calls to 62 functions (`MTXCopy` 930, `MTXConcat` 122, `MTXMultVec` 50, `MTXIdentity`, `VECScale`, `MTXInverse`, …) in 419 files. The SDK ships hand-written paired-single assembly (`PSMTX*`, in `libs/dolphin/src/mtx/`) and C reference versions (`C_MTX*`). For the port use straightforward C first, then NEON for the hot ones (`MTXConcat`, `MTXMultVec`, skinning). J3D and JMath have their own paired-single matrix and vector code. Most of it has no C fallback: on a host it is either undefined (`J3DPSMtxArrayConcat`) or compiles to an empty function. See [math.md](math.md).

## What you can delete

* `libs/revolution` (except any types you reuse), `libs/dolphin` sources for hardware you replace, `libs/PowerPC_EABI_Support` (MSL C/C++, Runtime, MetroTRK): use the toolchain's libc and `libstdc++`. `MSL_C` provides non-standard functions the code calls (`stricmp`, …) and `<cmath>` macros (`DEG_TO_RAD`); provide those in a shim header.
* `JHostIO`, `JAHostIO`, `JAWExtSystem`, `JAWWinLib`, `JAHNodeLib`, `NdevExi2A`, `odemuexi2`, `amcstubs`, `lingcod`: debug tooling and Shield patches.
* `TRK_MINNOW_DOLPHIN`: on-device debugger stub.
* All `#if DEBUG` HostIO code (`*_HIO_c` classes, ~2,900 `#if DEBUG` blocks in total) if you build without `DEBUG`.
