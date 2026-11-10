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
| `.thp` movie | 1 | 131 MiB | No: the movie player is replaced |
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

* **No compression** in the MVP. Members are stored decompressed and 32-byte aligned, so a mounted archive is directly usable (440 MiB+ of archives on the dev PC is fine). Compression (e.g. LZ4/zstd, for Vita storage) is a later, container-level option.
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

* **Container reader** behind `JKRArchive`. The four mount classes (Mem/Dvd/Aram/Comp) keep their roles and memory behaviour, but read `.nfa` instead of RARC. The ownership rules from the RARC design (`ResourceSlot`, `HeapBox`/`HeapArray` with explicit heaps) apply here. Nothing allocates through the implicit current-heap `operator new`.
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
| **6. Audio** | `.baa`, `.bms`, `.aw`, `.ast`. Whether sample data stays DSP-ADPCM or becomes PCM is decided together with the software mixer. | JAudio2 parsers. | Part of the audio phase in [roadmap.md](roadmap.md). |

## Testing

* **Layouts:** the C++ `static_assert`s on every build, and the `bindgen` layout tests in `cargo test`.
* **Converters:** unit tests with small synthetic inputs (no disc needed). Disc tests are gated behind an environment variable (`NF_DISC=orig/GZ2E01/GZ2E01.iso`). They parse each disc file, bake it, read the baked file back, and compare the decoded values. For textures that is texel for texel; for structured formats, field for field.
* **Determinism:** baking the same disc twice must give identical output.
* **Engine:** GoogleTest cases, gated on a bake in `build/baked/`, load baked files through the real loaders.
* **Coverage:** the bake report counts converted vs. raw files per format. It is the progress measure for steps 1–5.

## Open questions

1. **Compression** for Vita storage (LZ4/zstd per member or per archive), and whether a Vita bake also swizzles textures (`SCE_GXM_TEXTURE_SWIZZLED`). These would be per-target options on top of the one format; the layout rule stays.
2. **J3D display lists:** keep GX command streams (little-endian) for a GX-compatible layer, or decode them into vertex and index buffers in the bake. This is decided with the GX layer.
3. **Audio sample format** (DSP-ADPCM vs. PCM), decided with the software mixer.
4. **Memory model:** the container's load policy (preload to main RAM/ARAM/DVD) and the `ResourceSlot` heaps depend on the planned memory-model survey and design.
5. **Unidentified formats** (`.csw`, `.bct`, the `.dat`/`.bin` tables): identify their loaders before step 5.
