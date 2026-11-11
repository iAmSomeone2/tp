# Asset bake: design

> **Status: design, nothing built yet.** Decisions recorded here were made with the developer on 2026-11-10. Open questions are listed at the end.

The engine will no longer read the retail disc. A tool run once on the dev PC (the **bake**) converts the user's own disc image into a tree of *baked* files. Baked files are little-endian, contain no pointers, and use texture formats a modern GPU samples directly. The engine reads only baked data on every target (Linux, macOS, Vita).

This replaces the "convert on load first" recommendation in [endianness.md](endianness.md) (strategy B there, adopted outright). It is also how the 64-bit layout problems in [file-layout-64bit.md](file-layout-64bit.md) get fixed, and it absorbs the RARC fix discussed for that survey.

## Decisions

| Topic | Decision |
|---|---|
| Where conversion happens | Offline, in the bake. No byte swapping or relocating at run time. |
| MVP scope | Unpack archives, make all data little-endian, convert textures to standard formats, repack into a container designed for this fork. Optimised or upgraded assets are later work. |
| Layout rule | **One baked format for all targets.** All offsets in file data are `u32`; no pointers, `size_t` or `uintptr_t` in any baked struct. The engine's file structs change to match. File data is never written after loading (no in-place relocation); code resolves offsets when it reads them, or builds host-side structs. |
| Archive API | **`JKRArchive` stays the interface** for the ~2,400 resource fetches and `d_resorce`. It is reimplemented on top of the new container. |
| Textures | **Lossless**: every texel decodes to exactly what GX produced. `CMPR` → BC1. Per-format table in [vita-tex-fmts.md](vita-tex-fmts.md). The bake records what a texture means (intensity, luminance + alpha, colour), not a GXM enum, so each GPU backend picks its own format. |
| Tool language | **Rust** (Cargo workspace in `tools/bake/`). |
| Single source of truth | **C++ headers** in `include/nightfall/formats/` define every baked layout. `bindgen` generates the Rust types from them, including layout tests. Disc (source) formats exist only in the Rust tool. |
| Engine build | **Never needs Rust.** Only running a bake needs `cargo`. |

## What is on the disc

Measured on the user's `GZ2E01` image with `dtk vfs ls -r`. Sizes are as stored, after Yaz0 compression where it applies.

| Top-level files | Count | Size | Baked? |
|---|---:|---:|---|
| `.ast` audio streams | 127 | 381 MiB | Later (audio phase) |
| `.arc` RARC archives | 1,843 | 311 MiB (440 MiB decompressed, top level only) | Yes, the core of the bake |
| `.aw` wave banks | 232 | 148 MiB | Later (audio phase) |
| `.thp` movie | 1 | 131 MiB | Later: re-encoded to H.264 + AAC in MP4 (see *Movie*); the MVP stubs the player |
| `.jpc` particles | 65 | 11 MiB | Yes |
| `.map` linker maps (`map/Final/Release`) | 758 | 7 MiB | No: debug symbol maps of the RELs, not game data |
| `.rel` modules | 623 | 4 MiB | No: actors are statically linked |
| `Z2Sound.baa` audio bank | 1 | 2.3 MiB | Later (audio phase) |
| `str/`, `.bin`, banner | 6 | tiny | Case by case |

Of the 1,843 archives, 1,778 are Yaz0-compressed as a whole. Inside them are **22,608 files**, 1,808 of them compressed individually and **252 nested archives** (an `.arc` inside an `.arc`).

