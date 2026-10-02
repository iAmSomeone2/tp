# Graphics: from GX to the Vita GPU

Graphics is the largest single piece of the port. The original renders through **GX**, the GameCube/Wii GPU API, whose model (a fixed-function pipeline with programmable-*looking* "TEV" texture-combiner stages) is very different from the Vita's shader-based PowerVR SGX543 driven through **GXM**. This page inventories what the game actually uses, and lays out how to build a back end. Numbers are from [survey-data.md](survey-data.md).

## Who talks to the GPU

```
game code (42 files under src/)  ─┐  raw GX calls: rain, draw lists, fades, maps, cloth, movie, mirror …
                                  │
J2D (2D UI)     ─ 799 GX calls    ├──►  GX API  (184 distinct functions, ~4,640 calls) + GD display-list builders (12, 163 calls)
J3D (3D models) ─ ~310 GX + 148 GD│              │
JPA (particles) ─ ~150 GX         │              ▼
JUtility/JFramework (fonts, fader, XFB copy) ─┘   GX hardware registers / GP command FIFO
```

* **Outside JSystem only 42 files call GX directly.** The biggest are `d_kankyo_rain.cpp` (680 calls: rain/snow streaks in immediate mode), `d_drawlist.cpp` (467: draw-list execution, shadow volumes, screen filters), `m_Do_graphic.cpp` (425: frame setup, bloom, depth-of-field), `m_Do_ext.cpp` (342: debug shapes, 3D lines, packets), `d_particle.cpp` (191), `d_map_path*.cpp`, `d_a_movie_player.cpp` (THP frames), `d_ovlp_fade2/3.cpp` (transition wipes), `d_a_mirror.cpp`, cloth actors (`d_a_mant`, `d_a_obj_flag2/3`), `d_gameover`, `d_menu_window`, `d_error_msg`, and the grass/flower packets.
* Everything else (about 800 actors and most UI) reaches the GPU **only through J3D, J2D and JPA**, so those three libraries are the choke points.

## How J3D drives the hardware (important)

Inspecting `libs/JSystem/src/J3DGraphBase/` shows J3D is **not** a thin client of the GX API:

* **Materials** (`J3DMatBlock.cpp`, `J3DGD.cpp`): J3D builds per-material display lists by calling `GD*` writer functions and `J3DGDWriteBPCmd(...)` / `J3DGDWriteXFCmd(...)`, i.e. it writes **raw BP (pixel/TEV/texture) and XF (transform/lighting) register commands** into memory (`J3DGDSetGenMode`, `J3DGDSetLightPos`, `BP_GEN_MODE(...)`, …). At draw time it calls `GXCallDisplayList` on those lists.
* **Shapes** (`J3DShape*.cpp`): each shape's geometry is a display list *stored in the model file* (`GXCallDisplayList(mDisplayList, size)`): primitive opcodes, vertex counts and per-vertex indices/data laid out according to the current vertex descriptor (VCD) and format (VAT). A companion "VCD/VAT" list (`mVcdVatCmd`) sets those up via CP register writes.
* **Matrices**: vertices carry a position-matrix index attribute; J3D loads up to 10 matrices per group with `GXLoadPosMtxImm` and selects with `GXSetCurrentMtx`.

Consequence: **an API-level shim over GX calls is not enough**. To keep J3D unchanged you must also *interpret GX command streams* (BP/XF/CP register writes and vertex-primitive commands). Alternatively, replace J3D's material and shape back ends with structured data consumers (see options below). J2D and JPA, by contrast, use the API only.

## GX feature inventory (what a shader layer must reproduce)

