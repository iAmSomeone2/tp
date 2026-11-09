# Shader analysis: finding every GX shader configuration ahead of time

Status: **first pass built and measured.** The tools are in `tools/gx_shaders/`. Their results are in two generated pages: [shader-survey.md](shader-survey.md) (disc data) and [shader-code-paths.md](shader-code-paths.md) (source code). This page explains the method, what the numbers mean and what is left to do. It refines option 2 under "Shaders" in [graphics.md](graphics.md#shaders): enumerate configurations offline and precompile them, with the ubershader kept as the fallback.

## Results in brief

* **About 1,000 linked programs cover all of the disc's materials:** 725 distinct fragment programs and 285 vertex programs, in 986 distinct (vertex, fragment) pairs, at the recommended level ([§5](#5-measurements)). The materials are 13,369 J3D, 6,737 J2D and 7,666 JParticle, plus 7,913 variants created by code rewrites. An independent numeric check finds no wrongly merged programs ([§2](#2-canonicalisation)). The J2D figure is a lower bound, because J2D panes rebuild their TEV stages at run time ([§4](#4-load-time-and-run-time-mutations)).
* **The usage curve is steep:** 4 fragment programs cover 50 % of materials, 16 cover 80 % and 49 cover 90 %. 295 programs are used by exactly one material.
* **Code-defined draws add few programs.** The 53 resolvable draw sites whose GX state is set by code (not data) produce 35 programs, 8 of which the data already produces. Another 91 code-driven sites need a hand audit or loop unrolling ([§6](#6-code-driven-configurations)).
* **One runtime dimension dominates vertex programs.** `light_mask_type`, set per stage/room, changes the number of active lights. Keyed by light count, vertex programs grow from 426 to 1,552. With a fixed 8-light loop where masked lights are zeroed, there are 308.
* **A few patterns account for most pixels** ([§7](#7-shared-patterns-and-how-to-use-them)):
  * TP's standard world-lighting term `sat((C + mix(K, T, V)) * 4)` appears in 83 programs and 8,896 materials.
  * The J2D/JParticle colour remap `mix(C0, C1, T)` appears in 65 programs and 17,879 materials.
  * Colour and alpha are almost independent: 334 colour × 207 alpha expressions make up the 725 programs.

## Why this is tractable here but not in Dolphin

Dolphin has to run any GameCube/Wii program, so it can't know ahead of time which TEV/texgen/lighting combinations will show up. Its answer is ubershaders, plus asynchronous specialisation. This project runs one engine, with a fixed set of code paths and a fixed set of data files, so the set of configurations is finite and knowable.

**Most shader state comes from data files, not code.** Code mostly copies data into GX state:

* **J3D** (nearly every 3D pixel) loads materials from the `MAT3` section of `.bmd`/`.bdl` files and emits them as BP/XF display lists.
* **JParticle** builds its TEV setup from bit fields in the `.jpc` resources. `JPABaseShape::setGX` indexes static tables (`st_ca`, `st_aa`, …) with `pBsd->mFlags`.
* **J2D** materials come from `MAT1` in `.blo` files.

So the analysis is **"enumerate the data, then apply a small, hand-modelled set of code transforms"** ([§3](#3-data-enumeration), [§4](#4-load-time-and-run-time-mutations)). A static pass over the C++ covers the remainder ([§6](#6-code-driven-configurations)).

## Pipeline: data → pseudocode → native shader

Derived shaders **start as pseudocode** in a small, backend-neutral language, and are converted to native shaders only in a later, separate step:

```
disc data / source code ──► GX state ──► canonical IR = pseudocode (the key) ──► generator ──► Cg ──► psp2cgc ──► .gxp
   scan.py, gxcalls.py        j3d/blo/jpa      shader.py                          (not built yet)
```

The pseudocode is the canonical form, so two materials share a program exactly when their pseudocode text is equal; its hash is the program's key. This keeps the analysis reviewable (a human can read every program), testable (the canonicaliser is checked on the text), and independent of the Vita toolchain. The generator to Cg is the only GXM-specific part and is the next thing to build.

### The pseudocode language

Fragment programs (`build/gx_shaders/pseudo/fs_<key>.txt`):

| Form | Meaning |
|---|---|
| `tN = sample(mapN, uvN.xy)` / `uvN.xy / uvN.z` | texture sample (projected when the texgen is 3×4) |
| `indirect(uv, sample(...), fmt, bias, mtx, wrap, add_prev, lod, alpha)` | GX indirect texture coordinate (warps, refraction); fields are the raw GX enums |
| `vN`, `vN.a` | rasterised vertex colour channel (output of the vertex program) |
| `cN.rgb`, `cN.a` | a TEV register's value *before* the program writes it: a uniform (TEV colour registers are signed 11-bit, −4…4) |
| `kcN`, `kaN` | per-stage konst colour/alpha uniform; the CPU applies the material's K selector |
| `KN.x`, fractions | konst register / fixed fraction (only when K selectors are not uniforms) |
| `mix(a, b, c)` | `a·(1−c) + b·c` (GX TEV `lerp`) |
| `sat(x)` | clamp to [0, 1] (TEV stage with clamp on) |
| `wrap11(x)` | TEV register overflow (signed 11-bit wrap; stage with clamp off) |
| `u8(x)` | TEV operands a/b/c take the low 8 bits of a register; only emitted when the value can leave [0, 1] |
| `r8_gt(a, b) ? c : 0` etc. | TEV compare stages (`r8`, `gr16`, `bgr24`, `rgb8`/`a8`; `_gt`/`_eq`) |
| `x.aaa`, `vec3(x)` | alpha replicated into a colour operand |
| `eN = …` | common sub-expression used more than once |
| `if (!alpha_test(out.a)) discard` | alpha compare (functions and references as uniforms at the recommended level) |
| `mix(out.rgb, fog_color, fog_fn(depth[, range_adjust]))` | fog; `fog_fn` is a uniform-selected GX fog function |

Vertex programs (`vs_<key>.txt`): `pos = mtx * pos` or `skin(pnmtx[idx], pos)`; per colour channel `vN.rgb|a = (matcol|vtx_clrN) * sat((ambcol|vtx_clrN) + lights(n, diffuse, attn))` or unlit `= matcol|vtx_clrN`; per texture coordinate `uvN = mtx2x4|mtx3x4|bumpN|srtg(texmtx|identity, src)` (`then post_mtx` for models loaded with `J3DMLF_UsePostTexMtx`).

Every name in a program is canonical (numbered in order of first use). The per-draw **binding table** (`fs_bind` in `draws.csv`, e.g. `c0=reg1 c1=reg0 kc0=stage0 map0=texmap0 uv0=texcoord0 v0=chan0`) maps those names back to the material's TEV registers, stages, texture maps, texture coordinates and channels. The run time uses it to fill uniforms and bind samplers.

## 1. The shader key

| Goes into | State |
|---|---|
| **Fragment key** | The TEV expression trees for `out.rgb` and `out.a` (§2): which samples, vertex colours, registers and konsts feed which operations, compare stages, swizzles from swap tables, indirect setup, whether a sample is projected; alpha test (at least "can discard"); fog on/off and range adjust. |
| **Vertex key** | Per used colour channel: lit or not, material/ambient source (register or vertex), diffuse and attenuation function, light count (or a fixed loop); per used texture coordinate: texgen type, source, matrix or identity, post-matrix; skinning (`PNMTXIDX` present); normals when needed. |
| **Uniforms** | TEV register values, konst values (after the K selector), alpha-test functions and references, fog function and parameters, matrices, light parameters, material/ambient colours. Everything that `.brk`/`.btk`/`.btp`/`.bpk` animations change: `J3DMaterialAnm::calc` writes only material colours, texture indices, TEV/K colours and texture-matrix SRTs. |
| **Outside the offline key** | Blend mode, colour/depth output format and vertex attribute layout. GXM's shader patcher applies these when a compiled program is *instantiated* (`sceGxmShaderPatcherCreateFragmentProgram` takes `SceGxmBlendInfo` and the output format; `sceGxmShaderPatcherCreateVertexProgram` takes the attribute/stream layout), so they become a runtime program cache, not extra offline compiles. **Still to verify** against the vitasdk headers, including how expensive patching is on device. GXM blending has no logic ops; the disc data uses none (blend types per source are in the survey), the 11 J3D `subtract` materials map to reverse-subtract, and the few `GX_BM_LOGIC` uses in code ([graphics.md](graphics.md#gx-feature-inventory-what-a-shader-layer-must-reproduce)) need a check. Depth/stencil/cull and `ZCompLoc` are plain GXM state. |

Texture formats are not part of the key, as long as cooking expands palette formats (`C4`/`C8`/`C14X2`) to direct colour and intensity formats map to GXM formats with a swizzle that reproduces GX's replication. If palettes stay indexed, the key needs a per-sampler "palette lookup" bit.

## 2. Canonicalisation

`shader.py` executes the TEV stages **symbolically**. Each TEV register lane starts as its uniform input and holds an expression tree; after the last stage, the trees of the output registers are the program. This removes, by construction:

* register allocation (the same result computed in `reg0` and moved to `prev`, or computed in `prev` directly, gives the same tree);
* dead stages and stage halves (values never read are never part of the tree);
* pass-through copies (`prev = prev`).

Smart constructors then fold identities (`mix` with `c = 0`/`1`, zero and one operands, `d = 0`, `a == b`) and order commutative operands. An interval analysis drops `sat`, `wrap11` and `u8` where the operand provably stays in range. Leaves are renamed in order of first use: samples, maps, coordinates, channels, konsts and register inputs. The self-check in `shader.py` (`python3 tools/gx_shaders/shader.py`) verifies that four differently-encoded versions of `tex × vertex colour` produce one key.

Approximations, accepted on purpose: exact fixed-point rounding of the 8-bit TEV datapath is not modelled (so `a·b` and `b·a` are treated as equal), and identity folding treats GX's 8-bit `lerp` as exact at 0 and 1, which the hardware also does.

**Numeric check** (`check_equiv.py`). An independent TEV reference executes each material's stages straight from its fields, with Dolphin's semantics for operand widths, clamp/wrap and compare ops, on random inputs. Two properties are checked:

* the canonical trees compute the same `out.rgb`/`out.a` as the reference;
* materials that share a key compute the same result when their inputs are bound through their binding tables (no false merges).

Result on the disc: 922 distinct materials, 716 keys, **0 mismatches and 0 false merges** (8 trials each). The check does catch planted bugs: a wrong `mix` identity gives 46 mismatches and 1 false merge. One semantic bug was found and fixed along the way. R8/GR16/BGR24 compares in the *alpha* lane compare the stage's colour A/B operands, not the alpha ones. The disc never uses that form (only 278 colour-lane R8 stages and 8 alpha-lane A8 stages), so it is exercised only by code paths.

## 3. Data enumeration

| Source | Parser | Where on the disc | Notes |
|---|---|---|---|
| J3D models | `j3d.py` (layout from `J3DMaterialFactory`, `J3DShapeFactory`, `J3DModelLoader`) | 3,623 `.bmd`/`.bdl` in 1,733 archives, including archives nested in archives | `MAT3` materials, `SHP1` vertex descriptors (for the vertex key), `INF1` material→shape pairs, `TEX1` names/formats. The RARC directory tag (`BMDR`, `BMDE`, `BMWR`, …) is kept because it decides the load path (§4). One `bmd2` (`MAT2`) model is skipped. |
| J2D layouts | `blo.py` (`J2DMaterialFactory`) | 131 `SCRNblo2` files | `MAT1` materials. 87 materials with indirect stages are counted but their indirect settings are not decoded yet; one old `SCRNblo1` file is skipped. |
| Particles | `jpa.py` (reproduces `JPABaseShape::setGX`, `JPAResource::setPTev/setCTev`, `JPAEmitterManager::draw`) | 65 `.jpc` files, 6,906 resources | Child particles inherit the parent's stage-0 inputs. Particle fog is set by the game per draw group (`dKy_GxFog_set` in `d_particle.cpp`), so each particle program gets a fog-on and fog-off variant. |

## 4. Load-time and run-time mutations

Only a few places in code change *structural* material state. Each one is modelled as a variant of the affected materials:

| Where | What it does | Modelled as |
|---|---|---|
| `addWarpMaterial`, `dRes_info_c::on/offWarpMaterial` (`d_resorce.cpp`) | For `BMWR`/`BMWE` models: adds a texgen and a TEV stage (`texmap3 × prev`, the `Always` resource `0x5d`), an alpha compare and `ZCompLoc 0`; on/off toggles the extra stage. | `warp` variant next to the original (163 draws, 7 new programs). |
| `dRes_info_c::loaderBasicBmd` | Masks channel 0's light mask by `g_env_light.light_mask_type` (masks `0x04`…`0x7F`), set per stage/room (`d_s_play.cpp`, `d_kankyo.cpp`, `d_s_menu.cpp`, `d_a_kytag17`). | Extra vertex keys, shown as `vertex_rt` in the survey: this is why a fixed light loop is recommended. |
| `dKy_bg_MAxx_proc` (`d_kankyo.cpp`) | Room-BG materials named `??MA03/09/17/19…` get fog type 6 or 7; `??MA00/01/04/16` with `1` at name[6] switch between alpha test `GEQUAL 0x80 AND LEQUAL 0xFF` (camera under water) and none. Other rules change colours and matrices only. | `bgfog`, `bg_underwater`, `bg_opa` variants (84 draws, 5 new programs). |
| `d_kankyo.cpp:4376` (`m_fogtype` switch) | Rewrites every material's fog type from a HIO setting. | **Not modelled: `#if DEBUG` only.** In release builds the fog type is data plus the BG rule above. |
| `d_a_vrbox*`, `d_a_obj_ganonwall*` | Sky boxes set fog type 2; Ganon walls change fog colour/range. | Covered by keeping fog *type* as a uniform. |
| `J2DPictureEx`/`J2DWindowEx`/`J2DTextBoxEx` `setBlackWhite`, `insert`/`append`/`remove` | Rebuild the pane material's TEV stage count, orders, stages and K selectors from the texture count and whether black/white are the defaults. Game code calls `setBlackWhite` 187 times in 24 files. | **Not modelled yet**: the J2D count in the survey covers the `.blo` data only. The rebuilt configurations are a small function of (texture count, black/white default or not) and can be enumerated the way `jpa.py` does for particles. |
| `setIndirectTex` (`d_resorce.cpp`) | Textures named `fbtex_dummy`/`dummy`/`Zbuffer` are re-bound to the framebuffer or Z buffer. | No key change; 90 materials flagged (`rebind` in `draws.csv`) because they need a render-target texture, and `Zbuffer` needs depth as a colour texture on GXM. |
| `m_Do_ext.cpp:431-463`, `d_a_b_yo`, `d_a_e_dt`, `d_a_e_dk`, `d_a_e_vt` | Build or edit material blocks in code. | Not modelled yet: hand audit (small). |

## 5. Measurements

Full tables: [shader-survey.md](shader-survey.md). Each level adds one more "make it a uniform" decision:

| level | fragment | vertex | vertex incl. light masks | pairs |
|---|---:|---:|---:|---:|
| raw fields | 922 | 363 | 1,648 | 1,255 |
| canonical | 799 | 432 | 1,644 | 1,175 |
| + konst as uniform | 780 | 432 | 1,644 | 1,164 |
| + fixed 8-light loop | 780 | 380 | 410 | 1,123 |
| + identity texture matrices as uniforms | 780 | 285 | 308 | 1,041 |
| + alpha-test functions as uniforms | 745 | 285 | 308 | 1,006 |
| **+ fog function as uniform (recommended)** | **725** | **285** | **308** | **986** |
| + bias/scale as uniforms | 708 | 285 | 308 | 966 |
| + fog always applied | 613 | 285 | 308 | 858 |

(The vertex count rises from raw to canonical because the canonical key includes shape attributes and the post-matrix load path, which the raw comparison ignores.)

Reading the table:

* The raw data is already fairly uniform: the canonicaliser removes 13 % of fragment programs, and the cheap uniform choices another 9 %. The big lever is on the vertex side (light loop, texture matrices).
* The recommended level keeps per-stage bias/scale and "fog off" in the program; making them uniforms saves 17 and 95 programs, at the cost of ALU in *every* program. Only worth it if program count turns out to matter more than per-pixel cost.
* About 1,000 programs is small enough to **precompile everything**: no runtime compiler, and the ubershader becomes a miss-only fallback ([§8](#8-validation-loop)). Each compiled program should be a few kilobytes, which still needs measuring with `psp2cgc`.

## 6. Code-driven configurations

`gxcalls.py` parses every GameCube TU that issues GX state or draw calls with the system **libclang**, through `ctypes` (no extra Python packages). `-resource-dir` comes from `clang -print-resource-dir`, and the generated `assets/*.h` from the main checkout's `build/GZ2E01/include`. All 45 TUs parse without errors. The tool lists every GX call per function with arguments resolved to enumerators or constants (`build/gx_shaders/gx_calls.csv`). It then replays each root function through a GX state machine, inlining analysed callees, including inline helpers from headers, up to depth 3. At each `GXBegin`/`GXCallDisplayList` the state so far becomes a synthetic material, which goes through the same canonicaliser. A draw site is attributed to the root of the replay. It counts as data-driven only if the draw is issued by J3D/JParticle/J2D-Ex code *and* the path set no TEV state before it.

Full tables: [shader-code-paths.md](shader-code-paths.md). The source holds 3,614 GX/GD calls in 250 functions across 47 files. The page reconciles this with a text search (4,249): the difference is `#if DEBUG` drawing, other versions' arms, and enum casts that look like calls. That gives 205 draw sites after inlining:

* **61 are data-driven.** J3D shape/packet draws, `JPADraw*` and `J2D*Ex` draw with state from material display lists or resource data that §3 already covers.
* **53 of the 144 code-driven sites resolve to constants.** Most of them rely on some **inherited** state, which the tool fills from `GXInit`'s defaults and lists per site. These sites produce 35 programs, 8 of which the data also produces.
* **91 sites have arguments clang cannot fold.** These are mostly loops over stage indices (`for (i …) GXSetTevColorIn((GXTevStageID)i, …)` in `m_Do_graphic`, `d_drawlist`, `J2DWindow`, `J2DPicture`) and values passed in from data (grass/flower packets, cloth). Each needs a hand audit, or loop unrolling in the tool (next steps).

**Finding: inherited swap and indirect state.** About 115 code-driven sites never call `GXSetTevSwapMode`/`GXSetTevDirect`, so they rely on the previous draw having left identity swaps and direct TEV. On the GameCube that holds because every J3D material resets them. The port's GX layer must do the same (reset them for every non-material draw, or carry them in the shadow state). Otherwise a stale swap or indirect setting would silently select a different program.

## 7. Shared patterns, and how to use them

The survey mines the distinct programs for shared structure:

* **Two terms cover most pixels.** The world-lighting term `sat((C + mix(K, T, V)) * 4)` appears in 83 programs and 8,896 materials. In it, `C` is the TEV register the environment code fills with ambient, `K` the konst colour, `T` the texture and `V` the lit vertex colour. The remap `mix(u8(C0), u8(C1), T)` appears in 65 programs and 17,879 materials: J2D's black/white colours and JParticle's prm/env colours. Generate these as shared shader functions (`tp_lit`, `remap`), and write hand-tuned versions of the 4 programs that cover half of all materials.
* **Colour and alpha are nearly independent.** The 725 programs combine 334 distinct `out.rgb` expressions, 207 `out.a` expressions and 4 tails (alpha test, fog). GXM still needs whole compiled programs, but generator templates and reviews can work on the 334 + 207 expressions.
* **Families differ only in uniform slots.** Generalising which register or konst a program reads collapses 725 programs to 675 families; members of a family share code with a different binding table.
* **`u8(C)` can be computed on the CPU.** Register inputs are uniforms, so the CPU can upload their wrapped 8-bit value. This removes per-pixel work in the most common programs and does not change the count.
* **The tail is long but cheap.** 295 programs are used by one material each. Precompile them anyway; if program count ever matters, route the tail through the ubershader instead.

## 8. Validation loop

Static analysis supplies the set; the run time checks it:

* The GX layer keeps shadow GX state (it needs one anyway) and computes the canonical key at every draw with the same canonicaliser, ported to C++. The pseudocode text, or its hash, is the cache key.
* On a miss it draws with the **ubershader** and logs the key and the draw site once. The ubershader stays the always-correct fallback and the reference.
* A smoke-test playthrough then only has to show *zero misses*, instead of being the way the set is discovered.
* Differential check: render a key with its generated program and with the ubershader on the same inputs and compare. This also tests the canonicaliser.

## 9. Shipping

The analysis runs on the developer's disc. The generated programs are a function of the key only and contain no asset data, so the compiled `.gxp` files can ship with the port. This assumes the offline compiler (`psp2cgc` from the official SDK, or a usable open-source path) is available to whoever builds the port, so end users never compile shaders and the retail-firmware runtime compiler (`SceShaccCg`) is not needed. Deciding which compiler this depends on is still open.

## Tools

All in `tools/gx_shaders/`, Python 3 + pandas, no disc data in the repository:

```sh
build/tools/dtk vfs cp orig/GZ2E01/GZ2E01.iso:files/res <dir>          # extract the disc (Yaz0 is decompressed)
python3 tools/gx_shaders/scan.py --disc <dir> --report docs/port/shader-survey.md     # seconds
python3 tools/gx_shaders/gxcalls.py --data-programs build/gx_shaders/draws.csv \
        --report docs/port/shader-code-paths.md                                       # seconds, libclang
python3 tools/gx_shaders/shader.py                                                    # canonicaliser self-check
python3 tools/gx_shaders/check_equiv.py --disc <dir>                                  # numeric check vs a TEV reference
```

| File | Role |
|---|---|
| `rarc.py` | RARC archives (and Yaz0 when needed) |
| `j3d.py`, `blo.py`, `jpa.py` | J3D models, J2D layouts, JParticle resources → GX material state |
| `shader.py` | canonical IR (symbolic TEV, simplification, renaming) → pseudocode; vertex IR; warp transform; pattern helpers |
| `scan.py` | disc survey: variants from §4, canonicalisation levels, coverage, patterns; writes `build/gx_shaders/{draws.csv,pseudo/}` |
| `clang_c.py`, `gxcalls.py` | libclang via `ctypes`; GX call extraction and replay of code paths; writes `build/gx_shaders/{gx_calls.csv,code_sites.csv}` and code-path pseudocode |
| `check_equiv.py` | independent TEV reference; checks canonical trees and key merges numerically |

Outputs under `build/gx_shaders/` are derived from the disc and are not committed; the two generated pages contain only counts and expressions.

## Next steps

1. **Generator**: pseudocode → Cg for `psp2cgc`, starting with the 16 programs that cover 80 % of materials; check the shader-patcher assumptions in §1 on device.
2. **Ubershader + miss logging** in the GX layer (roadmap phase 5), using the same key.
3. **Code paths**: unroll constant-bound loops in `gxcalls.py`, then audit the remaining unresolved sites; model `m_Do_ext.cpp`'s code-built material and the four actors in §4.
4. **J2D run-time TEV rebuilds**: model `J2D*Ex::setBlackWhite`/`insert`/`append`/`remove` (§4), the way `jpa.py` models JParticle.
5. **Gaps in data coverage**: decode J2D indirect stages (87 materials), the one `MAT2` model and the one `SCRNblo1` layout.
6. **Other versions**: the scan is GZ2E01 only; run it on the PAL/JP discs if those are targeted.
