# 5. Glossary: mapping internal names to things people recognise

Twilight Princess's code names come from Nintendo's Japanese developers (romaji, abbreviations, numbers) plus generic engine jargon. This page decodes them. Sources: code comments, Doxygen `@brief`s, debug strings, and (where marked) community knowledge. **`(?)` = plausible but not proven by the source.** The generated [actor-index.md](actor-index.md) has the per-file table.

Contents: [Prefixes](#file-and-symbol-prefixes) · [Suffixes & member naming](#suffixes-member-and-variable-naming) · [Framework jargon](#framework-and-engine-jargon) · [Japanese words](#japanese-and-romaji-words-in-identifiers) · [Actor names](#decoding-actor-names) · [Items](#items-and-equipment) · [Dungeons](#dungeons-lv1lv9) · [Stage codes](#stage-codes) · [File types](#file-extensions-and-data-formats) · [Project jargon](#decompilation-project-jargon)

---

## File and symbol prefixes

Function/class prefixes usually mirror the file they live in.

### Framework (`f_*`)

| Prefix | Files | Stands for / does |
|--------|-------|-------------------|
| `fpcBs_` | `f_pc_base` | Base process |
| `fpcLf_` / `fpcNd_` | `f_pc_leaf` / `f_pc_node` | Leaf process (draws, has no children) / node process (owns a layer of children) |
| `fpcM_` | `f_pc_manager` | Process Manager façade (`fpcM_Create`, `fpcM_Delete`, `fpcM_Management`) |
| `fpcCt_`, `fpcCtRq_`, `fpcSCtRq_`, `fpcFCtRq_`, `fpcNdRq_` | `f_pc_creator`, `f_pc_create_req`, `f_pc_stdcreate_req`, `f_pc_fstcreate_req`, `f_pc_node_req` | Create queue and create requests: standard, fast (synchronous), node |
| `fpcEx_` | `f_pc_executor` | Executor (per-frame execute) |
| `fpcDt_`, `fpcDtTg_` | `f_pc_deletor`, `f_pc_delete_tag` | Delete queue |
| `fpcDw_`, `fpcDwPi_` | `f_pc_draw`, `f_pc_draw_priority` | Draw handler, draw priority values |
| `fpcLy_`, `fpcLyTg_`, `fpcLyIt_` | `f_pc_layer*` | Layer (+ layer tag, layer iterator) |
| `fpcLn_`, `fpcLnTg_`, `fpcLnIt_` | `f_pc_line*` | Execution line (+ tag, iterator) |
| `fpcPi_` | `f_pc_priority` | Priority queue for layer/line changes |
| `fpcPf_`, `fpcNm_` | `f_pc_profile`, `f_pc_name` | Profile lookup; process-name enum (`fpcNm_*_e`) |
| `fpcLd_` | `f_pc_load` | Loader (REL link/unlink on behalf of processes) |
| `fpcMtd_` | `f_pc_method` | Calls on a process's method table |
| `fpcPause_`, `fpcSch_` | `f_pc_pause`, `f_pc_searcher` | Pause flags; searcher (find a process by ID/name) |
| `fpcDbSv_` | `f_pc_debug_sv` | Debug service hooks |
| `fopAc_` / `fopAcM_` / `fopAcTg_` | `f_op_actor*` | Actor (`M` = manager functions, `Tg` = tag lists) |
| `fopEn_` | `f_op_actor.h` | Enemy actor base |
| `fopCam_` / `fopCamM_` | `f_op_camera*` | Camera |
| `fopVw_` | `f_op_view` | View (projection/view base of camera) |
| `fopKy_` / `fopKyM_` | `f_op_kankyo*` | Kankyo = environment |
| `fopMsg_` / `fopMsgM_` | `f_op_msg*` | Message-type 2D processes (also HUD, menus, timer) |
| `fopOvlp_` / `fopOvlpM_` | `f_op_overlap*` | Overlap: screen-transition overlay |
| `fopScn_` / `fopScnM_` / `fopScnRq_` | `f_op_scene*` | Scene (`Rq` = request) |
| `fopDwTg_`, `fopDwIt_` | `f_op_draw_tag/iter` | Draw-tag queue and iteration |
| `fapGm_` | `f_ap_game` | Application game entry (`fapGm_Create`, `fapGm_Execute`) |

### Machine layer (`m_Do_*`)

| Prefix | File | Does |
|--------|------|------|
| `mDoMain` / `main01` | `m_Do_main` | Program entry, main loop, debug displays. |
| `mDoMch_` | `m_Do_machine` | Heaps/system init, exception handling. |
| `mDoGph_` | `m_Do_graphic` | Graphics init, frame painter, fade, bloom. |
| `mDoExt_` | `m_Do_ext(2)` | "Extension" helpers on JSystem: animation wrappers, model creation, heaps, debug shapes, 3D lines. |
| `mDoMtx_` | `m_Do_mtx` | Matrix stack and matrix helpers. |
| `mDoLib_` | `m_Do_lib` | Screen-projection and misc helpers. |
| `mDoCPd_` (`mReCPd`) | `m_Do_controller_pad` (`m_Re_…`) | Controller pad (`Re` = Revolution, i.e. Wii remote). |
| `mDoAud_` | `m_Do_audio` | Audio system glue. |
| `mDoDvdThd_` | `m_Do_dvd_thread` | DVD thread and its command objects. |
| `mDoMemCd_` | `m_Do_MemCard*` | Memory card. |
| `mDoRst_` | `m_Do_Reset` | Reset / HOME-button state. |
| `mDoHIO_` | `m_Do_hostIO` | HostIO registration. |
| `mDoPrintf`/`OSReport…` | `m_Do_printf` | Logging. |

### Dolzel game layer (`d_*`) – the `d` prefix on identifiers

| Prefix | What | Meaning |
|--------|------|---------|
| `da…` | actors (`daAlink_c`, `daNpcT_c`, `daObjSwpush::Act_c`, `daE_BG_c`) | dolzel actor. Older actors use a lowercase C-ish style: `e_ba_class`, `b_gnd_class`, plus `<name>_Create/_Execute…` free functions. |
| `dComIfG_`, `dComIfGp_`, `dComIfGs_`, `dComIfGd_` | `d_com_inf_game` | "Common interface"; the suffix picks the group: `G`eneral, `p` play, `s` save, `d` draw. |
| `dStage_` | `d_stage` | Stage/room data and control. |
| `dSv_` | `d_save` | Save data. |
| `dEvt_` / `dEv…` / `dEvent_` / `dEvDt…` | `d_event*` | Event control / event manager / event data (staff, cut). |
| `dDemo_` | `d_demo` | Demo (JStudio cutscene) glue. |
| `dBgS_`, `dBgW`, `dBgWKCol`, `dBgPc`/`dBgPlc` | `d_bg_*` | Background collision: system (`S`), mesh/wall (`W`), polygon-code data. |
| `dCcD_`, `dCcS`, `dCcMassS_` | `d_cc_*` | Collision-check data (`D`), system (`S`), mass groups. |
| `dCam_`/`dCamera_c`, `dDbCam` | `d_camera` | Camera logic. |
| `dAtt…` | `d_attention` | Attention (targeting). |
| `dKy_`, `dKankyo_`, `dScnKy_` | `d_kankyo*` | Kankyo (environment). |
| `dPa_` | `d_particle` | Particles (JPA wrapper). |
| `dRes_` | `d_resorce` (sic) | Resource archive control. |
| `dDlst_` | `d_drawlist` | Draw-list items. |
| `dMenu_`, `dMw_` | `d_menu_*` | Menu screens; `Mw` = menu window. |
| `dMeter2_`, `dMeter*`, `dTimer_` | `d_meter*`, `d_timer` | HUD. |
| `dMsg…`, `dMsgFlow_` | `d_msg_*` | Message/dialogue. |
| `dScn…` | `d_s_*` | Scene classes (`dScnPly_c` = play scene, `dScnLogo_c`, `dScnName_c`, `dScnMenu_c`). |
| `dSelect_` | `d_select_*` | Reusable selection cursor/icon. |
| `dItem…`, `dItemNo_` | `d_item*` | Items, item number enum. |
| `dTres_` | `d_tresure` (sic) | Treasure. |
| `dPath_`, `dMpath_`, `dMap_` | `d_path`, `d_map*` | Paths, map paths, map drawing. |
| `dNpc…` | `d_npc*` | NPC helpers/mixins. |
| `dJnt…` | `d_jnt_col` | Joint collision ("hit-zones" on skeleton joints). |
| `dCsr_` | `d_cursor_mng` | Cursor manager (Wii pointer). |
| `dDbVw_`, `dJcame_`, `dJprev_` | `d_debug_*`, `d_jcam_editor`, `d_jpreviewer` | Debug viewer, JStudio camera editor / previewer. |
| `dEyeHL` | `d_eye_hl` | Eye highlight. |
| `dOvlp…` | `d_ovlp_fade*` | Overlap (wipe/fade) processes. |

### SSystem "SComponent" (`c*`, `c_*.cpp`)

`cXyz` float vector · `csXyz` (short-int vector, used for angles) · `cSAngle` / `cSPolar` / `cSGlobe` (fixed-point angle, polar and globe-coordinate types) · `cM3d*` / `cM3dG*` (3D math / geometry shapes: `Sph`, `Cyl`, `Cps` capsule, `Tri`, `Pla` plane, `Lin`, `Aab` axis-aligned box, `Cir`, `Vtx`) · `cM_` (math helpers, `cM_rnd`) · `cLib_` (lerp/approach helpers, e.g. `cLib_addCalc`) · `cCcD_`/`cCcS` (collision data/system) · `cBgS_`/`cBgW` (background collision base) · `cLs`/`cNd`/`cTr`/`cTg` list / node / tree / tag intrusive containers (used by the process system) · `cPhs_` create phases · `cReq` request · `cAPI` graphics/pad thin wrapper · `cCt_Counter` frame counter · `cDyl_` dynamic link (REL) · `cMl` allocation helper (`c_malloc`).

### Third-party layers

`JKR` (JKernel) · `JSU` (JSupport) · `JUT` (JUtility) · `JFW` (JFramework) · `J3D`/`J2D` · `JPA` (particles) · `JAI/JAS/JAU` (audio interface/sequencer/utility) · `JMS/JMessage` · `JSG/JStage`, `JStudio` (`STB`) · `JOR/JHI/JAH/JAW` (HostIO & audio tools) · `JGadget`, `JMA` (math) · `Z2` (Zelda audio) · `OS`/`GX`/`DVD`/`PAD`/`VI`/`CARD`/`AI`/`AX`/`DSP`/`MTX`/`EXI`/`HIO` (SDK) · `WPAD`/`KPAD`/`NAND`/`IPC`/`SC`/`TPL` (Revolution SDK) · `MSL` (Metrowerks Standard Library) · `TRK` (debug kernel).

---

## Suffixes, member and variable naming

| Pattern | Meaning |
|---------|---------|
| `Foo_c` | A **c**lass (the game's own convention). Preferred for engine classes. |
| `foo_class` | A struct/class in the older C-derived style (`fopAc_ac_c` vs `scene_class`, `kankyo_class`, `msg_class`, `camera_class`, `e_ba_class`). |
| `Foo_e`, `FOO_e` | An **e**num (`fpcNm_ALINK_e`, `dItemNo_SWORD_e`, `fopAc_ENEMY_e`). |
| `_HIO_c`, `HIO` | HostIO tuning structs (debug tooling). |
| `_Method`, `l_dScnPly_Method`, `Mthd_Table` | The process method table (`create/delete/execute/is_delete/draw`). |
| `daXxx_Create` / `daXxx_c::create` / `Mthd_Create` | Create phase(s). Same for `Delete`, `Execute`, `IsDelete`, `Draw`. |
| `mFoo`, `mpFoo`, `mFooArray[]` | Member; `mp` = pointer. |
| `field_0x1c`, `unk_0x14` | Unknown member at that byte offset (decompiler placeholder). |
| `i_arg`, `o_arg` | Input / output function parameters. |
| `l_foo` | File-**l**ocal static. |
| `g_foo` | **G**lobal (e.g., `g_dComIfG_gameInfo`, `g_env_light`, `g_profile_ALINK`, `g_regHIO`). |
| `sFoo` / `s_foo` / `m_foo` (static member) | Static state. |
| `var_r30`, `sp8`, `temp_f1` | Unnamed locals from the decompiler; rename when understood. |
| `@1234` | Compiler-generated local symbols (string literals, float constants, jump tables). |
| `_ct` / `_dt` | (De)constructor variants in symbol names. |
| `__vt__…`, `__RTTI__…` | Vtable / RTTI symbols. |

Namespaces are used sparingly (`daObjSwpush::Act_c` style in newer objects; anonymous `namespace {}` to keep `Mthd_*` file-local).

---

## Framework and engine jargon

| Term | Meaning |
|------|---------|
| **Process** (`proc`, `base_process_class`) | Any scheduled entity: scene, actor, camera, HUD, message box… |
| **Profile** | Constant descriptor for a process type (`g_profile_*`) holding size, layer, list ID, name, methods… |
| **Process name** (`fpcNm_*_e`, "profname") | Numeric ID of a profile; also used as the "actor type" everywhere (`fopAcM_create(fpcNm_…)`). |
| **Layer** | Tree of child processes owned by a scene node. |
| **Line / List ID** | One of 16 execution queues; determines run order. |
| **Draw priority** | Order for draw calls (`fpcDwPi_*`). |
| **Tag** | Intrusive list membership object embedded in a process (`create_tag_class`, `layer_tag`, `draw_tag`…). |
| **Phase** | A step of a multi-frame create function (`cPhs_*`), or of `dScnPly_c`'s startup (`phase_00…`). |
| **Append** | Extra data block passed at creation (`fopAcM_prm_class`: position, angle, params, room). |
| **Params / parameters** | Packed 32-bit configuration word for an actor from the map editor. |
| **setID** | Index of an actor within its room's placement list; used to remember "already spawned/collected". |
| **Room number / roomNo** | Sub-area of a stage; `-1` = none. |
| **Layer number (story layer)** | 0–14 alternate content sets of a stage (not the process layer). |
| **Start point / spawn point** | `PLYR` entry index where the player appears. |
| **Wipe** | Screen transition type (`dStage_nextStage_c::wipe`). |
| **Overlap** | The process that shows the wipe (`fopOvlp`). |
| **Event / staff / cut** | Cutscene data model (see 04). |
| **Demo** | JStudio-driven scripted sequence. |
| **Attention** | Z-targeting/talk-prompt system. |
| **BG** | **B**ack**g**round – the static/moving world geometry and its collision (`d_bg_*`, `d_a_bg`). |
| **CC** | **C**ollision **C**heck: actor-vs-actor hit boxes (AT/TG/CO roles). |
| **Acch** | Actor **c**ollision **ch**eck against BG (ground/wall/roof). |
| **Kankyo** | Environment: lighting, weather, time. |
| **Tev / tevStr** | GX "Texture Environment" (TEV): the GPU's programmable shading stages. `dKy_tevstr_c` holds the per-actor lighting/fog inputs fed to them. |
| **DL / display list** | GPU command list. `dDlst_*` draw-list objects are different: engine-level draw items. |
| **J3D / BMD / BDL** | Nintendo model format and library. |
| **Solid heap** | Bump-allocated heap freed all at once (actors' private heaps). |
| **Exp heap** | Expandable, general-purpose heap. |
| **Pause flag** | Bits on a process for pause groups (`fpcM_PauseEnable`). |
| **Suspend** | `d_a_suspend` (`daSus_c`): mechanism to skip creating/executing certain actors. |
| **Camera type/style/mode** | Camera behaviour selection (see 04). |
| **MapTool** (`mMapToolId`) | The stage-editing tool; IDs it assigns to events/camera areas appear in code. |

---

## Japanese and romaji words in identifiers

Words that show up in file names, class names or variables. `(?)` = my reading of the romaji.

| Word | Meaning | Where you'll see it |
|------|---------|---------------------|
| **kankyo** (環境) | environment | `d_kankyo`, `fopKy`, `kytag*`, `dKy_` |
| **kaze** | wind | `kazeneko` (weather vane), `dKankyo_windHIO` |
| **kusa** | grass | `kusax1`, `Obj_kusa…`, `d_a_grass` |
| **hana** | flower | `flower`, `kasi_hana` (Hannah) |
| **ki** | tree | `d_a_obj_ki` |
| **iwa** / rock | rock | `d_a_obj_rock` |
| **tubo / tsubo** | pot | `kiPot`, `oiltubo`, `Obj_Tubo` |
| **taru** | barrel | `gpTaru`, `onsenTaru` |
| **saku** | fence | `h_saku`, `usaku` (horse fence) |
| **ita** | plank / board | `itamato`, `sakuita` |
| **hashi** | bridge | `bhashi`, `ihasi`, `hhashi`, `thashi` |
| **kanban** | signboard | `kanban2`, `kkanban`, `d_msg_scrn_kanban` |
| **kantera / kandelaar** | lantern | `dItemNo_KANTERA`, `alink_kandelaar`, `d_kantera_icon_meter` |
| **kago** | basket | `obj_kago`, `tag_kago_fall`; `d_a_kago` is described in the repo as the player-controlled Kargorok (?) |
| **yami** | darkness → **twilight** | `tag_yami`, `dKy_darkworld_*`, `yamit/yamis/yamid` (Twili NPCs) |
| **seirei** | spirit | `npc_seirei` (Ordona), `seib/seic/seid` (Faron/Eldin/Lanayru spirits) |
| **yousei** | fairy | `obj_yousei`, `dItemNo_FAIRY` |
| **sekizo / sekizou** | stone statue | `obj_sekizo*`, `d_a_obj_sekizoa` |
| **sekidoor / sekiban** | stone door / tablet | |
| **taihou** | cannon | `Y_taihou`, `scannon` (sky cannon) |
| **hakai** | destruction / breakable | `hakai_brl`, `hakai_ftr` |
| **kabe** | wall | `gomikabe` ("garbage wall") |
| **kake** | cliff | `tgake` |
| **taki** | waterfall | `waterfall` |
| **onsen** | hot spring | `onsen*`, `dItemNo_HOT_SPRING` |
| **tenbin** | balance scale | `lv6Tenbin` |
| **pachi / pachinko** | slingshot | `tag_pachi`, `npc_pachi_*`, `dItemNo_PACHINKO` |
| **kakashi** | scarecrow | `npc_kakashi` |
| **kakera** | fragment / shard | `dItemNo_KAKERA_HEART` = **Piece of Heart** |
| **utawa** | (name of a heart item) | `UTAWA_HEART` = Heart Container |
| **kumo / sora** | cloud / sky | `dKy_set_vrboxkumocol_ratio`, `vrbox_sora` |
| **vrbox** | "**V**irtual **R**eality box" = sky box | `d_a_vrbox`, `vrbox2` |
| **jimaku** | subtitles | `d_msg_scrn_jimaku` |
| **haihai** | "giddy-up!" to a horse | `dMeterHaihai_c` |
| **hakusha** | horse-drawn carriage | `dMeterHakusha_c` |
| **koori** | ice | `e_kk` "Koori no Kenshi" = Chilfos ("ice swordsman") |
| **tekkyuu(hei)** | iron ball (soldier) | `e_th` Darkhammer |
| **kozaru** | little monkey | `Z2SE_KOSARU_*` sounds, `npc_ks` |
| **maji** | meaning unclear – a lighting mode used for map objects | `setLightTevColorType_MAJI` |
| **pikari / pika** | sparkle/glint | `kytag16` |
| **hitobj**, **swhit** | hit object / hit switch | `d_a_hitobj`, `d_a_swhit0` |
| **kaisou** | seaweed (?) | `obj_kaisou` |
| **ikada** | raft | `obj_ikada` |
| **kabuto / kuwagata / tonbo / batta / kamakiri…** | beetle / stag beetle / dragonfly / grasshopper / mantis | insect actors (Golden bugs) |
| **neko** | cat | `npc_ne`, `kazeneko` |
| **taka** | hawk | `npc_tk` |
| **poh / pou** | Poe (ghost) | `mPohNum`, `POU_FIRE*`, `POU_SPIRIT` |
| **chu / chuchu** | Chu (jelly enemy) | `CHUCHU_*` items |
| **hebi** | snake (Hebi Baba is the snake-headed Deku Baba variant) | `e_hb` |

---

## Decoding actor names

Pattern: `d_a_<family>_<code>`. The two-letter/short codes are abbreviations of the English or Japanese name; the [actor-index](actor-index.md) lists the meaning that the community has assigned. Treat as a hint.

### Player and helpers

`alink` = **A**ctor **Link** (human/wolf), `midna` / `dmidna` (Dying Midna) / `mirror`, `horse` (Epona), `hozelda` (Zelda on horse), `canoe`, `boomerang`, `arrow`, `nbomb` ("normal bomb"), `crod` ("copy rod" = **Dominion Rod**), `spinner`, `kago` (steerable Kargorok?), `mant` (Ganondorf's cloak).

### Bosses (`b_*`)

| Code | Boss (per repo briefs) | Code | Boss |
|------|------------------------|------|------|
| `b_bq` / `b_bh` | **Diababa** / its Baba hand | `b_oh`, `b_ob`, `b_oh2` | **Morpheel** (head, body, tentacle) |
| `b_go` | "Goron Golem" per repo (probably the Goron Mines boss **Fyrus** (?)) | `b_ds` | **Stallord** |
| `b_gm` | **Armogohma** | `b_yo`, `b_yo_ice` | **Blizzeta** (+ ice block) |
| `b_dr`, `b_dre` | **Argorok** | `b_zant*` | **Zant** (multiple forms/attacks) |
| `b_gnd` | **Ganondorf** | `b_mgn` | **Beast Ganon** |
| `b_gg`, `b_gos` | "Aeralfos (Gargoyle)" / small "Goron Golem"(?) per repo briefs – the source files disagree with the headers, treat as unverified | `b_tn` | Darknut (per repo) |

### Common enemy codes (`e_*`, from repo briefs)

`e_ai` Armos · `e_ba` Keese · `e_bee` Bee · `e_bg` Bomb Fish · `e_bi` Bomb Insect · `e_bs` Stalkin (baby Stal) · `e_bu` Bubble · `e_bug` Poison Mite · `e_cr` Bombskit ("Crazy Runner") · `e_db` Deku Baba · `e_dd` Dodongo · `e_df` Deku Flower · `e_dk` Bari (electric jelly) · `e_dn` Lizalfos · `e_dt` Deku Toad · `e_fb`/`e_fz` Freezard · `e_fk` Phantom Rider (?) · `e_fm` Fyrus/"Fire man" · `e_fs` puppet · `e_ge` Guay (crow) · `e_gi` Gibdo · `e_gm`/`e_kg` Baby Gohma / Young Gohma · `e_gob` Dangoro · `e_gs` Ghost Soldier · `e_hb` Hebi Baba · `e_hm` Torch Slug · `e_hz` Tile Worm · `e_is` Armos Titan(?) · `e_kk` Chilfos · `e_kr` Kargorok · `e_md` Suit of Armor · `e_mf` Dynalfos · `e_mk` Ook · `e_mm` Helmasaur · `e_ms` Rat · `e_nz` Ghoul Rat · `e_oc` Bokoblin · `e_ot` Toado · `e_ph` Peahat · `e_pm` Skull Kid · `e_po` Poe · `e_pz` Phantom Zant · `e_rb` Leever · `e_rd`/`e_rdb`/`e_rdy` (King) Bulblin riders · `e_s1` Shadow Beast · `e_sb` Shell Blade · `e_sf` Stalfos · `e_sg` Skullfish · `e_sh` Stalhound · `e_sm` Chu Worm · `e_st` Skulltula · `e_sw` Moldorm · `e_th` Darkhammer · `e_tk`/`e_tk2` Toadpoli · `e_tt` Tektite · `e_vt` Death Sword · `e_wb` Bullbo · `e_ws` Wall Skulltula · `e_ww` White Wolfos · `e_y*` **Twilight** versions (`e_yc` Twilight Kargorok, `e_yd` Twilight Deku Baba, `e_yg` vermin, `e_ym` insect…) · `e_zh` Ball Master(?) · `e_zm` Zant Mask · `e_zs` Staltroop.

**Sound-ID cross-check**: enemy sounds are `Z2SE_EN_<code>_*` (e.g., `Z2SE_EN_KK_*` for `e_kk`), useful to confirm what an unknown code is.

### NPCs (`npc_*`, from repo briefs)

| Code | Character | Code | Character |
|------|-----------|------|-----------|
| `yelia` | **Ilia** | `moi`, `moir` | **Rusl** |
| `uri` | **Uli** | `bou`/`bouS` | **Mayor Bo** |
| `maro` | **Malo** | `taro` | **Talo** |
| `kolin` | **Colin** | `jagar` | **Jaggle** |
| `kkri` | **Coro** | `henna` | **Hena** |
| `toby` | **Fyer** | `shad` | **Shad** |
| `ash` / `ashB` | **Ashei** (/ in Snowpeak garb) | `bans` | **Barnes** |
| `besu` | **Beth** | `doc` | **Dr. Borville** |
| `hanjo` | **Hanch** | `aru` | **Fado** |
| `len` | **Renado** | `the`, `theB` | **Telma** |
| `zelda`, `zelR`, `zelRo` | **Zelda** (variants) | `zant` | **Zant** |
| `midp` | **Midna** (true form) | `ins` | **Agitha** |
| `rafrel` | **Auru** | `raca` | **Falbi** |
| `seira`, `seira2` | **Sera** | `impal` | **Impaz** |
| `grd`, `gro`, `grr`, `grs`, `grz` | **Gor Coron, Gor Ebizo, Gor Liggs, Gor Amoto, Darbus** | `zrc` / `zra` / `zrz` | **Ralis** / Zora / **Rutela** |
| `ykm`, `ykw` | **Yeto**, **Yeta** | `tks`, `tkc`, `tkj` | **Ooccoo**, **Ooccoo Jr.**, Oocca |
| `kn` | **Hero's Shade** | `seirei` | **Ordona** (light spirit) |
| `peru` (actor) | **Louise** | `passer` | Hylian passers-by |

Other NPC codes: `coach` (wagon), `chat` (chatty NPC framework), `cd/cd2/cdn3` (crowd/townsfolk families), `p2`, `fairy` (Great Fairy), `lf` (little fish), `tr` (trout), `du` (duck), `ne` (cat), `df` (dragonfly).

### Tags (`tag_*`, `kytag*`)

Invisible controllers. Common ones: `tag_evt`/`evtarea`/`evtmsg`/`event` (start an event/cutscene when the player enters), `tag_msg`/`kmsg`/`mmsg` (show a message), `tag_camera` (camera hint), `tag_howl` (wolf-howl spot), `tag_hstop`/`mstop`/`mwait` (Midna-related stops/waits), `tag_lantern`, `tag_gstart` (game/minigame start), `tag_attention`, `tag_chkpoint`, `tag_ret_room`/`setrestart`/`chgrestart` (restart/return points), `tag_mist` (twilight mist), `tag_wljump` (wolf jump), `tag_wara_howl` etc. `kytag00…17`: weather/light/twilight/fog controllers with `@brief`s in the index (04 → [Environment](04-game-systems.md#environment-lighting-weather-time-twilight-kankyo)).

### Objects (`obj_*`)

Abbreviated by what they are + optional number: `swpush` (push switch), `movebox` (pushable box), `crystal` (crystal switch), `bombf` (bomb flower)… **Dungeon-specific objects are prefixed `lvN`** (see next section), e.g. `obj_lv4gear` = Arbiter's Grounds gear.

---

## Items and equipment

Item IDs are `dItemNo_*_e` (`include/d/d_item_data.h`). In-game name ↔ internal name:

| In game | Internal |
|---------|----------|
| Clawshot / Double Clawshot | `HOOKSHOT` / `W_HOOKSHOT`; actor code "hook" |
| Dominion Rod | `COPY_ROD` (`d_a_crod`) |
| Gale Boomerang | `BOOMERANG` |
| Slingshot | `PACHINKO` (+ `PACHINKO_SHOT`) |
| Lantern | `KANTERA` (oil: `OIL`, `OIL_BOTTLE`) |
| Iron Boots | `HVY_BOOTS` ("heavy boots") |
| Ball and Chain | `IRONBALL` |
| Spinner | `SPINNER` |
| Hawkeye | `HAWK_EYE` (+ `HAWK_ARROW`) |
| Heart Piece / Heart Container | `KAKERA_HEART` / `UTAWA_HEART` (`obj_life_container`) |
| Tear of Light / Vessel of Light | `LIGHT_DROP` / `DROP_CONTAINER*` (`obj_drop`) |
| Poe Soul | `POU_SPIRIT` (`POU_FIRE*` = flame) |
| Chu Jelly | `CHUCHU_*` |
| Golden bugs | `M_*` (male) / `F_*` (female): `ANT`, `BEETLE`, `BUTTERFLY`, `DANGOMUSHI` (pill bug), `DRAGONFLY`, `GRASSHOPPER`, `LADYBUG`, `MANTIS`, `MAYFLY`, `NANAFUSHI` (phasmid), `SNAIL`, `STAG_BEETLE` |
| Bottles | `EMPTY_BOTTLE`, `RED_BOTTLE`, `BLUE_BOTTLE`, `GREEN_BOTTLE`, `MILK_BOTTLE`, `OIL_BOTTLE`, `WATER_BOTTLE`, `FAIRY_DROP`, … |
| Wallet / bomb bag / quiver | `WALLET_LV1..3`, `BOMB_BAG_LV1..2`, `ARROW_LV1..3` (max upgrades) |
| Master Sword / Ordon Sword / Wooden Sword | `MASTER_SWORD`, `SWORD`, `WOOD_STICK` |
| Hylian Shield / Ordon Shield / Wooden Shield | `HYLIA_SHIELD`, `SHIELD`, `WOOD_SHIELD` |
| Rupees | `GREEN_/BLUE_/YELLOW_/RED_/PURPLE_/ORANGE_/SILVER_RUPEE` |
| Keys | `SMALL_KEY`, `BOSS_KEY`, `LV2_BOSS_KEY`, `LV5_BOSS_KEY`, `BOSSRIDER_KEY` |
| Dungeon items | `MAP`, `COMPUS` (compass) |
| Ooccoo | `DUNGEON_EXIT` / `DUNGEON_BACK` (?) – dungeon-exit items, see `d_a_npc_tks`/`tkc` |
| Smell items (wolf) | `SMELL_*` |
| Mirror shards | `MIRROR_PIECE_2/3/4` |
| Fused Shadows / Mirror etc. | in `dSv_player_collect_c` (`mCrystal`, `mMirror`) |

Player's item **slots** are the `ItemSlots` enum in `d_save.h`; a slot holds an item ID. Clothes: `WEAR_CASUAL`, `WEAR_KOKIRI`, `WEAR_ZORA`, `ARMOR` (Magic Armor).

---

## Dungeons (`lv1`…`lv9`)

Object/actor names prefixed **`lvN`** are dungeon-specific (the repo's `@brief`s spell these out). `LV1…LV9` also index the per-dungeon save memory (`dStage_SaveTbl_LV1…LV9`).

| Prefix | Dungeon | Stage codes (see [next section](#stage-codes)) |
|--------|---------|-------------------|
| `lv1` | **Forest Temple** | `D_MN01`, `D_MN01A/B` |
| `lv2` | **Goron Mines** | `D_MN04`, `D_MN04A/B` |
| `lv3` | **Lakebed Temple** | `D_MN05`, `D_MN05A/B` |
| `lv4` | **Arbiter's Grounds** | `D_MN10`, `D_MN10A/B` |
| `lv5` | **Snowpeak Ruins** | `D_MN11` (confirmed in code), `D_MN11A/B` |
| `lv6` | **Temple of Time** | `D_MN06`, `D_MN06A/B` |
| `lv7` | **City in the Sky** | `D_MN07`, `D_MN07A/B` |
| `lv8` | **Palace of Twilight** | `D_MN08`, `D_MN08A–D` |
| `lv9` | **Hyrule Castle** | `D_MN09` (confirmed in code), `D_MN09A–C` |

The `D_MNxx` ↔ `lvN` pairing for lv1–lv4, lv6–lv8 comes from the order in `src/Z2AudioLib/SpotName.h` and community knowledge, not from an explicit table. Dungeon rooms also use suffix letters (`A`, `B`, …) for **boss/mini-boss sub-stages**.

---

## Stage codes

Stage names are 7–8 character IDs (`char[8]`) that name the archive and every table that mentions a location. Format: `<T>_<KK><NNN>`.

| Prefix | Meaning |
|--------|---------|
| `F_SP…` | Outdoor field areas ("F" = field; "SP" is possibly *spot*) |
| `R_SP…` | Interior areas – houses, shops, sewers (the "R" probably means *room*) |
| `D_MN…` | Dungeons ("main" dungeons, plus the Cave of Ordeals-style `D_MN54`) |
| `D_SB…` | Sub-dungeons (caves/grottos; `D_SB10` is the Faron Woods cave) |
| `S_MV…` | Probably "movie" stages: `S_MV000` is the one stage where the HUD (`METER2`) is deliberately *not* created |
| `T_ENEMY` | Test stage for enemies (?), listed in `sSpotName[]` |

Those the code itself names (comments in `d_com_inf_game.cpp`, `getLayerNo_common_common`):

| Code | Location | Code | Location |
|------|----------|------|----------|
| `F_SP102` | Title/opening stage (`dComIfG_changeOpeningScene`) | `F_SP103` | Ordon Village |
| `R_SP01` | Ordon Village interiors | `F_SP104` | Ordon Spring |
| `F_SP00` | Ordon Ranch | `F_SP108` | Faron Woods |
| `R_SP108` | Faron Woods interiors | `D_SB10` | Faron Woods Cave |
| `F_SP109` | Kakariko Village (inferred) | `F_SP111` | Kakariko Graveyard (inferred) |
| `R_SP109` / `R_SP209` | Kakariko / Graveyard interiors | `F_SP110` / `R_SP110` | Death Mountain / interiors |
| `F_SP112` | Zora's River | `F_SP113` | Zora's Domain |
| `F_SP126` | Upper Zora's River | `F_SP114` | Snowpeak |
| `F_SP115` | Lake Hylia | `F_SP116` | Castle Town |
| `R_SP116` | Room 5 is Telma's Bar (other rooms not named in code) | `R_SP160` | Castle Town interiors (other) |
| `F_SP117` | Sacred Grove | `F_SP118` | Bulblin Camp |
| `F_SP121` | Hyrule Field | `F_SP122` | Outside Castle Town |
| `F_SP124` | Gerudo Desert | `F_SP128` | Hidden Village |
| `F_SP127` / `R_SP127` | Fishing Pond / Hena's Hut | `R_SP107` | Hyrule Castle Sewers |
| `D_MN09` | Hyrule Castle | `D_MN11` | Snowpeak Ruins |

Other codes exist (e.g. `F_SP123`, `F_SP125`, `F_SP200`, `R_SP161`, `R_SP300/301`, `D_SB00–09`, `D_MN54`, `F_NW01`) – their names aren't stated in the code. The full ordered list of "spots" is `sSpotName[]` in `src/Z2AudioLib/SpotName.h`.

Stage IDs also appear as **save-memory slots** (`dStage_SaveTbl_ORDON/PRISON/FARON/ELDIN/LANAYRU/FIELD/GROVE/SNOWPEAK/CASTLE_TOWN/DESERT/FISHING_POND/LV1…LV9/CAVE1/CAVE2/GROTTO`) and as archive names in `assets/*/res/FieldMap/D_MN*.h`.

---

## File extensions and data formats

| Ext | Meaning | Loaded by |
|-----|---------|-----------|
| `.dol` | Main executable (`framework`). | dtk / boot |
| `.rel` | Relocatable module (an actor or `f_pc_profile_lst`). | `DynamicModuleControl` |
| `.arc` | RARC archive (often Yaz0/SZS compressed). | `JKRArchive` family |
| `.dzs` / `.dzr` | Stage / room data (chunks with 4-char tags; the "DZ" isn't expanded anywhere in the code). | `d_stage.cpp` |
| `.dzb` | Collision mesh. | `dBgW` |
| `.bmd`, `.bdl` | J3D model (basic / with display lists). | J3DModelLoader |
| `.bck` `.btk` `.brk` `.btp` `.bpk` `.blk` | J3D animations: skeleton, texture-SRT, TEV-colour, texture-pattern, material colour, cluster (matching `mDoExt_*Anm` classes) | J3DAnmLoader |
| `.blo` | J2D screen layout. | J2DScreen |
| `.bti` / `ResTIMG` | Texture image. | J2D/J3D |
| `.bfn` / `ResFONT` | Bitmap font. | JUTResFont |
| `.stb` | JStudio cutscene timeline. | `dDemo_c` |
| `.bmg` | Message text. | JMessage |
| `.jpc` | JPA particle resource set. | `dPa_control_c` |
| `.baa` / `.bms` / `.aw` | Audio archive / sequence / wave bank. | JAudio2 / Z2 |
| `.thp` | Video (movie player). | `d_a_movie_player` |
| `.str` | Symbol string table read at boot (debug). | `cDyl_InitCallback` |
| `.alf` | Shield executable format (Rframework). | |
| `.pch` / `.mch` | Precompiled headers (source / compiled by Metrowerks). | build |
| `.inc` | Source fragment `#include`d into one TU. | |

---

## Decompilation project jargon

| Term | Meaning |
|------|---------|
| **Matching** | Compiled object is byte-identical to the original. |
| **Equivalent** | Same behaviour, not byte-identical. |
| **NonMatching** | Not linked; original code used. |
| **Weak function order** | Inline/template/vtable functions are emitted as weak symbols; their order in the object must match. A common cause of near-matches. |
| **Fakematch** | A hack that matches bytes but isn't plausibly the original source. Avoided/annotated. |
| **TU** | Translation unit. |
| **dtk / splits / symbols** | decomp-toolkit; splits define TU boundaries; symbols define names/sizes. |
| **PCH** | Precompiled header; `dolzel.pch` etc. |
| **sdata / sdata2** | Small-data sections (r13/r2-relative). RELs compile with `-sdata 0` so they use none. |
| **`AUDIO_INSTANCES`** | Macro in actor TUs forcing Z2/JAudio template statics to exist for weak-symbol ordering. |
| **`DECLARE_…` / `STATIC_ASSERT`** | Size checks on structs. |
| **Shield / ShieldD** | Nvidia Shield China builds; ShieldD is the debug build that carries names and asserts. |
| **`#if DEBUG` code** | Present in the debug build only; useful for finding names, invisible in retail. |
| **"(sic)"** | Real misspellings in original identifiers preserved on purpose: `cPhs_COMPLEATE_e`, `d_resorce`, `d_tresure`, `dKy…_wether`, `Distanse`. |
