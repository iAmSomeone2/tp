# 4. Overviews of the game systems

Each section says **what the system does**, **the main classes/files**, **how gameplay code talks to it**, and **where to dig deeper**. Read [03-engine-architecture.md](03-engine-architecture.md) first; this page assumes the process model.

Contents: [Global state](#global-game-state-dcomifg_) · [Player](#the-player-d_a_alink) · [Actor families](#actor-families) · [Stages & rooms](#stages-rooms-and-layers) · [World collision](#world-collision-bg) · [Hit detection](#actor-vs-actor-hit-detection-cc) · [Camera](#camera) · [Targeting](#targeting-and-interaction-dattention) · [Events/cutscenes](#events-cutscenes-and-demos) · [Save data & flags](#save-data-and-flags) · [Environment](#environment-lighting-weather-time-twilight-kankyo) · [Rendering](#rendering-and-models) · [Particles](#particles) · [HUD & menus](#hud-and-menus) · [Messages](#messages-and-dialogue) · [Audio](#audio) · [Input](#input) · [Resources](#resource-loading) · [Debug tooling](#debug-and-developer-tooling) · [Misc](#other-notable-systems)

---

## Global game state (`dComIfG_*`)

`src/d/d_com_inf_game.cpp`, `include/d/d_com_inf_game.h` (≈4,900 lines of mostly inline accessors).

There is one global, **`g_dComIfG_gameInfo`** (`dComIfG_inf_c`):

| Member | Type | Holds |
|--------|------|-------|
| `info` | `dSv_info_c` | **Save data** and per-stage/zone flag memory (the `dComIfGs_*` accessors, "s" = save). |
| `play` | `dComIfG_play_c` | **Runtime play state** (the `dComIfGp_*` accessors, "p" = play): current/next stage, collision worlds (`dBgS`, `dCcS`), event control, attention, vibration, particles, camera/window/view, player pointers, item info, timers, loaded archives (message, fonts, menus…). |
| `drawlist` | `dDlst_list_c` | Per-frame draw lists (`dComIfGd_*`, "d" = draw). |
| `mResControl` | `dRes_control_c` | Loaded resource archives (`dComIfG_*Res*`). |

So to find "the way to read X", grep the accessor family: `dComIfGs_` for anything that persists in a save file (rupees, items, flags), `dComIfGp_` for "current session" state (the player actor, the camera, the current stage), `dComIfGd_` when you're adding something to a draw list, `dComIfG_` for resources and misc. Examples:

```cpp
dComIfGp_getPlayer(0)          // fopAc_ac_c* of Link (or wolf Link – same actor)
dComIfGp_getLinkPlayer()       // as daPy_py_c*
dComIfGp_getHorseActor()       // Epona
dComIfGp_getStartStageName()   // "F_SP103"
dComIfGs_isEventBit(0x4510)    // story flag
dComIfGs_getItem(slot, …), dComIfGs_getRupee(), dComIfGs_getLife()
dComIfG_Bgsp()                 // world collision (dBgS)
dComIfG_Ccsp()                 // hit-detection (dCcS)
```

`dComIfG_ct()` is called from `main()`; `dComIfGp_setNextStage(...)` requests a stage change.

---

## The player (`d_a_alink`)

Files: `src/d/actor/d_a_alink.cpp` (~19,700 lines) plus ~30 `d_a_alink_*.inc`, `include/d/actor/d_a_alink.h` (~8,400 lines), base class `d_a_player.h`.

* **One actor is both Link and wolf Link** (`daAlink_c : daPy_py_c : fopAc_ac_c`, profile `ALINK`, in the DOL). Wolf mode is a state (`checkWolf()`); "Midna" on the wolf's back is a separate actor (`d_a_midna`), as is the horse (`d_a_horse`, Epona) and boat (`d_a_canoe`).
* **Behaviour is a big state machine of "procs".** `enum daAlink_PROC` (`PROC_WAIT`, `PROC_MOVE`, `PROC_ATN_MOVE`, `PROC_SIDESTEP`, …) names each state; each has a pair `procXxxInit()` / `procXxx()` (start, then per-frame). `mProcID` is the current state and `mpProcFunc` the current per-frame method (a pointer-to-member). `commonProcInit()` runs on every transition. `checkNextAction*()` decides which proc to enter next from button input and context.
* **Split by topic into `.inc` files** that are `#include`d into the one big TU: `_link` (base movement/setup), `_cut` (sword), `_damage`, `_guard` (shield), `_bow`, `_boom` (Gale Boomerang), `_copyrod`, `_hvyboots`, `_bomb`, `_grab`, `_sumou` (sumo), `_horse`, `_canoe`, `_crawl`, `_hang`, `_swim`, `_iceleaf`, `_hook` (Clawshot), `_spinner`, `_bottle`, `_kandelaar` (lantern), `_whistle`, `_ironball`, `_demo`, `_effect`, `_wolf`, `_swindow`, `_HIO`/`_HIO_data` (developer-tuning parameter classes and their default-value tables – the constants gameplay code reads).
* **Animation** is driven by `daPy_frameCtrl_c` and `daAlink_ANM` IDs; upper- and lower-body animations blend (`mUnderFrameCtrl` / upper). Resources come from the `Alink` archives.
* **Collision**: `dBgS_LinkAcch` (ground/wall/roof), several `dCcD_*` colliders for sword/shield/body.
* **Interacts with everything** via `eventInfo` (doors, talking, item get), `dComIfGp_event_*`, the attention system, `daMidna_c`, camera hooks.

Questions to ask: *What proc handles X?* (grep `PROC_` names) · *What input triggers it?* (`checkNextActionFromButton`, `checkNextAction`) · *Which `.inc` owns that item?*

---

## Actor families

About 770 actor files under `src/d/actor/`; the [actor-index](actor-index.md) lists them all. By filename:

| Pattern | Family | Notes |
|---------|--------|-------|
| `d_a_alink`, `d_a_player`, `d_a_midna`, `d_a_horse`, `d_a_canoe` | Player & mounts | |
| `d_a_b_*` | **B**osses | Boss-fight controllers; sub-actors like `b_zant_magic`, `b_oh2` (tentacle). |
| `d_a_e_*` | **E**nemies | Two-letter code (`e_ba`= Keese, `e_dn` = Lizalfos, …) mostly derived from the original Japanese name. Base `fopEn_enemy_c`. |
| `d_a_npc_*` | **NPC**s (also animals/spirits) | Written against several base-class generations (`daNpcT_c`, `daNpcF_c`, `daNpcCd_c`, `daBaseNpc_c`). |
| `d_a_obj_*` | **Obj**ects | Doors of puzzles, switches, movable blocks, pots, fences, decorations, temples' set pieces (`lv1…lv9`), insects, collectables. ~350 files. |
| `d_a_tag_*` | **Tag**s | Invisible trigger volumes/controllers: event areas, message triggers, camera hints, howl points, evt/hint tags… They exist to *do* something when the player is inside/near, not to be seen. |
| `d_a_kytag*` | **Environment** tags (`ky` = kankyo) | Trigger weather/light/twilight/fog changes (`kytag04` = twilight portal, `kytag17` = light mask, …). Numbered, not named. |
| `d_a_door_*` | Doors | knob, shutter, spiral, boss, double. |
| `d_a_sw*`, `d_a_andsw*`, `d_a_alldie` | Switch logic | Sensors and logic gates (AND-switch: set flag B when flags A… are set; `alldie`: set a switch when every enemy in the room is dead). |
| `d_a_bg`, `d_a_bg_obj`, `d_a_set_bgobj` | **Background** | Static level geometry model + collision (`d_a_bg` = the room model), movable bg objects. |
| `d_a_grass` | Vegetation | Grass and flowers as one instanced actor (`d_grass.inc`, `d_flower.inc`). |
| `d_a_demo*`, `d_a_movie_player`, `d_a_title`, `d_a_scene_exit*`, `d_a_econt`, `d_a_suspend` | Scene/system actors | Cutscene actors, THP movie player, title screen, stage exits, encounter controller, actor "suspend" filter (`daSus_c`, consulted by `dStage_actorInit`). |
| `d_a_tbox*`, `d_a_shop_item`, `d_a_obj_item`, `d_a_itembase` | Items & chests | |
| `d_a_mg_*` | Minigames | Fishing (`mg_rod`, `mg_fish`, `mg_fshop`). Other minigames are spread across `obj_*`/`npc_*` actors. |

**Anatomy of a typical actor `.cpp`** (open `d_a_obj_swpush.cpp` as an example, top to bottom): includes `d/dolzel_rel.h` → class `Act_c`/`daXxx_c` methods: `Create` (parameters → `resLoad` phase → `fopAcM_entrySolidHeap` → create model/colliders → `fopAcM_SetMtx`/`fopAcM_SetMin/Max` cull → return `cPhs_COMPLETE_e`), `Execute` (logic, hit checks, `setBaseMtx`), `Draw` (env light setup → `dComIfGd_setList…` → `mDoExt_modelUpdateDL`), `Delete` (release resources: `dComIfG_resDelete`) → a static method table → `g_profile_*` at the bottom.

**Actor parameters.** The map editor stores a 32-bit `parameters` word, an `argument` byte, position/rotation/scale, and an event/switch binding per placed actor. Each actor decodes its own bit fields (`getSwNo()`, `prm_get_type()`, `fopAcM_GetParamBit(actor, shift, bits)`). To learn what a map value does, find the `getXxx()`/`prm_get_*` helpers near the top of the actor's `.cpp`/`.h`.

---

## Stages, rooms and layers

Files: `src/d/d_stage.cpp` (~4,500 lines), `include/d/d_stage.h`, `d_s_room.cpp`, `d_com_inf_game.cpp` (`getLayerNo*`).

* **Stage** = a level (Ordon Village, Faron Woods, a dungeon…). Named by an 8-char code: `F_SP103`, `R_SP01`, `D_MN05`… (decoder in [glossary](05-glossary.md#stage-codes)). A stage has one or more **rooms** (numbered; each room is a sub-area with its own actors/background) and a **spawn point** number.
* **Stage data** (`dStage_stageDt_c`) lives in `stage.dzs`; **room data** (`dStage_roomDt_c`) in each room's `room.dzr`. Both are lists of tagged chunks (see 3.8).
* **`dStage_roomControl_c`** tracks which rooms are loaded (status per room: memory block, actor list, background), which room Link is in (`getStayNo`), and room-visibility rules. Rooms load/unload as you move (`dStage_roomInit`, `fopScnM_CreateReq(ROOM_SCENE…)`).
* **Start/next stage**: `dStage_startStage_c` (name, spawn point, room, layer, dark-area) and `dStage_nextStage_c` (adds enable flag, wipe type/speed). `dStage_changeScene(exitId, …)` reads the `SCLS` exit table.
* **Layers** (0–14): alternate actor/lighting sets for the same stage depending on story progress and time of day. `dComIfG_play_c::getLayerNo(...)` computes it from flags such as "Cleared Faron Twilight", "Finished Ordon Day 2" – a good place to read the story-flag logic in plain-language comments.
* **Per-stage save-memory index**: `dStage_SaveTbl_*` (Ordon, Faron, Eldin, Lanayru, Field, Sacred Grove, Snowpeak, Castle Town, Desert, Fishing Pond, `LV1…LV9` dungeons, caves, grotto).
* **Restart / warp**: `dSv_restart_c`, `dSv_turnRestart_c`, `dSv_player_return_place_c`.

---

## World collision (BG)

"BG" = **b**ack**g**round: static and moving level geometry.

* **Data**: triangle-mesh collision from `.dzb` files (plus grid-based `KCol`, height fields `hf`, and deformable `deform`). Wrapped by `dBgW` (`d_bg_w*.cpp`, `dBgWBase`, `dBgWKCol`, `dBgWSv`) and registered into the world `dBgS` (`d_bg_s.cpp`, `dComIfG_Bgsp()`), derived from `cBgS` in SSystem.
* **Polygon info** (`cBgS_PolyInfo`) carries per-triangle attributes (ground type/"code", sound, camera, exit ID, wall type, …) that other systems read: footsteps, sliding, twilight, scene exits (`dStage_changeSceneExitId(polyInfo…)`).
* **Queries**: line checks (`dBgS_LinChk`, `fopAcM_lc_c`), ground (`dBgS_GndChk`, `fopAcM_gc_c`), roof (`dBgS_RoofChk`), water (`dBgS_WtrChk`, `fopAcM_wt_c`), sphere (`dBgS_SphChk`).
* **Actor-vs-world movement**: `dBgS_Acch` ("actor check") + `dBgS_AcchCir` cylinders perform per-frame ground/wall/roof/water collision for an actor and expose results (`GetGroundH()`, `ChkGroundHit()`, `ChkWallHit()`, …). Specialisations: `dBgS_ObjAcch`, `dBgS_LinkAcch`, `dBgS_HorseAcch`, `dBgS_BombAcch`, `dBgS_StatueAcch`.
* **Moving platforms**: `dBgS_MoveBgActor` base for actors that own a moving collision mesh (elevators, doors, ships). The world carries riders along (`MoveBgCrrPos`).
* **Pass filters**: `dBgS_PolyPassChk`, `dBgS_GrpPassChk` let queries ignore certain polygons/groups (e.g., wolf-only surfaces).
* `d_bg_plc`/`d_bg_pc` – the per-polygon attribute ("PLC") tables that ride along with each collision mesh.

---

## Actor-vs-actor hit detection (CC)

"CC" = **c**ollision **c**heck between actors. Colliders are simple shapes (**sphere**, **cylinder**, **capsule** (`Cps`), **triangle**, **point**) built on `cM3dG*` geometry. Each collider has three independent roles:

| Role | Name | Meaning |
|------|------|---------|
| **AT** | Attack | "I deal damage" – sword swing, enemy claw, bomb blast. Has attack type/power. |
| **TG** | Target | "I can be hit" – hurtbox. Has flags for what types (arrow, sword, boomerang…) affect it. |
| **CO** | Co-contact / body | "I block/push" – body collision between actors, push-out. |

* Types: `cCcD_*` (SSystem, generic) → `dCcD_*` (game: `dCcD_Sph`, `dCcD_Cyl`, `dCcD_Cps`, `dCcD_Tri`, plus `dCcD_Stts` status: weight, damage-reaction, movement).
* Usage pattern in an actor: describe colliders once in a static `dCcD_SrcCyl` table (`initCcCylinder`); each frame update position (`mCyl.SetC(pos)`) and register with `dComIfG_Ccsp()->Set(&mCyl)`; next frame read results with `ChkTgHit()`, `GetTgHitObj()`, `ChkAtHit()`, `ChkCoHit()`.
* The `dCcS` manager (`d_cc_s.cpp`, SSystem `cCcS`) runs all pairs once per frame after execute, using spatial division (`cCcD_DivideArea`), then calls `SetPosCorrect` for CO pushes. Limits: 0x100 AT, 0x300 TG, 0x100 CO objects.
* `d_cc_uty` – helpers (damage tables, `at_power_check`), `d_cc_mass_s` – mass-based pushing groups, `c_damagereaction.cpp` – common damage reactions.

---

## Camera

Two layers:

* **`camera_class` process** (`f_op_camera.cpp`, `fpcNm_CAMERA_e`): provides the view/projection matrices to the renderer.
* **`dCamera_c`** (`d_camera.cpp` ~11,000 lines, `d_cam_param.cpp`, `d_ev_camera.cpp` ~4,000 lines): the logic. It selects a **camera type** (from stage `CAMR`/`RCAM` data and map-tool IDs; `GetCameraTypeFromMapToolID/CameraName`), reads tuning parameters from the `CamParam` archive (loaded in `dScnPly_c::phase_1_0`; `d_cam_param.cpp`), and each frame calls the behaviour for the current mode:

  `chaseCamera` (default follow), `lockonCamera` (Z-target), `talktoCamera`, `subjectCamera` (first-person / aiming), `magneCamera` (Magnet Boots), `hookshotCamera`, `rideCamera` (horse), `railCamera`/`paraRailCamera` (on-rails), `oneSideCamera`, `fixedFrameCamera`/`fixedPositionCamera`, `manualCamera`, `observeCamera`, `colosseumCamera`, `towerCamera`, `eventCamera` (scripted by a cutscene) and `letCamera`.

  It also handles collision avoidance (`lineBGCheck`, `bumpCheck`), shaking (`StartShake`), trim/letterbox (`SetTrimSize`), lock-on hand-off, and hint/talk camera modes. **Event cameras** (`d_ev_camera.cpp`) implement the cutscene camera *actions* ("Teacher/Midna hint talk", pans, jumps…) requested by the event system.
* Debug: `d_debug_camera.cpp`, `d_jcam_editor.cpp` (JStudio camera editor).

---

## Targeting and interaction (`dAttention`)

`d_attention.cpp`, `d_att_dist.cpp`. Provides Z-targeting, talk prompts, the action prompt on A, and the yellow/red reticles.

* Each actor advertises what it is via `attention_info` (distances/positions per **attention type**: `LOCK`, `TALK`, `BATTLE`, `SPEAK`, `CARRY`, `DOOR`, `JUEL` (probably "jewel", i.e. rupee-like pickups), `ETC`, `CHECK`; flags `fopAc_AttnFlag_*`).
* `dAttention_c` builds candidate lists each frame (`makeList`), weights them by distance/angle (`calcWeight`, tables in `d_att_dist`), picks lock-on target, action target (A button `getActionBtnB`) and X/Y item targets (`getActionBtnXY`), and draws reticles (`dAttDraw_c`). Lock-on hold/switch modes: `judgementStatus4Hold/Switch`.
* Lock-on targets are exposed to actors via `LockonTarget()`/`LockonTargetPId()`.

---

## Events, cutscenes and demos

There are three cooperating systems.

1. **Event control** – `dEvt_control_c` (`d_event.cpp`; accessed via `dComIfGp_event_*`). Actors *order* events: `fopAcM_orderTalkEvent`, `…DoorEvent`, `…ItemEvent`, `…OtherEvent(actor, "eventName", …)`, `…ChangeEventId`. Orders are queued with priority and the controller starts one at a time in a **mode**: `WAIT`, `TALK`, `DEMO`, `COMPULSORY`. While an event runs, `dComIfGp_event_moveApproval(actor)` decides whether each actor may still `execute` (see `fopAc_Execute`) – this is how the world "freezes" during conversations and cutscenes. Each actor carries a `dEvt_info_c` (`eventInfo`) with its command/condition flags (`dEvtCmd_INTALK_e`, `dEvtCnd_CANTALK_e`, …).
2. **Event manager** – `dEvent_manager_c` (`d_event_manager.cpp`, `d_event_data.cpp`, `d_event_lib.cpp`). Data-driven cutscenes: event lists are binary data from the `Event` archive and stage/room files, organised as **events → staff → cuts**:
   * an **event** (`dEvDtEvent_c`) has a name and priority;
   * it lists **staff** (`dEvDtStaff_c`): cast members identified by a *name* such as `"Alink"`, `"Midna"`, `"CAMERA"`, `"ALL"`, `"TIMEKEEPER"`, or an actor's `getMyStaffId(...)`;
   * each staff has a sequence of **cuts** (`dEvDtCut_c`): an action name plus parameters (`dEvDtData_c`).
   * Actors participating in an event do: `int staff = dComIfGp_evmng_getMyStaffId("Name", this, 0); int act = dComIfGp_evmng_getMyActIdx(staff, actionNames, N, …); switch(act){…}` and then `dComIfGp_evmng_cutEnd(staff)` when a cut finishes. Data sources (`BASE_*`): keep, actor, stage, rooms 0–5, demo.
3. **Demo (movie-like scripted scenes)** – `dDemo_c` (`d_demo.cpp`) glues **JStudio** (`libs/JSystem/src/JStudio*`, STB files) to the game via JStage adapters: `dDemo_actor_c : JStage::TActor`, `dDemo_camera_c`, `dDemo_light_c`, `dDemo_ambient_c`, `dDemo_fog_c`. An STB timeline moves actors/camera/lights, plays sounds/messages and spawns particles. Event cuts can start/wait on demos. Actors expose themselves to demos via `demoActorID`.

Event **flags** and "switches" are the persistent side of this (next section). Skipping cutscenes: `dEvt_control_c::setSkipProc/skipper`.

---

## Save data and flags

`include/d/d_save.h`, `src/d/d_save.cpp`, `d_save_init.cpp`, memory card in `src/m_Do/m_Do_MemCard*.cpp`, UI in `d_file_select.cpp`, `d_menu_save.cpp`.

* **`dSv_info_c` = everything the game remembers**, accessed with `dComIfGs_*`:
  * `mSavedata` (`dSv_save_c`, 0x958 bytes; this is what is written to a slot):
    * `mPlayer` (`dSv_player_c`): status (life, rupees, max, wallet…), items and slots (`dSv_player_item_c`), collectibles (`_collect_c` – swords, shields, clothes, Twilight mirror shards…), Poe count, letters, fishing records, config, wolf data, light drops (`dSv_light_drop_c`), return/horse place, last-stay info.
    * `mSave[STAGE_MAX]` (`dSv_memory_c`, one per stage: chests, switches, items picked up, keys, dungeon map/compass/boss key bits).
    * `mSave2[…]` (`dSv_memory2_c`, an 8-byte "visited room" bitmap per area used by the map),
    * `mEvent` (`dSv_event_c`, **256 bytes of story flags**),
    * `mMiniGame` (`dSv_MiniGame_c`).
  * Runtime-only current-stage copies: `mMemory` (current stage's memory), `mDan` (dungeon bits), `mZone[ZONE_MAX]` (per-zone temporary bits), `mRestart`, `mTurnRestart`, `mTmp` (temporary event bits), flag-file tools.
* **Event bits** (`dComIfGs_isEventBit(0x4510)` / `onEventBit`): a 16-bit ID = `(byteIndex << 8) | bitMask`. So `0x4510` = byte `0x45`, mask `0x10`. `dSv_event_flag_c::saveBitLabels[]` gives named indices; comments beside uses tell the story meaning ("Epona Tamed", "Cleared Forest Temple").
* **Switches** (`dComIfGs_isSwitch(no, room)`): one number space partitioned by range – `0x00–0x7F` saved per stage, `0x80–0xBF` per dungeon (`DAN_SWITCH`), `0xC0–0xDF` per zone (`ZONE_SWITCH`), `0xE0–0xEF` per-zone "one-shot" (`ONEZONE_SWITCH`); `0xFF`/`-1` = none. Map-placed switch actors and "AND switch" logic use these.
* **Items**: numeric IDs `dItemNo_*_e` in `include/d/d_item_data.h`; item-get/apply logic in `d_item.cpp` (`execItemGet`). Slots: `ItemSlots` enum in `d_save.h`.
* **Actors already collected/killed** are remembered by `setID` (`dComIfGs_isActor/onActor`), so a placed heart piece doesn't respawn.
* Wolf/human etc: `TF_STATUS_*` (transform state), `Wallets`, `Swords`, `Shields`, `Clothes`.

---

## Environment: lighting, weather, time, twilight (`kankyo`)

"Kankyo" (環境) = "environment". Files: `d_kankyo.cpp` (+ `_wether` (sic), `_rain`, `_data`, `_debug`), `d_kyeff*.cpp`, `d_ky_thunder.cpp`, and `d_a_kytag*` actors; `include/d/d_kankyo.h`.

* **`g_env_light`** (`dScnKy_env_light_c`) is the singleton: current time of day, colour sets, sun/moon, fog, sky-box colours (`vrbox`), rain/snow/lightning, wind, lantern/dungeon/boss/point lights, dark-world ("twilight") state, and the palette selection from stage data (`Env0/Col0/PAL0/LGT0/VRB0` chunks → `dStage_paletteInfo…`).
* Runs as a **process** (`fpcNm_KANKYO_e`, `f_op_kankyo`), created in `dStage_Create` via `dKankyo_create()`; extra effect processes `KYEFF/KYEFF2/KY_THUNDER/ENVSE` (env sound effects, `d_envse.cpp`).
* Every actor has a **`tevStr` (`dKy_tevstr_c`)** filled by `g_env_light.settingTevStruct(type, &pos, &tevStr)` so models are lit and fogged consistently; the type picks a lighting model (player, majority-object "MAJI", BG, etc.).
* Point lights: `dKy_plight_set/cut` (`LIGHT_INFLUENCE`), efflights, "dalkmist" (twilight mist), wind `WIND_INFLUENCE`.
* **Twilight**: `dKy_darkworld_Area_set`, `dKy_darkworld_check` + "dark" draw lists (`OpaListDark`, …) and layer selection. `d_a_kytag04` etc. drive the portal and spread.
* **Time of day**: game clock `dKy_getdaytime_hour()`, `dKy_set_nexttime`, `dKy_instant_timechg`; layers use it to pick day/night variants.

---

## Rendering and models

See 3.9. Additional pointers:

* **Model creation** helper flow in actors: `J3DModelData* data = (J3DModelData*)dComIfG_getObjectRes("Arc", BMD_INDEX); mpModel = mDoExt_J3DModel__create(data, 0x80000, 0x11000084);` then animations via `mDoExt_bckAnm`, `mDoExt_btkAnm`, `mDoExt_McaMorf` (+ `Z2SoundObjAnime` for sound-synced animation).
* **Matrices**: `mDoMtx_stack_c` (`m_Do_mtx.h`) is the global matrix stack helper used everywhere (`mDoMtx_stack_c::transS(pos); ::YrotM(angle); mpModel->setBaseTRMtx(mDoMtx_stack_c::get());`). Types: `cXyz` (float vec), `csXyz` (short angles; 0x10000 = 360°), `cSAngle`, `cM3dG*` shapes.
* **Draw sorting**: bucket + `J3DDrawBuffer`; "invisible" (Twilight/wolf-sense) bucket; Z-sorted translucent (`DB_LIST_Z_XLU`).
* **Shadows**: `dDlst_shadowControl_c` (simple blob shadows + real-time shadow textures), `dComIfGd_imageDrawShadow`.
* **Wide screen / Wii**: `WIDESCREEN_SUPPORT` builds; `mDoGph_gInf_c::isWide*`.
* **Simple models** (`d_simple_model.cpp`, `dSmplMdl_draw_c`): cheap shared draws for repeated map dressing. `d_model.cpp` (`dMdl_mng_c`): a shared manager for lightweight model draws.
* Grass/flower fields are special: `d_a_grass` with `d_grass.inc`/`d_flower.inc`, packets `dGrass_packet_c` (120 kB) / `dFlower_packet_c`.

---

## Particles

JSystem **JPA** (`libs/JSystem/src/JParticle`) wrapped by `dPa_control_c` (`d_particle.cpp`; `dComIfGp_particle_*`). Effects are `.jpc` resource files ("particle sets": a *scene* set chosen by the `STAG` chunk plus a common set); each effect has a numeric ID with a name enum in `d_particle_name.h` (names are made up from debug strings, e.g. `ID_AK_JN_O_APPEARLUPY`). Spawning: `dComIfGp_particle_set(id, &pos, &tevStr, &angle, …)` returns an emitter ID; callbacks (`dPa_levelEcallBack`) let actors control emitters. Drawing is phased into the draw pipeline (`particle_draw*`, fog priorities, dark-world, screen-space). `d_particle_copoly.cpp` spawns surface-dependent effects (dust, splashes) from polygon info.

---

## HUD and menus

* **HUD** – `dMeter2_c` (`d_meter2*.cpp`, process `METER2`, a `msg_class`): hearts, rupees, A/B/X/Y/Z item icons, minimap (`d_meter_map`), keys, oxygen, arrow count, `haihai` (horse-riding call/reins) and `hakusha` (horse-drawn cart) gauges, etc. `dMeter2Info_c` (`d_meter2_info.cpp`) is its global data hub. It is created by the PLYR init after Link spawns and in turn creates the menu window and message object.
* **Menu window** – `dMw_c` (`d_menu_window.cpp`, process `MENUWINDOW`) is the pause-menu state machine. Screens: `dMenu_Ring_c` (the **item wheel/ring**), `dMenu_Collect_c` (Collection screen), `dMenu_Fmap_c` (field map) and `dMenu_Dmap_c` (dungeon map), `dMenu_Option_c`, `dMenu_save_c`, `dMenu_Skill_c` (Hidden Skills), `dMenu_Insect_c` (Agitha's bugs), `dMenu_Letter_c`, `dMenu_Fishing_c`, `dMenu_Calibration_c`, `dMenu_Quit_c`, item explanation.
* **2D UI framework** – JSystem J2D screens loaded from `.blo` layouts in archives, wrapped by `CPaneMgr` / `CPaneMgrAlpha…` (`d_pane_class*.cpp`) for fade/scale/position helpers. Screens are drawn via `dDlst_*` 2D lists (`2DOpa`, `2DXlu`). `dSelect_cursor_c` and `dSelect_icon_c` are reusable selection widgets; Wii adds `dCsr_mng_c` (pointer cursor).
* **Others**: `dTimer_c` (minigame timers/counters), `d_gameover.cpp`, `d_error_msg.cpp` (DVD error / shutdown screens; `dDvdErrorMsg_c`), `d_file_select.cpp` (file select scene UI), `d_name.cpp` (name entry), `d_bright_check.cpp` (brightness calibration), `d_scope.cpp` (Hawkeye zoom).

---

## Messages and dialogue

* Text is stored in **BMG** message archives (per message group) and processed with **JMessage** (`libs/JSystem/…/JMessage`) – control codes for colour, icons, waits, choices, variable substitution.
* `dMsgObject_c` (`d_msg_object.cpp`, process `MSG_OBJECT`) is the single text-box controller. State procs: `waitProc`, `openProc`, `outnowProc`, `stopProc`, `selectProc`… An actor triggers text with `fopMsgM_messageSet(messageID, actor, …)` (or `…SetDemo` for cutscenes); `fopMsgM_messageGet` fetches a string.
* **Screens by kind** (`d_msg_scrn_*.cpp`): `talk`, `item` (item-get), `kanban` (signs), `jimaku` (subtitles), `boss`, `howl` (wolf-howl text), `place` (area names), `staff` (credits), `tree`, `explain`, `3select` (choice boxes), `arrow`, `light`.
* **Message flow** – `dMsgFlow_c` (`d_msg_flow.cpp`): a small node-graph interpreter attached to messages: node types `MESSAGE`, `BRANCH` (conditions on flags/items), `EVENT` (call an action such as "give item", "set flag", "open shop"). Most NPC conversation branching lives in the *data*, and the C++ implements the condition/event handlers.
* Text output uses `COutFont_c` for inline button icons; ruby (furigana) and language-specific font archives; message-group archive loaded per stage (`dMsgObject_readMessageGroup`).
* Sizes: the message heap is separate (`MsgExpHeap`, 0xA800) from the 2D heap (0xBB800).

---

## Audio

Two layers: **JAudio2** (`libs/JSystem/src/JAudio2`, generic engine – sequencer, waves, banks, DSP) and **Z2AudioLib** (`src/Z2AudioLib`, Zelda-specific).

* `mDoAud_zelAudio_c : Z2AudioMgr` (`m_Do_audio.cpp`) creates everything and loads `Z2Sound.baa`; `mDoAud_Execute()` runs each frame; helpers `mDoAud_seStart/bgmStart/…`.
* `Z2AudioMgr` inherits `Z2SeMgr` (sound effects), `Z2SeqMgr` (BGM sequences, battle music), `Z2SceneMgr` (per-stage/room music & reverb; `mDoAud_setSceneName`), `Z2StatusMgr` (day/night, wolf state…) and `Z2SoundObjMgr`. Alongside them are `Z2SoundMgr`, `Z2EnvSeMgr` (ambient sounds), `Z2SpeechMgr2` (NPC voice babble), `Z2WolfHowlMgr` (howl melodies), `Z2Audience` (3D listener) and `Z2FxLineMgr` (reverb/effects lines).
* **Sound objects**: actors own a sound object (`Z2CreatureLink`, `Z2CreatureEnemy`, `Z2CreatureCitizen`, `Z2SoundObjSimple`, `Z2SoundObjAnime`, …) that positions sounds on the actor and starts named SEs: `mSound.startCreatureSound(Z2SE_EN_KK_ATTACK, …)`. **`Z2SE_*` names** start with a category (by usage count: `EN` enemy, `OBJ` object, `AL` Link, `SY` system/UI, `HIT`, `WL` wolf, `CM` common, `MDN` Midna, `FN` footsteps, `ENV`, plus per-character ones like `KOSARU` monkeys, `GORON`, `INSCT` insects) and, for enemies, continue with the actor's two-letter code (`Z2SE_EN_KK_ATTACK` → `e_kk`), which is a handy way to identify unknown actors.
* Each actor TU that uses Z2 needs the macro `AUDIO_INSTANCES;` (`Z2Instances.h`) – it forces template static-member instantiation so weak-symbol ordering matches the original binary.
* `Z2AudioCS` = Wii-remote speaker.

---

## Input

* GameCube pads: `mDoCPd_c` (`m_Do_controller_pad.cpp`) wraps `JUTGamePad`; `mDoCPd_c::getTrig/getHold/getStickX…` per port. Wii remote/Nunchuk/pointer: `mReCPd` (`m_Re_controller_pad.cpp`), `d_cursor_mng.cpp` (`dCsr_mng_c`).
* Actors read buttons through `mDoCPd_c` or `dComIfGp_*` wrappers; Menu code uses a stick-to-direction helper class `STControl` (declared in several menu headers; not yet defined in the repo).
* Reset/HOME button handling: `mDoRst` (`m_Do_Reset.cpp`), `d_home_button.cpp` (Wii).

---

## Resource loading

* Resources live in **archives** (`.arc`, RARC format; Yaz0-compressed variants, optionally ARAM-resident) opened through `JKRArchive` and managed by `dRes_control_c` (`d_resorce.cpp`): **Object archives** per actor/model set (`dComIfG_resLoad(&phase, "Alink")`), **stage archives** (`dComIfG_getStageRes("stage.dzs")`), **layout/UI archives** (`Layout/*`, `FieldMap`, `Msgus`, `Fontus`, `CardIcon`…), and the always-loaded `Always` archive for shared models/textures.
* Contents are accessed by **index** (`dRes_INDEX_<ARC>_<TYPE>_<NAME>_e` in `assets/<ver>/res/`) or by name.
* Asynchronous loads run on the DVD thread (`m_Do_dvd_thread.cpp`). Ref counts mean an archive is shared between all actors that use it and freed when the last one deletes.
* Stage "banks": `bank.bin`/`name.bin` (see `dStage_roomControl_c`) say which object archives a room needs.
* File types you will meet: `.arc` archive · `.bmd/.bdl` model · `.bck` skeletal anim · `.btk` texture-SRT anim · `.brk` TEV-colour anim · `.btp` texture-pattern anim · `.bpk` colour anim · `.blk` cluster anim · `.blo` 2D layout · `.bti/.timg` texture · `.dzs/.dzr` stage/room data · `.dzb` collision · `.stb` cutscene timeline · `.bmg` messages · `.jpc` particle set · `.baa/.bms/.aw` audio archives/sequences/waves · `.rel` code module.

---

## Debug and developer tooling

Compiled mostly under `#if DEBUG` (ShieldD, or `--debug`), and a big source of intact names:

* **HostIO** (`JORReflexible`, `genMessage`, `mDoHIO_*`, `*_HIO_c` classes): each system exposes tunable parameters (e.g., `daAlinkHIO_c`, `dKankyo_*HIO_c`, `dBgS_HIO`). Hundreds of retail constants originate here.
* **Register HIO** (`g_regHIO`, macros `TREG_F(i)`, `DREG_S(i)`… in `d_s_play.h`): scratch "register" values developers use for tuning; sorted by letter per person/team. `tools/utilities/greg_calc.py` helps compute offsets.
* **Debug menus/viewers**: `d_s_menu.cpp` ("Debug Level Select Menu" scene), `d_debug_pad.cpp`, `d_debug_viewer.cpp`, `d_debug_camera.cpp`, `d_jpreviewer.cpp`, `d_event_debug.cpp`, `d_kankyo_debug.cpp`, `f_op_actor_map.cpp`, `f_pc_debug_sv.cpp`.
* **On-screen profiling & heap tools**: `JUTProcBar`, `JUTDbPrint`, `HeapCheck`, `FixedMemoryCheck`, `fapGm_HIO_c` (CPU timers, capture).

---

## Other notable systems

| System | Where | Summary |
|--------|-------|---------|
| Shops | `d_shop_system.cpp`, `d_shop_item_ctrl.cpp`, `d_shop_camera.cpp`, `d_a_shop_item*` | Item display/purchase logic used by Kakariko/Castle Town/Goron/Sky shops. |
| Treasure chests | `d_tresure.cpp`, `d_a_tbox*` | Chest state (`dTres_c`) + chest actors. |
| Doors & scene exits | `d_a_door_*`, `d_door_param2.cpp`, `d_a_scene_exit*` | Door parameter decoding; exits between rooms/stages. |
| Paths | `d_path.cpp`, `d_spline_path.cpp` | Route data (PATH/PPNT chunks) for NPCs, cameras, rails. |
| Maps | `d_map*.cpp`, `d_menu_fmap*.cpp`, `d_menu_dmap*.cpp` | Minimap, field map and dungeon map drawing from `DMAP`/`FLOR` and archive map data. |
| Bombs & items in world | `d_bomb.cpp`, `d_item.cpp`, `d_a_obj_item`, `d_a_itembase` | Bomb behaviour, drop/pickup handling. |
| Vibration | `d_vibration.cpp`, `d_vib_pattern.cpp` | Rumble patterns. |
| Insects / Golden bugs | `d_insect.cpp`, `d_a_obj_*` insect actors | Shared insect data. |
| Eye highlight | `d_eye_hl.cpp` | Eye-shine on character models. |
| Screen wipes | `d_ovlp_fade*.cpp` | Overlap processes: fade, iris, transitions between stages. |
| Fishing | `d_a_mg_rod`, `d_a_mg_fish`, `d_menu_fishing` | Fishing minigame (rod physics, fish AI, record menu). |
| Wolf senses / Midna | `d_a_midna`, `d_a_alink_wolf.inc`, `Z2WolfHowlMgr` | Wolf sense vision, howling songs, Midna's help. |
| Copy-rod / Statues / Spinner | `d_a_crod`, `d_a_cstatue`, `d_a_spinner` | Dominion Rod, statue control, Spinner. |
| THP movies | `d_a_movie_player.cpp` | Video playback. |
| Wii-only | `d_cursor_mng`, `d_home_button`, `d_rvl_fb_copy`, `Z2AudioCS`, `m_Re` | Pointer cursor, HOME menu, framebuffer copy, remote speaker/pad. |
