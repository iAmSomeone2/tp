# 2. Project and source-code hierarchy

## Top level

```
tp/
├── configure.py        Project manifest + build-file generator (see 01-project-overview.md)
├── Doxyfile            Doxygen config (input: docs/mainpage.h, src/, include/, README.md)
├── src/                Game-engine source (.cpp/.c)         ── Zelda-specific code
├── include/            Headers for src/ (mirrors its layout)
├── libs/               Everything that is *not* Zelda-specific: JSystem, SDKs, C runtime, TRK
├── config/<version>/   dtk configs: config.yml, splits.txt, symbols.txt, build.sha1, rels/
├── assets/<version>/   Generated resource-index enum headers (no real assets)
├── orig/<version>/     Where YOU put the disc image / extracted files (git-ignored)
├── tools/              Python helpers (project generator, converters, utilities)
├── docs/               This documentation + Doxygen theme
├── .github/workflows/  CI: build all versions, publish Doxygen site
└── build/              (generated) objects, build.ninja, map files, report.json
```

`src/` and `include/` are parallel trees: `src/d/d_stage.cpp` ↔ `include/d/d_stage.h`. Third-party/SDK code keeps its headers next to it: `libs/JSystem/src/...` and `libs/JSystem/include/JSystem/...`.

## `src/` and `include/` (Zelda-specific)

