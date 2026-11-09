# 3. Engine architecture

This page explains the *skeleton* of the game. Once you understand it, every actor, menu and cutscene is just a process plugged into it.

## 3.1 Layer cake

```
┌───────────────────────────────────────────────────────────────────────────┐
│  d_a_*  actors (RELs)   d_s_* scenes   d_menu/d_meter/d_msg  (game logic) │  src/d/…
├───────────────────────────────────────────────────────────────────────────┤
│  d_com_inf_game  – global game state ("g_dComIfG_gameInfo") + accessors   │  src/d/d_com_inf_game.cpp
│  d_stage / d_save / d_event / d_kankyo / d_bg / d_cc / d_camera / …       │
├───────────────────────────────────────────────────────────────────────────┤
│  f_ap  application    f_op  typed processes    f_pc  process core         │  src/f_*   ("framework")
├───────────────────────────────────────────────────────────────────────────┤
│  m_Do  machine glue: heaps, gfx, pads, DVD thread, memcard, anim helpers  │  src/m_Do
├──────────────────────────┬───────────────────────────┬────────────────────┤
│ SSystem (c_*)            │ JSystem (J3D, J2D, JKR, … │ Z2AudioLib         │
│ math, lists, colliders   │ JPA, JAudio2, JStudio…)   │ (on JAudio2)       │
├──────────────────────────┴───────────────────────────┴────────────────────┤
│  Dolphin / Revolution SDK (os, gx, dvd, pad, ai…)   MSL C lib   Runtime   │  libs/
└───────────────────────────────────────────────────────────────────────────┘
```

Dependencies mostly point downward, but not strictly: `f_pc_manager.cpp` includes `d_com_inf_game.h`, and framework code calls into `d_` for HUD/error screens. Think of "f_" as the generic scheduler and "d_" as game-specific code that both plugs into it and reaches back into it.

Naming pattern: **`f`**ramework, **`m`**achine, **`d`**olzel, **`c`** = SComponent. Function prefixes track file names: `fpc…` is `f_pc`, `fop…` is `f_op`, `fapGm_` is `f_ap_game`, `mDoMch_`/`mDoGph_`/`mDoExt_` are `m_Do_machine`/`_graphic`/`_ext`, `dComIfGp_`/`dComIfGs_`/`dComIfGd_` are accessors in `d_com_inf_game.h`.

## 3.2 Boot sequence

```
main()                                   src/m_Do/m_Do_main.cpp
 ├─ set up reset data, version_check(), dComIfG_ct()
 ├─ decide developmentMode (disc ID / console type)
 └─ create thread → main01()             (main thread then suspends itself)

main01()                                 src/m_Do/m_Do_main.cpp
 ├─ mDoMch_Create()        heaps (root/system/Zelda/game/archive/J2D/command), exception handler,
 │                         RNG seed, DVD-error thread, memory-card thread
 ├─ mDoGph_Create()        frame/Z buffers, GX display lists, fader, bloom
 ├─ mDoCPd_c::create()     controller pads (+ mReCPd::create() on Wii)
 ├─ mDoDvdThd_callback_c::create(...)      DVD-thread helper
 ├─ fapGm_Create()         ── the *framework* start-up (below)
 ├─ fopAcM_initManager()
 ├─ cDyl_InitAsync()       on the DVD thread: mount /, load symbol string table, link REL
 │                         `f_pc_profile_lst`, then fopScnM_CreateReq(fpcNm_LOGO_SCENE_e, …)
 └─ loop forever:
       mDoCPd_c::read()            poll pads
       fapGm_Execute()             ONE GAME FRAME  (see 3.3)
       mDoAud_Execute()            audio update
       debug()                     debug hotkeys/overlays (dev builds)

fapGm_Create()                           src/f_ap/f_ap_game.cpp
 ├─ fpcM_Init()            root layer (queue[10]) and the 16 execution "lines"
 ├─ fopScnM_Init() / fopOvlpM_Init() / fopCamM_Init()   scene, overlap, camera managers
 └─ fopDwTg_CreateQueue()  draw-tag queue
```

The first scene is `fpcNm_LOGO_SCENE_e` (`d_s_logo.cpp`). Scene flow (each arrow is `fopScnM_ChangeReq(...)` inside the respective `d_s_*.cpp`/`d_a_title.cpp`):