| Format inside archives | Files | Size | Engine loader |
|---|---:|---:|---|
| `.bmd`/`.bdl` models | 3,273 / 23 | 286 / 0.4 MiB | `J3DModelLoader`, `J3DMaterialFactory`, `J3DShapeFactory` |
| `.kcl` collision | 305 | 54 MiB | `dBgWKCol` |
| `.bck` joint animations | 8,732 | 51 MiB | `J3DAnmLoader` |
| `.bti` textures | 1,697 | 15 MiB | `JUTTexture` / `ResTIMG` |
| `.bls` clusters | 29 | 6 MiB | `J3DClusterLoader` |
| `.dat` (event lists, tables) | 1,053 | 2.8 MiB | various (`d_event_data`, …) |
| `.blo` 2D layouts | 132 | 1.8 MiB | `J2DScreen` (through `JSUInputStream`) |
| `.dzr`/`.dzs` stage/room | 305 / 423 | 1.4 / 0.9 MiB | `d_stage.cpp` chunk handlers |
| `.stb` cutscene timelines | 278 | 1.4 MiB | `JStudio` |
| `.bmg` messages | 10 | 1.1 MiB | `JMessage` |
| `.bms` sequences | 168 | 1.1 MiB | `JASSeqParser` (audio phase) |
| `.dzb` collision | 513 | 1.1 MiB | `cBgS::ConvDzb`, `cBgW` |
| `.btk`/`.btp`/`.brk`/`.bpk`/`.blk` animations | 2,239 / 1,919 / 492 / 90 / 178 | 2.2 MiB together | `J3DAnmLoader`, `J2DAnmLoader` |
| `.plc` collision attributes | 305 | 0.1 MiB | `dBgPlc` |
| `.bfn` fonts | 3 | 0.2 MiB | `JUTResFont` |
| `.csw`, `.bct`, `.bin`, `.txt`, `.csv` | 39 | ~1 MiB | to be identified |

### Audio streams

What the game's streams need (headers of all 127 `.ast` files, read from the disc):

| | Value |
|---|---|
| Codec | DSP-ADPCM (4-bit) in all 127; none use the PCM16 variant |
| Channels | stereo in 126, mono in 1 (`e_ganon_d02.ast`) |
| Sample rate | 44.1 kHz in all |
| Length | 134.8 minutes in total, about 49 KiB/s |
| Looping | 58 streams loop. In all 58 the loop runs from `loop_start` to the very end; in 57 of them `loop_start` is mid-stream (sample-exact) |
| Concurrency | **One stream at a time.** The audio manager creates a single ARAM stream buffer (`JAUStreamStaticAramMgr_<1>`, `Z2AudioMgr.cpp:93`). |

Nothing needs more than stereo; `JASAramStream`'s `CHANNEL_MAX = 6` is never used. The wave banks (`.aw`) behind the sequenced music and sound effects are a different case. They hold short instrument and effect samples, which the mixer plays as many simultaneous, pitch-shifted and looped voices. They stay with the software mixer in their own format, and none of the following applies to them.

**Plan:**

