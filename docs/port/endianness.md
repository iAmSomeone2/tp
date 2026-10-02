# Endianness and binary data formats

The GameCube/Wii are **big-endian**; the Vita's ARM Cortex-A9 runs **little-endian**. Every data file the game reads was authored for the former, and the source has **no byte-swap infrastructure at all** (`grep` finds no `bswap`/`__lwbrx`/`ntohl` outside the HostIO debug tooling): parsers simply cast a pointer to a struct and read fields in native order.

The good news, established by scanning the code ([survey-data.md](survey-data.md)):

* Game logic almost never touches raw file bytes. Of ~2,420 resource-fetch calls (`dComIfG_getObjectRes`, `dComIfG_getStageRes`, `JKRGetResource`, …), ~86% return **J3D models and animations**, which are parsed by a handful of loaders in `J3DGraphLoader`. Another ~7% are collision meshes (`cBgD_t`, 110 calls in 90 files, all consumed through `dBgW`) and textures (`ResTIMG`, 65). Only ~50 hand raw structs to game code (`dKy_*` tables, map parameters, bank/name tables, a few `u8` blobs).
* **All in-memory game state is endian-neutral C++** (fields are read and written as native values), including the save struct.
* Structs that embed *offsets* or pointer-sized integers (e.g. `ResTIMG::imageOffset` is a `uintptr_t`) keep their layout on 32-bit ARM. That would not be true on a 64-bit target.

The work is therefore: **make each format parser produce correct native-endian data**, and re-encode anything the *GPU or audio hardware* would have consumed in a big-endian layout.

## Inventory: where big-endian data is parsed

Line counts are the parser files' total size (the reading effort). Formats and the code that owns them:

| Format | Parser files | Lines | Notes |
|---|---|---:|---|
| RARC archives `.arc` (+ Yaz0/Yay0) | `JKRArchivePri`, `JKRMemArchive`, `JKRDvdArchive`, `JKRAramArchive`, `JKRCompArchive`, `JKRDecomp` (`libs/JSystem/src/JKernel`) | 1,868 | Every asset is inside one. Header and directory/file-entry tables (`SArcHeader`, `SDIFileEntry`, `SDIDirEntry`) are big-endian; file contents are untouched here. Yaz0/Yay0 stream headers carry BE sizes; the compressed bytes themselves are byte-oriented. |
| J3D models `.bmd/.bdl` | `J3DModelLoader`, `J3DMaterialFactory(_v21)`, `J3DShapeFactory`, `J3DJointFactory`, `J3DModelLoaderCalcSize` | 2,298 | Chunked format (magic `'J3D2'`, `'bmd3'`, then blocks with BE type/size). Contains GX display lists and vertex arrays; see below. |
| J3D animations `.bck/.btk/.brk/.btp/.bpk/.blk` | `J3DAnmLoader`, `J3DClusterLoader` | 792 | Tables of `s16`/`f32` keyframes, all BE. |
| J2D layouts `.blo`, J2D animations, fonts, textures | `J2DScreen`, `J2DAnmLoader`, `J2DMaterialFactory`, `JUTResFont`, `JUTTexture`, `JUTNameTab` | 2,093 | `.blo` is read through `JSUInputStream` (a central choke point; see below). Names (`'n_all'`) are 8-char constants compared as `u64`. |
| Stage/room data `.dzs/.dzr` | `d_stage.cpp` | 2,908 | ~45 chunk handlers (`dStage_*Init`); each walks a tagged table and stores raw pointers to BE structs (`stage_scls_info_class`, `stage_actor_data_class`, …) that are read by getters in many files. **Fix up chunk-by-chunk right after load.** |
| Collision `.dzb` | `dBgW` (`d_bg_w.cpp`), `d_bg_w_kcol`, `d_bg_w_base` | 5,005 | `cBgD_t` header with counts/offsets → vertex/triangle/group tables; KCol is a separate grid format. |
| Event scripts | `d_event_data.cpp`, `d_event_manager.cpp` | 3,111 | `event_binary_data_header` → events/staff/cuts/data. |
| Messages `.bmg` and message flow | `JMessage/resource`, `processor`, `d_msg_flow`, `d_msg_object` | 6,030 | BMG (sections `INF1`, `DAT1`, `STR1`, `MID1`, `FLW1`, `FLI1`) plus the flow-node table. |
| Particles `.jpc` | `JPAResourceLoader`, `JPAResourceManager` | 211 | Small, but the resource blocks (shapes, fields, keys) are read by many `JPA*` classes. |
| Cutscene timelines `.stb` | `JStudio/stb*`, `fvb*`, `ctb*` | 571+ | `JGadget::binary` helpers for variable-length fields. |
| Audio banks/waves/sequences | `JASBNKParser`, `JASWSParser`, `JASSeqParser`, `JASSeqReader`, `JAUAudioArcInterpreter`, `JASAramStream` | 2,409 | `Z2Sound.baa`, wave banks (`.aw`), sequence bytecode. `JASSeqReader::read16/read24` read through raw pointers (`read24` masks a 32-bit load at `cur-1`: correct only on big-endian). |
| Raw tables | `d_item.cpp` (`/res/ItemTable/*.bin`), `d_s_logo.cpp` (`/res/Menu/Menu1.dat`), `d_kankyo*` palette structs | – | Loaded with `mDoDvdThd_toMainRam_c`. |
| Save data / memory card | `d_save.cpp`, `m_Do_MemCard*` | 3,474 | Only matters if you want compatibility with original saves. |
| THP movies | `d_a_movie_player.cpp` | 4,224 | Video container plus a PPC-asm-heavy decoder; replace rather than port. |