```
LOGO_SCENE ──► OPENING_SCENE (a dScnPly_c that shows the title actor `d_a_title`, stage F_SP102)
              ├─► NAME_SCENE (file select / name entry: d_s_name.cpp, d_file_select.cpp)
              │      └─► PLAY_SCENE
              └─► (debug builds) MENU_SCENE (developer stage-select: d_s_menu.cpp) ─► PLAY_SCENE
PLAY_SCENE  ── stage change ──► PLAY_SCENE again (new stage; screen wipe via an "overlap" process)
```

`PLAY_SCENE` (`dScnPly_c`, `d_s_play.cpp`) *is* the game. OPENING/NAME/WARNING scenes reuse `dScnPly_c` or are small scenes of their own.

## 3.3 What one frame does

`fapGm_Execute()` → `fpcM_Management(NULL, fapGm_After)` in `src/f_pc/f_pc_manager.cpp`. In order:

1. **Guard**: if a DVD error or shutdown dialog is up (`dDvdErrorMsg_c`, `dShutdownErrorMsg_c`), do only that and pause game sound.
2. **`cAPIGph_Painter()`** – render the draw lists prepared by the *previous* frame's draw handler (see 3.9).
3. **`fpcDt_Handler()`** – **delete** queue: process everything marked for deletion.
4. **`fpcPi_Handler()`** – apply queued **priority/layer changes**.
5. **`fpcCt_Handler()`** – **create** queue: advance every pending create request one phase (this is where actors "come to life", load their archives, link RELs).
6. *(pre-execute hook: unused)*
7. **`fpcEx_Handler(fpcM_Execute)`** – **execute**: walk the 16 execution lines in order and call every process's `execute` method.
8. **`fpcDw_Handler(...)`** – **draw**: walk processes by draw priority and call every process's `draw` method; each draw method only *records* work into draw lists.
9. **`fapGm_After()`** – post hook: `fopScnM_Management()` (scene change requests), `fopOvlpM_Management()` (screen wipes), `fopCamM_Management()` (camera).
10. `dComIfGp_drawSimpleModel()`; then `cCt_Counter(0)` bumps the frame counter.

The game logic ticks at 30 Hz: `dScnPly_c` sets `mDoGph_gInf_c::setTickRate(OS_TIMER_CLOCK / 30)`.

## 3.4 The process model (`f_pc`)

A **process** is the unit of everything alive. `base_process_class` (`include/f_pc/f_pc_base.h`, size 0xB8):

| Field | Meaning |
|-------|---------|
| `type`, `subtype` | Runtime type tags (`g_fpcLf_type` leaf, `g_fpcNd_type` node, …) used for checked casts (`fpcBs_Is_JustOfType`). |
| `id` (`fpc_ProcID`) | Unique runtime ID. Actors are usually referenced by this ID, not a pointer. `0xFFFFFFFF` = error/none. |
| `name` / `profname` | The `fpcNm_*_e` process name (which *kind* of process). |
| `state.init_state`, `state.create_phase` | Create/execute/delete lifecycle. |
| `profile` | Pointer to the `process_profile_definition` it was made from. |
| `layer_tag`, `line_tag_`, `delete_tag`, `priority` | Intrusive list membership (see below). |
| `methods` | `process_method_class`: **create / delete / execute / is_delete** function pointers. |
| `append`, `parameters` | Creation argument block and the packed 32-bit parameter word. |
| `pause_flag` | Bit flags for pause groups. |

Process kinds are C-style single inheritance by embedding the parent as the first member:

```
base_process_class
 ├─ leafdraw_class   (adds draw method + draw priority)             src/f_pc/f_pc_leaf.cpp
 │    ├─ fopAc_ac_c    actor        (position, speed, cull, collision hooks…)   f_op_actor
 │    ├─ kankyo_class  environment  (lighting/weather singleton)                f_op_kankyo
 │    ├─ msg_class     message/HUD  (text box, meter, menu window, timer…)      f_op_msg
 │    ├─ overlap_task_class screen wipes / fades                                f_op_overlap
 │    └─ view_class → camera_class camera                                       f_op_view/camera
 └─ process_node_class (owns a *layer* of child processes)          src/f_pc/f_pc_node.cpp
      └─ scene_class   scene (play scene, room scene…)                          f_op_scene
```

### Profiles

Each process kind that exists in the game has a **profile** – a constant descriptor. Example, from `src/d/d_s_play.cpp`:

```cpp
scene_process_profile_definition g_profile_PLAY_SCENE = {
    /* Layer ID     */ fpcLy_ROOT_e,
    /* List ID      */ 1,                     // which of the 16 execute lines
    /* List Prio    */ fpcPi_CURRENT_e,
    /* Proc Name    */ fpcNm_PLAY_SCENE_e,    // its ID in include/f_pc/f_pc_name.h
    /* Proc SubMtd  */ &g_fpcNd_Method.base,  // node behaviour
    /* Size         */ sizeof(dScnPly_c),     // bytes to allocate
    /* Size Other   */ 0,
    /* Parameters   */ 0,
    /* Leaf SubMtd  */ &g_fopScn_Method.base, // scene behaviour
    /* Scene SubMtd */ &l_dScnPly_Method,     // ← the game code: {create, delete, execute, is_delete, draw}
};
```

and for an actor (`d_a_alink.cpp`): `actor_process_profile_definition g_profile_ALINK = { …, /*List ID*/ 5, …, /*Proc Name*/ fpcNm_ALINK_e, …, /*Draw Prio*/ fpcDwPi_ALINK_e, /*Actor SubMtd*/ &l_daAlink_Method, /*Status*/ …, /*Group*/ fopAc_PLAYER_e, /*Cull*/ fopAc_CULLBOX_0_e }`.

Profiles are stacked: `process_profile_definition` (size, methods) → `leaf_process_profile_definition` (+draw priority) → `actor_process_profile_definition` (+actor methods, status flags, group, cull type). Each level of the class hierarchy also contributes a method table: the generic `f_pc` methods call the `f_op` methods (e.g. `fopAc_Create`, `fopAc_Execute`), which in turn call the actor's own table (`l_daAlink_Method`).

The master list of all profiles is `g_fpcPfLst_ProfileList[]` in `src/f_pc/f_pc_profile_lst.cpp`, indexed by `fpcNm_*_e`. `fpcPf_Get(name)` reads it. That list lives in its own REL so it can point at profiles that are in other RELs.

### Layers, lines, priorities

* **Layer** (`layer_class`, `f_pc_layer*.cpp`): a *tree of processes*. The root layer (`fpcLy_ROOT_e = 0`) holds the scenes. Every **node** process (a scene) owns a child layer with up to 16 lists; things created "in the current layer" (`fpcLy_CurrentLayer()`, profile `fpcLy_CURRENT_e`) become children of the currently-running scene. When a scene is deleted, its whole layer is torn down – this is how "leave the stage" cleans up every actor.
* **Line** (`f_pc_line*.cpp`, `l_fpcLn_Line[16]`): the **execution schedule**. Each process's profile `List ID` picks one of 16 lines; `fpcEx_Handler` runs line 0 first, then 1, … then 15. A process only runs if its parent node is on a line too, so a paused/inactive scene doesn't run its children.
* **Draw priority** (`fpcDwPi_*_e` in `f_pc_draw_priority.h`): a separate order for drawing. Camera → environment → sky boxes → grass → many objects → Link etc.

Empirically, `List ID` values in the source tree tell you the stage of the frame: `0` overlaps/room scenes, `1` scenes and kankyo, `2` env sound & objects with early logic, `3` **objects**, `5` **Link**, `7` **the default for NPCs, enemies, bosses, tags, and many objects** (≈480 profiles), `8` late-running objects/enemies, `11` camera, `12` **HUD / message / menu / timer**. Rough rule: world logic before camera before UI.

### Create → execute → delete

Creation is *asynchronous and phased* so it can wait on disk I/O without stalling.

1. **Request**: `fpcM_Create(name, createFunc, append)` (or `fopAcM_create`, `fopScnM_CreateReq`, `fopKyM_create`, …) allocates a `create_request`, queued by `fpcCt_Handler`. `fpcM_FastCreate` (`fpcFCtRq_Request`) is the synchronous variant used when the creator needs the object *now*.
2. **Create phases**: each frame the create method runs again and returns a `cPhs_*` code (`include/SSystem/SComponent/c_phase.h`):

   | Code | Meaning |
   |------|---------|
   | `cPhs_INIT_e` | Not started / still waiting (e.g., REL not loaded yet). Called again next frame. |
   | `cPhs_LOADING_e` | Async load in progress. |
   | `cPhs_NEXT_e` | Advance to the next phase function in the same frame. |
   | `cPhs_COMPLETE_e` | Done (`cPhs_COMPLEATE_e` upstream; this fork corrected the spelling). The process becomes live. |
   | `cPhs_ERROR_e` | Abort; the process is discarded. |

   Actor create methods are usually a two-step: *load resources* (`dComIfG_resLoad(&mPhase, "ArcName")` returns `cPhs_INIT_e` until the archive is mounted), then *initialise* (create heap via `fopAcM_entrySolidHeap`, model, colliders, etc.). `fopAcM_ct(this, Class)` at the start of `Create` runs the C++ constructor via placement-new the first time through.