1. **MVP: keep DSP-ADPCM.** The bake swaps only the header and block-header fields (sizes, loop points, per-channel ADPCM history); the 4-bit sample data has no byte order. Decoding costs a few operations per sample. It is lossless with respect to the disc. Loops stay sample-exact the way the original does it, by streaming with `JASAramStream`'s ADPCM history handling, so no RAM cache is needed.
2. **Later: Opus.** This happens once a benchmark on the Vita confirms the decode cost; it's expected to be a few percent of one core for the single stereo stream.
   * **Size:** 96–128 kbit/s stereo is close to transparent at about 12–16 KiB/s, so the 381 MiB of streams shrink to roughly 100 MiB.
   * **One decoder everywhere:** libopus (BSD licence, NEON-optimised for ARMv7) runs on the dev PC, the Mac and the Vita alike.
   * **Sample rate:** libopus outputs only 8/12/16/24/48 kHz, so the bake resamples to 48 kHz and scales the loop points. It resamples the loop body as a circular signal, so the end runs straight into the loop start. The software mixer resamples anyway.
   * **Exact loops:** after a seek, Opus needs about 80 ms of decoding before its output is correct (RFC 7845's recommended pre-roll). The bake stores, per track, the packet to restart from and how many samples to discard to reach `loop_start` exactly.
   * **Storage:** raw Opus packets in the `.nfa` entry, with a packet table and the loop data. No Ogg container, so the engine has nothing to parse.
   * **Lossy on top of lossy**, unlike the lossless texture rule, but inaudible at these bitrates.
3. **Optional loop cache.** The developer can guarantee a 256 MiB RAM budget on the Vita, and only one looping track ever plays at a time. So the current track can be kept in RAM, which makes the seek to the loop's restart point a memory access. Its cost per format, for the largest looping track (`demo35.ast`, 5.4 minutes) and the median one:

   | Track kept resident as | Worst case | Median |
   |---|---:|---:|
   | 16-bit stereo PCM, fully decoded | 54.8 MiB | 13.6 MiB |
   | DSP-ADPCM (as on disc) | 15.4 MiB | ~4 MiB |
   | Opus at 128 kbit/s | ~5 MiB | ~1.3 MiB |

   The next largest looping tracks as PCM are 30.9, 30.1 and 26.2 MiB. Keeping the track compressed and decoding while playing costs a fraction of the memory of a PCM cache and still loops exactly.

**Considered and set aside: the Vita's hardware decoders** (ATRAC9, MP3, AAC, CELP). From the developer's tests, MP3 and AAC handle VBR up to 256 kbit/s stereo. Some VBR AAC files peaking above 256 kbit/s played and others didn't, so the limit may be in the music player app rather than the hardware. The SDK docs give no channel limits for AAC and ATRAC9, but stereo is all the streams need. These decoders have the same exact-loop issue as Opus. They exist only on the Vita, though, so the dev PC and Mac would need a second, software decode path. ATRAC9 also seems to need Sony's encoder, which the open toolchain doesn't include. AAC at 256 kbit/s would also be about twice the size of Opus at a similar quality.

### Movie

The disc has one movie, `Movie/demo_movie98_00.thp`. It plays at the end of the title screen's demo loop.

| | Value |
|---|---|
| Video | 608 × 448, 29.97 fps, 3,778 frames (2 min 6 s) |
| Audio | stereo, 32 kHz |
| Size | 131 MiB, about 8.7 Mbit/s |

THP stores every frame as a separate JPEG with no motion prediction, which is why it is so large for its resolution.

* **MVP: stubbed.** `src/nightfall/movie/movie_player.cpp` (`nf_movie`) replaces `d_a_movie_player.cpp`, which is excluded from `tp_actors`. The stub ends every movie at once, as if the player had skipped it, so the demo loop carries on.
* **Later: H.264 video + AAC audio in an MP4 container**, re-encoded by the bake. The developer found the quality loss imperceptible for this file, and a few Mbit/s of H.264 should cut it to a fraction of its size. Both dimensions are multiples of 16, so the frame size needs no padding for H.264. The bake would run an external encoder (e.g. `ffmpeg`) rather than link one.
* **Playback on the Vita:** either hardware path works.
  * `SceAvPlayer*` takes the MP4 file and delivers decoded video frames and audio. As far as I know it draws no playback controls and leaves showing the frames to the app; a quick test on the device should confirm that. It is the simpler choice for full-screen playback.
  * `SceAvcdec*` is a lower-level H.264 decoder whose output frames can be used directly as GPU textures (GXM's `YUV420P2`/`YUV420P3` formats). Audio would then need its own decoder.
* **Playback on the hosts:** H.264 decoding is available almost everywhere: libavcodec in software, or hardware through VideoToolbox on macOS and VA-API or the Vulkan Video extensions on Linux. AAC in software costs next to nothing. Which one to use is decided when the host video path is built.

## Pipeline

```
GZ2E01.iso ──read (nod)──▶ disc tree ──▶ per file: unpack (RARC, Yaz0, nested archives)
                                              │
                                              ▼
                                  identify format (extension + magic)
                                              │
                          ┌───────────────────┴───────────────────┐
                          ▼                                       ▼
                 converter exists:                      no converter yet:
            parse disc format (binrw, BE)             copy bytes unchanged,
            → write baked format (LE, u32 offsets,    tagged "raw big-endian"
              textures converted)                       (the engine refuses to
                          │                              hand these out)
                          └───────────────────┬───────────────────┘
                                              ▼
                           repack into the baked container (.nfa)
                                              ▼
                     build/baked/<version>/  +  bake report (per-format counts, raw leftovers)
```

* **Disc input:** the ISO is read directly with the `nod` crate, the disc library decomp-toolkit's `dtk vfs` is built on, so the user doesn't need to extract anything first.
* **Output** goes to `build/baked/<version>/`. It is derived from the user's disc and never committed.
* **Deterministic:** the same disc and tool version give byte-identical output, so bakes can be diffed and tested.
* **Parallel:** files are independent; the tool converts them on all cores (`rayon`).
* **Honest about gaps:** a file without a converter is still packed, tagged as raw big-endian. The bake report lists every such file, so partial bakes are usable during development, and nothing is silently left big-endian.

## The baked container (`.nfa`)

It replaces RARC. It has to support everything the `JKRArchive` API exposes, because that API stays:

* **Lookups:** by index, by file ID, by name, by type (directory 4CC) and name, and by path through the directory tree. That includes `JKRArchive::getDirEntry`, `JKRArcFinder`, `countFile`, and the index arithmetic `entry - mFiles` that `d_resorce.cpp` and `m_Do_ext.cpp` use.
* **Same order:** file indices, IDs and directory order stay as on the disc. Game code and resource tables refer to files by index or ID.
* **Load policy:** the per-file flags that tell a compressed mount which members to preload into main RAM, ARAM or leave on DVD (`0x10`/`0x20`/`0x40`) are kept. They are memory policy, which the memory-model work will revisit.

Differences from RARC:

* **No compression inside the archive.** Members are stored decompressed and 32-byte aligned, so a mounted archive is directly usable. Compression, if any, belongs to the pack layer below, per entry, and is undone when the entry is loaded.
* **No nested archives:** the 252 archives inside archives are baked recursively and stay nested as `.nfa` members, so code that mounts them from memory still works.
* **Per-file format tag:** each file entry records what the bake turned the file into (format tag + version), or "raw big-endian". The tag is in the entry, *not* in front of the file data, because game code casts fetched resources directly to structs (e.g. 106 casts to `cBgD_t*`). In debug builds the engine checks the tag on fetch.
* **No runtime fields:** RARC's `SDIFileEntry::data` (the cache slot that breaks on 64-bit) does not exist. Per-file runtime state lives in the archive object: the `ResourceSlot` design (a resource is empty, borrowed from the archive's buffer, or owned in a heap allocation with a heap-aware deleter; `removeResource` resets, `detachResource` releases), with the expanded-size array folded in.

Sketch of the layout (all little-endian, every struct `static_assert`ed):

```cpp
// include/nightfall/formats/nfa.h  (sketch; field names final in the implementation)
struct NfaHeader   { u32 magic /*'NFA1'*/; u32 version; u32 file_size;
                     u32 dir_count, dir_offset; u32 file_count, file_offset;
                     u32 strings_size, strings_offset; u32 data_offset; u32 reserved[2]; };
struct NfaDir      { u32 type /*4CC*/; u32 name_offset; u16 name_hash; u16 file_count; u32 first_file; };
struct NfaFile     { u16 id; u16 name_hash; u32 name_offset; u32 attr /*RARC flags*/;
                     u32 data_offset; u32 data_size; u32 format /*tag + version*/; };
```

## Pack files and the index

> **Status: decided 2026-11-11 from Vita measurements** ([vita-storage.md](vita-storage.md), `tools/vita-bench/`).

Vita storage reads at only 8–10 MiB/s, has no OS read cache, and costs about 11–13 ms extra per file opened compared with reading the same bytes from an already open file (opens themselves are under 1 ms). TP opens a separate file for every stage, room and object archive. The bake therefore writes the baked tree into **a few large pack files plus an index**:

* **Opened once.** The engine opens the packs at boot and keeps the handles. Every load is one whole-entry read on an open handle (`sceIoPread` on the Vita, `pread` on the hosts). On the benchmark corpus that is 19–25% faster than loose files. Reads stay whole-entry: each request costs 3–4 ms, so many small reads would be slow.
* **The index replaces disc offsets and paths.** It maps today's DVD entry numbers and paths to (pack, offset, stored size, size, compression). That is the "disc offsets become lookup keys" decision ([compiler-fixes.md](compiler-fixes.md), item 19), made concrete.
* **Entries are `.nfa` archives and loose files,** 32-byte aligned in the pack.
* **LZ4HC compression per entry.** Measured on the Vita: ratio 1.70, decompression 62 MiB/s on one core, about 24% faster loads than uncompressed. zstd compresses better (1.96) but decompresses at only 24 MiB/s, which nearly cancels the smaller reads. The bake compresses at level 12 (the same speed to decompress as level 9; the measured ratios were equal). Entries that don't shrink (audio, and any entry whose compressed size isn't meaningfully smaller) are stored uncompressed. The engine keeps the compressed bytes as read and decompresses once into the destination heap; see the cache tier in [memory-design.md](memory-design.md#resident-set-and-cache-tier).
* **On the hosts** the same packs work. A loose-file mode for development (the baked tree as plain files) can stay as an option.
* **Saves are unaffected.** They're rare, and the game already makes the player wait for a save to finish.

## Format headers: the single source of truth

Every baked layout is a C++ struct in `include/nightfall/formats/<format>.h`, following these rules:

* **Self-contained:** includes only `<cstdint>` (and other format headers), never `global.h` or engine headers, so `bindgen` can read it in isolation.
* **Plain data:** standard-layout, trivially copyable, no virtual functions, no pointers, no `size_t`/`uintptr_t`/`long`. Padding is written out as explicit fields.
* **Checked:** every struct has `static_assert(sizeof(T) == N)` and the assert holds on 32- and 64-bit, because nothing in it is pointer-sized. `bindgen` adds matching size and offset tests on the Rust side.
* **Offsets:** `u32`, relative to the start of the baked file. Arrays are `{u32 offset; u32 count}`; strings are offsets to NUL-terminated text.
* **Little-endian**, fixed by the format. The tool has a compile-time check that it runs on a little-endian host, so it can write the structs as they are.
* **Legacy classes adopt them:** engine classes that read file data today (`cBgD_t`, the stage chunk headers, J3D block headers, `ResTIMG`, …) become these structs or derive from them, with the pointer fields replaced by offsets. Reading goes through small checked helpers (e.g. `nf::formats::at<T>(base, offset)`, returning spans, with bounds checks in debug builds) instead of reinterpret casts scattered through the code.

On the Rust side, a `nf-formats-sys` crate runs `bindgen` in its `build.rs` over these headers. It uses the system libclang: Fedora's `clang-libs`, which `tools/gx_shaders/gxcalls.py` already uses, or Homebrew LLVM on macOS. A thin safe layer wraps the generated structs (newtypes, enums for tags).

## Tool layout (`tools/bake/`)

| Crate | Contents |
|---|---|
| `nf-formats-sys` | Generated bindings to `include/nightfall/formats/*.h`, plus the generated layout tests. |
| `nf-formats` | Safe wrappers and writers for the baked formats (container writer, offset/array builders, string tables). |
| `nf-disc` | Readers for the disc formats (`binrw`, big-endian): RARC, Yaz0, J3D, stage chunks, … Only the tool knows these formats. |
| `nf-bake` | The CLI: `nf-bake --disc orig/GZ2E01/GZ2E01.iso --out build/baked/GZ2E01 [--only <glob>] [--report <file>]`. |

Conventions: `cargo fmt` and `clippy` clean; no `unsafe` outside `nf-formats-sys` and the byte-level writer; errors are typed (`thiserror`), and a file that fails to convert is reported and left raw, not a panic.

## Engine side

* **Container reader** behind `JKRArchive`. The four mount classes (Mem/Dvd/Aram/Comp) keep their roles and memory behaviour, but read `.nfa` instead of RARC. The ownership rules from the RARC design apply here: `ResourceSlot`, built on `nf::Box`/`nf::BoxArray` from [memory-design.md](memory-design.md), which replace the earlier `HeapBox`/`HeapArray` names. Nothing allocates through the implicit current-heap `operator new`.
* **DVD layer:** `DVDConvertPathToEntrynum` and friends resolve paths against the baked tree through a lookup table. Entry numbers become indices into that table, which matches the earlier decision that disc offsets become lookup keys. The SDL3 platform layer provides the file I/O.
* **Per format:** each loader switches to the baked struct when its format is converted. Formats not converted yet fail loudly when fetched in a debug build (raw big-endian tag).

## Order of work

The bake and the engine move one format at a time. Each step ends with the engine loading that format from a real bake.

| Step | Bake | Engine | Done when |
|---|---|---|---|
| **0. Pipeline** | Workspace, `bindgen` pipeline, `nod` input, RARC + Yaz0 + nested unpacking, `.nfa` writer, every file packed as raw, bake report. | `.nfa` reader behind `JKRArchive` (with `ResourceSlot`), baked-tree DVD lookup. | A GoogleTest mounts a baked archive and fetches members by index, ID, name and path, and `removeResource`/`detachResource` behave as designed. |
| **1. Textures** | `.bti` and the `ResTIMG` header (`u32` offsets) with lossless conversion. | `JUTTexture`/`ResTIMG` read the baked header. | Texel-exact check: GX reference decoder vs. decoding the baked texture, for every `.bti` on the disc. |
| **2. Boot path 2D** | `.bfn`, `.blo`, `.bmg`, J2D animations, texture tables inside them. | `JUTResFont`, `J2DScreen`, `JMessage`. | Logo and title screen resources load (rendering comes with the GX layer). |
| **3. Stage and collision** | `.dzs`/`.dzr` chunks, `.dzb`, `.kcl`, `.plc`, map paths, BG parts, event `.dat`. | Stage chunk handlers, `cBgS`/`dBgW`, `dBgWKCol`, `dMpath_c`, `d_event_data` on offsets instead of relocated pointers (fixes pattern B and `StaffWork`). | A full stage (`F_SP103`, Ordon Village) loads without asserts. |
| **4. J3D** | `.bmd`/`.bdl` blocks, all animation types, `.bls`. Display lists (`SHP1`) and vertex arrays converted to little-endian. Whether to decode them into native buffers instead depends on the GX layer design. | `J3DModelLoader` and friends on `u32` offsets (fixes pattern A). | Every model and animation on the disc loads; joint/material counts match a disc-side parse. |
| **5. The rest** | `.jpc`, `.stb`, the unidentified formats (`.csw`, `.bct`, …). | `JPA*`, `JStudio`. | The bake report shows no raw files outside audio. |
| **6. Audio** | `.baa`, `.bms`, `.aw`, `.ast` (headers little-endian; stream samples stay DSP-ADPCM in the MVP, Opus later, see *Audio streams* above). Whether the mixer's wave samples are decoded to PCM in the bake is decided together with the software mixer. | JAudio2 parsers. | Part of the audio phase in [roadmap.md](roadmap.md). |

## Testing

* **Layouts:** the C++ `static_assert`s on every build, and the `bindgen` layout tests in `cargo test`.
* **Converters:** unit tests with small synthetic inputs (no disc needed). Disc tests are gated behind an environment variable (`NF_DISC=orig/GZ2E01/GZ2E01.iso`). They parse each disc file, bake it, read the baked file back, and compare the decoded values. For textures that is texel for texel; for structured formats, field for field.
* **Determinism:** baking the same disc twice must give identical output.
* **Engine:** GoogleTest cases, gated on a bake in `build/baked/`, load baked files through the real loaders.
* **Coverage:** the bake report counts converted vs. raw files per format. It is the progress measure for steps 1–5.

## Open questions

1. **Pack layer details** (see *Pack files and the index*): how many packs, and the threshold below which an entry stays uncompressed. Also whether a Vita bake swizzles textures (`SCE_GXM_TEXTURE_SWIZZLED`). These are per-target options on top of the one format; the layout rule stays.
2. **J3D display lists:** keep GX command streams (little-endian) for a GX-compatible layer, or decode them into vertex and index buffers in the bake. This is decided with the GX layer.
3. **Audio:** streams stay DSP-ADPCM in the MVP, with Opus planned after a Vita benchmark (see *Audio streams*). Still open: whether the stream is kept resident (loop cache), and whether wave-bank samples become PCM. Both are decided with the software mixer.
4. **Memory model:** the container's load policy (preload to main RAM/ARAM/DVD) and the `ResourceSlot` heaps depend on the planned memory-model survey and design.
5. **Unidentified formats** (`.csw`, `.bct`, the `.dat`/`.bin` tables): identify their loaders before step 5.
