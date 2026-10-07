# 6. Exploring the code: recipes, worked examples and good questions

How to go from a question ("where does X happen?") to the right file, plus a list of gotchas that trip up newcomers. Everything here uses commands and paths that exist in the repo.

## Golden rules

1. **Start from a name, not a directory.** Almost everything is findable by grepping a distinctive identifier (`fpcNm_…`, `dItemNo_…`, `Z2SE_…`, a `g_profile_…`, a stage name string).
2. **Header first.** `include/…/*.h` shows the class layout and public API with offsets; the `.cpp` shows behaviour. For actors the header often has a `@brief` in-game name.
3. **Follow the process, not the include graph.** To understand *when* code runs, find the object's profile (`g_profile_*`) and its method table (`create/execute/draw`); to understand *what data it uses*, find its parameter getters and archives.
4. **Data isn't here.** Stage layouts, message text, animation names, event scripts, and model contents come from the disc. The code tells you *how* they're interpreted (chunk tags, parameter bit fields, index enums in `assets/`), not the values.
5. **Beware version conditionals.** Look for `#if VERSION`, `PLATFORM_*` and `#if DEBUG` before trusting a block; enum values shift.
6. **Names can be wrong.** Community names (`@brief`, `field_0x…`, `/* … ? */`) are best guesses. Cross-check with sound IDs, resource names and debug strings (Japanese strings in `OS_REPORT` are gold).

## Grep recipes

```sh
# Where is an actor's process ID defined, listed, mapped to a REL, and implemented?
grep -rn "fpcNm_ALINK_e"        include src            # enum entry + every place it's used
grep -rn "g_profile_ALINK"      src                    # the profile
grep -n  "d_a_obj_swpush"       src/c/c_dylink.cpp configure.py   # REL name table + build entry

# Who spawns actor X?  Who creates the HUD?  Who requests a scene change?
grep -rn "fopAcM_create(fpcNm_Obj_Yousei_e\|fopAcM_Create(fpcNm_Obj_Yousei_e" src
grep -rn "fopMsgM_Create(fpcNm_METER2_e" src
grep -rn "fopScnM_ChangeReq" src

# A story flag or item
grep -rn "0x4510"        src include                   # raw event bit
grep -rn "dItemNo_KANTERA_e" src | head                # item ID users
grep -rn "saveBitLabels\[47\]" src                     # named event-flag index

# Sound / particle / archive names → who uses them
grep -rln "Z2SE_EN_KK_ATTACK"  src include
grep -rn  "ID_AK_JN_O_APPEARLUPY" src
grep -rn  "\"Alink\""          src

# A stage name string (see 05-glossary.md for meanings)
grep -rn "\"F_SP103\"" src
grep -rn "\"D_MN05\"" src

# All actors of a family or all users of a system
ls src/d/actor | grep '^d_a_e_'
grep -rln "dComIfG_Ccsp()->Set" src/d/actor | wc -l       # actors with colliders
grep -rn  "dComIfGp_event_runCheck" src | head            # code that reacts to running events
grep -rn  "mDoAud_seStart" src | head

# Parameters of an actor
grep -n "fopAcM_GetParamBit\|prm_get\|getSwNo" include/d/actor/d_a_andsw.h
```

Where a symbol lives in the *original binary*:

```sh
grep -n "^fopAc_Execute\|fopAc_Execute" config/GZ2E01/symbols.txt       # address, size, scope
grep -n "f_op/f_op_actor.cpp" config/GZ2E01/splits.txt                   # address ranges per section
ls config/GZ2E01/rels/d_a_andsw/                                          # a REL's own splits/symbols
```

## Worked examples

### A. "Where does Link's sword attack get decided?"

1. `include/d/actor/d_a_alink.h` → `enum daAlink_PROC` shows states (`PROC_CUT_…`, `PROC_ATN_…`).
2. `src/d/actor/d_a_alink_cut.inc` holds the sword procs (`procCut…Init/procCut…`).
3. Entering them: `daAlink_c::checkNextAction*` in `d_a_alink.cpp`/`_link.inc` (`checkNextActionFromButton`).
4. Damage numbers come from HIO tables (`d_a_alink_HIO_data.inc`, `daAlinkHIO_cut*_c`), hits go through `dCcD_*` colliders (`d/d_cc_d.h`), and enemy reactions in each `d_a_e_*.cpp` (`ChkTgHit()` / `GetTgHitObj()`).

### B. "What happens when I walk through a door / off the edge of an area?"