| Feature group | Calls | Notes |
|---|---:|---|
| TEV / texgen setup (`GXSetTevOrder/ColorIn/ColorOp/AlphaIn/AlphaOp/…`, `GXSetNumTevStages`, `GXSetTexCoordGen`, `GXSetNumTexGens`) | 1,145 | The heart of GX shading. J3D blocks support 1, 2, 4 or 16 TEV stages (`J3DTevBlock1/2/4/16`). Texgen kinds seen in the code: `GX_TG_MTX2` (82), `TEX0…7` chaining, `MTX3`, `POS`. |
| Immediate-mode vertices (`GXBegin/GXEnd`, `GXPosition*`, `GXColor*`, `GXTexCoord*`) | 1,032 | Used by UI, particles, rain, wipes, debug shapes. Batch into dynamic vertex buffers. |
| Vertex formats/arrays (`GXSetVtxDesc`, `GXSetVtxAttrFmt`, `GXSetArray`, `GXClearVtxDesc`) | 603 | Direct vs. indexed (8/16-bit) vertex data; fixed-point position/texcoord formats. |
| Z/alpha/blend/cull/dither/clip (`GXSetZMode`, `GXSetBlendMode`, `GXSetAlphaCompare`, `GXSetCullMode`, …) | 461 | Blend modes seen: `GX_BM_BLEND` (71), `NONE` (29), `SUBTRACT` (4), `LOGIC` (4). Alpha compare is an **alpha test**: do it with `discard` in the fragment shader. |
| Matrices/projection/viewport/scissor | 305 | Mind GX's clip-space depth convention when building projection matrices. |
| Texture objects and palettes (`GXInitTexObj`, `GXLoadTexObj`, `GXInitTlutObj`, `GXLoadTlut`) | 147 | See [Textures](#textures). |
| Lighting (`GXSetChanCtrl`, `GXInitLight*`, `GXLoadLightObj`, `GXSetChanMatColor`) | 194 | Per-channel hardware lighting (up to 8 lights per channel): implement in the vertex shader. |
| Indirect texturing (`GXSetIndTex*`, `GXSetTevIndirect`, `GXSetNumIndStages`) | 96 | Warps/refraction (water, heat shimmer, warp effects). Needs a dedicated shader path. |
| Fog (`GXSetFog`) | 43 | Per-fragment fog with range adjustment tables; implement in the shader. |
| EFB copies / render-to-texture (`GXCopyTex`, `GXSetTexCopySrc/Dst`, `GXCopyDisp`) | 59 | See [Render targets](#render-targets-and-efb-copies). |
| Display lists (`GXCallDisplayList`, `GXBegin/EndDisplayList`) | 57 | Includes every J3D draw. |

Other observations:

* Only **184 distinct GX functions** appear; the API surface is bounded.
* Depth readback is rare: one `GXPeekZ` (in `dComIfGd_peekZdata`, `d_drawlist.cpp`) and one `GX_TF_Z16` copy (`m_Do_graphic.cpp`, used for depth-of-field).
* Texture-format usage in code (not counting data files): `RGBA8` (10), `C8` (5), `Z24X8` (3), `RGB5A3`, `IA8`, `I8` (3 each), `Z8`, `Z16`, `RGB565`, `I4` (1 each); the data files use the full set.

## Three ways to build the back end

| Option | What it is | Pros | Cons |
|---|---|---|---|
| **A. GX emulation layer** | Implement the GX API *and* a command-stream interpreter (BP/XF/CP + primitive opcodes) on top of GXM. J3D/J2D/JPA/game code run unchanged. | Fastest route to a running game; preserves all behaviour; small game-code diff. | Interpreter must be exact for what J3D emits; risk of poor performance if display lists are re-decoded every frame; TEV → shader translation needed anyway. |
| **B. Native J3D/J2D/JPA back ends** | Rewrite `J3DShape`/`J3DMaterial`/`J3DPacket` to consume the *structured* data they already hold (`J3DTevBlock`, `J3DTexGenBlock`, …) and pre-decoded vertex buffers; do the same for J2D and JPA. | Best performance; shaders/buffers built once; no command interpreter. | More invasive; must replicate every J3D behaviour by hand; the 42 direct-GX files still need something. |
| **C. Hybrid (recommended)** | Do **A** for the API layer (needed by the direct-GX files, J2D, JPA), and treat J3D specially: **decode model display lists once at load** into native vertex/index buffers and drive materials from their structured blocks (skipping the BP/XF display lists). | Gets J3D (the bulk of pixels) fast without a general command interpreter; A stays small (no BP/XF decoding except what you choose). | Two paths to keep consistent; requires touching `J3DShape*`/`J3DMaterial*`/`J3DPacket*` (about 2,000 lines of `J3DGraphBase`). |

Suggested order: build the API layer first (option A without the interpreter), get J2D UI drawing, then implement the J3D path per option C. Add an interpreter later only if a case forces it.

### Vertex data

* Models use indexed attributes: the display list holds 8/16-bit indices into position/normal/colour/texcoord arrays that are separate in memory. GXM draws one index per vertex, so **de-index while decoding** (gather into interleaved vertex buffers, one per shape or per model).
* Convert fixed-point formats (`GX_S16` with fractional bits) to floats or normalised shorts at decode time.
* Skinning uses **per-vertex matrix indices (`GX_VA_PNMTXIDX`) into a palette** of position matrices (GX has ten slots) per draw group: pass the index as a vertex attribute and the matrices as a uniform array (or pre-skin on the CPU for a first version; `J3DMtxBuffer`/`J3DSkinDeform` already do CPU work for some cases).
* Primitives: GX has `TRIANGLES`, `QUADS`, `TRIANGLESTRIP`, `TRIANGLEFAN`, `LINES`, `LINESTRIP`, `POINTS`. Convert quads and fans to triangle lists.

### Shaders

The set of material configurations is **finite and enumerable**: it is the union of the TEV/texgen/indirect/fog/lighting settings found in every model's material block plus those set directly by the ~40 game files. Choices:

1. **Generate at run time** from the material state and compile with the Cg runtime compiler. Simple, but the Vita's runtime compiler is a separately-installed system module on retail firmware, which complicates distribution and startup time.
2. **Enumerate offline and precompile.** Scan all models/UI/effects once (with the user's data), hash each distinct configuration, generate the shader source, compile with the SDK's offline compiler (`psp2cgc`), and ship the compiled programs keyed by configuration hash (they contain no asset data). Anything not covered falls back to a generic shader.
3. **Ubershader**: one program interpreting a TEV description from uniforms. Easiest to get working; likely slower on this GPU, but valuable as the always-correct fallback and for bring-up.

Recommendation: start with 3 (correctness), add 2 once the configuration set is known.

## Textures

* GX stores textures in **tiled layouts** (4×4, 8×4, 8×8 blocks depending on format) with big-endian 16-bit texels. Texture formats in data: `I4`, `I8`, `IA4`, `IA8`, `RGB565`, `RGB5A3`, `RGBA8`, `C4`, `C8`, `C14X2` (palette-indexed with a separate palette), `CMPR`. The header is `ResTIMG` (0x20 bytes).
* Convert on upload to formats the Vita supports: 8-bit luminance/alpha and 16-bit variants map naturally; `RGB5A3` and `RGBA8` to 16/32-bit RGBA; palette textures either expanded to RGBA8 or kept indexed with a palette lookup in the shader; **`CMPR` needs conversion to standard BC1/DXT1** (colour word byte order and 2-bit index ordering differ) or to RGBA8.
* Mipmaps, LOD bias (`GXInitTexObjLOD`, 32 uses), wrap modes and filter modes map to sampler state.
* Texture animations (`btk`, `btp` swaps) are data-driven and need no special handling once the material path works.
* This is also a good place for *cooking*: pre-convert every texture at install time ([endianness.md](endianness.md)).

## Render targets and EFB copies

The GameCube renders into a 640×528 embedded framebuffer (EFB) and copies regions into textures with `GXCopyTex`. The game uses this for:

* real-time **shadow textures** (`dComIfGd_imageDrawShadow`, `d_drawlist.cpp`), map drawing (`d_map_path.cpp`), screen **snapshots** used by the game-over, menu-window and error screens (`d_gameover`, `d_menu_window`, `d_error_msg`), wipes (`d_ovlp_fade3`), **bloom**, blur and **depth-of-field** (`m_Do_graphic`, `mDoGph_gInf_c::bloom_c`), plus the final EFB→XFB copy in `JFWDisplay`/`JUTVideo`. (`d_a_mirror` draws through raw GX calls but does not appear among the `GXCopyTex` users; check how it gets its reflection texture before planning.)

On the Vita (a tile-based deferred renderer) each of these becomes "render to an offscreen target, then sample it". Points to plan for:

* Choose offscreen target sizes to match what the original copies (many are half-resolution).
* **Depth as texture**: `GX_TF_Z16` copies (depth of field) need an explicit depth-writing or depth-linearising pass on a GPU that can't sample the depth buffer directly (verify for GXM); `GXPeekZ` (`dComIfGd_peekZdata`) reads single depth values for gameplay effects: replace with CPU ray tests against collision (`dBgS_LinChk`) or an occlusion query, since reading back a tile-based GPU's depth is expensive.
* Preserve **draw order**: the frame is built in fixed passes (sky → BG → opaque → translucent → particles → UI; see [../03-engine-architecture.md](../03-engine-architecture.md#39-rendering-pipeline)), which maps well to render passes.
* The Vita's memory bandwidth and tile memory reward fewer, larger passes; consider fusing the small post-processing passes.

## Screen size and aspect ratio

* Original render size is 608×448 on GameCube (`FB_WIDTH_BASE/FB_HEIGHT_BASE`) and 640×456 when `WIDESCREEN_SUPPORT` is on (`m_Do_graphic.h`), both nominally 4:3 layouts. The Vita screen is 960×544.
* The Wii and Shield builds already contain a **widescreen mode** (`WIDESCREEN_SUPPORT`, 52 references, in `m_Do_graphic`, `d_camera`, `d_kankyo*`, HUD/message code, some water and mirror actors). If you want native widescreen, porting those code paths from a Wii base is likely cheaper than inventing your own. Pillar-boxed 4:3 is the least work for a first milestone.
* UI is authored for 4:3 (`.blo` layouts); a 16:9 HUD will need anchoring changes (the Wii versions show how).

## Direct-GX files: grouping for the port

| Group | Files | Suggested handling |
|---|---|---|
| Frame/state plumbing | `m_Do_graphic`, `m_Do_ext`, `JFWDisplay`, `JUTVideo`, `d_drawlist` | Re-implement on the new back end; these define passes, targets and the draw-list executor. |
| Screen effects | `d_ovlp_fade*`, `d_gameover`, `d_error_msg`, `d_menu_window`, `d_a_mirror` | Port after render targets work; simple quads/copies. |
| World effects using raw primitives | `d_kankyo_rain`, `d_particle`, `d_a_alink_effect.inc`, `d_grass.inc`, `d_flower.inc`, `d_a_mant`, `d_a_obj_flag2/3`, `d_a_obj_*chain` | Immediate-mode calls; batching in the GX layer is usually enough. Candidates for later native rewrites (rain especially, 680 calls). |
| Maps | `d_map`, `d_map_path*`, `d_menu_fmap_map`, `d_menu_dmap_map` | 2D primitives with palette/texture use. |
| Video | `d_a_movie_player` | Replace the THP path with a video decoder that produces textures. |
| Wii only | `d_home_button` | Skip on a GameCube base. |

## Suggested milestones

1. Clear colour + present, frame pacing at 30 Hz.
2. `GXBegin` immediate mode + fixed colour shader; draw a coloured quad (UI fader).
3. Textured 2D (J2D): `.blo` loading (with endianness fix-ups), texture conversion, TEV subset for UI.
4. Static J3D models with correct materials (title screen).
5. Skinned models, lighting, fog; translucency ordering.
6. Particles, shadows, mirror, bloom, DoF.
7. Indirect texturing (water), remaining effects, then performance work (batching, precompiled shaders).