## Three strategies

| Strategy | Idea | Pros | Cons |
|---|---|---|---|
| **A. Convert on load** | After a file is read into memory, run a per-format *fix-up pass* that byte-swaps every multi-byte field in place, then let the existing code run unchanged. | Small, local changes; game logic and getters untouched; works with existing archives. | Runtime cost (small); every format needs a complete, correct field map; must not swap twice. |
| **B. Cook at install time** | A tool converts the whole disc's data once into a little-endian "cooked" tree (and can also re-encode textures, decode display lists, decompress archives). | Zero runtime cost; can pre-build GPU-ready buffers; can drop Yaz0. | The tool must understand *every* format and every embedded array up front; harder to debug; bigger install. |
| **C. Swap on read** | Wrap every field read in an accessor (`be32(p->x)`). | Format stays big-endian. | Touches thousands of sites and is easy to miss; slower; no benefit. **Not recommended.** |

**Recommendation: start with A, one format at a time, and promote stable formats to B later.** A gives you a running game earliest; B is an optimisation and an asset-quality step (textures/models). Both share the same *field maps*, so work done for A is reusable in the cooker.

### Building blocks

1. A tiny header (`platform/endian.h`) with `be16/be32/be64/bef32` helpers and a `PLATFORM_LITTLE_ENDIAN` switch (compiled out on big-endian so upstream still builds).
2. Read parsers through **`memcpy`-based loads** rather than pointer casts. ARMv7 tolerates unaligned integer loads but **not** unaligned `f32`/`VLDR` accesses, and the current parsers freely cast `u8*` → `f32*`/`u32*`.
3. A convention: each parsed struct gets a `void Fixup(T*)` (or `SwapInPlace`) next to its definition, called exactly once by the loader, plus a debug-only "already swapped" marker to catch double-swaps.
4. Existing choke points to fix first (small changes, wide effect):
   * `JSUInputStream::readU16/U32/S16/S32/read16b/read32b/operator>>` (`libs/JSystem/include/JSystem/JSupport/JSUInputStream.h`): all `.blo` parsing (J2DScreen, J2DPane, J2DPicture, J2DTextBox, J2DWindow, J2DManage) reads integers this way.
   * `JASSeqReader::get16/get32/read16/read24` for sequence bytecode.
   * The `JKR*Archive` header/entry reads.
   * `J3DModelLoader::load` and per-block loaders (`readMaterial`, `readShape`, …).
   * `dStage_dt_c_stageLoader` and the chunk `FuncTable` handlers in `d_stage.cpp`.
   * `dBgW::Set` for `.dzb`.

## Format-specific notes

### Textures (`ResTIMG`, GX texture formats)

The 0x20-byte header is BE (`width`, `height`, `numColors`, offsets). The pixel data is stored in **GX's tiled layouts** (e.g. 4×4/8×4/8×8 tiles), with formats `I4`, `I8`, `IA4`, `IA8`, `RGB565`, `RGB5A3`, `RGBA8`, `C4/C8/C14X2` (palettised) and `CMPR` (a DXT1-like block format). The 16-bit formats need a byte swap; **all of them need re-tiling or a shader-side de-tile**, and `CMPR` needs per-block conversion (byte order of the two colours and the order of the 2-bit indices differ from standard BC1). Do this in the texture upload path (`JUTTexture`, `J3DTexture`, `mDoLib_setResTimgObj`), not in the generic archive code, because the same bytes may be shared by GX API calls. See [graphics.md](graphics.md#textures).

### Models (`.bmd/.bdl`)