1. Map data puts a `d_a_scene_exit` (SCEX) actor or a door actor at the boundary; `src/d/actor/d_a_scene_exit.cpp` checks whether Link is inside its box and calls `player->onSceneChangeArea(...)`.
2. Link's execute code (`d_a_alink.cpp` near `dStage_changeScene(`) or an event calls `dStage_changeScene(exitId, …)` (`src/d/d_stage.cpp`), which looks up the `SCLS` exit entry and calls `dComIfGp_setNextStage(...)`.
3. `dScnPly_c::execute` (`src/d/d_s_play.cpp`, "`dComIfGp_isEnableNextStage()`") starts an **overlap** (wipe) and calls `fopScnM_ChangeReq(scene, fpcNm_PLAY_SCENE_e, …)`.
4. The old play scene's layer is deleted (all actors gone); a new `dScnPly_c` runs `phase_*` → `dStage_Create()` → `dStage_actorInit` spawns the new stage's actors.
5. Player spawn: `dStage_playerInit` picks the `PLYR` entry for `startPoint`, creates `ALINK`, then creates the HUD (`fopMsgM_Create(fpcNm_METER2_e…)`).

### C. "How does this NPC hold a conversation?"

Take `src/d/actor/d_a_npc_ash.cpp` (Ashei) as a template:

