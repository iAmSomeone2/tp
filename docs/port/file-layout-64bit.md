# File data on 64-bit hosts: pointers inside loaded files

Dev builds run as native 64-bit programs (Linux, Apple-silicon macOS), so `sizeof(void*)` is 8. The game, though, reads its data files by casting a pointer into a loaded buffer to a struct and reading fields directly. The disc files were laid out for a 32-bit console, so **any such struct with a pointer-sized field no longer matches the bytes** on a 64-bit host. Every later field reads from the wrong offset, and arrays of such structs get the wrong stride. Many of these files also have offsets rewritten into pointers *in place*, and a 64-bit pointer doesn't fit in a 4-byte slot.

The Vita (ILP32) is unaffected by this. It is a separate problem from byte order ([endianness.md](endianness.md)), but both need the same per-format field knowledge, so they should be designed together. This page is the survey that design starts from. For pointer-sized *integers* narrowed in code, see [pointer-truncation.md](pointer-truncation.md).

## Summary

| Pattern | What the code does | Breaks on LP64 because | Formats |
|---|---|---|---|
| **A. Offsets declared as pointers, read-only** | A file struct declares an offset field as `void*`/`T*` (or `uintptr_t`). The loader passes it to `JSUConvertOffsetToPtr(base, const void* offset)` and stores the result in a *host* object. The file itself is not written. | The struct layout: each such field is 8 bytes instead of 4. | J3D models (`.bmd/.bdl` blocks), J3D/J2D animations, J3D clusters (`.bls`), `ResTIMG` |
| **B. In-place relocation** | The loader adds the buffer's base address to an offset field and writes the pointer back into the file. Code then reads the field as a pointer. Most sites guard against relocating twice. | The layout, and a 64-bit pointer doesn't fit in the 4-byte slot. The guards rely on console addresses being ≥ `0x80000000`. | Stage/room `.dzs/.dzr` (node table, room table, paths), `.dzb` collision, `.kcl`, map paths, BG parts, J3D vertex-colour animations, J3D clusters (in a copy) |
| **C. File memory used as runtime storage** | A field or padding inside the loaded file is used to cache a host pointer. | The layout, and the pointer doesn't fit. | RARC file table (`SDIFileEntry::data`), event data (`dEvDtStaff_c::mWork`) |
| *Host object pointing into a file* | A host-side class keeps pointers into a file buffer (e.g. `JKRArchive::mNodes`, `JUTTexture::mTexData`, `dMsgFlow_c::mFlowNodeTBL`). | Nothing: the pointers live in host memory. | (many; fine) |

There are **no findings in JPA particles, JStudio cutscenes, JMessage/BMG, the JAudio banks/sequences or the J2D screen files**. Their file structs store offsets as `u32` and convert them into host pointers, which works on any pointer size. The scan can only see pointer and pointer-sized fields, so a struct that breaks for some other reason (e.g. a `u64` alignment difference) would not show up.

### Design-relevant observations

* **The RARC archive is the root of all loading** (pattern C). Every resource fetch goes through `SDIFileEntry`, whose 0x14-byte disc layout becomes 0x18 bytes once `data` is a pointer, so on 64-bit even the file *table* is misread. This has to be fixed first, and the fix is local: keep the cached pointers in a host-side array indexed by file index.
* **The relocation guards assume console addresses** (`(uintptr_t)p < 0x80000000` means "still an offset"). The guards exist because the same resident buffer can go through its init handler more than once, so whatever replaces in-place relocation has to stay valid for the resource's whole life, not just one parse.
* **Some relocations cross chunks:** path points are relative to the *points* chunk (`getPntInf()->m_pnt_offset`), and room-table entries are relative to a base the caller passes in (`param_3`). A per-struct fix-up alone is not enough for these.
* **Pattern A needs no design decision.** Changing those fields to `u32` offsets (and making `JSUConvertOffsetToPtr` take the `u32`) restores the layout. That fix is identical on 32 and 64 bits, and it already matches how most of JSystem's other formats work.
* **Layout `STATIC_ASSERT`s don't cover these structs.** The tree has 764 `STATIC_ASSERT(sizeof(...))` checks, but only one of them, `dEvDtStaff_c`, is on a file struct. Adding `static_assert(sizeof(T) == 0x..)` to every file struct gives a compile-time check on both 32 and 64 bits.