3. **Execute**: `execute_method` once per frame. For actors this passes through `fopAc_Execute` first (see 3.5).
4. **Delete**: `fpcM_Delete`/`fopAcM_delete` only *queues* deletion. `fpcDt_Handler` next frame calls `is_delete` (return `1` = ok to delete now, `0` = ask again later, e.g. while a sound is still playing), then `delete_method`, then frees the REL reference (`fpcLd_Free`).

Every profile therefore reduces to a 5-function table `{create, delete, execute, is_delete, draw}`; look for `Mthd_Create`, `Mthd_Delete`, … or `daXxx_Create` at the bottom of any actor `.cpp`.

## 3.5 Actors (`f_op_actor` / `fopAc_ac_c`)

`fopAc_ac_c` (`include/f_op/f_op_actor.h`, size 0x568) is the base of every world object. Fields you'll meet constantly:

| Field | Meaning |
|-------|---------|
| `home` / `old` / `current` (`actor_place`) | Spawn pose, previous-frame pose, current pose (`.pos`, `.angle`, `.roomNo`). |
| `shape_angle`, `scale`, `speed`, `speedF`, `gravity`, `maxFallSpeed` | Visual orientation, scale, velocity vector, forward speed, gravity. |
| `model` | Main `J3DModel*`. |
| `heap` | The actor's private `JKRSolidHeap` (models etc. are allocated here so they die with it). |
| `eventInfo` (`dEvt_info_c`) | Talk/cutscene/door/item "event" hook; see [events](04-game-systems.md#events-cutscenes-and-demos). |
| `attention_info` | How the targeting system sees this actor (distances per attention type, position). |
| `tevStr` (`dKy_tevstr_c`) | Per-actor lighting/fog state from the environment system. |
| `cull` / `cullType` / `cullSizeFar` | Frustum culling volume; `cullType` selects a preset box/sphere (`fopAc_CULLBOX_*`, `fopAc_CULLSPHERE_*`) or `CUSTOM`. |
| `actor_status` / `actor_condition` | Bit flags (`fopAcStts_*`: NOEXEC, CULL, FREEZE, CARRY_NOW, NODRAW, BOSS, NOPAUSE…). |
| `group` | `fopAc_ACTOR/PLAYER/ENEMY/ENV/NPC` – coarse category (drives targeting, "enemies present" checks). |
| `setID` | Index of this actor in the room's placement table; used to remember "already collected/killed" (`dComIfGs_isActor`). |
| `argument`, `parameters` (in base) | Sub-type byte and the packed 32-bit parameter word from the map editor. Each actor has `getXxx()` helpers that slice this word with bit shifts/masks – **the first thing to check when you wonder how an actor is configured**. |
| `health` | Hit points where applicable. |

Specialisations: `fopEn_enemy_c` (enemies: down/dead/wolf-bite flags, item drop info), `daPy_py_c` (player base, `d_a_player.h`) → `daAlink_c` (Link), the NPC base classes `daNpcT_c` (`d_a_npc.h`), `daNpcF_c` (`d_a_npc4.h`), `daNpcCd_c` / `daNpcCd2_c` (`d_a_npc_cd*.h`, townsfolk) and `daBaseNpc_c` (`d_a_npc2.h`) – different NPCs were written against different generations of these, `dBgS_MoveBgActor` (moving collision).

`fopAc_Execute` (`src/f_op/f_op_actor.cpp`) wraps each actor's `execute`. It skips the call if the game/scene is paused or if the current **event** doesn't approve movement (`dComIfGp_event_moveApproval`), and it keeps `old = current` for the frame. `fopAc_Draw` culls (box/sphere vs. the camera) and then calls the actor's draw. `fopAc_Create` copies the placement (position, angle, scale ×0.1, `setID`, room, parameters) from the `fopAcM_prm_class` "append" into the actor, sets attention defaults and may veto the actor (e.g. map-tool switch says "enemy group already cleared").

Managers in `f_op_actor_mng.h`: `fopAcM_create(procName, params, pos, room, angle, scale, arg)`, `fopAcM_delete`, `fopAcM_SearchByID/Name`, `fopAcM_entrySolidHeap`, item spawners (`fopAcM_createItem*`), line/ground/roof/water check helpers (`fopAcM_lc_c`, `fopAcM_gc_c`, `fopAcM_rc_c`, `fopAcM_wt_c`), etc.

## 3.6 Scenes and stage transitions

A **scene** is a node process whose layer holds the world:

* `dScnPly_c` (`d_s_play.cpp`) – the play scene (profiles `PLAY_SCENE`, `OPENING_SCENE`, and name/warning variants).
* `room_class` (`d_s_room.cpp`, profile `ROOM_SCENE`) – **one per loaded room**, child of the play scene, that owns the room's background (`fpcNm_BG_e`) and room-local actors.
* `dScnLogo_c` (`d_s_logo.cpp`), `dScnName_c` (`d_s_name.cpp`), `dScnMenu_c` (`d_s_menu.cpp`) – the non-gameplay scenes; the title screen itself is the *actor* `d_a_title`.

`dScnPly_c`'s create method is a chain of `phase_*` functions (`phase_00` … `phase_6`, `phase_compleate`) which, across several frames: loads the shared `Stg_00` stage archive, creates stage data (`dStage_infoCreate`), loads `Event` and `CamParam` archives, loads scene particles and the stage's message group, sets up the collision system (`dComIfG_Bgsp().Ct()`, `dComIfG_Ccsp()->Ct()`), demo system (`dDemo_c::create`), 2D heaps, attention, vibration, then calls **`dStage_Create()`** which parses `stage.dzs` and spawns the initial actors, environment (`dKankyo_create`), sky boxes, and event manager.

**Changing stage** is data-driven. Each stage/room has an exit table (`SCLS` chunk) whose entries hold *destination stage name, spawn point, room, layer, wipe type/time and time-of-day*. A scene-exit actor (`d_a_scene_exit`) flags Link (`onSceneChangeArea`), and Link's own code (`daAlink_c`, e.g. `d_a_alink.cpp` ~line 13800), events (`dStage_changeScene4Event`) or other actors call `dStage_changeScene(exitId, …)`, which copies the chosen entry into `dComIfGp_setNextStage(stageName, spawnPoint, roomNo, layer, …, wipe, …)`; `dScnPly_c::execute` notices `isEnableNextStage()`, an **overlap** process (`d_ovlp_fade*.cpp`, `fpcNm_OVERLAP*`) plays the wipe, and `fopScnM_ChangeReq(scene, fpcNm_PLAY_SCENE_e, wipeType, …)` replaces the play scene. That deletes the old scene's layer (every actor in it) and creates a new play scene for the target stage.

Scene manager files: `f_op_scene_mng.cpp` (`fopScnM_*`), `f_op_scene_req.cpp` (`fopScnRq_*`), `f_op_scene_pause.cpp`; overlap: `f_op_overlap*.cpp`.

## 3.7 Dynamic modules (RELs)

Most actors are separate REL files loaded from the disc when needed:

1. Every process name has a slot in `DMC[]` (`src/c/c_dylink.cpp`). `DynamicNameTable` maps `fpcNm_*_e → "d_a_xxx"`; each becomes a `DynamicModuleControl` (`src/DynamicLink.cpp`). Names not in the table (DOL-resident code) have a `NULL` slot and count as always-linked.
2. `cDyl_LinkASync(profName)` (called from `fpcLd_Load` during the create phases) returns `cPhs_INIT_e` until the REL has been read from disc on the DVD thread, then links it (`OSLink`, running `_prolog` from `src/REL/executor.c`) and returns `cPhs_COMPLETE_e`. Reference-counted: `link()`/`unlink()` (`mLinkCount`).
3. Once linked, the profile is reachable through `g_fpcPfLst_ProfileList[name]` and creation proceeds.
4. When the last process of that name is deleted, `fpcLd_Free` → `cDyl_Unlink` frees the module.
5. REL/archive **pre-loading** exists too: `dScnLogo_c` (`preLoad_dyl`) warms some modules during the logo, and `dScnPly_c` has a `PreLoadInfo` table (`dylKeyTbl` = process names to link, `resNameTbl` = archives to load), currently populated for a single stage (`T_JOINT`, which loads the cow actor and the `Always` archive).

Because REL code is relocated at runtime, symbol addresses inside RELs are module-relative (`config/<ver>/rels/<rel>/…`). Cross-module calls go through relocations, not direct calls – which is why DOL-resident base classes matter.

## 3.8 From map data to living actors

Stage and room data are binary files inside the disc's archives (not in this repo). Roughly:

```
Stage archive  (Stg_00 = "current stage")  → stage.dzs   DZS = a table of 4-char-tagged chunks. Seen in src/d/d_stage.cpp:
        STAG  stage info (near/far planes, camera type, particle set, message group…)     SCLS  scene-exit list (where each exit leads)
        Env0/Col0/PAL0/LGT0/Virt/VRB0  lighting, colour-set selection, palettes, sky-box colours
        RTBL  which rooms exist / are read together       MULT  room placement offsets       PLYR  player spawn points
        RCAM/CAMR  camera definitions                     ACTR/TGOB  actors present regardless of room
        PATH/PPNT  paths & points (NPC routes, rails)     FLOR/DMAP  floor & dungeon-map data     REVT  map events (cutscene triggers)
        SOND/SON0…  sound/BGM regions                     TRES/TRE0  treasure chests               FILI/Door/Doo0  file list, door data
Room archive   "R##_##"                    → room.dzr    same mechanism: ACT0…/ACTR (actors), SCOB/TGSC/TGDR (scaled objects, triggers, doors),
                                                         TRE0 (chests), EVLY (event list), MPAT/MPA0 (map path), LBNK (resource "banks")…
Collision                                  → *.dzb       per-model triangle collision loaded by dBgW
Models / anims                             → .bmd/.bdl/.bck/.btk/.brk/… in actor archives (J3D formats)
```

(Chunk tags are four-character strings dispatched through `FuncTable` arrays in `src/d/d_stage.cpp`, e.g. `{"ACTR", dStage_actorInit_always}`, `{"STAG", dStage_stagInfoInit}`. Some tags exist once per story "layer" – the trailing character (`SON0`…`SONe`, `UNI0`…`UNI3`) is the layer number 0–14.)

The key spawn path (`src/d/d_stage.cpp`):

```
dStage_Create()
  → dStage_dt_c_stageLoader(stage.dzs)         parse chunks; each chunk → an *Init function
  → dStage_roomInit(startRoom)                 load the starting room's chunks
       → dStage_actorInit(..., ACTR data)      for every entry: skip if already-collected (dComIfGs_isActor)
            → dStage_actorCreate(entry, prm)   
                 → dStage_searchName(entry.name)      8-char object name ("Bans", "door", "kusax1") → l_objectName[] →
                                                      {proc name (fpcNm_*), sub-type}
                 → fopAcM_Create(procName, NULL, prm) create request → REL link → resource load → daXxx_Create
```

`l_objectName[]` (`d_stage.cpp`, ~lines 486–1400) is *the* map from level-editor object names to process names – e.g. `"Bans"→fpcNm_NPC_BANS_e` (Barnes), `"kusax1"→fpcNm_GRASS_e`, `"door"→fpcNm_DOOR20_e`. Object archives are then chosen by each actor (usually a fixed archive name in its create phase).

**Layers** (not to be confused with process layers): a stage/room has up to 15 *story layers*; `dComIfG_play_c::getLayerNo(...)` chooses one from event flags and time of day (e.g. Ordon Village has different actor sets on Day 1, Day 2, post-Twilight…). See the long `getLayerNo_common_common` in `src/d/d_com_inf_game.cpp` for the exact rules.

## 3.9 Rendering pipeline

Nothing draws immediately. Draw methods **record** into per-frame lists; a painter draws the lists in a fixed order.

* **Recording** – an actor's draw method typically does:
  ```cpp
  g_env_light.settingTevStruct(type, &current.pos, &tevStr);   // light/fog from environment
  dComIfGd_setListBG();  /* or setList(), setListDark(), setListSky()… selects the bucket */
  mDoExt_modelUpdateDL(mpModel);                               // adds model packets to the current J3DDrawBuffer
  dComIfGd_setList();
  ```
  or `dComIfGd_set2DOpa(&someDlst)` for 2D/screen-space `dDlst_base_c` objects with a `draw()` method. Lists live in `dDlst_list_c` (`d_drawlist.h`), inside `g_dComIfG_gameInfo.drawlist`. J3D packets are sorted into `J3DDrawBuffer`s (opaque = by material; translucent = by depth).
* **Buckets** (`dDlst_list_c::DB_*`): `SKY`, `BG` (+`DARK_BG`), `OPA` (+`DARK`), `XLU` (+`DARK`), `OPA_PACKET`, `FILTER`, `ITEM3D`, `INVISIBLE`, `Z_XLU`, `2D_SCREEN`, `MIDDLE`, `3D_LAST`, (Wii: `CURSOR`)… plus 2D lists (`2DOpa`, `2DOpaTop`, `2DXlu`) and a `Copy2D` list.
* **Painter** (`mDoGph_Painter`, `src/m_Do/m_Do_graphic.cpp`) per frame: begin render → 2D copy → shadow-texture pass → per camera: **sky** → **BG opaque** → "middle" → particles (pri 0, behind) → shadows → **opaque actors** (plus "dark world" variants for twilight) → packets → **BG translucent** → particles → **translucent actors** → motion blur / depth-of-field → invisible-object pass → filter list → particles (foreground, fog priorities, screen-space) → **2D UI** (`2DOpa`, `2DXlu`, meter/messages) → bloom → end render and `JFWDisplay` swap. Motion blur (`motionBlure`) and depth-of-field (`drawDepth2`) are applied to the 3D image before the UI.
* **Cameras and windows**: `dComIfGp_getWindow(i)`/`dComIfGp_getCamera(i)`; `view_class`/`camera_class` hold projection and view matrices; `dDlst_window_c` holds the viewport.
* **Models and animations** use J3D: `J3DModelData` (shared, from an archive) + `J3DModel` (per-instance) + `mDoExt_*Anm` wrappers for `bck` (skeletal), `btk` (texture scroll), `brk` (colour), `btp` (texture swap), `bpk` (material colour), `blk`, and `mDoExt_McaMorf*` (skeletal animation with blending and sound triggers).

## 3.10 Memory and threads

**Heaps** (`m_Do_machine.cpp`, `m_Do_ext.cpp`, built in `mDoMch_Create`; names shown in the debug heap displays):

| Heap | Role |
|------|------|
| Root / System | JKR root and system heaps (`JKRGetRootHeap`, `JKRGetSystemHeap`). |
| **Zelda heap** | Persistent engine data (kept for the whole session). |
| **Game heap** | Per-scene data; grows/frees as stages change (`mDoExt_getGameHeap`). |
| **Archive heap** | Mounted resource archives (`.arc`); tracked by `dRes_control_c`. |
| **J2D heap** | 2D UI resources. |
| **Command heap** | DVD-thread command objects. |
| **HostIO heap** | Debug tooling only. |
| Per-actor solid heap | Each actor's `heap` (`JKRSolidHeap`) via `fopAcM_entrySolidHeap`; freed as a block when the actor dies. |
| Extra: 2D `ExpHeap2D`, `MsgExpHeap`, room "memory blocks" (`dStage_roomControl_c::createMemoryBlock`) | UI/message/room-specific budgets. |

`mDoExt_setCurrentHeap(heap)` switches where `new` allocates; you will see the save/restore pattern around loads. Many crashes/matches hinge on which heap an allocation lands in.

**Threads**: main game thread (`main01`), DVD/loader thread (`m_Do_dvd_thread.cpp`, commands like `mDoDvdThd_mountArchive_c`/`toMainRam_c`/`callback_c` that game code queues and later `sync()`s), audio thread (JAudio2), memory-card thread (`m_Do_MemCard*.cpp`), DVD-error thread (`m_Do_DVDError.cpp`), plus SDK threads. That is why so much code has "start load / poll in later frame" structure.

**Resource loading** flow: `dComIfG_setObjectRes("Name", …)` / `dComIfG_resLoad(&phase, "Name")` → `dRes_control_c` (`d_resorce.cpp`) looks up or creates a `dRes_info_c` (ref-counted, up to 9-char archive name, `mCount`) → queues an archive mount on the DVD thread (`mDoDvdThd_mountArchive_c`, into the archive heap, either ARAM-backed or in-memory `JKRMemArchive`) → returns `cPhs_INIT_e` until done. Then code fetches by index: `dComIfG_getObjectRes("Name", dRes_INDEX_NAME_BMD_MODEL_e)`; the index enums are in `assets/<ver>/res/**` (e.g. `dRes_INDEX__BG0000_BMD_MODEL0_e`), which is what those generated headers are for.