| Directory | Files (src) | What it is |
|-----------|------------:|-----------|
| `src/m_Do/` | 17 | **Machine-dependent layer** (`m` = machine; the original meaning of "Do" is not documented). Entry point, heaps, graphics init, pads, DVD thread, memory card, matrices, model/animation helpers. |
| `src/f_ap/` | 1 | **Framework – application.** `f_ap_game.cpp`: creates the framework and drives one game frame. |
| `src/f_op/` | 23 | **Framework – object/operation layer.** Typed process classes on top of `f_pc`: actor, scene, camera, view, kankyo (environment), message, overlap (screen wipe); plus their managers. |
| `src/f_pc/` | 32 | **Framework – process/"proc" core.** Generic process manager: create/execute/delete queues, layers, priorities, profiles, dynamic loading. |
| `src/d/` | ~175 + `actor/` | **"Dolzel" game logic** (name probably = Dolphin + Zelda). Stage/room loading, collision (`d_bg_*`, `d_cc_*`), camera, attention, events, HUD (`d_meter*`), menus (`d_menu_*`), messages (`d_msg_*`), save data, environment (`d_kankyo*`), particles, drawlists, items, scenes (`d_s_*`)… |
| `src/d/actor/` | ~805 | **Actors** – every game object: `d_a_alink` (Link), `d_a_npc_*`, `d_a_e_*`, `d_a_b_*`, `d_a_obj_*`, `d_a_tag_*`, `d_a_kytag*`… ~770 are their own REL. See [actor-index.md](actor-index.md). |
| `src/c/` | 2 | `c_dylink.cpp` (process-name → REL table + REL loading glue), `c_damagereaction.cpp`. |
| `src/SSystem/` | 40 | **SSystem** (Nintendo's own utility library, home of the `c*` "SComponent" classes): basic math/geometry (`c_m3d*`, `c_xyz`, `c_angle`), intrusive lists/trees/tags (`c_list`, `c_tree`, `c_tag`), phase/request helpers, and the collision-data structures `c_cc_d`/`c_cc_s` (attack/target/body colliders) and `c_bg_*` (BG collision queries). |
| `src/Z2AudioLib/` | 28 | **Z2 audio** – Zelda's sound layer on top of JAudio2 (SE/BGM managers, per-creature sound objects, wolf-howl, scene-based BGM). |
| `src/Z2AudioCS/` | 8 | Wii-remote speaker ("CS" = controller speaker) audio (Wii/Shield). |
| `src/DynamicLink.cpp` | 1 | `DynamicModuleControl`: loads/links/unlinks RELs. |
| `src/CaptureScreen.cpp` | 1 | Debug screenshot capture. |
| `src/m_Re/` | 1 | Wii-remote pad (`m_Re_controller_pad`): "Re" = Revolution. |
| `src/REL/` | 1 | `executor.c` – each REL's `_prolog` / `_epilog` / `_unresolved` entry points; run the module's static constructors/destructors. |
| `src/lingcod/`, `src/NdevExi2A/`, `src/odemuexi2/`, `src/odenotstub/`, `src/amcstubs/` | 1–2 each | Shield-specific patch (`lingcod`) and dev-kit debugger-communication drivers/stubs (EXI/`odemu`/`amc`). |

Everything in `include/` under those names has the headers. `include/d/actor/*.h` holds the actor class definitions (one per actor, with Doxygen `@brief` names); `include/f_pc/f_pc_name.h` holds the master process-name enum.

## `libs/` (not Zelda-specific)

| Path | Size | What it is |
|------|-----:|-----------|
| `libs/JSystem/` | ~290 src / ~325 include | **JSystem** – Nintendo's internal middleware library (used by many first-party games). Sub-libraries below. |
| `libs/dolphin/` | ~200 src | **Dolphin SDK** (GameCube): `os`, `gx` (GPU), `dvd`, `pad`, `card`, `ai`/`ax`/`dsp` (audio hardware), `mtx`, `vi`, `exi`, `hio`, `gf`… Used for GCN builds. |
| `libs/revolution/` | ~350 src | **Revolution SDK** (Wii): the above plus `wpad`/`kpad` (Wii remote), `nand`, `ipc`, `sc`, `usb`, `homebuttonLib`… Used for Wii and Shield builds. |
| `libs/PowerPC_EABI_Support/` | ~150 | Metrowerks C/C++ library: **MSL_C** (C standard library), **MSL_C++**, **Runtime** (`__init_cpp`, exception/destructor chains, `__mem` helpers). |
| `libs/TRK_MINNOW_DOLPHIN/` | ~30 | **TRK** – the on-device debugger stub. |

### JSystem sub-libraries

All under `libs/JSystem/{src,include/JSystem}`. The `J` + 2–3 letter prefix tells you the module.

| Dir | Prefix | Purpose |
|-----|--------|---------|
| `JKernel` | `JKR*` | Memory & file kernel: heaps (`JKRHeap`, `JKRExpHeap`, `JKRSolidHeap`), archives (`JKRArchive`, `JKRMemArchive`, `JKRAramArchive`, `JKRDvdArchive`), DVD/ARAM I/O, decompression (Yaz0/Yay0/SZS). **Every game resource goes through here.** |
| `JSupport` | `JSU*` | Streams, intrusive lists (`JSUList`/`JSULink`). |
| `JUtility` | `JUT*` | Game-pad (`JUTGamePad`), fonts, console/debug print (`JUTConsole`, `JUTDbPrint`), fader, exception handler, asserts, palettes, textures. |
| `JMath` | `JMA*` | Trig tables, RNG. `JGeometry.h`: vector/matrix templates. |
| `JGadget` | – | STL-like containers (`std-list`, `vector`, `linklist`) used by JStudio/JMessage. |
| `JFramework` | `JFW*` | `JFWSystem` (boot: root heap, fonts, console, graphics init) and `JFWDisplay` (frame begin/end, video swap, fader). |
| `J3DGraphBase` | `J3D*` | **3D rendering core**: materials, shapes, TEV, display-list `J3DPacket`s, `J3DDrawBuffer` sort buckets, `J3DSys` state. |
| `J3DGraphAnimator` | `J3D*` | `J3DModel`, `J3DModelData`, skeleton/joints, skinning, animation classes (`J3DAnmTransform`, `…Color`, `…TextureSRTKey`…). |
| `J3DGraphLoader` | `J3D*Loader` | Loads BMD/BDL models and BCK/BTK/BRK/… animations from binary. |
| `J3DU` | `J3DU*` | J3D utilities (clipper, display-list helpers). |
| `J2DGraph` | `J2D*` | **2D UI**: `J2DScreen` (loads `.blo` layouts), `J2DPane`, `J2DPicture`, `J2DTextBox`, `J2DWindow`, 2D animations. |
| `JParticle` | `JPA*` | **Particle system** (JPA): emitters, resources (`.jpc`), shapes, fields, keys. |
| `JAudio2` | `JAI*`, `JAS*`, `JAU*` | **Audio engine**: sequencer (`JASTrack`), sound data, ARAM streaming, `JAISeMgr`/`JAISeqMgr`/`JAIStreamMgr`, DSP, banks/waves. The Zelda-specific layer is `Z2AudioLib`. |
| `JMessage` | `JMessage`/`JMS*` | Message (BMG) text control/processing. Zelda's `d_msg_*` builds on it. |
| `JStage` | `JSG*` | "Stage" abstraction for cutscenes: actors/cameras/lights/fog as scriptable objects. |
| `JStudio` (+ `_JStage`, `_JAudio2`, `_JParticle`, `JStudioCameraEditor`, `…Previewer`, `…ToolLibrary`) | `JStudio`/`STB` | **Cutscene/timeline engine** ("STB" files) that drives JStage objects, sound and particles. Zelda's `d_demo.cpp` glues it in. |
| `JHostIO`, `JAHostIO`, `JAHNodeLib`, `JAWExtSystem`, `JAWWinLib` | `JOR*`, `JHI*`, `JAH*`, `JAW*` | Developer tooling: HostIO server/reflection, audio tweak windows. Mostly debug-only. |

## Which code lives in the DOL vs. in RELs

The build splits code into:

* **DOL (`main.dol`, "framework")** – the always-loaded executable. Libraries declared in `configure.py`: `machine` (m_Do), `framework` (f_ap/f_op/f_pc), `dolzel2` (nearly all of `src/d/*.cpp` plus a handful of actors), `SSystem`, JSystem, Z2, SDK, Metrowerks runtime.
  * Actors that are in the DOL because everything depends on them: `d_a_alink` (Link), `d_a_player` (player base), `d_a_npc` / `d_a_npc_cd` / `d_a_npc_cd2` (NPC base classes), `d_a_itembase`, `d_a_obj_item`, `d_a_obj_ss_base`, `d_a_no_chg_room`.
* **RELs** – one per remaining actor, plus `f_pc_profile_lst` which holds the master list of profile pointers (`g_fpcPfLst_ProfileList[]`). In `configure.py` these are the `ActorRel(...)` lines (≈ 750). The "REL" library entry at the end of the list holds the REL runtime glue.
  * At runtime, which REL provides which process is defined by the table in `src/c/c_dylink.cpp` (`DynamicNameTable`: `{fpcNm_…_e, "d_a_…"}`), and loading is done by `DynamicModuleControl` (`src/DynamicLink.cpp`).
  * Shield-debug moves a few RELs into the DOL (`ActorRel(..., isInDol=True)`, `__FORCE_REL_IN_DOL__`).

The practical upshot: **the actor you are looking at might not be in memory**. It is linked on demand when a stage needs it (see [03-engine-architecture.md](03-engine-architecture.md#37-dynamic-modules-rels)). "Base class" code (`d_a_npc.cpp`, `d_a_obj.cpp`, `d_a_horse_static.cpp`, `…_static.cpp`) is DOL-resident so that RELs can call into it.

## File-name prefix cheat sheet

Full details in [05-glossary.md](05-glossary.md#file-and-symbol-prefixes).

| Prefix | Where | Meaning |
|--------|-------|---------|
| `f_pc_` | `src/f_pc` | Framework **P**rocess **C**ore |
| `f_op_` | `src/f_op` | Framework **O**bject **P**rocess types (actor, scene, camera, …) |
| `f_ap_` | `src/f_ap` | Framework **Ap**plication (game) |
| `m_Do_` | `src/m_Do` | Machine-dependent "Do" (Zelda engine glue) |
| `d_` | `src/d` | "Dolzel" game logic |
| `d_a_` | `src/d/actor` | **A**ctor |
| `d_bg_*`, `d_cc_*`, `d_s_*`, `d_menu_*`, `d_meter*`, `d_msg_*`, `d_kankyo*`/`d_ky*`, `d_event*`, `d_save*`, `d_map*`, `d_a_obj*` (helpers) | `src/d` | Background collision / collision-check / scenes / menus / HUD / messages / environment / events / save / maps |
| `c_` | `src/SSystem/SComponent`, `src/c` | SComponent utilities (`c_xyz`, `c_m3d`…) and dynamic-link glue |
| `J*` | `libs/JSystem` | JSystem (see table above) |
| `Z2*` | `src/Z2AudioLib` | Zelda audio (`Z2` echoes the `GZ2…` game IDs) |
| `os*`, `GX*`, `DVD*`, `PAD*`, `CARD*`, `AI*`, `VI*` | `libs/dolphin`, `libs/revolution` | Nintendo SDK |

## Quick "where do I look?" table

| I want to… | Start in |
|------------|----------|
| Follow program start-up | `src/m_Do/m_Do_main.cpp` (`main` → `main01`), `src/f_ap/f_ap_game.cpp` |
| See the per-frame process update | `src/f_pc/f_pc_manager.cpp` (`fpcM_Management`) |
| Add/find an actor's class | `include/d/actor/d_a_<name>.h`, `src/d/actor/d_a_<name>.cpp` |
| See all process IDs | `include/f_pc/f_pc_name.h` (`fpcNm_*_e`) |
| See how a stage's map data gets turned into actors | `src/d/d_stage.cpp` (`dStage_actorInit`, `l_objectName[]`) |
| Change what happens in the main gameplay scene | `src/d/d_s_play.cpp` |
| Read/write game state (items, flags, rupees…) | `include/d/d_save.h`, accessors `dComIfGs_*` in `include/d/d_com_inf_game.h` |
| Find global game state (current stage, player pointer, camera, draw lists) | `include/d/d_com_inf_game.h` (`g_dComIfG_gameInfo`, `dComIfGp_*`, `dComIfGd_*`) |
| Player behaviour | `src/d/actor/d_a_alink*.{cpp,inc}`, `include/d/actor/d_a_alink.h`, `d_a_player.h` |
| Collision with the world | `src/d/d_bg_s*.cpp`, `d_bg_w*.cpp`, `include/d/d_bg_s_acch.h` |
| Hit detection between actors | `include/d/d_cc_d.h`, `src/d/d_cc_*.cpp`, `SSystem/SComponent/c_cc_*` |
| Lighting, weather, time of day, twilight | `src/d/d_kankyo*.cpp`, `d_kyeff*.cpp`, `d_a_kytag*` actors |
| HUD and menus | `src/d/d_meter2*.cpp`, `d_menu_*.cpp` |
| Text boxes, dialogue | `src/d/d_msg_*.cpp`, `d_msg_flow.cpp` |
| Sound | `src/Z2AudioLib/*`, `include/Z2AudioLib/Z2Instances.h` |
| Resource loading | `src/d/d_resorce.cpp`, `src/m_Do/m_Do_dvd_thread.cpp`, `libs/JSystem/src/JKernel/` |
| Which archive/file indices exist | `assets/<ver>/res/**` |