1. Create → sets up model/animations/colliders and an `eventInfo` name; `Execute` calls a *talk state machine* (`daNpcAsh_c::Execute`, `talk()`).
2. Talk is initiated through the event system (`fopAcM_orderTalkEvent`, `dEvt_control_c::talkCheck` in `d_event.cpp`) after the attention system (`d_attention.cpp`) offered the talk prompt (`fopAc_AttnFlag_TALK_e`).
3. The dialog text is chosen by **message ID** (`fopMsgM_messageSet(id, …)`) – often a function of story flags – and the branching inside a message comes from `dMsgFlow_c` node data (`d_msg_flow.cpp`).
4. Cutscene-driven behaviour uses `dComIfGp_evmng_getMyStaffId("Name", this, 0)` and `getMyActIdx(...)` to receive commands from the event data (see [04 – Events](04-game-systems.md#events-cutscenes-and-demos)).

### D. "Why isn't my actor in the game / how do I add one?"

An actor needs (a) a profile (`g_profile_Xxx`) with a unique name in `f_pc_name.h` and a slot in `g_fpcPfLst_ProfileList[]` (the list is indexed by the enum, so **the order must match**); (b) an `ActorRel(...)` line in `configure.py` (or DOL registration); (c) a `DynamicNameTable` entry in `c_dylink.cpp`; (d) a draw-priority entry in `f_pc_draw_priority.h`; and (e) an object-name mapping in `l_objectName[]` (`d_stage.cpp`) so map data can place it. Look at how a recent small actor (e.g. `d_a_obj_swpush.cpp`, `d_a_andsw.cpp`) touches each of those places with `grep -rn "Swpush"`.

### E. "What lights/fogs this model?" 

Find the model's `Draw`: `g_env_light.settingTevStruct(type, &pos, &tevStr)` then `setLightTevColorType_*`; read `dScnKy_env_light_c::settingTevStruct` in `src/d/d_kankyo.cpp`. Colours come from the stage's palette chunks (`PAL0`, `Col0`, `Env0`) and the time of day.

### F. "Where does story flag 0x… get set?"

`grep -rn "onEventBit(0x…)"`, or the labelled form `saveBitLabels[N]`; the comment next to each use usually states the story meaning. The list `dSv_event_flag_c::saveBitLabels` (in `d_save.cpp`) maps an index to the raw bit.

### G. "I want to help decompile a function"

1. Pick a TU marked `Object(NonMatching, …)` in `configure.py` (or one that's `MatchingFor` GCN only, for Wii/Shield work). Read its symbol addresses in `config/<ver>/splits.txt`.
2. `python configure.py --version GZ2E01 && ninja`, open the project in **objdiff**, choose the object, iterate until the function diff is clean.
3. Use `tools/decompctx.py src/d/<file>.cpp` to produce a context file for a **decomp.me** scratch (`scratch_preset_id` 69 = DOL, 70 = REL).
4. Keep names consistent with 05-glossary and the existing style; consult <https://zsrtp.link/contribute> and the Discord (link in `README.md`).

## Gotchas

* **Enum/offset drift between versions** (see 01). A `/* 0xNN */` comment may be GCN-only; `/* 0xNN (0xMM) */` gives both.
* **`.inc` files are not standalone.** `d_a_alink_*.inc`, `d_grass.inc`, `d_flower.inc`, `kn_*.inc`, `zra.inc`, HIO data `.inc` files are `#include`d in the middle of a `.cpp`; edit/read them in that context.
* **Weak-function order and "AUDIO_INSTANCES"** – match-critical oddities (dummy functions such as `dummy()`/`dummy2()`/`dummy5()` exist only to influence compiler output order or pull in inline functions).
* **`JUT_ASSERT(line, …)` line numbers** are literal; don't renumber them.
* **Misspellings are canonical** (`d_resorce`, `d_tresure`, `_wether`, `Distanse`). This fork renamed one, `cPhs_COMPLEATE_e`, to `cPhs_COMPLETE_e`.
* **Two vocabularies of "layer"** (process layers vs. story layers) and **of "scene"** (`scene_class` process vs. stage) – see glossary.
* **Actors are addressed by ID**, not pointer: `fpc_ProcID` + `fopAcM_SearchByID`; pointers may dangle after deletion.
* **Debug-only code** (`#if DEBUG`) is often the only place a subsystem's real names survive.

## Questions to ask next (for humans and agents)

Use these to drill deeper – each maps to a concrete starting file.

**Process framework**
* What exactly happens between `fpcM_Create` and the first `execute` of an actor? (`f_pc_create_req.cpp`, `f_pc_stdcreate_req.cpp`, `f_pc_creator.cpp`)
* How do layers get torn down, and in what order are children deleted? (`f_pc_layer.cpp`, `f_pc_deletor.cpp`, `f_pc_node.cpp`)
* What do the 16 execution lines mean, and which list IDs do enemies vs. NPCs vs. objects use? (`grep "List ID"`)
* How do pause flags interact with `fopAcStts_NOPAUSE_e` / `FREEZE`? (`f_pc_pause.cpp`, `f_op_scene_pause.cpp`)

**World & simulation**
* How does `dBgS_Acch` resolve wall/ground/roof each frame (order, cylinder use, water)? (`d_bg_s_acch.cpp`)
* How are moving-platform riders carried? (`d_bg_s_movebg_actor.cpp`, `dBgS::MoveBgCrrPos`)
* Which `cCcD_Obj` flags decide AT vs. TG interactions (hit types, weapon flags, damage-reaction)? (`d_cc_d.cpp`, `c_cc_d.cpp`, `d_cc_uty.cpp`)
* How is the room streaming decision made (which rooms are loaded)? (`d_stage.cpp`: `dStage_roomControl_c`, `d_s_room.cpp`)
* How are layers chosen from story flags? (`d_com_inf_game.cpp` `getLayerNo_common_common`)

**Player**
* Which procs exist for a given item and what buttons enter them? (`d_a_alink*.inc`, `checkNextAction*`)
* How does human ↔ wolf transformation work (proc, model swap, Midna)? (`d_a_alink_wolf.inc`, `d_a_midna.cpp`)
* How does Link ride Epona (state sharing, `d_a_alink_horse.inc`, `d_a_horse.cpp`)?

**Presentation**
* How does the camera pick its style, and how do event cameras override it? (`d_camera.cpp` `Run`, `onTypeChange`, `d_ev_camera.cpp`)
* What is the complete draw order for one frame, and where do particles slot in? (`m_Do_graphic.cpp` `mDoGph_Painter`)
* How does `dKy` compute colours from palettes + time + weather? (`d_kankyo.cpp`, `d_kankyo_data.cpp`, `d_kankyo_wether.cpp`)
* How does the text box parse control codes and choose fonts/screens? (`d_msg_object.cpp`, `d_msg_class.cpp`, `d_msg_scrn_*.cpp`, `JMessage`)
* How are menus composed from `.blo` panes and `CPaneMgr`? (`d_menu_ring.cpp`, `d_pane_class.cpp`)

**Data & saves**
* What is the on-disk layout of a save slot, checksum included? (`d_save.cpp`, `m_Do_MemCard*.cpp`)
* Which event bits / switches gate a given puzzle? (grep `saveBitLabels`, `dComIfGs_isSwitch` in the actor)
* How are per-stage/dungeon memory bits initialised and swapped on stage change? (`dSv_info_c::init`, `putSave`/`getSave`, `dStage_Delete`)

**Loading**
* Which REL/archives does stage X preload? (`d_s_play.cpp` `PreLoadInfo`, `cDyl_*`, actor create phases)
* How are Yaz0 archives read, cached, and freed? (`JKRArchive`, `JKRDvdRipper`, `d_resorce.cpp`)

**Audio**
* How does `Z2SceneMgr` choose BGM and reverb for a stage/room? (`Z2SceneMgr.cpp`, `mDoAud_setSceneName`)
* How do enemies map to `Z2Creature*` sound objects? (`Z2Creature.cpp`, `Z2SoundObject.cpp`)

**Build / matching**
* Why does this TU only match for GCN? What's blocking the Wii build? (`configure.py` comments; compare `MatchingFor` lists; `objdiff` on the Wii version)
* What differs between GCN and Wii in this class (offsets, `#if PLATFORM_WII` blocks)?

## Regenerating the actor index

`docs/actor-index.md` is generated from the source tree:

```sh
python3 tools/utilities/gen_actor_index.py
```

It reads each `src/d/actor/d_a_*.cpp` (profile → process name), the `ActorRel(...)` entries in `configure.py` (REL vs DOL) and the `@brief` in each `include/d/actor/*.h`.