Blocks (`INF1`, `VTX1`, `EVP1`, `DRW1`, `JNT1`, `SHP1`, `MAT3` (or the older `MAT2`), `TEX1`, `MDL3`) each carry BE counts and offsets; offset fields are converted to pointers in place at load (the block structs are declared with `/* 0x.. */` offsets, and pointer fields are pointer-sized on both platforms). Two payloads need special handling:

* **Vertex arrays** (`VTX1`): typed arrays (`f32`, `s16` fixed-point, `u8` colours). Swap according to the array's component type (`GXCompType`) as recorded in the vertex-attribute table.
* **Shape display lists** (`SHP1`): raw **GX command streams** (primitive opcode, BE vertex count, then per-vertex data whose element widths depend on the VCD/VAT). These are consumed by the GPU. Either (a) keep them in BE and interpret them with a GX command decoder that reads BE ([graphics.md](graphics.md)), or (b) decode once at load into native vertex/index buffers (preferred for performance).
* **Materials** (`MAT3`) are structured data (tev stages, colours, indices): swap fields; J3D turns them into hardware register lists at run time through `J3DGD*` (see graphics).

### Stage data (`.dzs/.dzr`) and `cBgD_t`

Tagged chunk tables (`dStage_fileHeader` = chunk count, then `dStage_nodeHeader { u32 tag; int entryNum; u32 offset; }` entries): swap the table, then each chunk according to its tag. Since code accesses these structures through many different getters, converting **at chunk-load time** (inside the `dStage_*Init` handlers) is the safest place. Collision (`cBgD_t`) has a header with counts/offsets and typed tables; `dBgW::Set` is the single entry point.

### Messages, events, particles, cutscenes

Straightforward field-by-field swaps; each has a single top-level parser (`JMessage::TResource`, `dEvDtBase_c`, `JPAResourceLoader`, `JStudio::stb::TParse_THeader`/`TParse_TBlock`). The message flow nodes (`FLW1`/`FLI1`) are small tables.

### Audio

Sequence data is bytecode with BE 16/24-bit operands; wave banks carry BE header fields but the sample payload is DSP-ADPCM bytes (no swap) or 16-bit PCM (swap). The DSP microcode blob embedded in `dsptask.cpp` is irrelevant once you replace the DSP.

### Save data

`dSv_save_c` is a memory image copied straight to the card. If you only support saves from your own port, keep native (little-endian) layout and add a version/magic word. If you want to import original saves later, write a converter that walks the `dSv_*` structs (all fields are fixed-width integers/arrays).

## Hazards specific to the little-endian switch

| Hazard | Where | Action |
|---|---|---|
| `u32` reads at odd offsets (`read24`, `get32` on `u8*`) | `JASSeqReader` | Rewrite with byte assembly. |
| Bit-field overlays on binary data | Survey found **62 bit-fields in 7 files**, mostly in JAudio2/Z2 in-memory state (`JAISound.h`, `JASTrack.h`, `Z2SeqMgr.h`), `c_request.h` | No action for in-memory state; audit any struct that is also memory-mapped from a file. |
| `*(u8*)&word`-style value punning | 4 occurrences (`Z2Audience`, `JAUAudibleParam.h`, `JUTException`) | Fix individually. (Plain `(u8*)&x` byte-pointer arithmetic, e.g. in `JKRMemArchive`, is fine.) |
| Multi-char tag constants (`'J3D2'`, `'INF1'`, pane IDs) | ~146 files | Correct once the *file word* has been byte-swapped; compilers give `'ABCD'` the value `0x41424344` regardless of endianness. |
| 64-bit pane IDs assembled from two 32-bit BE reads | J2D | Read the high word first, then the low word. |
| Unaligned `f32`/`u16` access | parsers casting `u8*` | Use `memcpy` loads. |
| `float` ↔ `u32` reinterpretation | some J3D/J2D animation loaders | Swap as `u32`, then `memcpy` to `f32`. |

## Verification ideas

* **Structural asserts in every fix-up**: block sizes sum to the file size, offsets fall inside the buffer, counts are below sane limits. A missed swap almost always trips one.
* **Golden dumps**: write an independent big-endian-aware reader (e.g. in Python using `struct` with `>`) for RARC/J3D/DZB/BMG and compare the C++ post-fix-up structures against it for many files from the disc.
* **Enable the layout `STATIC_ASSERT`s** (788 in the tree) for the Vita build to catch structure-size mistakes; relax the ones on classes containing member pointers ([toolchain.md](toolchain.md#type-sizes-and-layout)).
* **Visual/behavioural checks** on a few well-understood assets (Ordon Village, Link's model, the title screen) before bulk conversion.