## A. Offsets declared as pointers (read-only)

Found by matching calls to the `const void*` overload of `JSUConvertOffsetToPtr` whose offset argument is a struct field: **176 calls on 26 structs**, all in J3D.

| Struct (header) | Calls | Used in |
|---|---|---|
| `J3DMaterialBlock` (`J3DModelLoader.h:95`) | 32 | `J3DMaterialFactory.cpp`, `J3DModelLoader.cpp` |
| `J3DMaterialBlock_v21` (`J3DModelLoader.h:133`) | 28 | `J3DMaterialFactory_v21.cpp`, `J3DModelLoader.cpp` |
| `J3DAnmTextureSRTKeyData` (`J3DAnimation.h:274`) | 18 | `J3DAnmLoader.cpp`, `J2DAnmLoader.cpp` |
| `J3DAnmTevRegKeyData` (`J3DAnimation.h:353`) | 14 | `J3DAnmLoader.cpp` |
| `J3DAnmColorFullData` (`J3DAnimation.h:390`) | 11 | `J3DAnmLoader.cpp`, `J2DAnmLoader.cpp` |
| `J3DAnmVtxColorFullData` (`J3DAnimation.h:191`) | 8 | `J3DAnmLoader.cpp`, `J2DAnmLoader.cpp` |
| `J3DAnmColorKeyData` (`J3DAnimation.h:249`) | 7 | `J3DAnmLoader.cpp` |
| `J3DMaterialDLBlock` (`J3DModelLoader.h:168`) | 5 | `J3DMaterialFactory.cpp`, `J3DModelLoader.cpp` |
| `J3DClusterBlock` (`J3DClusterLoader.h:18`) | 5 | `J3DClusterLoader.cpp` |
| `J3DVertexBlock`, `J3DEnvelopeBlock`, `J3DTextureBlock` (`J3DModelLoader.h`) | 4 each | `J3DModelLoader.cpp` |
| `J3DAnmTransformFullData`, `J3DAnmTexPatternFullData`, `J3DAnmTransformKeyData`, `J3DAnmVtxColorKeyData` (`J3DAnimation.h`) | 4 each | `J3DAnmLoader.cpp` |
| `J3DAnmVisibilityFullData`, `J3DCluster` | 3 each | `J3DAnmLoader.cpp`/`J2DAnmLoader.cpp`, `J3DClusterLoader.cpp` |
| `J3DClusterKey`, `J3DClusterVertex`, `J3DModelInfoBlock`, `J3DDrawBlock`, `J3DAnmClusterFullData`, `J3DAnmClusterKeyData` | 2 each | `J3DClusterLoader.cpp`, `J3DModelLoader.cpp`, `J3DAnmLoader.cpp` |
| `J3DJointBlock`, `J3DShapeBlock` | 1 each | `J3DModelLoader.cpp` |

Also in this pattern:

* **`ResTIMG`** (`JUTTexture.h:18`): `paletteOffset` (0x0C) and `imageOffset` (0x1C) are `uintptr_t`, so on 64-bit the 0x20-byte texture header is 0x30 bytes. This affects every texture: BTI files and the texture tables inside BMD/BDL. Upstream picked `uintptr_t` (commit `b45a089e15`) to make the pointer arithmetic compile; on the console the effect was the same as `u32`.
* `J3DModelLoader.cpp:337–369` computes vertex counts as differences between pointer-typed offset fields of `J3DVertexBlock` (these lines are also in [pointer-truncation.md](pointer-truncation.md)). They become correct once the fields are `u32`.

## B. In-place relocation

