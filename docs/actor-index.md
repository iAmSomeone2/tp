# Actor index (generated)

Every `src/d/actor/d_a_*.cpp` translation unit, with its process name(s), where it lives at runtime, and a human-readable name.

- **Profile** – the `g_profile_*` symbol defined in the file; **Process name** – the `fpcNm_*_e` enum entry (see [`include/f_pc/f_pc_name.h`](../include/f_pc/f_pc_name.h)) used to spawn it.
- **Runs from** – `REL` = separate relocatable module loaded on demand (`ActorRel(...)` in `configure.py`); `DOL` = part of the always-resident main executable.
- **In-game name** – taken from the `@brief` in the actor's header when one exists. These are community-written and some are guesses (marked `?` in the source) or wrong. Blank means nobody has written one yet. Treat as a hint, then confirm in the code (resource names, sound IDs `Z2SE_*`, message text).
- Regenerate with `python3 tools/utilities/gen_actor_index.py` (see [06-exploring.md](06-exploring.md#regenerating-the-actor-index)).

Total: 767 actor TUs.


## Player (1)

| Source | Profile → process name | Runs from | In-game name |
|---|---|---|---|
| [`d_a_alink`](../src/d/actor/d_a_alink.cpp) | `ALINK` → `fpcNm_ALINK_e` | DOL | Player (Link) Actor |

## Bosses (21)

| Source | Profile → process name | Runs from | In-game name |
|---|---|---|---|
| [`d_a_b_bh`](../src/d/actor/d_a_b_bh.cpp) | `B_BH` → `fpcNm_B_BH_e` | REL | Diababa - Baba Hand |
| [`d_a_b_bq`](../src/d/actor/d_a_b_bq.cpp) | `B_BQ` → `fpcNm_B_BQ_e` | REL | Diababa |
| [`d_a_b_dr`](../src/d/actor/d_a_b_dr.cpp) | `B_DR` → `fpcNm_B_DR_e` | REL | Argorok |
| [`d_a_b_dre`](../src/d/actor/d_a_b_dre.cpp) | `B_DRE` → `fpcNm_B_DRE_e` | REL | Argorok (child actor?) |
| [`d_a_b_ds`](../src/d/actor/d_a_b_ds.cpp) | `B_DS` → `fpcNm_B_DS_e` | REL | Stallord |
| [`d_a_b_gg`](../src/d/actor/d_a_b_gg.cpp) | `B_GG` → `fpcNm_B_GG_e` | REL | Aeralfos (Gargoyle) |
| [`d_a_b_gm`](../src/d/actor/d_a_b_gm.cpp) | `B_GM` → `fpcNm_B_GM_e` | REL | Armogohma |
| [`d_a_b_gnd`](../src/d/actor/d_a_b_gnd.cpp) | `B_GND` → `fpcNm_B_GND_e` | REL | Ganondorf |
| [`d_a_b_go`](../src/d/actor/d_a_b_go.cpp) | `B_GO` → `fpcNm_B_GO_e` | REL | Goron Golem |
| [`d_a_b_gos`](../src/d/actor/d_a_b_gos.cpp) | `B_GOS` → `fpcNm_B_GOS_e` | REL | Goron Golem (small) |
| [`d_a_b_mgn`](../src/d/actor/d_a_b_mgn.cpp) | `B_MGN` → `fpcNm_B_MGN_e` | REL | Beast Ganon |
| [`d_a_b_ob`](../src/d/actor/d_a_b_ob.cpp) | `B_OB` → `fpcNm_B_OB_e` | REL | Morpheel (body) |
| [`d_a_b_oh`](../src/d/actor/d_a_b_oh.cpp) | `B_OH` → `fpcNm_B_OH_e` | REL | Morpheel (head) |
| [`d_a_b_oh2`](../src/d/actor/d_a_b_oh2.cpp) | `B_OH2` → `fpcNm_B_OH2_e` | REL | Morpheel (tentacle) |
| [`d_a_b_tn`](../src/d/actor/d_a_b_tn.cpp) | `B_TN` → `fpcNm_B_TN_e` | REL | Darknut |
| [`d_a_b_yo`](../src/d/actor/d_a_b_yo.cpp) | `B_YO` → `fpcNm_B_YO_e` | REL | Blizzeta |
| [`d_a_b_yo_ice`](../src/d/actor/d_a_b_yo_ice.cpp) | `B_YOI` → `fpcNm_B_YOI_e` | REL | Blizzeta Second Phase Ice Block |
| [`d_a_b_zant`](../src/d/actor/d_a_b_zant.cpp) | `B_ZANT` → `fpcNm_B_ZANT_e` | REL | Zant |
| [`d_a_b_zant_magic`](../src/d/actor/d_a_b_zant_magic.cpp) | `B_ZANTM` → `fpcNm_B_ZANTM_e` | REL | Zant - Magic Attack |
| [`d_a_b_zant_mobile`](../src/d/actor/d_a_b_zant_mobile.cpp) | `B_ZANTZ` → `fpcNm_B_ZANTZ_e` | REL | Zant (Mobile) |
| [`d_a_b_zant_sima`](../src/d/actor/d_a_b_zant_sima.cpp) | `B_ZANTS` → `fpcNm_B_ZANTS_e` | REL | Zant (Goron Mines Phase) |

## Enemies (96)

| Source | Profile → process name | Runs from | In-game name |
|---|---|---|---|
| [`d_a_e_ai`](../src/d/actor/d_a_e_ai.cpp) | `E_AI` → `fpcNm_E_AI_e` | REL | Armos |
| [`d_a_e_arrow`](../src/d/actor/d_a_e_arrow.cpp) | `E_ARROW` → `fpcNm_E_ARROW_e` | REL | Enemy Arrow |
| [`d_a_e_ba`](../src/d/actor/d_a_e_ba.cpp) | `E_BA` → `fpcNm_E_BA_e` | REL | Keese |
| [`d_a_e_bee`](../src/d/actor/d_a_e_bee.cpp) | `E_BEE` → `fpcNm_E_BEE_e` | REL | Bee |
| [`d_a_e_bg`](../src/d/actor/d_a_e_bg.cpp) | `E_BG` → `fpcNm_E_BG_e` | REL | Bomb Fish |
| [`d_a_e_bi`](../src/d/actor/d_a_e_bi.cpp) | `E_BI` → `fpcNm_E_BI_e` | REL | Bomb Insect |
| [`d_a_e_bi_leaf`](../src/d/actor/d_a_e_bi_leaf.cpp) | `E_BI_LEAF` → `fpcNm_E_BI_LEAF_e` | REL | Bomb Insect - Leaf |
| [`d_a_e_bs`](../src/d/actor/d_a_e_bs.cpp) | `E_BS` → `fpcNm_E_BS_e` | REL | Stalkin (Baby Stal) |
| [`d_a_e_bu`](../src/d/actor/d_a_e_bu.cpp) | `E_BU` → `fpcNm_E_BU_e` | REL | Bubble |
| [`d_a_e_bug`](../src/d/actor/d_a_e_bug.cpp) | `E_BUG` → `fpcNm_E_BUG_e` | REL | Poison Mite |
| [`d_a_e_cr`](../src/d/actor/d_a_e_cr.cpp) | `E_CR` → `fpcNm_E_CR_e` | REL | Bombskit (Crazy Runner) |
| [`d_a_e_cr_egg`](../src/d/actor/d_a_e_cr_egg.cpp) | `E_CR_EGG` → `fpcNm_E_CR_EGG_e` | REL | Bombskit Egg |
| [`d_a_e_db`](../src/d/actor/d_a_e_db.cpp) | `E_DB` → `fpcNm_E_DB_e` | REL | Deku Baba |
| [`d_a_e_db_leaf`](../src/d/actor/d_a_e_db_leaf.cpp) | `E_DB_LEAF` → `fpcNm_E_DB_LEAF_e` | REL | Deku Baba - Leaf |
| [`d_a_e_dd`](../src/d/actor/d_a_e_dd.cpp) | `E_DD` → `fpcNm_E_DD_e` | REL | Dodongo |
| [`d_a_e_df`](../src/d/actor/d_a_e_df.cpp) | `E_DF` → `fpcNm_E_DF_e` | REL | Deku Flower |
| [`d_a_e_dk`](../src/d/actor/d_a_e_dk.cpp) | `E_DK` → `fpcNm_E_DK_e` | REL | Bari |
| [`d_a_e_dn`](../src/d/actor/d_a_e_dn.cpp) | `E_DN` → `fpcNm_E_DN_e` | REL | Lizalfos |
| [`d_a_e_dt`](../src/d/actor/d_a_e_dt.cpp) | `E_DT` → `fpcNm_E_DT_e` | REL | Deku Toad |
| [`d_a_e_fb`](../src/d/actor/d_a_e_fb.cpp) | `E_FB` → `fpcNm_E_FB_e` | REL | Freezard |
| [`d_a_e_fk`](../src/d/actor/d_a_e_fk.cpp) | `E_FK` → `fpcNm_E_FK_e` | REL | Phantom Rider |
| [`d_a_e_fm`](../src/d/actor/d_a_e_fm.cpp) | `E_FM` → `fpcNm_E_FM_e` | REL | Fyrus (Fire Man) |
| [`d_a_e_fs`](../src/d/actor/d_a_e_fs.cpp) | `E_FS` → `fpcNm_E_FS_e` | REL | Wooden Puppet |
| [`d_a_e_fz`](../src/d/actor/d_a_e_fz.cpp) | `E_FZ` → `fpcNm_E_FZ_e` | REL | Freezard header file. |
| [`d_a_e_ga`](../src/d/actor/d_a_e_ga.cpp) | `E_GA` → `fpcNm_E_GA_e` | REL | Decorative Moth |
| [`d_a_e_gb`](../src/d/actor/d_a_e_gb.cpp) | `E_GB` → `fpcNm_E_GB_e` | REL | Giant Baba |
| [`d_a_e_ge`](../src/d/actor/d_a_e_ge.cpp) | `E_GE` → `fpcNm_E_GE_e` | REL | Guay |
| [`d_a_e_gi`](../src/d/actor/d_a_e_gi.cpp) | `E_GI` → `fpcNm_E_GI_e` | REL | Gibdo |
| [`d_a_e_gm`](../src/d/actor/d_a_e_gm.cpp) | `E_GM` → `fpcNm_E_GM_e` | REL | Baby Gohma / Gohma Eye |
| [`d_a_e_gob`](../src/d/actor/d_a_e_gob.cpp) | `E_GOB` → `fpcNm_E_GOB_e` | REL | Dangoro (Goron Boss) |
| [`d_a_e_gs`](../src/d/actor/d_a_e_gs.cpp) | `E_GS` → `fpcNm_E_GS_e` | REL | Ghost Soldier |
| [`d_a_e_hb`](../src/d/actor/d_a_e_hb.cpp) | `E_HB` → `fpcNm_E_HB_e` | REL | Hebi Baba |
| [`d_a_e_hb_leaf`](../src/d/actor/d_a_e_hb_leaf.cpp) | `E_HB_LEAF` → `fpcNm_E_HB_LEAF_e` | REL | Hebi Baba - Leaf |
| [`d_a_e_hm`](../src/d/actor/d_a_e_hm.cpp) | `E_HM` → `fpcNm_E_HM_e` | REL | Torch Slug |
| [`d_a_e_hp`](../src/d/actor/d_a_e_hp.cpp) | `E_HP` → `fpcNm_E_HP_e` | REL | Huge Poe? |
| [`d_a_e_hz`](../src/d/actor/d_a_e_hz.cpp) | `E_HZ` → `fpcNm_E_HZ_e` | REL | Tile Worm |
| [`d_a_e_hzelda`](../src/d/actor/d_a_e_hzelda.cpp) | `E_HZELDA` → `fpcNm_E_HZELDA_e` | REL | Puppet Zelda |
| [`d_a_e_is`](../src/d/actor/d_a_e_is.cpp) | `E_IS` → `fpcNm_E_IS_e` | REL | Armos Titan (Idelia Statue) |
| [`d_a_e_kg`](../src/d/actor/d_a_e_kg.cpp) | `E_KG` → `fpcNm_E_KG_e` | REL | Young Gohma |
| [`d_a_e_kk`](../src/d/actor/d_a_e_kk.cpp) | `E_KK` → `fpcNm_E_KK_e` | REL | Chilfos (Koori no Kenshi) |
| [`d_a_e_kr`](../src/d/actor/d_a_e_kr.cpp) | `E_KR` → `fpcNm_E_KR_e` | REL | Kargorok |
| [`d_a_e_mb`](../src/d/actor/d_a_e_mb.cpp) | `E_MB` → `fpcNm_E_MB_e` | REL | Ook - Diababa Fight (Monkey Boomerang) |
| [`d_a_e_md`](../src/d/actor/d_a_e_md.cpp) | `E_MD` → `fpcNm_E_MD_e` | REL | Suit of Armor |
| [`d_a_e_mf`](../src/d/actor/d_a_e_mf.cpp) | `E_MF` → `fpcNm_E_MF_e` | REL | Dynalfos |
| [`d_a_e_mk`](../src/d/actor/d_a_e_mk.cpp) | `E_MK` → `fpcNm_E_MK_e` | REL | Ook |
| [`d_a_e_mk_bo`](../src/d/actor/d_a_e_mk_bo.cpp) | `E_MK_BO` → `fpcNm_E_MK_BO_e` | REL | Ook's Boomerang |
| [`d_a_e_mm`](../src/d/actor/d_a_e_mm.cpp) | `E_MM` → `fpcNm_E_MM_e` | REL | Helmasaur |
| [`d_a_e_mm_mt`](../src/d/actor/d_a_e_mm_mt.cpp) | `E_MM_MT` → `fpcNm_E_MM_MT_e` | REL | Helmasaur Shell |
| [`d_a_e_ms`](../src/d/actor/d_a_e_ms.cpp) | `E_MS` → `fpcNm_E_MS_e` | REL | Rat |
| [`d_a_e_nest`](../src/d/actor/d_a_e_nest.cpp) | `E_NEST` → `fpcNm_E_NEST_e` | REL | Beehive |
| [`d_a_e_nz`](../src/d/actor/d_a_e_nz.cpp) | `E_NZ` → `fpcNm_E_NZ_e` | REL | Ghoul Rat |
| [`d_a_e_oc`](../src/d/actor/d_a_e_oc.cpp) | `E_OC` → `fpcNm_E_OC_e` | REL | Bokoblin |
| [`d_a_e_oct_bg`](../src/d/actor/d_a_e_oct_bg.cpp) | `E_OctBg` → `fpcNm_E_OctBg_e` | REL | Morpheel Bomb Fish |
| [`d_a_e_ot`](../src/d/actor/d_a_e_ot.cpp) | `E_OT` → `fpcNm_E_OT_e` | REL | Toado |
| [`d_a_e_ph`](../src/d/actor/d_a_e_ph.cpp) | `E_PH` → `fpcNm_E_PH_e` | REL | Peahat |
| [`d_a_e_pm`](../src/d/actor/d_a_e_pm.cpp) | `E_PM` → `fpcNm_E_PM_e` | REL | Skullkid |
| [`d_a_e_po`](../src/d/actor/d_a_e_po.cpp) | `E_PO` → `fpcNm_E_PO_e` | REL | Poe |
| [`d_a_e_pz`](../src/d/actor/d_a_e_pz.cpp) | `E_PZ` → `fpcNm_E_PZ_e` | REL | Phantom Zant |
| [`d_a_e_rb`](../src/d/actor/d_a_e_rb.cpp) | `E_RB` → `fpcNm_E_RB_e` | REL | Leever (Riiba) |
| [`d_a_e_rd`](../src/d/actor/d_a_e_rd.cpp) | `E_RD` → `fpcNm_E_RD_e` | REL | Rider (Bulblin / King Bulblin on Boar) |
| [`d_a_e_rdb`](../src/d/actor/d_a_e_rdb.cpp) | `E_RDB` → `fpcNm_E_RDB_e` | REL | King Bulblin |
| [`d_a_e_rdy`](../src/d/actor/d_a_e_rdy.cpp) | `E_RDY` → `fpcNm_E_RDY_e` | REL | Shadow Bulblin |
| [`d_a_e_s1`](../src/d/actor/d_a_e_s1.cpp) | `E_S1` → `fpcNm_E_S1_e` | REL | Shadow Beast |
| [`d_a_e_sb`](../src/d/actor/d_a_e_sb.cpp) | `E_SB` → `fpcNm_E_SB_e` | REL | Shell Blade |
| [`d_a_e_sf`](../src/d/actor/d_a_e_sf.cpp) | `E_SF` → `fpcNm_E_SF_e` | REL | Stalfos |
| [`d_a_e_sg`](../src/d/actor/d_a_e_sg.cpp) | `E_SG` → `fpcNm_E_SG_e` | REL | Skullfish |
| [`d_a_e_sh`](../src/d/actor/d_a_e_sh.cpp) | `E_SH` → `fpcNm_E_SH_e` | REL | Stalhound |
| [`d_a_e_sm`](../src/d/actor/d_a_e_sm.cpp) | `E_SM` → `fpcNm_E_SM_e` | REL | Chu Worm |
| [`d_a_e_sm2`](../src/d/actor/d_a_e_sm2.cpp) | `E_SM2` → `fpcNm_E_SM2_e` | REL | Chuchu 2 |
| [`d_a_e_st`](../src/d/actor/d_a_e_st.cpp) | `E_ST` → `fpcNm_E_ST_e` | REL | Skulltula |
| [`d_a_e_st_line`](../src/d/actor/d_a_e_st_line.cpp) | `E_ST_LINE` → `fpcNm_E_ST_LINE_e` | REL | Skulltula Web Line |
| [`d_a_e_sw`](../src/d/actor/d_a_e_sw.cpp) | `E_SW` → `fpcNm_E_SW_e` | REL | Moldorm |
| [`d_a_e_th`](../src/d/actor/d_a_e_th.cpp) | `E_TH` → `fpcNm_E_TH_e` | REL | Darkhammer (Tekkyuuhei) |
| [`d_a_e_th_ball`](../src/d/actor/d_a_e_th_ball.cpp) | `E_TH_BALL` → `fpcNm_E_TH_BALL_e` | REL | Darkhammer Ball and Chain |
| [`d_a_e_tk`](../src/d/actor/d_a_e_tk.cpp) | `E_TK` → `fpcNm_E_TK_e` | REL | Water Toadpoli |
| [`d_a_e_tk2`](../src/d/actor/d_a_e_tk2.cpp) | `E_TK2` → `fpcNm_E_TK2_e` | REL | Fire Toadpoli |
| [`d_a_e_tk_ball`](../src/d/actor/d_a_e_tk_ball.cpp) | `E_TK_BALL` → `fpcNm_E_TK_BALL_e` | REL | Fire/Water Toadpoli Ball |
| [`d_a_e_tt`](../src/d/actor/d_a_e_tt.cpp) | `E_TT` → `fpcNm_E_TT_e` | REL | Tektite |
| [`d_a_e_vt`](../src/d/actor/d_a_e_vt.cpp) | `E_VT` → `fpcNm_E_VT_e` | REL | Death Sword |
| [`d_a_e_warpappear`](../src/d/actor/d_a_e_warpappear.cpp) | `E_WAP` → `fpcNm_E_WAP_e` | REL | Shadow Beast Warp Appear |
| [`d_a_e_wb`](../src/d/actor/d_a_e_wb.cpp) | `E_WB` → `fpcNm_E_WB_e` | REL | Bullbo (Wild Boar) |
| [`d_a_e_ws`](../src/d/actor/d_a_e_ws.cpp) | `E_WS` → `fpcNm_E_WS_e` | REL | Wall Skulltula |
| [`d_a_e_ww`](../src/d/actor/d_a_e_ww.cpp) | `E_WW` → `fpcNm_E_WW_e` | REL | White Wolfos |
| [`d_a_e_yc`](../src/d/actor/d_a_e_yc.cpp) | `E_YC` → `fpcNm_E_YC_e` | REL | Twilight Kargorok |
| [`d_a_e_yd`](../src/d/actor/d_a_e_yd.cpp) | `E_YD` → `fpcNm_E_YD_e` | REL | Twilight Deku Baba |
| [`d_a_e_yd_leaf`](../src/d/actor/d_a_e_yd_leaf.cpp) | `E_YD_LEAF` → `fpcNm_E_YD_LEAF_e` | REL | Twilight Deku Baba - Leaf |
| [`d_a_e_yg`](../src/d/actor/d_a_e_yg.cpp) | `E_YG` → `fpcNm_E_YG_e` | REL | Twilight Vermin |
| [`d_a_e_yh`](../src/d/actor/d_a_e_yh.cpp) | `E_YH` → `fpcNm_E_YH_e` | REL | Twilight Hebi Baba |
| [`d_a_e_yk`](../src/d/actor/d_a_e_yk.cpp) | `E_YK` → `fpcNm_E_YK_e` | REL | Shadow Keese current action. |
| [`d_a_e_ym`](../src/d/actor/d_a_e_ym.cpp) | `E_YM` → `fpcNm_E_YM_e` | REL | Twilight Insect |
| [`d_a_e_ym_tag`](../src/d/actor/d_a_e_ym_tag.cpp) | `E_YM_TAG` → `fpcNm_E_YM_TAG_e` | DOL | Twilight Insect Tag |
| [`d_a_e_ymb`](../src/d/actor/d_a_e_ymb.cpp) | `E_YMB` → `fpcNm_E_YMB_e` | REL | Twilight Insect Boss |
| [`d_a_e_yr`](../src/d/actor/d_a_e_yr.cpp) | `E_YR` → `fpcNm_E_YR_e` | REL | Twilight Kargorok Rider? |
| [`d_a_e_zh`](../src/d/actor/d_a_e_zh.cpp) | `E_ZH` → `fpcNm_E_ZH_e` | REL | Ball Master |
| [`d_a_e_zm`](../src/d/actor/d_a_e_zm.cpp) | `E_ZM` → `fpcNm_E_ZM_e` | REL | Zant Mask |
| [`d_a_e_zs`](../src/d/actor/d_a_e_zs.cpp) | `E_ZS` → `fpcNm_E_ZS_e` | REL | Staltroop |

## NPCs and creatures (125)

| Source | Profile → process name | Runs from | In-game name |
|---|---|---|---|
| [`d_a_npc`](../src/d/actor/d_a_npc.cpp) | — | DOL |  |
| [`d_a_npc2`](../src/d/actor/d_a_npc2.cpp) | — | DOL |  |
| [`d_a_npc4`](../src/d/actor/d_a_npc4.cpp) | — | DOL |  |
| [`d_a_npc_aru`](../src/d/actor/d_a_npc_aru.cpp) | `NPC_ARU` → `fpcNm_NPC_ARU_e` | REL | Fado |
| [`d_a_npc_ash`](../src/d/actor/d_a_npc_ash.cpp) | `NPC_ASH` → `fpcNm_NPC_ASH_e` | REL | Ashei |
| [`d_a_npc_ashB`](../src/d/actor/d_a_npc_ashB.cpp) | `NPC_ASHB` → `fpcNm_NPC_ASHB_e` | REL | Ashei (Yeti Garb) |
| [`d_a_npc_bans`](../src/d/actor/d_a_npc_bans.cpp) | `NPC_BANS` → `fpcNm_NPC_BANS_e` | REL | Barnes |
| [`d_a_npc_besu`](../src/d/actor/d_a_npc_besu.cpp) | `NPC_BESU` → `fpcNm_NPC_BESU_e` | REL | Beth |
| [`d_a_npc_blue_ns`](../src/d/actor/d_a_npc_blue_ns.cpp) | `NPC_BLUENS` → `fpcNm_NPC_BLUENS_e` | REL | Shadow Beast (Twili) |
| [`d_a_npc_bou`](../src/d/actor/d_a_npc_bou.cpp) | `NPC_BOU` → `fpcNm_NPC_BOU_e` | REL | Mayor Bo |
| [`d_a_npc_bouS`](../src/d/actor/d_a_npc_bouS.cpp) | `NPC_BOU_S` → `fpcNm_NPC_BOU_S_e` | REL | Mayor Bo (inside house) |
| [`d_a_npc_cd`](../src/d/actor/d_a_npc_cd.cpp) | — | DOL |  |
| [`d_a_npc_cd2`](../src/d/actor/d_a_npc_cd2.cpp) | — | DOL |  |
| [`d_a_npc_cdn3`](../src/d/actor/d_a_npc_cdn3.cpp) | `NPC_CD3` → `fpcNm_NPC_CD3_e` | REL | Hylian Adult |
| [`d_a_npc_chat`](../src/d/actor/d_a_npc_chat.cpp) | `NPC_CHAT` → `fpcNm_NPC_CHAT_e` | REL | NPC Chat |
| [`d_a_npc_chin`](../src/d/actor/d_a_npc_chin.cpp) | `NPC_CHIN` → `fpcNm_NPC_CHIN_e` | REL | Purlo |
| [`d_a_npc_clerka`](../src/d/actor/d_a_npc_clerka.cpp) | `NPC_CLERKA` → `fpcNm_NPC_CLERKA_e` | REL | Chudley |
| [`d_a_npc_clerkb`](../src/d/actor/d_a_npc_clerkb.cpp) | `NPC_CLERKB` → `fpcNm_NPC_CLERKB_e` | REL | Malver |
| [`d_a_npc_clerkt`](../src/d/actor/d_a_npc_clerkt.cpp) | `NPC_CLERKT` → `fpcNm_NPC_CLERKT_e` | REL | Ooccaa (City in the Sky Shop) |
| [`d_a_npc_coach`](../src/d/actor/d_a_npc_coach.cpp) | `NPC_COACH` → `fpcNm_NPC_COACH_e` | REL | Coach |
| [`d_a_npc_df`](../src/d/actor/d_a_npc_df.cpp) | `NPC_DF` → `fpcNm_NPC_DF_e` | REL | Dragonfly |
| [`d_a_npc_doc`](../src/d/actor/d_a_npc_doc.cpp) | `NPC_DOC` → `fpcNm_NPC_DOC_e` | REL | Dr. Borville |
| [`d_a_npc_doorboy`](../src/d/actor/d_a_npc_doorboy.cpp) | `NPC_DOORBOY` → `fpcNm_NPC_DOORBOY_e` | REL | Door Boy (This isn't Soal?) |
| [`d_a_npc_drainSol`](../src/d/actor/d_a_npc_drainSol.cpp) | `NPC_DRSOL` → `fpcNm_NPC_DRSOL_e` | REL | Drain Soldier (Hyrule Castle Sewer Soldier?) |
| [`d_a_npc_du`](../src/d/actor/d_a_npc_du.cpp) | `NPC_DU` → `fpcNm_NPC_DU_e` | REL | Duck |
| [`d_a_npc_fairy`](../src/d/actor/d_a_npc_fairy.cpp) | `NPC_FAIRY` → `fpcNm_NPC_FAIRY_e` | REL | Great Fairy |
| [`d_a_npc_fairy_seirei`](../src/d/actor/d_a_npc_fairy_seirei.cpp) | `NPC_FAIRY_SEIREI` → `fpcNm_NPC_FAIRY_SEIREI_e` | REL | Fairy Spirit |
| [`d_a_npc_fguard`](../src/d/actor/d_a_npc_fguard.cpp) | `NPC_FGUARD` → `fpcNm_NPC_FGUARD_e` | REL |  |
| [`d_a_npc_fish`](../src/d/actor/d_a_npc_fish.cpp) | `NPC_FISH` → `fpcNm_NPC_FISH_e` | REL | Fish |
| [`d_a_npc_gnd`](../src/d/actor/d_a_npc_gnd.cpp) | `NPC_GND` → `fpcNm_NPC_GND_e` | REL | Ganondorf |
| [`d_a_npc_gra`](../src/d/actor/d_a_npc_gra.cpp) | `NPC_GRA` → `fpcNm_NPC_GRA_e` | REL | Goron (Adult) |
| [`d_a_npc_grc`](../src/d/actor/d_a_npc_grc.cpp) | `NPC_GRC` → `fpcNm_NPC_GRC_e` | REL | Goron (Child) |
| [`d_a_npc_grd`](../src/d/actor/d_a_npc_grd.cpp) | `NPC_GRD` → `fpcNm_NPC_GRD_e` | REL | Gor Coron |
| [`d_a_npc_grm`](../src/d/actor/d_a_npc_grm.cpp) | `NPC_GRM` → `fpcNm_NPC_GRM_e` | REL | Goron Adult (Shopkeeper) |
| [`d_a_npc_grmc`](../src/d/actor/d_a_npc_grmc.cpp) | `NPC_GRMC` → `fpcNm_NPC_GRMC_e` | REL | Goron Child (Shopkeeper) |
| [`d_a_npc_gro`](../src/d/actor/d_a_npc_gro.cpp) | `NPC_GRO` → `fpcNm_NPC_GRO_e` | REL | Gor Ebizo |
| [`d_a_npc_grr`](../src/d/actor/d_a_npc_grr.cpp) | `NPC_GRR` → `fpcNm_NPC_GRR_e` | REL | Gor Liggs |
| [`d_a_npc_grs`](../src/d/actor/d_a_npc_grs.cpp) | `NPC_GRS` → `fpcNm_NPC_GRS_e` | REL | Gor Amoto |
| [`d_a_npc_grz`](../src/d/actor/d_a_npc_grz.cpp) | `NPC_GRZ` → `fpcNm_NPC_GRZ_e` | REL | Darbus |
| [`d_a_npc_guard`](../src/d/actor/d_a_npc_guard.cpp) | `NPC_GUARD` → `fpcNm_NPC_GUARD_e` | REL | Guard (Hyrule Castle Town Guard?) |
| [`d_a_npc_gwolf`](../src/d/actor/d_a_npc_gwolf.cpp) | `NPC_GWOLF` → `fpcNm_NPC_GWOLF_e` | REL | Golden Wolf |
| [`d_a_npc_hanjo`](../src/d/actor/d_a_npc_hanjo.cpp) | `NPC_HANJO` → `fpcNm_NPC_HANJO_e` | REL | Hanch |
| [`d_a_npc_henna`](../src/d/actor/d_a_npc_henna.cpp) | `NPC_HENNA` → `fpcNm_NPC_HENNA_e` | REL | Hena |
| [`d_a_npc_henna0`](../src/d/actor/d_a_npc_henna0.cpp) | `NPC_HENNA0` → `fpcNm_NPC_HENNA0_e` | DOL | Henna 0 (unused?) |
| [`d_a_npc_hoz`](../src/d/actor/d_a_npc_hoz.cpp) | `NPC_HOZ` → `fpcNm_NPC_HOZ_e` | REL | Iza |
| [`d_a_npc_impal`](../src/d/actor/d_a_npc_impal.cpp) | `NPC_IMPAL` → `fpcNm_NPC_IMPAL_e` | REL | Impaz |
| [`d_a_npc_inko`](../src/d/actor/d_a_npc_inko.cpp) | `NPC_INKO` → `fpcNm_NPC_INKO_e` | REL | Inko (Hena's Bird) |
| [`d_a_npc_ins`](../src/d/actor/d_a_npc_ins.cpp) | `NPC_INS` → `fpcNm_NPC_INS_e` | REL | Agitha |
| [`d_a_npc_jagar`](../src/d/actor/d_a_npc_jagar.cpp) | `NPC_JAGAR` → `fpcNm_NPC_JAGAR_e` | REL | Jaggle |
| [`d_a_npc_kakashi`](../src/d/actor/d_a_npc_kakashi.cpp) | `NPC_KAKASHI` → `fpcNm_NPC_KAKASHI_e` | REL | Scarecrow |
| [`d_a_npc_kasi_hana`](../src/d/actor/d_a_npc_kasi_hana.cpp) | `NPC_KASIHANA` → `fpcNm_NPC_KASIHANA_e` | REL | Hannah |
| [`d_a_npc_kasi_kyu`](../src/d/actor/d_a_npc_kasi_kyu.cpp) | `NPC_KASIKYU` → `fpcNm_NPC_KASIKYU_e` | REL | Kili |
| [`d_a_npc_kasi_mich`](../src/d/actor/d_a_npc_kasi_mich.cpp) | `NPC_KASIMICH` → `fpcNm_NPC_KASIMICH_e` | REL | Misha |
| [`d_a_npc_kdk`](../src/d/actor/d_a_npc_kdk.cpp) | `NPC_KDK` → `fpcNm_NPC_KDK_e` | DOL | Temporary Cutscene Guy? |
| [`d_a_npc_kkri`](../src/d/actor/d_a_npc_kkri.cpp) | `NPC_KKRI` → `fpcNm_NPC_KKRI_e` | REL | Coro |
| [`d_a_npc_kn`](../src/d/actor/d_a_npc_kn.cpp) | `NPC_KN` → `fpcNm_NPC_KN_e` | REL | Hero's Shade |
| [`d_a_npc_knj`](../src/d/actor/d_a_npc_knj.cpp) | `NPC_KNJ` → `fpcNm_NPC_KNJ_e` | REL | Sage |
| [`d_a_npc_kolin`](../src/d/actor/d_a_npc_kolin.cpp) | `NPC_KOLIN` → `fpcNm_NPC_KOLIN_e` | REL | Colin |
| [`d_a_npc_kolinb`](../src/d/actor/d_a_npc_kolinb.cpp) | `NPC_KOLINB` → `fpcNm_NPC_KOLINB_e` | REL | Colin (Bedridden) / Ralis (Bedridden) |
| [`d_a_npc_ks`](../src/d/actor/d_a_npc_ks.cpp) | `NPC_KS` → `fpcNm_NPC_KS_e` | REL | Monkey NPC (Kozaru) |
| [`d_a_npc_kyury`](../src/d/actor/d_a_npc_kyury.cpp) | `NPC_KYURY` → `fpcNm_NPC_KYURY_e` | REL | Pergie |
| [`d_a_npc_len`](../src/d/actor/d_a_npc_len.cpp) | `NPC_LEN` → `fpcNm_NPC_LEN_e` | REL | Renado |
| [`d_a_npc_lf`](../src/d/actor/d_a_npc_lf.cpp) | `NPC_LF` → `fpcNm_NPC_LF_e` | REL | Little Fish |
| [`d_a_npc_lud`](../src/d/actor/d_a_npc_lud.cpp) | `NPC_LUD` → `fpcNm_NPC_LUD_e` | REL | Luda |
| [`d_a_npc_maro`](../src/d/actor/d_a_npc_maro.cpp) | `NPC_MARO` → `fpcNm_NPC_MARO_e` | REL | Malo |
| [`d_a_npc_midp`](../src/d/actor/d_a_npc_midp.cpp) | `NPC_MIDP` → `fpcNm_NPC_MIDP_e` | REL | Midna (True Form) |
| [`d_a_npc_mk`](../src/d/actor/d_a_npc_mk.cpp) | `NPC_MK` → `fpcNm_NPC_MK_e` | REL |  |
| [`d_a_npc_moi`](../src/d/actor/d_a_npc_moi.cpp) | `NPC_MOI` → `fpcNm_NPC_MOI_e` | REL | Rusl |
| [`d_a_npc_moir`](../src/d/actor/d_a_npc_moir.cpp) | `NPC_MOIR` → `fpcNm_NPC_MOIR_e` | REL | Rusl (Resistance) |
| [`d_a_npc_myna2`](../src/d/actor/d_a_npc_myna2.cpp) | `MYNA2` → `fpcNm_MYNA2_e` | REL | Plumm |
| [`d_a_npc_ne`](../src/d/actor/d_a_npc_ne.cpp) | `NPC_NE` → `fpcNm_NPC_NE_e` | REL | Cat (Neko) |
| [`d_a_npc_p2`](../src/d/actor/d_a_npc_p2.cpp) | `NPC_P2` → `fpcNm_NPC_P2_e` | REL |  |
| [`d_a_npc_pachi_besu`](../src/d/actor/d_a_npc_pachi_besu.cpp) | `NPC_PACHI_BESU` → `fpcNm_NPC_PACHI_BESU_e` | REL | Beth (Slingshot Tutorial) |
| [`d_a_npc_pachi_maro`](../src/d/actor/d_a_npc_pachi_maro.cpp) | `NPC_PACHI_MARO` → `fpcNm_NPC_PACHI_MARO_e` | REL | Malo (Slingshot Tutorial) |
| [`d_a_npc_pachi_taro`](../src/d/actor/d_a_npc_pachi_taro.cpp) | `NPC_PACHI_TARO` → `fpcNm_NPC_PACHI_TARO_e` | REL | Talo (Slingshot Tutorial) |
| [`d_a_npc_passer`](../src/d/actor/d_a_npc_passer.cpp) | `NPC_PASSER` → `fpcNm_NPC_PASSER_e` | REL | Hylian Passerby |
| [`d_a_npc_passer2`](../src/d/actor/d_a_npc_passer2.cpp) | `NPC_PASSER2` → `fpcNm_NPC_PASSER2_e` | REL | Low-Poly Hylian Passerby |
| [`d_a_npc_post`](../src/d/actor/d_a_npc_post.cpp) | `NPC_POST` → `fpcNm_NPC_POST_e` | REL | Postman |
| [`d_a_npc_pouya`](../src/d/actor/d_a_npc_pouya.cpp) | `NPC_POUYA` → `fpcNm_NPC_POUYA_e` | REL | Poe Merchant (Jovani?) |
| [`d_a_npc_prayer`](../src/d/actor/d_a_npc_prayer.cpp) | `NPC_PRAYER` → `fpcNm_NPC_PRAYER_e` | REL | Charlo |
| [`d_a_npc_raca`](../src/d/actor/d_a_npc_raca.cpp) | `NPC_RACA` → `fpcNm_NPC_RACA_e` | REL | Falbi |
| [`d_a_npc_rafrel`](../src/d/actor/d_a_npc_rafrel.cpp) | `NPC_RAFREL` → `fpcNm_NPC_RAFREL_e` | REL | Auru |
| [`d_a_npc_saru`](../src/d/actor/d_a_npc_saru.cpp) | `NPC_SARU` → `fpcNm_NPC_SARU_e` | REL | Monkey NPC |
| [`d_a_npc_seib`](../src/d/actor/d_a_npc_seib.cpp) | `NPC_SEIB` → `fpcNm_NPC_SEIB_e` | REL | Faron Spirit |
| [`d_a_npc_seic`](../src/d/actor/d_a_npc_seic.cpp) | `NPC_SEIC` → `fpcNm_NPC_SEIC_e` | REL | Eldin Spirit |
| [`d_a_npc_seid`](../src/d/actor/d_a_npc_seid.cpp) | `NPC_SEID` → `fpcNm_NPC_SEID_e` | REL | Lanayru Spirit |
| [`d_a_npc_seira`](../src/d/actor/d_a_npc_seira.cpp) | `NPC_SEIRA` → `fpcNm_NPC_SEIRA_e` | REL | Sera |
| [`d_a_npc_seira2`](../src/d/actor/d_a_npc_seira2.cpp) | `NPC_SERA2` → `fpcNm_NPC_SERA2_e` | REL | Sera (Shopkeeper) |
| [`d_a_npc_seirei`](../src/d/actor/d_a_npc_seirei.cpp) | `NPC_SEIREI` → `fpcNm_NPC_SEIREI_e` | REL | Light Spirit Ordona |
| [`d_a_npc_shad`](../src/d/actor/d_a_npc_shad.cpp) | `NPC_SHAD` → `fpcNm_NPC_SHAD_e` | REL | Shad |
| [`d_a_npc_shaman`](../src/d/actor/d_a_npc_shaman.cpp) | `NPC_SHAMAN` → `fpcNm_NPC_SHAMAN_e` | REL | Fanadi |
| [`d_a_npc_shoe`](../src/d/actor/d_a_npc_shoe.cpp) | `NPC_SHOE` → `fpcNm_NPC_SHOE_e` | REL | Soal |
| [`d_a_npc_shop0`](../src/d/actor/d_a_npc_shop0.cpp) | `NPC_SHOP0` → `fpcNm_NPC_SHOP0_e` | REL | Shop 0? |
| [`d_a_npc_shop_maro`](../src/d/actor/d_a_npc_shop_maro.cpp) | `NPC_SMARO` → `fpcNm_NPC_SMARO_e` | REL | Malo (Shopkeeper) |
| [`d_a_npc_sola`](../src/d/actor/d_a_npc_sola.cpp) | `NPC_SOLA` → `fpcNm_NPC_SOLA_e` | REL | Soldier A (Castle Town?) |
| [`d_a_npc_soldierA`](../src/d/actor/d_a_npc_soldierA.cpp) | `NPC_SOLDIERa` → `fpcNm_NPC_SOLDIERa_e` | REL | Soldier A (Castle Town?) |
| [`d_a_npc_soldierB`](../src/d/actor/d_a_npc_soldierB.cpp) | `NPC_SOLDIERb` → `fpcNm_NPC_SOLDIERb_e` | REL | Soldier B (Castle Town?) |
| [`d_a_npc_sq`](../src/d/actor/d_a_npc_sq.cpp) | `NPC_SQ` → `fpcNm_NPC_SQ_e` | REL | Squirrel (Talking, Ordon Village) |
| [`d_a_npc_taro`](../src/d/actor/d_a_npc_taro.cpp) | `NPC_TARO` → `fpcNm_NPC_TARO_e` | REL | Talo |
| [`d_a_npc_the`](../src/d/actor/d_a_npc_the.cpp) | `NPC_THE` → `fpcNm_NPC_THE_e` | REL | Telma |
| [`d_a_npc_theB`](../src/d/actor/d_a_npc_theB.cpp) | `NPC_THEB` → `fpcNm_NPC_THEB_e` | REL | Telma B |
| [`d_a_npc_tk`](../src/d/actor/d_a_npc_tk.cpp) | `NPC_TK` → `fpcNm_NPC_TK_e` | REL | Hawk (Taka) |
| [`d_a_npc_tkc`](../src/d/actor/d_a_npc_tkc.cpp) | `NPC_TKC` → `fpcNm_NPC_TKC_e` | REL | Ooccoo Jr. |
| [`d_a_npc_tkj`](../src/d/actor/d_a_npc_tkj.cpp) | `NPC_TKJ` → `fpcNm_NPC_TKJ_e` | REL | Oocca |
| [`d_a_npc_tkj2`](../src/d/actor/d_a_npc_tkj2.cpp) | `NPC_TKJ2` → `fpcNm_NPC_TKJ2_e` | REL | Oocca 2 |
| [`d_a_npc_tks`](../src/d/actor/d_a_npc_tks.cpp) | `NPC_TKS` → `fpcNm_NPC_TKS_e` | REL | Ooccoo |
| [`d_a_npc_toby`](../src/d/actor/d_a_npc_toby.cpp) | `NPC_TOBY` → `fpcNm_NPC_TOBY_e` | REL | Fyer |
| [`d_a_npc_tr`](../src/d/actor/d_a_npc_tr.cpp) | `NPC_TR` → `fpcNm_NPC_TR_e` | REL | Trout |
| [`d_a_npc_uri`](../src/d/actor/d_a_npc_uri.cpp) | `NPC_URI` → `fpcNm_NPC_URI_e` | REL | Uli |
| [`d_a_npc_worm`](../src/d/actor/d_a_npc_worm.cpp) | `NPC_WORM` → `fpcNm_NPC_WORM_e` | REL | Worm |
| [`d_a_npc_wrestler`](../src/d/actor/d_a_npc_wrestler.cpp) | `NPC_WRESTLER` → `fpcNm_NPC_WRESTLER_e` | REL | Goron (Fat) |
| [`d_a_npc_yamid`](../src/d/actor/d_a_npc_yamid.cpp) | `NPC_YAMID` → `fpcNm_NPC_YAMID_e` | REL | Twili (Fat) |
| [`d_a_npc_yamis`](../src/d/actor/d_a_npc_yamis.cpp) | `NPC_YAMIS` → `fpcNm_NPC_YAMIS_e` | REL | Twili (Short) |
| [`d_a_npc_yamit`](../src/d/actor/d_a_npc_yamit.cpp) | `NPC_YAMIT` → `fpcNm_NPC_YAMIT_e` | REL | Twili (Tall) |
| [`d_a_npc_yelia`](../src/d/actor/d_a_npc_yelia.cpp) | `NPC_YELIA` → `fpcNm_NPC_YELIA_e` | REL | Ilia |
| [`d_a_npc_ykm`](../src/d/actor/d_a_npc_ykm.cpp) | `NPC_YKM` → `fpcNm_NPC_YKM_e` | REL | Yeto |
| [`d_a_npc_ykw`](../src/d/actor/d_a_npc_ykw.cpp) | `NPC_YKW` → `fpcNm_NPC_YKW_e` | REL | Yeta |
| [`d_a_npc_zanb`](../src/d/actor/d_a_npc_zanb.cpp) | `NPC_ZANB` → `fpcNm_NPC_ZANB_e` | REL | Zant Boss (Unused?) |
| [`d_a_npc_zant`](../src/d/actor/d_a_npc_zant.cpp) | `NPC_ZANT` → `fpcNm_NPC_ZANT_e` | REL | Zant |
| [`d_a_npc_zelR`](../src/d/actor/d_a_npc_zelR.cpp) | `NPC_ZELR` → `fpcNm_NPC_ZELR_e` | REL | Zelda (Cloaked & Hooded) |
| [`d_a_npc_zelRo`](../src/d/actor/d_a_npc_zelRo.cpp) | `NPC_ZELRO` → `fpcNm_NPC_ZELRO_e` | REL | Zelda (Cloaked) |
| [`d_a_npc_zelda`](../src/d/actor/d_a_npc_zelda.cpp) | `NPC_ZELDA` → `fpcNm_NPC_ZELDA_e` | REL | Zelda |
| [`d_a_npc_zra`](../src/d/actor/d_a_npc_zra.cpp) | `NPC_ZRA` → `fpcNm_NPC_ZRA_e` | REL | Zora (Adult) |
| [`d_a_npc_zrc`](../src/d/actor/d_a_npc_zrc.cpp) | `NPC_ZRC` → `fpcNm_NPC_ZRC_e` | REL | Ralis |
| [`d_a_npc_zrz`](../src/d/actor/d_a_npc_zrz.cpp) | `NPC_ZRZ` → `fpcNm_NPC_ZRZ_e` | REL | Rutela |

## Objects (348)

| Source | Profile → process name | Runs from | In-game name |
|---|---|---|---|
| [`d_a_obj_Lv5Key`](../src/d/actor/d_a_obj_Lv5Key.cpp) | `Obj_Lv5Key` → `fpcNm_Obj_Lv5Key_e` | REL | Snowpeak Ruins Key Lock |
| [`d_a_obj_Turara`](../src/d/actor/d_a_obj_Turara.cpp) | `Obj_Turara` → `fpcNm_Obj_Turara_e` | REL | Icicle |
| [`d_a_obj_TvCdlst`](../src/d/actor/d_a_obj_TvCdlst.cpp) | `Obj_TvCdlst` → `fpcNm_Obj_TvCdlst_e` | REL | Ordon Torch Stand |
| [`d_a_obj_Y_taihou`](../src/d/actor/d_a_obj_Y_taihou.cpp) | `Obj_Ytaihou` → `fpcNm_Obj_Ytaihou_e` | REL | Snowpeak Ruins Cannon |
| [`d_a_obj_amiShutter`](../src/d/actor/d_a_obj_amiShutter.cpp) | `Obj_AmiShutter` → `fpcNm_Obj_AmiShutter_e` | REL | Drain Gate |
| [`d_a_obj_ari`](../src/d/actor/d_a_obj_ari.cpp) | `Obj_Ari` → `fpcNm_Obj_Ari_e` | REL | Insect - Ant |
| [`d_a_obj_automata`](../src/d/actor/d_a_obj_automata.cpp) | `OBJ_AUTOMATA` → `fpcNm_OBJ_AUTOMATA_e` | REL | Falbi's Music Box |
| [`d_a_obj_avalanche`](../src/d/actor/d_a_obj_avalanche.cpp) | `Obj_Avalanche` → `fpcNm_Obj_Avalanche_e` | REL | Avalanche |
| [`d_a_obj_balloon`](../src/d/actor/d_a_obj_balloon.cpp) | `OBJ_BALLOON` → `fpcNm_OBJ_BALLOON_e` | REL | Balloon |
| [`d_a_obj_barDesk`](../src/d/actor/d_a_obj_barDesk.cpp) | `Obj_BarDesk` → `fpcNm_Obj_BarDesk_e` | REL | Kakariko House Desk |
| [`d_a_obj_batta`](../src/d/actor/d_a_obj_batta.cpp) | `Obj_Batta` → `fpcNm_Obj_Batta_e` | REL | Insect - Grasshopper (Batta) |
| [`d_a_obj_bbox`](../src/d/actor/d_a_obj_bbox.cpp) | `Obj_BBox` → `fpcNm_Obj_BBox_e` | REL | B Box |
| [`d_a_obj_bed`](../src/d/actor/d_a_obj_bed.cpp) | `OBJ_BED` → `fpcNm_OBJ_BED_e` | REL | Bed |
| [`d_a_obj_bemos`](../src/d/actor/d_a_obj_bemos.cpp) | `Obj_Bemos` → `fpcNm_Obj_Bemos_e` | REL | Beamos (Movable Object) |
| [`d_a_obj_bhashi`](../src/d/actor/d_a_obj_bhashi.cpp) | `Obj_BHASHI` → `fpcNm_Obj_BHASHI_e` | REL | Pillar |
| [`d_a_obj_bhbridge`](../src/d/actor/d_a_obj_bhbridge.cpp) | `Obj_Bhbridge` → `fpcNm_Obj_Bhbridge_e` | REL | BH Bridge? |
| [`d_a_obj_bk_leaf`](../src/d/actor/d_a_obj_bk_leaf.cpp) | `Obj_BkLeaf` → `fpcNm_Obj_BkLeaf_e` | REL | Baba Stem Leaf |
| [`d_a_obj_bkdoor`](../src/d/actor/d_a_obj_bkdoor.cpp) | `Obj_BkDoor` → `fpcNm_Obj_BkDoor_e` | REL | Ranch Door |
| [`d_a_obj_bky_rock`](../src/d/actor/d_a_obj_bky_rock.cpp) | `BkyRock` → `fpcNm_BkyRock_e` | REL | Bomb Shack Rock |
| [`d_a_obj_bmWindow`](../src/d/actor/d_a_obj_bmWindow.cpp) | `Obj_BmWindow` → `fpcNm_Obj_BmWindow_e` | REL | Boomerang Window? |
| [`d_a_obj_bmshutter`](../src/d/actor/d_a_obj_bmshutter.cpp) | `Obj_BoomShutter` → `fpcNm_Obj_BoomShutter_e` | REL | Boomerang Shutter |
| [`d_a_obj_bombf`](../src/d/actor/d_a_obj_bombf.cpp) | `Obj_Bombf` → `fpcNm_Obj_Bombf_e` | REL | Bomb Flower |
| [`d_a_obj_bosswarp`](../src/d/actor/d_a_obj_bosswarp.cpp) | `Obj_BossWarp` → `fpcNm_Obj_BossWarp_e` | REL | Boss Warp |
| [`d_a_obj_boumato`](../src/d/actor/d_a_obj_boumato.cpp) | `OBJ_BOUMATO` → `fpcNm_OBJ_BOUMATO_e` | REL | Stick Target |
| [`d_a_obj_brakeeff`](../src/d/actor/d_a_obj_brakeeff.cpp) | `OBJ_BEF` → `fpcNm_OBJ_BEF_e` | REL |  |
| [`d_a_obj_brg`](../src/d/actor/d_a_obj_brg.cpp) | `OBJ_BRG` → `fpcNm_OBJ_BRG_e` | REL | Bridge |
| [`d_a_obj_bsGate`](../src/d/actor/d_a_obj_bsGate.cpp) | `Obj_BsGate` → `fpcNm_Obj_BsGate_e` | REL | Boss Gate |
| [`d_a_obj_bubblePilar`](../src/d/actor/d_a_obj_bubblePilar.cpp) | `Obj_awaPlar` → `fpcNm_Obj_awaPlar_e` | REL | Bubble Pillar |
| [`d_a_obj_burnbox`](../src/d/actor/d_a_obj_burnbox.cpp) | `Obj_BurnBox` → `fpcNm_Obj_BurnBox_e` | REL | Burn Box |
| [`d_a_obj_carry`](../src/d/actor/d_a_obj_carry.cpp) | `Obj_Carry` → `fpcNm_Obj_Carry_e` | REL | Carryable Object |
| [`d_a_obj_catdoor`](../src/d/actor/d_a_obj_catdoor.cpp) | `Obj_CatDoor` → `fpcNm_Obj_CatDoor_e` | REL | Cat Door |
| [`d_a_obj_cb`](../src/d/actor/d_a_obj_cb.cpp) | `OBJ_CB` → `fpcNm_OBJ_CB_e` | REL | Castle Block? |
| [`d_a_obj_cblock`](../src/d/actor/d_a_obj_cblock.cpp) | `Obj_ChainBlock` → `fpcNm_Obj_ChainBlock_e` | REL | Castle Block? |
| [`d_a_obj_cboard`](../src/d/actor/d_a_obj_cboard.cpp) | `Obj_Cboard` → `fpcNm_Obj_Cboard_e` | REL | Clear Board |
| [`d_a_obj_cdoor`](../src/d/actor/d_a_obj_cdoor.cpp) | `Obj_Cdoor` → `fpcNm_Obj_Cdoor_e` | REL | Chain Door |
| [`d_a_obj_chandelier`](../src/d/actor/d_a_obj_chandelier.cpp) | `Obj_Chandelier` → `fpcNm_Obj_Chandelier_e` | REL | Hyrule Castle Chandelier |
| [`d_a_obj_chest`](../src/d/actor/d_a_obj_chest.cpp) | `Obj_Chest` → `fpcNm_Obj_Chest_e` | REL | Cabinet |
| [`d_a_obj_cho`](../src/d/actor/d_a_obj_cho.cpp) | `Obj_Cho` → `fpcNm_Obj_Cho_e` | REL | Insect - Butterfly |
| [`d_a_obj_cowdoor`](../src/d/actor/d_a_obj_cowdoor.cpp) | `Obj_Cowdoor` → `fpcNm_Obj_Cowdoor_e` | REL | Ordon Ranch Stable Door |
| [`d_a_obj_crope`](../src/d/actor/d_a_obj_crope.cpp) | `Obj_Crope` → `fpcNm_Obj_Crope_e` | REL | Wolf Tightrope |
| [`d_a_obj_crvfence`](../src/d/actor/d_a_obj_crvfence.cpp) | `Obj_CRVFENCE` → `fpcNm_Obj_CRVFENCE_e` | REL | Caravan Fence |
| [`d_a_obj_crvgate`](../src/d/actor/d_a_obj_crvgate.cpp) | `Obj_CRVGATE` → `fpcNm_Obj_CRVGATE_e` | REL | Caravan Gate |
| [`d_a_obj_crvhahen`](../src/d/actor/d_a_obj_crvhahen.cpp) | `Obj_CRVHAHEN` → `fpcNm_Obj_CRVHAHEN_e` | REL | Bulblin Camp Caraven Wooden Fence Fragments |
| [`d_a_obj_crvlh_down`](../src/d/actor/d_a_obj_crvlh_down.cpp) | `Obj_CRVLH_DW` → `fpcNm_Obj_CRVLH_DW_e` | REL | Bulblin Tower (Lower Half) |
| [`d_a_obj_crvlh_up`](../src/d/actor/d_a_obj_crvlh_up.cpp) | `Obj_CRVLH_UP` → `fpcNm_Obj_CRVLH_UP_e` | REL | Bulblin Tower (Upper Half) |
| [`d_a_obj_crvsteel`](../src/d/actor/d_a_obj_crvsteel.cpp) | `Obj_CRVSTEEL` → `fpcNm_Obj_CRVSTEEL_e` | REL | Caravan Steel |
| [`d_a_obj_crystal`](../src/d/actor/d_a_obj_crystal.cpp) | `Obj_Crystal` → `fpcNm_Obj_Crystal_e` | REL | Crystal Switch |
| [`d_a_obj_cwall`](../src/d/actor/d_a_obj_cwall.cpp) | `Obj_ChainWall` → `fpcNm_Obj_ChainWall_e` | REL | Chain Wall |
| [`d_a_obj_damCps`](../src/d/actor/d_a_obj_damCps.cpp) | `Obj_DamCps` → `fpcNm_Obj_DamCps_e` | DOL | Damage Cylinder |
| [`d_a_obj_dan`](../src/d/actor/d_a_obj_dan.cpp) | `Obj_Dan` → `fpcNm_Obj_Dan_e` | REL | Insect - Pillbug |
| [`d_a_obj_digholl`](../src/d/actor/d_a_obj_digholl.cpp) | `Obj_Digholl` → `fpcNm_Obj_Digholl_e` | REL | Wolf Dig Place (Passage) |
| [`d_a_obj_digplace`](../src/d/actor/d_a_obj_digplace.cpp) | `Obj_Digpl` → `fpcNm_Obj_Digpl_e` | REL | Wolf Dig Place (Treasure) |
| [`d_a_obj_digsnow`](../src/d/actor/d_a_obj_digsnow.cpp) | `Obj_DigSnow` → `fpcNm_Obj_DigSnow_e` | REL | Wolf Dig Place (Snow) |
| [`d_a_obj_dmelevator`](../src/d/actor/d_a_obj_dmelevator.cpp) | `Obj_Elevator` → `fpcNm_Obj_Elevator_e` | REL | Death Mountain Elevator |
| [`d_a_obj_drop`](../src/d/actor/d_a_obj_drop.cpp) | `Obj_Drop` → `fpcNm_Obj_Drop_e` | REL | Tear of Light |
| [`d_a_obj_dust`](../src/d/actor/d_a_obj_dust.cpp) | `Obj_DUST` → `fpcNm_Obj_DUST_e` | REL | Dust |
| [`d_a_obj_eff`](../src/d/actor/d_a_obj_eff.cpp) | `Obj_Eff` → `fpcNm_Obj_Eff_e` | REL | Object Effect? |
| [`d_a_obj_enemy_create`](../src/d/actor/d_a_obj_enemy_create.cpp) | `Obj_E_CREATE` → `fpcNm_Obj_E_CREATE_e` | REL | Enemy Spawner |
| [`d_a_obj_fallobj`](../src/d/actor/d_a_obj_fallobj.cpp) | `Obj_FallObj` → `fpcNm_Obj_FallObj_e` | REL | Fall Object? |
| [`d_a_obj_fan`](../src/d/actor/d_a_obj_fan.cpp) | `Obj_Fan` → `fpcNm_Obj_Fan_e` | REL | (City in the Sky?) Fan) |
| [`d_a_obj_fchain`](../src/d/actor/d_a_obj_fchain.cpp) | `Obj_Fchain` → `fpcNm_Obj_Fchain_e` | REL | Wolf Chain Shackle |
| [`d_a_obj_fireWood`](../src/d/actor/d_a_obj_fireWood.cpp) | `Obj_FireWood` → `fpcNm_Obj_FireWood_e` | REL | Kakariko Stove Flame? |
| [`d_a_obj_fireWood2`](../src/d/actor/d_a_obj_fireWood2.cpp) | `Obj_FireWood2` → `fpcNm_Obj_Lv1Cdl00_e` | REL | Firewood 2 (Flame) |
| [`d_a_obj_firepillar`](../src/d/actor/d_a_obj_firepillar.cpp) | `Obj_FirePillar` → `fpcNm_Obj_FirePillar_e` | REL | Fire Pillar |
| [`d_a_obj_firepillar2`](../src/d/actor/d_a_obj_firepillar2.cpp) | `Obj_FirePillar2` → `fpcNm_Obj_FirePillar2_e` | REL | Lava Fire Pillar |
| [`d_a_obj_flag`](../src/d/actor/d_a_obj_flag.cpp) | `Obj_Flag` → `fpcNm_Obj_Flag_e` | REL | Flag 1 |
| [`d_a_obj_flag2`](../src/d/actor/d_a_obj_flag2.cpp) | `Obj_Flag2` → `fpcNm_Obj_Flag2_e` | REL | Flag 2 |
| [`d_a_obj_flag3`](../src/d/actor/d_a_obj_flag3.cpp) | `Obj_Flag3` → `fpcNm_Obj_Flag3_e` | REL | Flag 3 |
| [`d_a_obj_fmobj`](../src/d/actor/d_a_obj_fmobj.cpp) | `OBJ_FMOBJ` → `fpcNm_OBJ_FMOBJ_e` | REL | Fyrus Object? |
| [`d_a_obj_food`](../src/d/actor/d_a_obj_food.cpp) | `OBJ_FOOD` → `fpcNm_OBJ_FOOD_e` | REL | Dog Bone |
| [`d_a_obj_fw`](../src/d/actor/d_a_obj_fw.cpp) | `OBJ_FW` → `fpcNm_OBJ_FW_e` | REL | Firewood |
| [`d_a_obj_gadget`](../src/d/actor/d_a_obj_gadget.cpp) | `OBJ_GADGET` → `fpcNm_OBJ_GADGET_e` | REL | Gadget |
| [`d_a_obj_ganonwall`](../src/d/actor/d_a_obj_ganonwall.cpp) | `Obj_GanonWall` → `fpcNm_Obj_GanonWall_e` | REL | Gannon Wall |
| [`d_a_obj_ganonwall2`](../src/d/actor/d_a_obj_ganonwall2.cpp) | `Obj_GanonWall2` → `fpcNm_Obj_GanonWall2_e` | REL | Ganon Wall 2 |
| [`d_a_obj_gb`](../src/d/actor/d_a_obj_gb.cpp) | `OBJ_GB` → `fpcNm_OBJ_GB_e` | REL | Ganondorf Barrier |
| [`d_a_obj_geyser`](../src/d/actor/d_a_obj_geyser.cpp) | `Obj_Geyser` → `fpcNm_Obj_Geyser_e` | REL | Geyser |
| [`d_a_obj_glowSphere`](../src/d/actor/d_a_obj_glowSphere.cpp) | `Obj_glowSphere` → `fpcNm_Obj_glowSphere_e` | REL | STAR Game Glow Sphere |
| [`d_a_obj_gm`](../src/d/actor/d_a_obj_gm.cpp) | `OBJ_GM` → `fpcNm_OBJ_GM_e` | REL | Gohma Egg Container |
| [`d_a_obj_goGate`](../src/d/actor/d_a_obj_goGate.cpp) | `Obj_GoGate` → `fpcNm_Obj_GoGate_e` | REL | Goron Gate |
| [`d_a_obj_gomikabe`](../src/d/actor/d_a_obj_gomikabe.cpp) | `Obj_GOMIKABE` → `fpcNm_Obj_GOMIKABE_e` | REL | Garbage Wall |
| [`d_a_obj_gpTaru`](../src/d/actor/d_a_obj_gpTaru.cpp) | `Obj_GpTaru` → `fpcNm_Obj_GpTaru_e` | REL | Gunpowder Barrel |
| [`d_a_obj_gra2`](../src/d/actor/d_a_obj_gra2.cpp) | `OBJ_GRA` → `fpcNm_OBJ_GRA_e` | REL | Goron A |
| [`d_a_obj_graWall`](../src/d/actor/d_a_obj_graWall.cpp) | `GRA_WALL` → `fpcNm_GRA_WALL_e` | REL | Goron Wall? |
| [`d_a_obj_gra_rock`](../src/d/actor/d_a_obj_gra_rock.cpp) | `Obj_GraRock` → `fpcNm_Obj_GraRock_e` | REL | Goron Entombing Rock |
| [`d_a_obj_grave_stone`](../src/d/actor/d_a_obj_grave_stone.cpp) | `Obj_GraveStone` → `fpcNm_Obj_GraveStone_e` | REL | Zora Gravestone |
| [`d_a_obj_groundwater`](../src/d/actor/d_a_obj_groundwater.cpp) | `GRDWATER` → `fpcNm_GRDWATER_e` | REL | Ground Water |
| [`d_a_obj_grz_rock`](../src/d/actor/d_a_obj_grz_rock.cpp) | `Obj_GrzRock` → `fpcNm_Obj_GrzRock_e` | REL | Hidden Village Rockslide |
| [`d_a_obj_h_saku`](../src/d/actor/d_a_obj_h_saku.cpp) | `Obj_H_Saku` → `fpcNm_Obj_H_Saku_e` | REL | H - Fence |
| [`d_a_obj_hakai_brl`](../src/d/actor/d_a_obj_hakai_brl.cpp) | `Obj_HBarrel` → `fpcNm_Obj_HBarrel_e` | REL | Destructable Barrel |
| [`d_a_obj_hakai_ftr`](../src/d/actor/d_a_obj_hakai_ftr.cpp) | `Obj_HFtr` → `fpcNm_Obj_HFtr_e` | REL | Destruction Furniture |
| [`d_a_obj_hasu2`](../src/d/actor/d_a_obj_hasu2.cpp) | `Obj_MHasu` → `fpcNm_Obj_MHasu_e` | REL | Lily Pad? |
| [`d_a_obj_hata`](../src/d/actor/d_a_obj_hata.cpp) | `Obj_Hata` → `fpcNm_Obj_Hata_e` | REL | Flag ??? |
| [`d_a_obj_hb`](../src/d/actor/d_a_obj_hb.cpp) | `OBJ_HB` → `fpcNm_OBJ_HB_e` | REL | Huge Baba Seed |
| [`d_a_obj_hbombkoya`](../src/d/actor/d_a_obj_hbombkoya.cpp) | `Obj_HBombkoya` → `fpcNm_Obj_HBombkoya_e` | REL | Destructable Bomb House |
| [`d_a_obj_heavySw`](../src/d/actor/d_a_obj_heavySw.cpp) | `Obj_HeavySw` → `fpcNm_Obj_HeavySw_e` | REL | Heavy Switch |
| [`d_a_obj_hfuta`](../src/d/actor/d_a_obj_hfuta.cpp) | `Obj_Hfuta` → `fpcNm_Obj_Hfuta_e` | REL | Crawling? |
| [`d_a_obj_hhashi`](../src/d/actor/d_a_obj_hhashi.cpp) | `Obj_HHASHI` → `fpcNm_Obj_HHASHI_e` | REL | Pillar |
| [`d_a_obj_hsTarget`](../src/d/actor/d_a_obj_hsTarget.cpp) | `Obj_HsTarget` → `fpcNm_Obj_HsTarget_e` | REL | Clawshot Target |
| [`d_a_obj_ice_l`](../src/d/actor/d_a_obj_ice_l.cpp) | `Obj_Ice_l` → `fpcNm_Obj_Ice_l_e` | REL | Ice (Large) |
| [`d_a_obj_ice_s`](../src/d/actor/d_a_obj_ice_s.cpp) | `Obj_Ice_s` → `fpcNm_Obj_Ice_s_e` | REL | Ice (Small) |
| [`d_a_obj_iceblock`](../src/d/actor/d_a_obj_iceblock.cpp) | `Obj_IceBlock` → `fpcNm_Obj_IceBlock_e` | REL | Sliding Ice Block |
| [`d_a_obj_iceleaf`](../src/d/actor/d_a_obj_iceleaf.cpp) | `Obj_IceLeaf` → `fpcNm_Obj_IceLeaf_e` | REL | Ice Leaf |
| [`d_a_obj_ihasi`](../src/d/actor/d_a_obj_ihasi.cpp) | `OBJ_IHASI` → `fpcNm_OBJ_IHASI_e` | REL | Ice Bridge? |
| [`d_a_obj_ikada`](../src/d/actor/d_a_obj_ikada.cpp) | `Obj_Ikada` → `fpcNm_Obj_Ikada_e` | REL | Raft |
| [`d_a_obj_inobone`](../src/d/actor/d_a_obj_inobone.cpp) | `Obj_InoBone` → `fpcNm_Obj_InoBone_e` | REL | Boar Bone |
| [`d_a_obj_ita`](../src/d/actor/d_a_obj_ita.cpp) | `Obj_ITA` → `fpcNm_Obj_ITA_e` | REL | Plank |
| [`d_a_obj_itamato`](../src/d/actor/d_a_obj_itamato.cpp) | `OBJ_ITAMATO` → `fpcNm_OBJ_ITAMATO_e` | REL | Plank Target |
| [`d_a_obj_item`](../src/d/actor/d_a_obj_item.cpp) | `ITEM` → `fpcNm_ITEM_e` | DOL | Item (Rupee, Arrow, Heart, etc) Object Actor |
| [`d_a_obj_ito`](../src/d/actor/d_a_obj_ito.cpp) | `OBJ_ITO` → `fpcNm_OBJ_ITO_e` | REL |  |
| [`d_a_obj_kabuto`](../src/d/actor/d_a_obj_kabuto.cpp) | `Obj_Kabuto` → `fpcNm_Obj_Kabuto_e` | REL | Insect - Beetle |
| [`d_a_obj_kag`](../src/d/actor/d_a_obj_kag.cpp) | `Obj_Kag` → `fpcNm_Obj_Kag_e` | REL | Insect - Dayfly |
| [`d_a_obj_kage`](../src/d/actor/d_a_obj_kage.cpp) | `OBJ_KAGE` → `fpcNm_OBJ_KAGE_e` | REL | House - Cage |
| [`d_a_obj_kago`](../src/d/actor/d_a_obj_kago.cpp) | `OBJ_KAGO` → `fpcNm_OBJ_KAGO_e` | REL | Basket |
| [`d_a_obj_kaisou`](../src/d/actor/d_a_obj_kaisou.cpp) | `Obj_Kaisou` → `fpcNm_Obj_Kaisou_e` | REL | Seaweed |
| [`d_a_obj_kamakiri`](../src/d/actor/d_a_obj_kamakiri.cpp) | `Obj_Kam` → `fpcNm_Obj_Kam_e` | REL | Insect - Mantis (Kamakiri) |
| [`d_a_obj_kanban2`](../src/d/actor/d_a_obj_kanban2.cpp) | `OBJ_KANBAN2` → `fpcNm_OBJ_KANBAN2_e` | REL | Sign 2 (Shredded Sign) |
| [`d_a_obj_kantera`](../src/d/actor/d_a_obj_kantera.cpp) | `Obj_Kantera` → `fpcNm_Obj_Kantera_e` | REL | Lantern |
| [`d_a_obj_katatsumuri`](../src/d/actor/d_a_obj_katatsumuri.cpp) | `Obj_Kat` → `fpcNm_Obj_Kat_e` | REL | Insect - Snail (Katatsumuri) |
| [`d_a_obj_kazeneko`](../src/d/actor/d_a_obj_kazeneko.cpp) | `Obj_KazeNeko` → `fpcNm_Obj_KazeNeko_e` | REL | Weather Vane |
| [`d_a_obj_kbacket`](../src/d/actor/d_a_obj_kbacket.cpp) | `OBJ_KBACKET` → `fpcNm_OBJ_KBACKET_e` | REL | (Kakariko?) Bucket |
| [`d_a_obj_kbox`](../src/d/actor/d_a_obj_kbox.cpp) | `OBJ_KBOX` → `fpcNm_OBJ_KBOX_e` | REL | (Kakariko?) Crate |
| [`d_a_obj_key`](../src/d/actor/d_a_obj_key.cpp) | `OBJ_KEY` → `fpcNm_OBJ_KEY_e` | REL | Key |
| [`d_a_obj_keyhole`](../src/d/actor/d_a_obj_keyhole.cpp) | `OBJ_KEYHOLE` → `fpcNm_OBJ_KEYHOLE_e` | REL | Small Key Door Chains? |
| [`d_a_obj_kgate`](../src/d/actor/d_a_obj_kgate.cpp) | `Obj_KkrGate` → `fpcNm_Obj_KkrGate_e` | REL | Coro Gate |
| [`d_a_obj_ki`](../src/d/actor/d_a_obj_ki.cpp) | `OBJ_KI` → `fpcNm_OBJ_KI_e` | REL | Tree |
| [`d_a_obj_kiPot`](../src/d/actor/d_a_obj_kiPot.cpp) | `Obj_KiPot` → `fpcNm_Obj_KiPot_e` | REL | Coro Pot |
| [`d_a_obj_kita`](../src/d/actor/d_a_obj_kita.cpp) | `OBJ_KITA` → `fpcNm_OBJ_KITA_e` | REL | Wind Plank |
| [`d_a_obj_kjgjs`](../src/d/actor/d_a_obj_kjgjs.cpp) | `Obj_KJgjs` → `fpcNm_Obj_KJgjs_e` | REL | Object - Kjgjs |
| [`d_a_obj_kkanban`](../src/d/actor/d_a_obj_kkanban.cpp) | `Obj_KKanban` → `fpcNm_Obj_KKanban_e` | REL | Kakariko Sign |
| [`d_a_obj_klift00`](../src/d/actor/d_a_obj_klift00.cpp) | `Obj_KLift00` → `fpcNm_Obj_KLift00_e` | REL | Water Wheel/Gear Lift |
| [`d_a_obj_knBullet`](../src/d/actor/d_a_obj_knBullet.cpp) | `KN_BULLET` → `fpcNm_KN_BULLET_e` | REL | Hero's Shade Energy Ball? (Knight Bullet) |
| [`d_a_obj_kshutter`](../src/d/actor/d_a_obj_kshutter.cpp) | `Obj_Kshutter` → `fpcNm_Obj_Kshutter_e` | REL | Lakebed Temple Boss Door |
| [`d_a_obj_ktOnFire`](../src/d/actor/d_a_obj_ktOnFire.cpp) | `Tag_KtOnFire` → `fpcNm_Tag_KtOnFire_e` | REL | Lantern Fire |
| [`d_a_obj_kuwagata`](../src/d/actor/d_a_obj_kuwagata.cpp) | `Obj_Kuw` → `fpcNm_Obj_Kuw_e` | REL | Insect - Stag Beetle |
| [`d_a_obj_kwheel00`](../src/d/actor/d_a_obj_kwheel00.cpp) | `Obj_KWheel00` → `fpcNm_Obj_KWheel00_e` | REL | Water Wheel/Gear |
| [`d_a_obj_kwheel01`](../src/d/actor/d_a_obj_kwheel01.cpp) | `Obj_KWheel01` → `fpcNm_Obj_KWheel01_e` | REL | Water Wheel/Gear |
| [`d_a_obj_kznkarm`](../src/d/actor/d_a_obj_kznkarm.cpp) | `Obj_KznkArm` → `fpcNm_Obj_KznkArm_e` | REL |  |
| [`d_a_obj_ladder`](../src/d/actor/d_a_obj_ladder.cpp) | `Obj_Ladder` → `fpcNm_Obj_Ladder_e` | REL | Ladder |
| [`d_a_obj_laundry`](../src/d/actor/d_a_obj_laundry.cpp) | `Obj_Laundry` → `fpcNm_Obj_Laundry_e` | REL | Laundry |
| [`d_a_obj_laundry_rope`](../src/d/actor/d_a_obj_laundry_rope.cpp) | `Obj_LndRope` → `fpcNm_Obj_LndRope_e` | REL | Laundry Rope |
| [`d_a_obj_lbox`](../src/d/actor/d_a_obj_lbox.cpp) | `OBJ_LBOX` → `fpcNm_OBJ_LBOX_e` | REL | L - Box (Large Box?) |
| [`d_a_obj_life_container`](../src/d/actor/d_a_obj_life_container.cpp) | `Obj_LifeContainer` → `fpcNm_Obj_LifeContainer_e` | REL | Heart Piece |
| [`d_a_obj_lp`](../src/d/actor/d_a_obj_lp.cpp) | `OBJ_LP` → `fpcNm_OBJ_LP_e` | REL | Lily Pad |
| [`d_a_obj_lv1Candle00`](../src/d/actor/d_a_obj_lv1Candle00.cpp) | `Obj_Lv1Cdl00` → `fpcNm_Obj_Lv1Cdl00_e` | REL | Forest Temple Torch 00 |
| [`d_a_obj_lv1Candle01`](../src/d/actor/d_a_obj_lv1Candle01.cpp) | `Obj_Lv1Cdl01` → `fpcNm_Obj_Lv1Cdl01_e` | REL | Forest Temple Torch 01 |
| [`d_a_obj_lv2Candle`](../src/d/actor/d_a_obj_lv2Candle.cpp) | `Obj_Lv2Candle` → `fpcNm_Obj_Lv2Candle_e` | REL | Goron Mines Torch |
| [`d_a_obj_lv3Candle`](../src/d/actor/d_a_obj_lv3Candle.cpp) | `Obj_Lv3Candle` → `fpcNm_Obj_Lv3Candle_e` | REL | Lakebed Temple Torch |
| [`d_a_obj_lv3Water`](../src/d/actor/d_a_obj_lv3Water.cpp) | `Obj_Lv3Water` → `fpcNm_Obj_Lv3Water_e` | REL | Lakebed Temple Water |
| [`d_a_obj_lv3Water2`](../src/d/actor/d_a_obj_lv3Water2.cpp) | `Obj_Lv3Water2` → `fpcNm_Obj_Lv3Water2_e` | REL | Lakebed Temple Central Room Water |
| [`d_a_obj_lv3WaterB`](../src/d/actor/d_a_obj_lv3WaterB.cpp) | `OBJ_LV3WATERB` → `fpcNm_OBJ_LV3WATERB_e` | REL | Lakebed Temple Water (Boss) |
| [`d_a_obj_lv3saka00`](../src/d/actor/d_a_obj_lv3saka00.cpp) | `Obj_Lv3R10Saka` → `fpcNm_Obj_Lv3R10Saka_e` | REL | Lakebed Temple Spiral 00 |
| [`d_a_obj_lv3waterEff`](../src/d/actor/d_a_obj_lv3waterEff.cpp) | `Obj_WaterEff` → `fpcNm_Obj_WaterEff_e` | REL | Room 09 Water |
| [`d_a_obj_lv4CandleDemoTag`](../src/d/actor/d_a_obj_lv4CandleDemoTag.cpp) | `Tag_Lv4CandleDm` → `fpcNm_Tag_Lv4CandleDm_e` | REL | Arbiter's Grounds Torch Cutscene Tag |
| [`d_a_obj_lv4CandleTag`](../src/d/actor/d_a_obj_lv4CandleTag.cpp) | `Tag_Lv4Candle` → `fpcNm_Tag_Lv4Candle_e` | REL | Arbiter's Grounds Torch Tag |
| [`d_a_obj_lv4EdShutter`](../src/d/actor/d_a_obj_lv4EdShutter.cpp) | `Obj_Lv4EdShutter` → `fpcNm_Obj_Lv4EdShutter_e` | REL | Arbiter's Grounds Death Sword Gate |
| [`d_a_obj_lv4Gate`](../src/d/actor/d_a_obj_lv4Gate.cpp) | `Obj_Lv4Gate` → `fpcNm_Obj_Lv4Gate_e` | REL | Arbiter's Grounds Gate |
| [`d_a_obj_lv4HsTarget`](../src/d/actor/d_a_obj_lv4HsTarget.cpp) | `Obj_Lv4HsTarget` → `fpcNm_Obj_Lv4HsTarget_e` | REL | Arbiter's Grounds Clawshot Target |
| [`d_a_obj_lv4PoGate`](../src/d/actor/d_a_obj_lv4PoGate.cpp) | `Obj_Lv4PoGate` → `fpcNm_Obj_Lv4PoGate_e` | REL | Arbiter's Grounds Poe Gate |
| [`d_a_obj_lv4RailWall`](../src/d/actor/d_a_obj_lv4RailWall.cpp) | `Obj_Lv4RailWall` → `fpcNm_Obj_Lv4RailWall_e` | REL | Arbiters Grounds Spinner Rail Wall (Stallord Arena) |
| [`d_a_obj_lv4SlideWall`](../src/d/actor/d_a_obj_lv4SlideWall.cpp) | `Obj_Lv4SlideWall` → `fpcNm_Obj_Lv4SlideWall_e` | REL | Arbiter's Grounds Sliding Wall |
| [`d_a_obj_lv4bridge`](../src/d/actor/d_a_obj_lv4bridge.cpp) | `Obj_Lv4Bridge` → `fpcNm_Obj_Lv4Bridge_e` | REL | Arbiter's Grounds Bridge |
| [`d_a_obj_lv4chandelier`](../src/d/actor/d_a_obj_lv4chandelier.cpp) | `Obj_Lv4Chan` → `fpcNm_Obj_Lv4Chan_e` | REL | Arbiter's Grounds Chandelier |
| [`d_a_obj_lv4digsand`](../src/d/actor/d_a_obj_lv4digsand.cpp) | `Obj_Lv4DigSand` → `fpcNm_Obj_Lv4DigSand_e` | REL | Arbiter's Grounds Digging Sand |
| [`d_a_obj_lv4floor`](../src/d/actor/d_a_obj_lv4floor.cpp) | `Obj_Lv4Floor` → `fpcNm_Obj_Lv4Floor_e` | REL | Arbiter's Grounds Floor |
| [`d_a_obj_lv4gear`](../src/d/actor/d_a_obj_lv4gear.cpp) | `Obj_Lv4Gear` → `fpcNm_Obj_Lv4Gear_e` | REL | Arbiter's Grounds Spinner Gear |
| [`d_a_obj_lv4prelvtr`](../src/d/actor/d_a_obj_lv4prelvtr.cpp) | `Obj_PRElvtr` → `fpcNm_Obj_PRElvtr_e` | REL | Arbiter's Grounds Elevator? |
| [`d_a_obj_lv4prwall`](../src/d/actor/d_a_obj_lv4prwall.cpp) | `Obj_Lv4PRwall` → `fpcNm_Obj_Lv4PRwall_e` | REL | Arbiter's Grounds Rail Wall (Not Stallord?) |
| [`d_a_obj_lv4sand`](../src/d/actor/d_a_obj_lv4sand.cpp) | `Obj_Lv4Sand` → `fpcNm_Obj_Lv4Sand_e` | REL | Arbiter's Grounds Sand (Stallord Arena) |
| [`d_a_obj_lv5FloorBoard`](../src/d/actor/d_a_obj_lv5FloorBoard.cpp) | `Obj_Lv5FBoard` → `fpcNm_Obj_Lv5FBoard_e` | REL | Snowpeak Ruins Destructable Floor |
| [`d_a_obj_lv5IceWall`](../src/d/actor/d_a_obj_lv5IceWall.cpp) | `Obj_IceWall` → `fpcNm_Obj_IceWall_e` | REL | Snowpeak Ruins Ice Wall |
| [`d_a_obj_lv5SwIce`](../src/d/actor/d_a_obj_lv5SwIce.cpp) | `Obj_Lv5SwIce` → `fpcNm_Obj_Lv5SwIce_e` | REL | Snowpeak Ruins Ice Switch |
| [`d_a_obj_lv5ychndlr`](../src/d/actor/d_a_obj_lv5ychndlr.cpp) | `Obj_Ychndlr` → `fpcNm_Obj_Ychndlr_e` | REL | Snowpeak Ruins Chandelier (Swinging Platform) |
| [`d_a_obj_lv5yiblltray`](../src/d/actor/d_a_obj_lv5yiblltray.cpp) | `Obj_YIblltray` → `fpcNm_Obj_YIblltray_e` | REL | Snowpeak Ruins Cannonball Transporter |
| [`d_a_obj_lv6ChangeGate`](../src/d/actor/d_a_obj_lv6ChangeGate.cpp) | `Obj_Lv6ChgGate` → `fpcNm_Obj_Lv6ChgGate_e` | REL | Temple of Time Change Block? |
| [`d_a_obj_lv6FurikoTrap`](../src/d/actor/d_a_obj_lv6FurikoTrap.cpp) | `Obj_Lv6FuriTrap` → `fpcNm_Obj_Lv6FuriTrap_e` | REL | Temple of Time Pendulum Trap |
| [`d_a_obj_lv6Lblock`](../src/d/actor/d_a_obj_lv6Lblock.cpp) | `Obj_Lv6Lblock` → `fpcNm_Obj_Lv6Lblock_e` | REL | Temple of Time L Block |
| [`d_a_obj_lv6SwGate`](../src/d/actor/d_a_obj_lv6SwGate.cpp) | `Obj_Lv6SwGate` → `fpcNm_Obj_Lv6SwGate_e` | REL | Temple of Time Switch Gate |
| [`d_a_obj_lv6SzGate`](../src/d/actor/d_a_obj_lv6SzGate.cpp) | `Obj_Lv6SzGate` → `fpcNm_Obj_Lv6SzGate_e` | REL | Temple of Time Stone Statue Gate |
| [`d_a_obj_lv6Tenbin`](../src/d/actor/d_a_obj_lv6Tenbin.cpp) | `Obj_Lv6Tenbin` → `fpcNm_Obj_Lv6Tenbin_e` | REL | Temple of Time Scale |
| [`d_a_obj_lv6TogeRoll`](../src/d/actor/d_a_obj_lv6TogeRoll.cpp) | `Obj_Lv6TogeRoll` → `fpcNm_Obj_Lv6TogeRoll_e` | REL | Temple of Time Spike Roller |
| [`d_a_obj_lv6TogeTrap`](../src/d/actor/d_a_obj_lv6TogeTrap.cpp) | `Obj_Lv6TogeTrap` → `fpcNm_Obj_Lv6TogeTrap_e` | REL | Temple of Time Spiked Trap |
| [`d_a_obj_lv6bemos`](../src/d/actor/d_a_obj_lv6bemos.cpp) | `Obj_Lv6bemos` → `fpcNm_Obj_Lv6bemos_e` | REL | Temple of Time Beamos (Unused) |
| [`d_a_obj_lv6bemos2`](../src/d/actor/d_a_obj_lv6bemos2.cpp) | `Obj_Lv6bemos2` → `fpcNm_Obj_Lv6bemos2_e` | REL | Temple of Time Beamos |
| [`d_a_obj_lv6egate`](../src/d/actor/d_a_obj_lv6egate.cpp) | `Obj_Lv6EGate` → `fpcNm_Obj_Lv6EGate_e` | REL | Temple of Time Electric Gate |
| [`d_a_obj_lv6elevta`](../src/d/actor/d_a_obj_lv6elevta.cpp) | `Obj_Lv6ElevtA` → `fpcNm_Obj_Lv6ElevtA_e` | REL | Temple of Time Elevator |
| [`d_a_obj_lv6swturn`](../src/d/actor/d_a_obj_lv6swturn.cpp) | `Obj_Lv6SwTurn` → `fpcNm_Obj_Lv6SwTurn_e` | REL | Temple of Time Turn Switch |
| [`d_a_obj_lv7BsGate`](../src/d/actor/d_a_obj_lv7BsGate.cpp) | `Obj_Lv7BsGate` → `fpcNm_Obj_Lv7BsGate_e` | REL | City in the Sky Boss Door |
| [`d_a_obj_lv7PropellerY`](../src/d/actor/d_a_obj_lv7PropellerY.cpp) | `Obj_Lv7PropY` → `fpcNm_Obj_Lv7PropY_e` | REL | City in the Sky Propeller |
| [`d_a_obj_lv7bridge`](../src/d/actor/d_a_obj_lv7bridge.cpp) | `Obj_Lv7Bridge` → `fpcNm_Obj_Lv7Bridge_e` | REL | City in the Sky Bridge |
| [`d_a_obj_lv8KekkaiTrap`](../src/d/actor/d_a_obj_lv8KekkaiTrap.cpp) | `Obj_Lv8KekkaiTrap` → `fpcNm_Obj_Lv8KekkaiTrap_e` | REL | Palace of Twilight Barrier Trap |
| [`d_a_obj_lv8Lift`](../src/d/actor/d_a_obj_lv8Lift.cpp) | `Obj_Lv8Lift` → `fpcNm_Obj_Lv8Lift_e` | REL | Palace of Twilight Platform Lift |
| [`d_a_obj_lv8OptiLift`](../src/d/actor/d_a_obj_lv8OptiLift.cpp) | `Obj_Lv8OptiLift` → `fpcNm_Obj_Lv8OptiLift_e` | REL | Palace of Twilight Optilift |
| [`d_a_obj_lv8UdFloor`](../src/d/actor/d_a_obj_lv8UdFloor.cpp) | `Obj_Lv8UdFloor` → `fpcNm_Obj_Lv8UdFloor_e` | REL | Palace of Twilight Switch Step |
| [`d_a_obj_lv9SwShutter`](../src/d/actor/d_a_obj_lv9SwShutter.cpp) | `Obj_Lv9SwShutter` → `fpcNm_Obj_Lv9SwShutter_e` | REL | Hyrule Castle Switch Shutter |
| [`d_a_obj_magLift`](../src/d/actor/d_a_obj_magLift.cpp) | `Obj_MagLift` → `fpcNm_Obj_MagLift_e` | REL | Magnetic Lift |
| [`d_a_obj_magLiftRot`](../src/d/actor/d_a_obj_magLiftRot.cpp) | `Obj_MagLiftRot` → `fpcNm_Obj_MagLiftRot_e` | REL | Rotating Magnetic Lift |
| [`d_a_obj_magne_arm`](../src/d/actor/d_a_obj_magne_arm.cpp) | `Obj_MagneArm` → `fpcNm_Obj_MagneArm_e` | REL | Magnetic Arm |
| [`d_a_obj_maki`](../src/d/actor/d_a_obj_maki.cpp) | `OBJ_MAKI` → `fpcNm_OBJ_MAKI_e` | REL | Stick Bundle? |
| [`d_a_obj_master_sword`](../src/d/actor/d_a_obj_master_sword.cpp) | `Obj_MasterSword` → `fpcNm_Obj_MasterSword_e` | REL | Master Sword |
| [`d_a_obj_mato`](../src/d/actor/d_a_obj_mato.cpp) | `Obj_Mato` → `fpcNm_Obj_Mato_e` | REL | Flight by Fowl Rupee Target |
| [`d_a_obj_metalbox`](../src/d/actor/d_a_obj_metalbox.cpp) | `Obj_MetalBox` → `fpcNm_Obj_MetalBox_e` | REL | Metal Box |
| [`d_a_obj_mgate`](../src/d/actor/d_a_obj_mgate.cpp) | `Obj_MGate` → `fpcNm_Obj_MGate_e` | REL | Ordon Spring Gate |
| [`d_a_obj_mhole`](../src/d/actor/d_a_obj_mhole.cpp) | `Obj_MHole` → `fpcNm_Obj_MHole_e` | REL | Magnet Hole |
| [`d_a_obj_mie`](../src/d/actor/d_a_obj_mie.cpp) | `OBJ_MIE` → `fpcNm_OBJ_MIE_e` | REL | Gengle (Cat) |
| [`d_a_obj_mirror_6pole`](../src/d/actor/d_a_obj_mirror_6pole.cpp) | `Obj_Mirror6Pole` → `fpcNm_Obj_Mirror6Pole_e` | REL | Twilight Mirror Pole |
| [`d_a_obj_mirror_chain`](../src/d/actor/d_a_obj_mirror_chain.cpp) | `Obj_MirrorChain` → `fpcNm_Obj_MirrorChain_e` | REL | Twilight Mirror Chain |
| [`d_a_obj_mirror_sand`](../src/d/actor/d_a_obj_mirror_sand.cpp) | `Obj_MirrorSand` → `fpcNm_Obj_MirrorSand_e` | REL | Twilight Mirror Sand |
| [`d_a_obj_mirror_screw`](../src/d/actor/d_a_obj_mirror_screw.cpp) | `Obj_MirrorScrew` → `fpcNm_Obj_MirrorScrew_e` | REL | Twilight Mirror Screw |
| [`d_a_obj_mirror_table`](../src/d/actor/d_a_obj_mirror_table.cpp) | `Obj_MirrorTable` → `fpcNm_Obj_MirrorTable_e` | REL | Twilight Mirror Table |
| [`d_a_obj_movebox`](../src/d/actor/d_a_obj_movebox.cpp) | `Obj_Movebox` → `fpcNm_Obj_Movebox_e` | REL | Multi-Purpose Moving Box |
| [`d_a_obj_msima`](../src/d/actor/d_a_obj_msima.cpp) | `OBJ_MSIMA` → `fpcNm_OBJ_MSIMA_e` | REL | Dangoro Boss Stage (Magnetic Island) |
| [`d_a_obj_mvstair`](../src/d/actor/d_a_obj_mvstair.cpp) | `Obj_MvStair` → `fpcNm_Obj_MvStair_e` | REL | Moving Stairs |
| [`d_a_obj_myogan`](../src/d/actor/d_a_obj_myogan.cpp) | `OBJ_MYOGAN` → `fpcNm_OBJ_MYOGAN_e` | REL | Dangoro Arena Lava |
| [`d_a_obj_nagaisu`](../src/d/actor/d_a_obj_nagaisu.cpp) | `Obj_Nagaisu` → `fpcNm_Obj_Nagaisu_e` | REL | Couch |
| [`d_a_obj_nameplate`](../src/d/actor/d_a_obj_nameplate.cpp) | `Obj_NamePlate` → `fpcNm_Obj_NamePlate_e` | REL | Ordon Village Nameplate |
| [`d_a_obj_nan`](../src/d/actor/d_a_obj_nan.cpp) | `Obj_Nan` → `fpcNm_Obj_Nan_e` | REL | Insect - Phasmid |
| [`d_a_obj_ndoor`](../src/d/actor/d_a_obj_ndoor.cpp) | `OBJ_NDOOR` → `fpcNm_OBJ_NDOOR_e` | REL | Cat Door? |
| [`d_a_obj_nougu`](../src/d/actor/d_a_obj_nougu.cpp) | `OBJ_NOUGU` → `fpcNm_OBJ_NOUGU_e` | REL | Farm Tools |
| [`d_a_obj_octhashi`](../src/d/actor/d_a_obj_octhashi.cpp) | `OCTHASHI` → `fpcNm_OCTHASHI_e` | REL | Morpheel Pillar |
| [`d_a_obj_oiltubo`](../src/d/actor/d_a_obj_oiltubo.cpp) | `OBJ_OILTUBO` → `fpcNm_OBJ_OILTUBO_e` | REL | Oil Jar |
| [`d_a_obj_onsen`](../src/d/actor/d_a_obj_onsen.cpp) | `Obj_Onsen` → `fpcNm_Obj_Onsen_e` | REL | Hot Spring |
| [`d_a_obj_onsenFire`](../src/d/actor/d_a_obj_onsenFire.cpp) | `OBJ_ONSEN_FIRE` → `fpcNm_OBJ_ONSEN_FIRE_e` | REL | Hot Spring Fire |
| [`d_a_obj_onsenTaru`](../src/d/actor/d_a_obj_onsenTaru.cpp) | `Obj_OnsenTaru` → `fpcNm_Obj_OnsenTaru_e` | REL | Hotspring Water Barrel |
| [`d_a_obj_ornament_cloth`](../src/d/actor/d_a_obj_ornament_cloth.cpp) | `Obj_OnCloth` → `fpcNm_Obj_OnCloth_e` | REL | Ordon Village Flag |
| [`d_a_obj_pdoor`](../src/d/actor/d_a_obj_pdoor.cpp) | `Obj_PushDoor` → `fpcNm_Obj_PushDoor_e` | REL | Push Door |
| [`d_a_obj_pdtile`](../src/d/actor/d_a_obj_pdtile.cpp) | `Obj_PDtile` → `fpcNm_Obj_PDtile_e` | REL | P - Drop Tile |
| [`d_a_obj_pdwall`](../src/d/actor/d_a_obj_pdwall.cpp) | `Obj_PDwall` → `fpcNm_Obj_PDwall_e` | REL | P - D Wall? |
| [`d_a_obj_picture`](../src/d/actor/d_a_obj_picture.cpp) | `Obj_Picture` → `fpcNm_Obj_Picture_e` | REL | Hyrule Castle Painting |
| [`d_a_obj_pillar`](../src/d/actor/d_a_obj_pillar.cpp) | `Obj_Pillar` → `fpcNm_Obj_Pillar_e` | REL | Forest Temple Totem Pole |
| [`d_a_obj_pleaf`](../src/d/actor/d_a_obj_pleaf.cpp) | `OBJ_PLEAF` → `fpcNm_OBJ_PLEAF_e` | REL | Ordon Pumpkin Vine |
| [`d_a_obj_poCandle`](../src/d/actor/d_a_obj_poCandle.cpp) | `Obj_poCandle` → `fpcNm_Obj_poCandle_e` | REL | Poe Torch |
| [`d_a_obj_poFire`](../src/d/actor/d_a_obj_poFire.cpp) | `Obj_poFire` → `fpcNm_Tag_Lv4Candle_e` | REL | Poe Fire |
| [`d_a_obj_poTbox`](../src/d/actor/d_a_obj_poTbox.cpp) | `Obj_poTbox` → `fpcNm_Obj_poTbox_e` | REL | Poe Treasure Chest |
| [`d_a_obj_prop`](../src/d/actor/d_a_obj_prop.cpp) | `Obj_Prop` → `fpcNm_Obj_Prop_e` | REL | City in the Sky Propeller? |
| [`d_a_obj_pumpkin`](../src/d/actor/d_a_obj_pumpkin.cpp) | `OBJ_PUMPKIN` → `fpcNm_OBJ_PUMPKIN_e` | REL | Ordon Village Pumpkin |
| [`d_a_obj_rcircle`](../src/d/actor/d_a_obj_rcircle.cpp) | `Obj_RCircle` → `fpcNm_Obj_RCircle_e` | REL | River Circle |
| [`d_a_obj_rfHole`](../src/d/actor/d_a_obj_rfHole.cpp) | `Obj_RfHole` → `fpcNm_Obj_RfHole_e` | REL | (Kakariko?) Roof Hole |
| [`d_a_obj_rgate`](../src/d/actor/d_a_obj_rgate.cpp) | `Obj_RiderGate` → `fpcNm_Obj_RiderGate_e` | REL | Rider Gate |
| [`d_a_obj_riverrock`](../src/d/actor/d_a_obj_riverrock.cpp) | `Obj_RIVERROCK` → `fpcNm_Obj_RIVERROCK_e` | REL | (Zora?) River Rock |
| [`d_a_obj_rock`](../src/d/actor/d_a_obj_rock.cpp) | `OBJ_ROCK` → `fpcNm_OBJ_ROCK_e` | REL | Rock |
| [`d_a_obj_rope_bridge`](../src/d/actor/d_a_obj_rope_bridge.cpp) | `Obj_RopeBridge` → `fpcNm_Obj_RopeBridge_e` | REL | Small / Big Rope Bridge |
| [`d_a_obj_rotBridge`](../src/d/actor/d_a_obj_rotBridge.cpp) | `Obj_RotBridge` → `fpcNm_Obj_RotBridge_e` | REL | Rotating Bridge |
| [`d_a_obj_rotTrap`](../src/d/actor/d_a_obj_rotTrap.cpp) | `Obj_RotTrap` → `fpcNm_Obj_RotTrap_e` | REL | Rotating Skull Trap |
| [`d_a_obj_roten`](../src/d/actor/d_a_obj_roten.cpp) | `OBJ_ROTEN` → `fpcNm_OBJ_ROTEN_e` | REL | Goron Child Stall |
| [`d_a_obj_rstair`](../src/d/actor/d_a_obj_rstair.cpp) | `Obj_RotStair` → `fpcNm_Obj_RotStair_e` | REL | Rail Staircase |
| [`d_a_obj_rw`](../src/d/actor/d_a_obj_rw.cpp) | `OBJ_RW` → `fpcNm_OBJ_RW_e` | REL | Wild Boar Roast |
| [`d_a_obj_sWallShutter`](../src/d/actor/d_a_obj_sWallShutter.cpp) | `Obj_SwallShutter` → `fpcNm_Obj_SwallShutter_e` | REL | Shutter Wall (Switch) |
| [`d_a_obj_saidan`](../src/d/actor/d_a_obj_saidan.cpp) | `Obj_Saidan` → `fpcNm_Obj_Saidan_e` | REL | Altar |
| [`d_a_obj_sakuita`](../src/d/actor/d_a_obj_sakuita.cpp) | `Obj_Sakuita` → `fpcNm_Obj_Sakuita_e` | REL | Rope Plank |
| [`d_a_obj_sakuita_rope`](../src/d/actor/d_a_obj_sakuita_rope.cpp) | `Obj_ItaRope` → `fpcNm_Obj_ItaRope_e` | REL | Rope Banner Fence |
| [`d_a_obj_scannon`](../src/d/actor/d_a_obj_scannon.cpp) | `Obj_SCannon` → `fpcNm_Obj_SCannon_e` | REL | Sky Cannon (City in the Sky) |
| [`d_a_obj_scannon_crs`](../src/d/actor/d_a_obj_scannon_crs.cpp) | `Obj_SCannonCrs` → `fpcNm_Obj_SCannonCrs_e` | REL | Sky Cannon (Broken) |
| [`d_a_obj_scannon_ten`](../src/d/actor/d_a_obj_scannon_ten.cpp) | `Obj_SCannonTen` → `fpcNm_Obj_SCannonTen_e` | REL | Sky Cannon (Lake Hylia, Fixed) |
| [`d_a_obj_sekidoor`](../src/d/actor/d_a_obj_sekidoor.cpp) | `OBJ_SEKIDOOR` → `fpcNm_OBJ_SEKIDOOR_e` | REL | Stone Door |
| [`d_a_obj_sekizo`](../src/d/actor/d_a_obj_sekizo.cpp) | `OBJ_SEKIZO` → `fpcNm_OBJ_SEKIZO_e` | REL | Stone Statue |
| [`d_a_obj_sekizoa`](../src/d/actor/d_a_obj_sekizoa.cpp) | `OBJ_SEKIZOA` → `fpcNm_OBJ_SEKIZOA_e` | REL | Stone Guardian Statue(s) |
| [`d_a_obj_shield`](../src/d/actor/d_a_obj_shield.cpp) | `Obj_Shield` → `fpcNm_Obj_Shield_e` | REL | Ordon Shield |
| [`d_a_obj_sm_door`](../src/d/actor/d_a_obj_sm_door.cpp) | `Obj_SM_DOOR` → `fpcNm_Obj_SM_DOOR_e` | REL | Sacred Meadow Door |
| [`d_a_obj_smallkey`](../src/d/actor/d_a_obj_smallkey.cpp) | `Obj_SmallKey` → `fpcNm_Obj_SmallKey_e` | REL | Small Key |
| [`d_a_obj_smgdoor`](../src/d/actor/d_a_obj_smgdoor.cpp) | `Obj_SmgDoor` → `fpcNm_Obj_SmgDoor_e` | REL | Sacred Meadow Grove Door |
| [`d_a_obj_smoke`](../src/d/actor/d_a_obj_smoke.cpp) | `Obj_Smoke` → `fpcNm_Obj_Smoke_e` | REL | Elde Inn Stove Smoke |
| [`d_a_obj_smtile`](../src/d/actor/d_a_obj_smtile.cpp) | `OBJ_SMTILE` → `fpcNm_OBJ_SMTILE_e` | REL | Sacred Meadow Tile |
| [`d_a_obj_smw_stone`](../src/d/actor/d_a_obj_smw_stone.cpp) | `Obj_SmWStone` → `fpcNm_Obj_SmWStone_e` | REL | Howling Stone |
| [`d_a_obj_snowEffTag`](../src/d/actor/d_a_obj_snowEffTag.cpp) | `Tag_SnowEff` → `fpcNm_Tag_SnowEff_e` | REL | Snow Effect Tag |
| [`d_a_obj_snow_soup`](../src/d/actor/d_a_obj_snow_soup.cpp) | `Obj_SnowSoup` → `fpcNm_Obj_SnowSoup_e` | REL | Snowpeak Ruins Soup Pot? |
| [`d_a_obj_so`](../src/d/actor/d_a_obj_so.cpp) | `OBJ_SO` → `fpcNm_OBJ_SO_e` | REL | Monkey Cage (Saru Ori) |
| [`d_a_obj_spinLift`](../src/d/actor/d_a_obj_spinLift.cpp) | `Obj_SpinLift` → `fpcNm_Obj_SpinLift_e` | REL | Spinner Lift |
| [`d_a_obj_ss_base`](../src/d/actor/d_a_obj_ss_base.cpp) | — | DOL |  |
| [`d_a_obj_ss_drink`](../src/d/actor/d_a_obj_ss_drink.cpp) | `OBJ_SSDRINK` → `fpcNm_OBJ_SSDRINK_e` | REL |  |
| [`d_a_obj_ss_item`](../src/d/actor/d_a_obj_ss_item.cpp) | `OBJ_SSITEM` → `fpcNm_OBJ_SSITEM_e` | REL |  |
| [`d_a_obj_stairBlock`](../src/d/actor/d_a_obj_stairBlock.cpp) | `Obj_StairBlock` → `fpcNm_Obj_StairBlock_e` | REL | Stair Block |
| [`d_a_obj_stick`](../src/d/actor/d_a_obj_stick.cpp) | `OBJ_STICK` → `fpcNm_OBJ_STICK_e` | REL | Stick |
| [`d_a_obj_stone`](../src/d/actor/d_a_obj_stone.cpp) | `Obj_Stone` → `fpcNm_Obj_Stone_e` | REL | Small / Large Stones |
| [`d_a_obj_stoneMark`](../src/d/actor/d_a_obj_stoneMark.cpp) | `Obj_StoneMark` → `fpcNm_Obj_StoneMark_e` | REL | Stone Mark |
| [`d_a_obj_stopper`](../src/d/actor/d_a_obj_stopper.cpp) | `Obj_Stopper` → `fpcNm_Obj_Stopper_e` | REL | Stopper |
| [`d_a_obj_stopper2`](../src/d/actor/d_a_obj_stopper2.cpp) | `Obj_Stopper2` → `fpcNm_Obj_Stopper2_e` | REL | Door Stop |
| [`d_a_obj_suisya`](../src/d/actor/d_a_obj_suisya.cpp) | `OBJ_SUISYA` → `fpcNm_OBJ_SUISYA_e` | REL | Water Wheel |
| [`d_a_obj_sw`](../src/d/actor/d_a_obj_sw.cpp) | `OBJ_SW` → `fpcNm_OBJ_SW_e` | REL | Switch |
| [`d_a_obj_swBallA`](../src/d/actor/d_a_obj_swBallA.cpp) | `Obj_SwBallA` → `fpcNm_Obj_SwBallA_e` | REL | Ball Switch A |
| [`d_a_obj_swBallB`](../src/d/actor/d_a_obj_swBallB.cpp) | `Obj_SwBallB` → `fpcNm_Obj_SwBallB_e` | REL | Ball Switch B |
| [`d_a_obj_swBallC`](../src/d/actor/d_a_obj_swBallC.cpp) | `Obj_SwBallC` → `fpcNm_Obj_SwBallC_e` | REL | Ball Switch C |
| [`d_a_obj_swLight`](../src/d/actor/d_a_obj_swLight.cpp) | `Obj_SwLight` → `fpcNm_Obj_SwLight_e` | REL | Light Switch |
| [`d_a_obj_swchain`](../src/d/actor/d_a_obj_swchain.cpp) | `Obj_SwChain` → `fpcNm_Obj_SwChain_e` | REL | Chain Switch |
| [`d_a_obj_swhang`](../src/d/actor/d_a_obj_swhang.cpp) | `Obj_SwHang` → `fpcNm_Obj_SwHang_e` | REL | Dangle A |
| [`d_a_obj_sword`](../src/d/actor/d_a_obj_sword.cpp) | `Obj_Sword` → `fpcNm_Obj_Sword_e` | REL | Ordon Sword |
| [`d_a_obj_swpropeller`](../src/d/actor/d_a_obj_swpropeller.cpp) | `Obj_Swpropeller` → `fpcNm_Obj_Swpropeller_e` | REL | Boomerang Switch |
| [`d_a_obj_swpush`](../src/d/actor/d_a_obj_swpush.cpp) | `Obj_Swpush` → `fpcNm_Obj_Swpush_e` | REL | Push Switch |
| [`d_a_obj_swpush2`](../src/d/actor/d_a_obj_swpush2.cpp) | `Obj_Swpush2` → `fpcNm_Obj_Swpush2_e` | REL | Push Switch 2 |
| [`d_a_obj_swpush5`](../src/d/actor/d_a_obj_swpush5.cpp) | `Obj_Swpush5` → `fpcNm_Obj_Swpush5_e` | REL | Push Switch 5 |
| [`d_a_obj_swspinner`](../src/d/actor/d_a_obj_swspinner.cpp) | `Obj_SwSpinner` → `fpcNm_Obj_SwSpinner_e` | REL | Spinner Switch |
| [`d_a_obj_swturn`](../src/d/actor/d_a_obj_swturn.cpp) | `Obj_SwTurn` → `fpcNm_Obj_SwTurn_e` | REL | Arbiter's Ground Turn Switch |
| [`d_a_obj_syRock`](../src/d/actor/d_a_obj_syRock.cpp) | `Obj_SyRock` → `fpcNm_Obj_SyRock_e` | REL | Stalactite Rock |
| [`d_a_obj_szbridge`](../src/d/actor/d_a_obj_szbridge.cpp) | `Obj_SZbridge` → `fpcNm_Obj_SZbridge_e` | REL | Stone Statue Bridge |
| [`d_a_obj_taFence`](../src/d/actor/d_a_obj_taFence.cpp) | `Obj_TaFence` → `fpcNm_Obj_TaFence_e` | REL | Fench/Mesh? |
| [`d_a_obj_table`](../src/d/actor/d_a_obj_table.cpp) | `Obj_Table` → `fpcNm_Obj_Table_e` | REL | Table |
| [`d_a_obj_takaraDai`](../src/d/actor/d_a_obj_takaraDai.cpp) | `Obj_TakaraDai` → `fpcNm_Obj_TakaraDai_e` | REL | Flight-by-Fowl Platform |
| [`d_a_obj_tatigi`](../src/d/actor/d_a_obj_tatigi.cpp) | `OBJ_TATIGI` → `fpcNm_OBJ_TATIGI_e` | REL |  |
| [`d_a_obj_ten`](../src/d/actor/d_a_obj_ten.cpp) | `Obj_Ten` → `fpcNm_Obj_Ten_e` | REL | Insect - Ladybug |
| [`d_a_obj_testcube`](../src/d/actor/d_a_obj_testcube.cpp) | `Obj_TestCube` → `fpcNm_Obj_TestCube_e` | REL |  |
| [`d_a_obj_tgake`](../src/d/actor/d_a_obj_tgake.cpp) | `Obj_Gake` → `fpcNm_Obj_Gake_e` | REL | Howling Cliff |
| [`d_a_obj_thashi`](../src/d/actor/d_a_obj_thashi.cpp) | `Obj_THASHI` → `fpcNm_Obj_THASHI_e` | REL | T Pillar |
| [`d_a_obj_thdoor`](../src/d/actor/d_a_obj_thdoor.cpp) | `Obj_TDoor` → `fpcNm_Obj_TDoor_e` | REL | Telma's Bar Door |
| [`d_a_obj_timeFire`](../src/d/actor/d_a_obj_timeFire.cpp) | `Obj_TimeFire` → `fpcNm_Obj_TimeFire_e` | REL | Time Fire |
| [`d_a_obj_timer`](../src/d/actor/d_a_obj_timer.cpp) | `Obj_Timer` → `fpcNm_Obj_Timer_e` | REL | Timer |
| [`d_a_obj_tks`](../src/d/actor/d_a_obj_tks.cpp) | `OBJ_TKS` → `fpcNm_OBJ_TKS_e` | REL | Ooccoo Jr. (small) |
| [`d_a_obj_tmoon`](../src/d/actor/d_a_obj_tmoon.cpp) | `Obj_TMoon` → `fpcNm_Obj_TMoon_e` | REL | Howling Moon |
| [`d_a_obj_toaru_maki`](../src/d/actor/d_a_obj_toaru_maki.cpp) | `Obj_ToaruMaki` → `fpcNm_Obj_ToaruMaki_e` | REL | Ordon Bundle |
| [`d_a_obj_toby`](../src/d/actor/d_a_obj_toby.cpp) | `OBJ_TOBY` → `fpcNm_OBJ_TOBY_e` | REL | Fyer (Object) |
| [`d_a_obj_tobyhouse`](../src/d/actor/d_a_obj_tobyhouse.cpp) | `Obj_TobyHouse` → `fpcNm_Obj_TobyHouse_e` | REL | Fyer's House |
| [`d_a_obj_togeTrap`](../src/d/actor/d_a_obj_togeTrap.cpp) | `Obj_TogeTrap` → `fpcNm_Obj_TogeTrap_e` | REL | Blade Trap |
| [`d_a_obj_tombo`](../src/d/actor/d_a_obj_tombo.cpp) | `Obj_Tombo` → `fpcNm_Obj_Tombo_e` | REL | Insect - Dragonfly (Tonbo) |
| [`d_a_obj_tornado`](../src/d/actor/d_a_obj_tornado.cpp) | `Obj_Tornado` → `fpcNm_Obj_Tornado_e` | REL | Wind Column |
| [`d_a_obj_tornado2`](../src/d/actor/d_a_obj_tornado2.cpp) | `Obj_Tornado2` → `fpcNm_Obj_Tornado2_e` | REL | Strong Wind Column |
| [`d_a_obj_tp`](../src/d/actor/d_a_obj_tp.cpp) | `OBJ_TP` → `fpcNm_OBJ_TP_e` | REL | Shadow Beast Barrier Pole (Twilight Pole) |
| [`d_a_obj_treesh`](../src/d/actor/d_a_obj_treesh.cpp) | `TREESH` → `fpcNm_TREESH_e` | REL | Conifer Tree |
| [`d_a_obj_twGate`](../src/d/actor/d_a_obj_twGate.cpp) | `Obj_TwGate` → `fpcNm_Obj_TwGate_e` | REL | Twilight Gate (Wall?) |
| [`d_a_obj_udoor`](../src/d/actor/d_a_obj_udoor.cpp) | `OBJ_UDOOR` → `fpcNm_OBJ_UDOOR_e` | REL | Stable Door |
| [`d_a_obj_usaku`](../src/d/actor/d_a_obj_usaku.cpp) | `OBJ_USAKU` → `fpcNm_OBJ_USAKU_e` | REL | Horse Fence |
| [`d_a_obj_vground`](../src/d/actor/d_a_obj_vground.cpp) | `Obj_VolcGnd` → `fpcNm_Obj_VolcGnd_e` | REL | Volcano Ground |
| [`d_a_obj_volcball`](../src/d/actor/d_a_obj_volcball.cpp) | `Obj_VolcanicBall` → `fpcNm_Obj_VolcanicBall_e` | REL | Volcano Ball |
| [`d_a_obj_volcbom`](../src/d/actor/d_a_obj_volcbom.cpp) | `Obj_VolcanicBomb` → `fpcNm_Obj_VolcanicBomb_e` | REL | Volcano Bomb? |
| [`d_a_obj_warp_kbrg`](../src/d/actor/d_a_obj_warp_kbrg.cpp) | `Obj_KakarikoBrg` → `fpcNm_Obj_KakarikoBrg_e` | REL | Kakariko Gorge Warp Bridge |
| [`d_a_obj_warp_obrg`](../src/d/actor/d_a_obj_warp_obrg.cpp) | `Obj_OrdinBrg` → `fpcNm_Obj_OrdinBrg_e` | REL | Eldin Warp Bridge |
| [`d_a_obj_waterGate`](../src/d/actor/d_a_obj_waterGate.cpp) | `Obj_WtGate` → `fpcNm_Obj_WtGate_e` | REL | Water Gate |
| [`d_a_obj_waterPillar`](../src/d/actor/d_a_obj_waterPillar.cpp) | `Obj_WaterPillar` → `fpcNm_Obj_WaterPillar_e` | REL | Water Column/Pillar |
| [`d_a_obj_waterfall`](../src/d/actor/d_a_obj_waterfall.cpp) | `Obj_WaterFall` → `fpcNm_Obj_WaterFall_e` | REL | Waterfall Collision |
| [`d_a_obj_wchain`](../src/d/actor/d_a_obj_wchain.cpp) | `Obj_Wchain` → `fpcNm_Obj_Wchain_e` | REL | Wolf Chain |
| [`d_a_obj_wdStick`](../src/d/actor/d_a_obj_wdStick.cpp) | `Obj_WdStick` → `fpcNm_Obj_WdStick_e` | REL | Wooden Stick |
| [`d_a_obj_web0`](../src/d/actor/d_a_obj_web0.cpp) | `OBJ_WEB0` → `fpcNm_OBJ_WEB0_e` | REL | Wall Web |
| [`d_a_obj_web1`](../src/d/actor/d_a_obj_web1.cpp) | `OBJ_WEB1` → `fpcNm_OBJ_WEB1_e` | REL | Floor Web |
| [`d_a_obj_well_cover`](../src/d/actor/d_a_obj_well_cover.cpp) | `Obj_WellCover` → `fpcNm_Obj_WellCover_e` | REL | Graveyard Well Cover |
| [`d_a_obj_wflag`](../src/d/actor/d_a_obj_wflag.cpp) | `OBJ_WFLAG` → `fpcNm_OBJ_WFLAG_e` | REL |  |
| [`d_a_obj_wind_stone`](../src/d/actor/d_a_obj_wind_stone.cpp) | `Obj_WindStone` → `fpcNm_Obj_WindStone_e` | REL | Howling Stone (Hole) |
| [`d_a_obj_window`](../src/d/actor/d_a_obj_window.cpp) | `Obj_Window` → `fpcNm_Obj_Window_e` | REL | Destructable Kakariko Village Window |
| [`d_a_obj_wood_pendulum`](../src/d/actor/d_a_obj_wood_pendulum.cpp) | `Obj_WoodPendulum` → `fpcNm_Obj_WoodPendulum_e` | REL | Wooden Pendulum |
| [`d_a_obj_wood_statue`](../src/d/actor/d_a_obj_wood_statue.cpp) | `Obj_WoodStatue` → `fpcNm_Obj_WoodStatue_e` | REL | Wooden Statue |
| [`d_a_obj_wsword`](../src/d/actor/d_a_obj_wsword.cpp) | `Obj_WoodenSword` → `fpcNm_Obj_WoodenSword_e` | REL | Wooden Sword |
| [`d_a_obj_yel_bag`](../src/d/actor/d_a_obj_yel_bag.cpp) | `OBJ_YBAG` → `fpcNm_OBJ_YBAG_e` | REL | Ilia's Bag |
| [`d_a_obj_yobikusa`](../src/d/actor/d_a_obj_yobikusa.cpp) | `Obj_Yobikusa` → `fpcNm_Obj_Yobikusa_e` | REL | Hawk Grass |
| [`d_a_obj_yousei`](../src/d/actor/d_a_obj_yousei.cpp) | `Obj_Yousei` → `fpcNm_Obj_Yousei_e` | REL | Fairy |
| [`d_a_obj_ystone`](../src/d/actor/d_a_obj_ystone.cpp) | `OBJ_YSTONE` → `fpcNm_OBJ_YSTONE_e` | REL | ??? (Shadow Stone?) |
| [`d_a_obj_zcloth`](../src/d/actor/d_a_obj_zcloth.cpp) | `Obj_ZoraCloth` → `fpcNm_Obj_ZoraCloth_e` | REL | Zora Armor? |
| [`d_a_obj_zdoor`](../src/d/actor/d_a_obj_zdoor.cpp) | `Obj_ZDoor` → `fpcNm_Obj_ZDoor_e` | REL | Zelda Door |
| [`d_a_obj_zrTurara`](../src/d/actor/d_a_obj_zrTurara.cpp) | `Obj_zrTurara` → `fpcNm_Obj_zrTurara_e` | REL | Zora Drop |
| [`d_a_obj_zrTuraraRock`](../src/d/actor/d_a_obj_zrTuraraRock.cpp) | `Obj_zrTuraraRc` → `fpcNm_Obj_zrTuraraRc_e` | REL | Zora Drop Rock |
| [`d_a_obj_zraMark`](../src/d/actor/d_a_obj_zraMark.cpp) | `ZRA_MARK` → `fpcNm_ZRA_MARK_e` | REL | Iza's River Ride Destructible Buoy |
| [`d_a_obj_zra_freeze`](../src/d/actor/d_a_obj_zra_freeze.cpp) | `OBJ_ZRAFREEZE` → `fpcNm_OBJ_ZRAFREEZE_e` | REL | Zora (Frozen) |
| [`d_a_obj_zra_rock`](../src/d/actor/d_a_obj_zra_rock.cpp) | `Obj_ZraRock` → `fpcNm_Obj_ZraRock_e` | REL | Zora Rock |

## Doors (9)

| Source | Profile → process name | Runs from | In-game name |
|---|---|---|---|
| [`d_a_door_boss`](../src/d/actor/d_a_door_boss.cpp) | `BOSS_DOOR` → `fpcNm_BOSS_DOOR_e` | REL | Boss Door (Unused?) |
| [`d_a_door_bossL1`](../src/d/actor/d_a_door_bossL1.cpp) | `L1BOSS_DOOR` → `fpcNm_L1BOSS_DOOR_e` | REL | Boss Door |
| [`d_a_door_bossL5`](../src/d/actor/d_a_door_bossL5.cpp) | `L5BOSS_DOOR` → `fpcNm_L5BOSS_DOOR_e` | REL | Snowpeak Ruins Boss Door |
| [`d_a_door_dbdoor00`](../src/d/actor/d_a_door_dbdoor00.cpp) | `DBDOOR` → `fpcNm_DBDOOR_e` | REL | Double Door |
| [`d_a_door_knob00`](../src/d/actor/d_a_door_knob00.cpp) | `KNOB20` → `fpcNm_KNOB20_e` | REL | Knob Door |
| [`d_a_door_mbossL1`](../src/d/actor/d_a_door_mbossL1.cpp) | `L1MBOSS_DOOR` → `fpcNm_L1MBOSS_DOOR_e` | REL | Mini Boss Door |
| [`d_a_door_push`](../src/d/actor/d_a_door_push.cpp) | `PushDoor` → `fpcNm_PushDoor_e` | REL | Push Door |
| [`d_a_door_shutter`](../src/d/actor/d_a_door_shutter.cpp) | `DOOR20` → `fpcNm_DOOR20_e` | REL | Sliding Door |
| [`d_a_door_spiral`](../src/d/actor/d_a_door_spiral.cpp) | `SPIRAL_DOOR` → `fpcNm_SPIRAL_DOOR_e` | REL |  |

## Tags (invisible triggers / controllers) (90)

| Source | Profile → process name | Runs from | In-game name |
|---|---|---|---|
| [`d_a_kytag00`](../src/d/actor/d_a_kytag00.cpp) | `KYTAG00` → `fpcNm_KYTAG00_e` | REL | Twilight Tag 0 |
| [`d_a_kytag01`](../src/d/actor/d_a_kytag01.cpp) | `KYTAG01` → `fpcNm_KYTAG01_e` | REL | Twilight Tag 1 |
| [`d_a_kytag02`](../src/d/actor/d_a_kytag02.cpp) | `KYTAG02` → `fpcNm_KYTAG02_e` | REL | Twilight Tag 2 |
| [`d_a_kytag03`](../src/d/actor/d_a_kytag03.cpp) | `KYTAG03` → `fpcNm_KYTAG03_e` | REL | Smell Effect Generation Tag |
| [`d_a_kytag04`](../src/d/actor/d_a_kytag04.cpp) | `KYTAG04` → `fpcNm_KYTAG04_e` | REL | Twilight Portal Tag |
| [`d_a_kytag05`](../src/d/actor/d_a_kytag05.cpp) | `KYTAG05` → `fpcNm_KYTAG05_e` | DOL | Peep Hole Tag |
| [`d_a_kytag06`](../src/d/actor/d_a_kytag06.cpp) | `KYTAG06` → `fpcNm_KYTAG06_e` | REL | Weather Handler Tag |
| [`d_a_kytag07`](../src/d/actor/d_a_kytag07.cpp) | `KYTAG07` → `fpcNm_KYTAG07_e` | REL | Plight Setter Tag |
| [`d_a_kytag08`](../src/d/actor/d_a_kytag08.cpp) | `KYTAG08` → `fpcNm_KYTAG08_e` | REL | Fog Avoid Tag |
| [`d_a_kytag09`](../src/d/actor/d_a_kytag09.cpp) | `KYTAG09` → `fpcNm_KYTAG09_e` | REL | Twilight Film Tag |
| [`d_a_kytag10`](../src/d/actor/d_a_kytag10.cpp) | `KYTAG10` → `fpcNm_KYTAG10_e` | REL | Lava Particles Tag |
| [`d_a_kytag11`](../src/d/actor/d_a_kytag11.cpp) | `KYTAG11` → `fpcNm_KYTAG11_e` | REL | Time Control Tag |
| [`d_a_kytag12`](../src/d/actor/d_a_kytag12.cpp) | `KYTAG12` → `fpcNm_KYTAG12_e` | REL | Palace of Twilight - Dark Fog Tag |
| [`d_a_kytag13`](../src/d/actor/d_a_kytag13.cpp) | `KYTAG13` → `fpcNm_KYTAG13_e` | REL | Blowing Snow Tag |
| [`d_a_kytag14`](../src/d/actor/d_a_kytag14.cpp) | `KYTAG14` → `fpcNm_KYTAG14_e` | REL | Save Memory to File Tag |
| [`d_a_kytag15`](../src/d/actor/d_a_kytag15.cpp) | `KYTAG15` → `fpcNm_KYTAG15_e` | REL | Z Shake Tag |
| [`d_a_kytag16`](../src/d/actor/d_a_kytag16.cpp) | `KYTAG16` → `fpcNm_KYTAG16_e` | REL | Pikari Tag |
| [`d_a_kytag17`](../src/d/actor/d_a_kytag17.cpp) | `KYTAG17` → `fpcNm_KYTAG17_e` | DOL | Light Mask Tag |
| [`d_a_tag_CstaSw`](../src/d/actor/d_a_tag_CstaSw.cpp) | `Tag_CstaSw` → `fpcNm_Tag_CstaSw_e` | REL | Overworld Statue switch trigger |
| [`d_a_tag_Lv6Gate`](../src/d/actor/d_a_tag_Lv6Gate.cpp) | `Tag_Lv6Gate` → `fpcNm_Tag_Lv6Gate_e` | REL |  |
| [`d_a_tag_Lv7Gate`](../src/d/actor/d_a_tag_Lv7Gate.cpp) | `Tag_Lv7Gate` → `fpcNm_Tag_Lv7Gate_e` | REL |  |
| [`d_a_tag_Lv8Gate`](../src/d/actor/d_a_tag_Lv8Gate.cpp) | `Tag_Lv8Gate` → `fpcNm_Tag_Lv8Gate_e` | REL |  |
| [`d_a_tag_TWgate`](../src/d/actor/d_a_tag_TWgate.cpp) | `Tag_TWGate` → `fpcNm_Tag_TWGate_e` | REL |  |
| [`d_a_tag_ajnot`](../src/d/actor/d_a_tag_ajnot.cpp) | `Tag_AJnot` → `fpcNm_Tag_AJnot_e` | REL |  |
| [`d_a_tag_allmato`](../src/d/actor/d_a_tag_allmato.cpp) | `TAG_ALLMATO` → `fpcNm_TAG_ALLMATO_e` | REL |  |
| [`d_a_tag_arena`](../src/d/actor/d_a_tag_arena.cpp) | `Tag_Arena` → `fpcNm_Tag_Arena_e` | DOL |  |
| [`d_a_tag_assistance`](../src/d/actor/d_a_tag_assistance.cpp) | `Tag_Assist` → `fpcNm_Tag_Assist_e` | REL |  |
| [`d_a_tag_attack_item`](../src/d/actor/d_a_tag_attack_item.cpp) | `Tag_AttackItem` → `fpcNm_Tag_AttackItem_e` | REL |  |
| [`d_a_tag_attention`](../src/d/actor/d_a_tag_attention.cpp) | `Tag_Attp` → `fpcNm_Tag_Attp_e` | REL |  |
| [`d_a_tag_bottle_item`](../src/d/actor/d_a_tag_bottle_item.cpp) | `TAG_BTLITM` → `fpcNm_TAG_SSDRINK_e` | REL |  |
| [`d_a_tag_camera`](../src/d/actor/d_a_tag_camera.cpp) | `TAG_CAMERA` → `fpcNm_TAG_CAMERA_e` | REL |  |
| [`d_a_tag_chgrestart`](../src/d/actor/d_a_tag_chgrestart.cpp) | `Tag_ChgRestart` → `fpcNm_Tag_ChgRestart_e` | REL |  |
| [`d_a_tag_chkpoint`](../src/d/actor/d_a_tag_chkpoint.cpp) | `TAG_CHKPOINT` → `fpcNm_TAG_CHKPOINT_e` | REL |  |
| [`d_a_tag_csw`](../src/d/actor/d_a_tag_csw.cpp) | `TAG_CSW` → `fpcNm_TAG_CSW_e` | REL |  |
| [`d_a_tag_escape`](../src/d/actor/d_a_tag_escape.cpp) | `Tag_Escape` → `fpcNm_Tag_Escape_e` | DOL |  |
| [`d_a_tag_event`](../src/d/actor/d_a_tag_event.cpp) | `TAG_EVENT` → `fpcNm_TAG_EVENT_e` | REL |  |
| [`d_a_tag_evt`](../src/d/actor/d_a_tag_evt.cpp) | `TAG_EVT` → `fpcNm_TAG_EVT_e` | REL |  |
| [`d_a_tag_evtarea`](../src/d/actor/d_a_tag_evtarea.cpp) | `TAG_EVTAREA` → `fpcNm_TAG_EVTAREA_e` | REL |  |
| [`d_a_tag_evtmsg`](../src/d/actor/d_a_tag_evtmsg.cpp) | `TAG_EVTMSG` → `fpcNm_TAG_MSG_e` | REL |  |
| [`d_a_tag_firewall`](../src/d/actor/d_a_tag_firewall.cpp) | `Tag_FWall` → `fpcNm_Tag_FWall_e` | REL |  |
| [`d_a_tag_gra`](../src/d/actor/d_a_tag_gra.cpp) | `TAG_GRA` → `fpcNm_TAG_GRA_e` | DOL | Tag - Goron A |
| [`d_a_tag_gstart`](../src/d/actor/d_a_tag_gstart.cpp) | `Tag_Gstart` → `fpcNm_Tag_Gstart_e` | REL |  |
| [`d_a_tag_guard`](../src/d/actor/d_a_tag_guard.cpp) | `TAG_GUARD` → `fpcNm_TAG_GUARD_e` | DOL | Tag - Guard |
| [`d_a_tag_hinit`](../src/d/actor/d_a_tag_hinit.cpp) | `Tag_Hinit` → `fpcNm_Tag_Hinit_e` | REL |  |
| [`d_a_tag_hjump`](../src/d/actor/d_a_tag_hjump.cpp) | `Tag_Hjump` → `fpcNm_Tag_Hjump_e` | REL | Epona fence jump trigger / object |
| [`d_a_tag_howl`](../src/d/actor/d_a_tag_howl.cpp) | `TAG_HOWL` → `fpcNm_TAG_HOWL_e` | REL |  |
| [`d_a_tag_hstop`](../src/d/actor/d_a_tag_hstop.cpp) | `Tag_Hstop` → `fpcNm_Tag_Hstop_e` | REL |  |
| [`d_a_tag_instruction`](../src/d/actor/d_a_tag_instruction.cpp) | `Tag_Instruction` → `fpcNm_Tag_Instruction_e` | DOL |  |
| [`d_a_tag_kago_fall`](../src/d/actor/d_a_tag_kago_fall.cpp) | `Tag_KagoFall` → `fpcNm_Tag_KagoFall_e` | DOL |  |
| [`d_a_tag_kmsg`](../src/d/actor/d_a_tag_kmsg.cpp) | `TAG_KMSG` → `fpcNm_TAG_KMSG_e` | REL |  |
| [`d_a_tag_lantern`](../src/d/actor/d_a_tag_lantern.cpp) | `TAG_LANTERN` → `fpcNm_TAG_LANTERN_e` | REL | Tag - Lantern |
| [`d_a_tag_lightball`](../src/d/actor/d_a_tag_lightball.cpp) | `Tag_LightBall` → `fpcNm_Tag_LightBall_e` | DOL |  |
| [`d_a_tag_lv2prchk`](../src/d/actor/d_a_tag_lv2prchk.cpp) | `Tag_Lv2PrChk` → `fpcNm_Tag_Lv2PrChk_e` | DOL | Boomerang Switch Puzzle Tag |
| [`d_a_tag_lv5soup`](../src/d/actor/d_a_tag_lv5soup.cpp) | `TAG_LV5SOUP` → `fpcNm_TAG_SSDRINK_e` | REL |  |
| [`d_a_tag_lv6CstaSw`](../src/d/actor/d_a_tag_lv6CstaSw.cpp) | `Tag_Lv6CstaSw` → `fpcNm_Tag_Lv6CstaSw_e` | REL | Temple of Time Statue switch trigger |
| [`d_a_tag_magne`](../src/d/actor/d_a_tag_magne.cpp) | `Tag_Magne` → `fpcNm_Tag_Magne_e` | REL |  |
| [`d_a_tag_mhint`](../src/d/actor/d_a_tag_mhint.cpp) | `Tag_Mhint` → `fpcNm_Tag_Mhint_e` | REL | Tag - Midna Hint |
| [`d_a_tag_mist`](../src/d/actor/d_a_tag_mist.cpp) | `Tag_Mist` → `fpcNm_Tag_Mist_e` | REL |  |
| [`d_a_tag_mmsg`](../src/d/actor/d_a_tag_mmsg.cpp) | `Tag_Mmsg` → `fpcNm_Tag_Mmsg_e` | REL |  |
| [`d_a_tag_msg`](../src/d/actor/d_a_tag_msg.cpp) | `TAG_MSG` → `fpcNm_TAG_MSG_e` | REL |  |
| [`d_a_tag_mstop`](../src/d/actor/d_a_tag_mstop.cpp) | `Tag_Mstop` → `fpcNm_Tag_Mstop_e` | REL | Midna Stop Tag |
| [`d_a_tag_mwait`](../src/d/actor/d_a_tag_mwait.cpp) | `Tag_Mwait` → `fpcNm_Tag_Mwait_e` | REL | Midna Wait Trigger |
| [`d_a_tag_myna2`](../src/d/actor/d_a_tag_myna2.cpp) | `TAG_MYNA2` → `fpcNm_TAG_MYNA2_e` | REL |  |
| [`d_a_tag_myna_light`](../src/d/actor/d_a_tag_myna_light.cpp) | `TAG_MNLIGHT` → `fpcNm_TAG_MNLIGHT_e` | REL |  |
| [`d_a_tag_pachi`](../src/d/actor/d_a_tag_pachi.cpp) | `TAG_PATI` → `fpcNm_TAG_PATI_e` | REL |  |
| [`d_a_tag_poFire`](../src/d/actor/d_a_tag_poFire.cpp) | `Tag_poFire` → `fpcNm_Tag_poFire_e` | REL | Tag - Poe Fire |
| [`d_a_tag_push`](../src/d/actor/d_a_tag_push.cpp) | `TAG_PUSH` → `fpcNm_TAG_PUSH_e` | REL |  |
| [`d_a_tag_qs`](../src/d/actor/d_a_tag_qs.cpp) | `TAG_QS` → `fpcNm_TAG_QS_e` | REL |  |
| [`d_a_tag_ret_room`](../src/d/actor/d_a_tag_ret_room.cpp) | `Tag_RetRoom` → `fpcNm_Tag_RetRoom_e` | REL |  |
| [`d_a_tag_river_back`](../src/d/actor/d_a_tag_river_back.cpp) | `Tag_RiverBack` → `fpcNm_Tag_RiverBack_e` | REL | Tag - River Back |
| [`d_a_tag_rmbit_sw`](../src/d/actor/d_a_tag_rmbit_sw.cpp) | `Tag_RmbitSw` → `fpcNm_Tag_RmbitSw_e` | REL |  |
| [`d_a_tag_schedule`](../src/d/actor/d_a_tag_schedule.cpp) | `Tag_Schedule` → `fpcNm_Tag_Schedule_e` | DOL |  |
| [`d_a_tag_setBall`](../src/d/actor/d_a_tag_setBall.cpp) | `Tag_SetBall` → `fpcNm_Tag_SetBall_e` | DOL |  |
| [`d_a_tag_setrestart`](../src/d/actor/d_a_tag_setrestart.cpp) | `Tag_Restart` → `fpcNm_Tag_Restart_e` | REL | RMBack0 |
| [`d_a_tag_shop_camera`](../src/d/actor/d_a_tag_shop_camera.cpp) | `TAG_SHOPCAM` → `fpcNm_TAG_SHOPCAM_e` | REL | Tag - Shop Camera |
| [`d_a_tag_shop_item`](../src/d/actor/d_a_tag_shop_item.cpp) | `TAG_SHOPITM` → `fpcNm_TAG_SHOPITM_e` | REL | Tag - Shop Item |
| [`d_a_tag_smk_emt`](../src/d/actor/d_a_tag_smk_emt.cpp) | `Tag_SmkEmt` → `fpcNm_Tag_SmkEmt_e` | REL | Tag - Smoke Emit |
| [`d_a_tag_spinner`](../src/d/actor/d_a_tag_spinner.cpp) | `Tag_Spinner` → `fpcNm_Tag_Spinner_e` | REL | Tag - Spinner |
| [`d_a_tag_sppath`](../src/d/actor/d_a_tag_sppath.cpp) | `Tag_Sppath` → `fpcNm_Tag_Sppath_e` | REL | Tag - Spinner Path |
| [`d_a_tag_spring`](../src/d/actor/d_a_tag_spring.cpp) | `Tag_Spring` → `fpcNm_Tag_Spring_e` | REL |  |
| [`d_a_tag_ss_drink`](../src/d/actor/d_a_tag_ss_drink.cpp) | `TAG_SSDRINK` → `fpcNm_TAG_SSDRINK_e` | REL |  |
| [`d_a_tag_statue_evt`](../src/d/actor/d_a_tag_statue_evt.cpp) | `Tag_Statue` → `fpcNm_Tag_Statue_e` | REL |  |
| [`d_a_tag_stream`](../src/d/actor/d_a_tag_stream.cpp) | `Tag_Stream` → `fpcNm_Tag_Stream_e` | REL |  |
| [`d_a_tag_telop`](../src/d/actor/d_a_tag_telop.cpp) | `TAG_TELOP` → `fpcNm_TAG_TELOP_e` | DOL |  |
| [`d_a_tag_theB_hint`](../src/d/actor/d_a_tag_theB_hint.cpp) | `Tag_TheBHint` → `fpcNm_Tag_TheBHint_e` | REL | Tag - Telma B Hint |
| [`d_a_tag_wara_howl`](../src/d/actor/d_a_tag_wara_howl.cpp) | `Tag_WaraHowl` → `fpcNm_Tag_WaraHowl_e` | REL |  |
| [`d_a_tag_watchge`](../src/d/actor/d_a_tag_watchge.cpp) | `Tag_WatchGe` → `fpcNm_Tag_WatchGe_e` | REL | Tag - Guay |
| [`d_a_tag_waterfall`](../src/d/actor/d_a_tag_waterfall.cpp) | `Tag_WaterFall` → `fpcNm_Tag_WaterFall_e` | REL | Waterfall Without Collision Tag |
| [`d_a_tag_wljump`](../src/d/actor/d_a_tag_wljump.cpp) | `Tag_Wljump` → `fpcNm_Tag_Wljump_e` | REL |  |
| [`d_a_tag_yami`](../src/d/actor/d_a_tag_yami.cpp) | `TAG_YAMI` → `fpcNm_TAG_YAMI_e` | REL | Tag - Yami (Twili) |

## Minigames (3)

| Source | Profile → process name | Runs from | In-game name |
|---|---|---|---|
| [`d_a_mg_fish`](../src/d/actor/d_a_mg_fish.cpp) | `MG_FISH` → `fpcNm_MG_FISH_e` | REL | Fishing line ("keburu") |
| [`d_a_mg_fshop`](../src/d/actor/d_a_mg_fshop.cpp) | `FSHOP` → `fpcNm_FSHOP_e` | REL | Hena's Shop |
| [`d_a_mg_rod`](../src/d/actor/d_a_mg_rod.cpp) | `MG_ROD` → `fpcNm_MG_ROD_e` | REL | Fishing Rod |

## Misc actors (74)

| Source | Profile → process name | Runs from | In-game name |
|---|---|---|---|
| [`d_a_L7demo_dr`](../src/d/actor/d_a_L7demo_dr.cpp) | `DR` → `fpcNm_DR_e` | REL | * |
| [`d_a_L7low_dr`](../src/d/actor/d_a_L7low_dr.cpp) | `L7lowDr` → `fpcNm_L7lowDr_e` | REL | * |
| [`d_a_L7op_demo_dr`](../src/d/actor/d_a_L7op_demo_dr.cpp) | `L7ODR` → `fpcNm_L7ODR_e` | REL | * |
| [`d_a_alldie`](../src/d/actor/d_a_alldie.cpp) | `ALLDIE` → `fpcNm_ALLDIE_e` | REL | * |
| [`d_a_andsw`](../src/d/actor/d_a_andsw.cpp) | `ANDSW` → `fpcNm_ANDSW_e` | DOL | * |
| [`d_a_andsw2`](../src/d/actor/d_a_andsw2.cpp) | `ANDSW2` → `fpcNm_ANDSW2_e` | REL | * |
| [`d_a_arrow`](../src/d/actor/d_a_arrow.cpp) | `ARROW` → `fpcNm_ARROW_e` | REL | Arrow |
| [`d_a_balloon_2D`](../src/d/actor/d_a_balloon_2D.cpp) | `BALLOON2D` → `fpcNm_BALLOON2D_e` | REL |  |
| [`d_a_bd`](../src/d/actor/d_a_bd.cpp) | `BD` → `fpcNm_BD_e` | REL | Bird |
| [`d_a_bg`](../src/d/actor/d_a_bg.cpp) | `BG` → `fpcNm_BG_e` | REL | Background |
| [`d_a_bg_obj`](../src/d/actor/d_a_bg_obj.cpp) | `BG_OBJ` → `fpcNm_BG_OBJ_e` | REL | Moving Background Obj? |
| [`d_a_boomerang`](../src/d/actor/d_a_boomerang.cpp) | `BOOMERANG` → `fpcNm_BOOMERANG_e` | REL | Gale Boomerang |
| [`d_a_bullet`](../src/d/actor/d_a_bullet.cpp) | `BULLET` → `fpcNm_BULLET_e` | REL | Bullet (Unused?) |
| [`d_a_canoe`](../src/d/actor/d_a_canoe.cpp) | `CANOE` → `fpcNm_CANOE_e` | REL | Canoe |
| [`d_a_coach_2D`](../src/d/actor/d_a_coach_2D.cpp) | `COACH2D` → `fpcNm_COACH2D_e` | REL | Coach 2D |
| [`d_a_coach_fire`](../src/d/actor/d_a_coach_fire.cpp) | `COACH_FIRE` → `fpcNm_COACH_FIRE_e` | REL | Coach Fire |
| [`d_a_cow`](../src/d/actor/d_a_cow.cpp) | `COW` → `fpcNm_COW_e` | REL | Ordon Goat |
| [`d_a_crod`](../src/d/actor/d_a_crod.cpp) | `CROD` → `fpcNm_CROD_e` | REL | Dominion Rod |
| [`d_a_cstaF`](../src/d/actor/d_a_cstaF.cpp) | `CSTAF` → `fpcNm_CSTAF_e` | REL | Dominion Rod Statue |
| [`d_a_cstatue`](../src/d/actor/d_a_cstatue.cpp) | `CSTATUE` → `fpcNm_CSTATUE_e` | REL | Dominion Rod Statue |
| [`d_a_demo00`](../src/d/actor/d_a_demo00.cpp) | `DEMO00` → `fpcNm_DEMO00_e` | REL | Cutscene |
| [`d_a_demo_item`](../src/d/actor/d_a_demo_item.cpp) | `Demo_Item` → `fpcNm_Demo_Item_e` | REL | Cutscene Item |
| [`d_a_disappear`](../src/d/actor/d_a_disappear.cpp) | `DISAPPEAR` → `fpcNm_DISAPPEAR_e` | REL | Enemy Death Effect |
| [`d_a_dmidna`](../src/d/actor/d_a_dmidna.cpp) | `DMIDNA` → `fpcNm_DMIDNA_e` | REL | Dying Midna (White Midna) |
| [`d_a_do`](../src/d/actor/d_a_do.cpp) | `DO` → `fpcNm_DO_e` | REL | Dog |
| [`d_a_dshutter`](../src/d/actor/d_a_dshutter.cpp) | `DSHUTTER` → `fpcNm_DSHUTTER_e` | REL | Death Sword Shutter Gate |
| [`d_a_econt`](../src/d/actor/d_a_econt.cpp) | `ECONT` → `fpcNm_ECONT_e` | REL | Encounter |
| [`d_a_ep`](../src/d/actor/d_a_ep.cpp) | `EP` → `fpcNm_EP_e` | REL |  |
| [`d_a_formation_mng`](../src/d/actor/d_a_formation_mng.cpp) | `FORMATION_MNG` → `fpcNm_FORMATION_MNG_e` | REL |  |
| [`d_a_fr`](../src/d/actor/d_a_fr.cpp) | `FR` → `fpcNm_FR_e` | REL | Frog |
| [`d_a_grass`](../src/d/actor/d_a_grass.cpp) | `GRASS` → `fpcNm_GRASS_e` | REL | Grass |
| [`d_a_guard_mng`](../src/d/actor/d_a_guard_mng.cpp) | `GUARD_MNG` → `fpcNm_GUARD_MNG_e` | REL | Guard Manager? |
| [`d_a_hitobj`](../src/d/actor/d_a_hitobj.cpp) | `HITOBJ` → `fpcNm_HITOBJ_e` | REL | Hit Object? |
| [`d_a_horse`](../src/d/actor/d_a_horse.cpp) | `HORSE` → `fpcNm_HORSE_e` | REL | Epona |
| [`d_a_hozelda`](../src/d/actor/d_a_hozelda.cpp) | `HOZELDA` → `fpcNm_HOZELDA_e` | REL | Zelda (Horseback) |
| [`d_a_itembase`](../src/d/actor/d_a_itembase.cpp) | — | DOL | Item Actor base |
| [`d_a_izumi_gate`](../src/d/actor/d_a_izumi_gate.cpp) | `Izumi_Gate` → `fpcNm_Izumi_Gate_e` | REL | Ordon Spring Gate |
| [`d_a_kago`](../src/d/actor/d_a_kago.cpp) | `KAGO` → `fpcNm_KAGO_e` | REL | Player-controlled Kargarok |
| [`d_a_mant`](../src/d/actor/d_a_mant.cpp) | `MANT` → `fpcNm_MANT_e` | REL | Ganondorf's Cloak |
| [`d_a_midna`](../src/d/actor/d_a_midna.cpp) | `MIDNA` → `fpcNm_MIDNA_e` | REL | Midna |
| [`d_a_mirror`](../src/d/actor/d_a_mirror.cpp) | `MIRROR` → `fpcNm_MIRROR_e` | REL | Mirror |
| [`d_a_movie_player`](../src/d/actor/d_a_movie_player.cpp) | `MOVIE_PLAYER` → `fpcNm_MOVIE_PLAYER_e` | REL | Movie Player |
| [`d_a_myna`](../src/d/actor/d_a_myna.cpp) | `MYNA` → `fpcNm_MYNA_e` | REL | Trill |
| [`d_a_nbomb`](../src/d/actor/d_a_nbomb.cpp) | `NBOMB` → `fpcNm_NBOMB_e` | REL | Bomb |
| [`d_a_ni`](../src/d/actor/d_a_ni.cpp) | `NI` → `fpcNm_NI_e` | REL | Cucco |
| [`d_a_no_chg_room`](../src/d/actor/d_a_no_chg_room.cpp) | `NO_CHG_ROOM` → `fpcNm_NO_CHG_ROOM_e` | DOL |  |
| [`d_a_passer_mng`](../src/d/actor/d_a_passer_mng.cpp) | `PASSER_MNG` → `fpcNm_PASSER_MNG_e` | REL | Hylian Passerby Manager? |
| [`d_a_path_line`](../src/d/actor/d_a_path_line.cpp) | `PATH_LINE` → `fpcNm_PATH_LINE_e` | REL |  |
| [`d_a_peru`](../src/d/actor/d_a_peru.cpp) | `PERU` → `fpcNm_PERU_e` | REL | Louise |
| [`d_a_player`](../src/d/actor/d_a_player.cpp) | — | DOL | Base Player Actor functionality |
| [`d_a_ppolamp`](../src/d/actor/d_a_ppolamp.cpp) | `PPolamp` → `fpcNm_PPolamp_e` | REL | P Poe Lamp |
| [`d_a_scene_exit`](../src/d/actor/d_a_scene_exit.cpp) | `SCENE_EXIT` → `fpcNm_SCENE_EXIT_e` | REL | Scene Exit |
| [`d_a_scene_exit2`](../src/d/actor/d_a_scene_exit2.cpp) | `SCENE_EXIT2` → `fpcNm_SCENE_EXIT2_e` | REL | Scene Exit 2 |
| [`d_a_set_bgobj`](../src/d/actor/d_a_set_bgobj.cpp) | `SET_BG_OBJ` → `fpcNm_SET_BG_OBJ_e` | REL | Set Background Object |
| [`d_a_shop_item`](../src/d/actor/d_a_shop_item.cpp) | `ShopItem` → `fpcNm_ShopItem_e` | REL | Shop Item Actor |
| [`d_a_skip_2D`](../src/d/actor/d_a_skip_2D.cpp) | `SKIP2D` → `fpcNm_SKIP2D_e` | REL |  |
| [`d_a_spinner`](../src/d/actor/d_a_spinner.cpp) | `SPINNER` → `fpcNm_SPINNER_e` | REL | Spinner |
| [`d_a_sq`](../src/d/actor/d_a_sq.cpp) | `SQ` → `fpcNm_SQ_e` | REL | Squirrel |
| [`d_a_startAndGoal`](../src/d/actor/d_a_startAndGoal.cpp) | `START_AND_GOAL` → `fpcNm_START_AND_GOAL_e` | REL |  |
| [`d_a_suspend`](../src/d/actor/d_a_suspend.cpp) | `SUSPEND` → `fpcNm_SUSPEND_e` | REL | Suspend |
| [`d_a_swBall`](../src/d/actor/d_a_swBall.cpp) | `SwBall` → `fpcNm_SwBall_e` | REL | Switch Ball |
| [`d_a_swLBall`](../src/d/actor/d_a_swLBall.cpp) | `SwLBall` → `fpcNm_SwLBall_e` | REL | Switch L Ball |
| [`d_a_swTime`](../src/d/actor/d_a_swTime.cpp) | `SwTime` → `fpcNm_SwTime_e` | DOL | Switch Time |
| [`d_a_swc00`](../src/d/actor/d_a_swc00.cpp) | `SWC00` → `fpcNm_SWC00_e` | REL | Switch Area C |
| [`d_a_swhit0`](../src/d/actor/d_a_swhit0.cpp) | `SWHIT0` → `fpcNm_SWHIT0_e` | REL | Crystal Switch (?) |
| [`d_a_talk`](../src/d/actor/d_a_talk.cpp) | `TALK` → `fpcNm_TALK_e` | REL | Talk (Unused?) |
| [`d_a_tbox`](../src/d/actor/d_a_tbox.cpp) | `TBOX` → `fpcNm_TBOX_e` | REL | Treasure Box |
| [`d_a_tbox2`](../src/d/actor/d_a_tbox2.cpp) | `TBOX2` → `fpcNm_TBOX2_e` | REL | Treasure Box 2 |
| [`d_a_tboxSw`](../src/d/actor/d_a_tboxSw.cpp) | `TBOX_SW` → `fpcNm_TBOX_SW_e` | DOL | Treasure Box Switch |
| [`d_a_title`](../src/d/actor/d_a_title.cpp) | `TITLE` → `fpcNm_TITLE_e` | REL | Title Logo |
| [`d_a_vrbox`](../src/d/actor/d_a_vrbox.cpp) | `VRBOX` → `fpcNm_VRBOX_e` | REL | VR Box |
| [`d_a_vrbox2`](../src/d/actor/d_a_vrbox2.cpp) | `VRBOX2` → `fpcNm_VRBOX2_e` | REL | VR Box 2 |
| [`d_a_warp_bug`](../src/d/actor/d_a_warp_bug.cpp) | `WarpBug` → `fpcNm_WarpBug_e` | REL | Warp Bug (unused) |
| [`d_a_ykgr`](../src/d/actor/d_a_ykgr.cpp) | `Ykgr` → `fpcNm_Ykgr_e` | REL | Floor Gravity On/Off |