| Site | Data | Guard against relocating twice | Notes |
|---|---|---|---|
| `d_stage.cpp:2281` `dStage_dt_c_offsetToPtr` | `.dzs/.dzr` node table: `dStage_nodeHeader::m_offset` (a `u32`) += buffer address | `m_offset < 0x80000000` | Each chunk handler then reads its chunk as `{int num; T* m_entries;}` over `{entryNum, offset}`. About 25 such header structs in `d_stage.h` (`stage_actor_class`, `stage_tgsc_class`, `stage_camera_class`, `stage_arrow_class`, `stage_tresure_class`, `stage_scls_info_dummy_class`, `stage_map_info_dummy_class`, `map_path_class`, `roomRead_class`, `dStage_dPath_c`, `dStage_Multi_c`, `dStage_SoundInfo_c`, `dStage_FileList2_c`, `dStage_FloorInfo_c`, `dStage_Lbnk_c`, `dStage_Elst_c`, `dStage_DMap_c`, `dStage_MapEventInfo_c`, `dStage_MemoryMap_c`, `dStage_MemoryConfig_c`, …) plus `dTres_c::list_class`. The node-table add is also a truncation site in [pointer-truncation.md](pointer-truncation.md). |
| `d_stage.cpp:2073–2075` `dStage_roomReadInit` | Room table: `roomRead_data_class**` array in the file, and each entry's `m_rooms` | `(intptr_t)rtbl[i] < 0x80000000` | The base is the caller's `param_3`, not the chunk. |
| `d_stage.cpp:2108`, `2133` (`pathInfoInit`, `rpatInfoInit`) | `dPath::m_points` | `< 0x80000000` / `>= 0x80000000` | **Cross-chunk:** relative to the point chunk (`getPntInf()/getPnt2Inf()->m_pnt_offset`). |
| `d_bg_s.cpp:345` `cBgS::ConvDzb` | `.dzb` collision: six table offsets of the header, plus `strOffset` of each group | Flag bit `0x80000000` in `m_flags` | Writes through a `u32` mirror struct `cBgD_t_`, then everything reads the header through `cBgD_t`, which has `T*` fields. **106 casts** to `cBgD_t*` in actors (`dComIfG_getObjectRes` → `dBgW::Set`). `d_bg_s.cpp:372` (`strOffset`) is in [pointer-truncation.md](pointer-truncation.md). |
| `d_bg_w_kcol.cpp:22–25` `dBgWKCol::initKCollision` | `.kcl`: `KC_Header` position/normal/prism/block pointers | **None** | Assumes it is called once per loaded buffer. `KC_Header` is cast in 13 places in `d_bg_w_kcol.cpp`. |
| `d_map_path_dmap.cpp:419–444` `dMpath_c::setPointer` | Map paths: room → floor → group → line/poly → `u16` data (five levels) | `room->mpFloor >= 0x80000000` (line 393) | `dDrawPath_c::room_class/floor_class/group_class/line_class/poly_class` (`d_map_path.h`). |
| `d_bg_parts.cpp:748–751` `dBgp_c::setPointer` | BG parts map unit: `groups`, `unk_0x8`, `unk_0xc` | `groups < 0x80000000` | `stage_map_unit_class` / `unit_class`. |
| `J3DAnmLoader.cpp:386–389`, `667–670` | Vertex-colour animations (`.bva`, both full and key): `J3DAnmVtxColorIndexData::mpData` in an array in the file | None | `{u16 mNum; void* mpData;}`: stride 8 on the console, 16 on LP64. |
| `J3DClusterLoader.cpp:79–111` | `.bls` clusters: copies the block to a new buffer, then relocates `J3DCluster`/`J3DClusterKey`/`J3DClusterVertex` pointers in the copy | n/a (fresh copy) | The copy still has the file layout. Fixing the layout here also means converting the copy into native structs. |

## C. File memory used as runtime storage

| Site | Field | Problem |
|---|---|---|
| RARC archives: `JKRMemArchive.cpp:129`, `JKRDvdArchive.cpp:161`, `JKRCompArchive.cpp:235/242/253`, `JKRAramArchive.cpp:212`; cleared in `JKRArchivePub.cpp:253–277` and the archive `remove*` paths | `SDIFileEntry::data` (`JKRArchive.h:73`): a 4-byte slot after `data_size` in the file-entry table, 0 on disc, used to cache the loaded resource's address | The table is part of the archive's info block (read from the file). On 64-bit each entry is 0x18 bytes instead of 0x14, so every lookup past entry 0 is wrong. |
| `d_event_data.cpp:530` (`dEvDtStaff_c`) | `mWork[0x18]` scratch area inside each event staff record, reinterpreted as `StaffWork { fpc_ProcID; msg_class*; u32; struct { int; int*; } }` | On 64-bit `StaffWork` is 40 bytes, so it overruns the 24-byte `mWork` into `mTagID`, `mIndex`, `mFlagID`, … of the same record. |

## How this was found

The scan uses `clang-query` (clang-tools-extra) over the configured CMake build. A full run takes about 40 seconds per matcher on 16 cores. The matchers below were run against `build/cmake/clang/compile_commands.json`, skipping tests and `_deps`. Classifying the results into the tables above was done by hand.

1. **Structs cast from raw bytes that contain pointer-sized fields** (243 casts, 70 structs; this list includes host objects passed through `void*` callbacks, which were removed by hand):

   ```
   explicitCastExpr(
     hasDestinationType(pointsTo(cxxRecordDecl(
       hasDescendant(fieldDecl(anyOf(
         hasType(hasCanonicalType(pointerType())),
         hasType(hasCanonicalType(constantArrayType(hasElementType(pointerType())))),
         hasType(hasCanonicalType(memberPointerType())),
         hasType(qualType(hasCanonicalType(qualType(asString("unsigned long"))))),
         hasType(qualType(hasCanonicalType(qualType(asString("long")))))))),
       unless(anyOf(isSameOrDerivedFrom(PROC), has(fieldDecl(hasType(hasCanonicalType(
                      hasDeclaration(cxxRecordDecl(isSameOrDerivedFrom(PROC))))))),
                    hasMethod(isVirtual()), isDerivedFrom(cxxRecordDecl(hasMethod(isVirtual()))))))
       .bind("rec"))),
     hasSourceExpression(ignoringParenImpCasts(expr(anyOf(
       hasType(pointsTo(builtinType())), hasType(isInteger()))))))
   ```

   `PROC` is `hasAnyName("base_process_class", "fopAc_ac_c", "leafdraw_class", "process_node_class", "kankyo_class", "scene_class", "view_class", "camera_class", "overlap_task_class", "msg_class", "CMemBlock", "JKRHeap")`. It removes actor and process classes, which are cast from `void*` all over the code. "unsigned long" also matches `u64`, so `ResFONT` (`u64 magic`) is a false positive.

2. **Pattern A:** `callExpr(callee(functionDecl(hasName("JSUConvertOffsetToPtr"), hasParameter(1, hasType(pointerType())))), hasArgument(1, ignoringParenImpCasts(memberExpr(hasDeclaration(fieldDecl(hasParent(recordDecl().bind("rec"))))))))`.

3. **Pointer fields assigned from base + offset** (patterns B, C and host objects, 64 sites): `binaryOperator(isAssignmentOperator(), hasLHS(ignoringParenImpCasts(memberExpr(hasDeclaration(fieldDecl(hasType(hasCanonicalType(pointerType()))))))), hasRHS(ignoringParenImpCasts(explicitCastExpr(hasDestinationType(hasCanonicalType(pointerType())), hasSourceExpression(ignoringParenImpCasts(binaryOperator(hasOperatorName("+"))))))))`. Separately, `grep` for comparisons against `0x80000000` finds the relocation guards.

Run one with `clang-query -p build/cmake/clang <files…> -c "set output diag" -c "set bind-root false" -c "match <matcher>"`.
