#!/usr/bin/env python3

###
# Surveys the source tree for the things a port to a non-PowerPC, little-endian
# platform has to deal with, and writes docs/port/survey-data.md.
#
#   * which Nintendo SDK APIs the game/middleware code calls, and from where
#   * how much raw GX (GPU) code exists outside JSystem
#   * PowerPC-specific code (inline asm, intrinsics, cache operations)
#   * byte-order / ABI hazards (bit-fields, multi-char constants, type punning)
#   * binary-format parser inventory
#
# The numbers are regex-based estimates, meant to size the work, not to be exact.
#
# Usage (from anywhere):
#   python3 tools/utilities/port_survey.py
###

import collections
import os
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
os.chdir(ROOT)

SRC_EXT = (".cpp", ".c", ".inc", ".h")
GAME_ROOTS = ["src", "libs/JSystem", "include"]


def files(roots, exts=SRC_EXT):
    for root in roots:
        for dp, _, fns in os.walk(root):
            for fn in sorted(fns):
                if fn.endswith(exts):
                    p = os.path.join(dp, fn)
                    yield p, open(p, errors="replace").read()


def area(p):
    """Bucket a path into a coarse area used by the tables."""
    parts = p.split("/")
    if parts[0] == "libs" and parts[1] == "JSystem":
        # libs/JSystem/src/<Lib>/... or libs/JSystem/include/JSystem/<Lib>/...
        idx = 3 if parts[2] == "src" else 4
        if len(parts) > idx + 1:
            return "JSystem/" + parts[idx]
        return "JSystem (top-level headers)"
    if parts[0] == "src" and parts[1] == "d":
        return "src/d/actor" if len(parts) > 2 and parts[2] == "actor" else "src/d (core)"
    if parts[0] == "src":
        return "src/" + parts[1] if len(parts) > 2 else "src (root)"
    if parts[0] == "include":
        return "include"
    return parts[0]


# --------------------------------------------------------------------------- SDK families
FAMILIES = {
    "OS (threads, time, memory, reset, logging)": r"\bOS[A-Z][A-Za-z0-9]*(?=\()",
    "GX (GPU)": r"\bGX[A-Z][A-Za-z0-9]*(?=\()",
    "GD (GX display-list builders)": r"\bGD[A-Z][A-Za-z0-9]*(?=\()",
    "VI (video output / retrace)": r"\bVI[A-Z][A-Za-z0-9]*(?=\()",
    "DVD (disc I/O)": r"\bDVD[A-Z][A-Za-z0-9]*(?=\()",
    "PAD/SI (GameCube pad)": r"\b(?:PAD|SI)[A-Z][A-Za-z0-9]*(?=\()",
    "CARD (memory card)": r"\bCARD[A-Z][A-Za-z0-9]*(?=\()",
    "AI/AX/DSP (audio hardware)": r"\b(?:AI|AX|AXFX|DSP)[A-Z][A-Za-z0-9]*(?=\()",
    "AR/ARQ (ARAM)": r"\bARQ?[A-Z][A-Za-z0-9]*(?=\()",
    "MTX/VEC/QUAT (SDK matrix math)": r"\b(?:PSMTX|C_MTX|MTX|PSVEC|C_VEC|VEC|PSQUAT|C_QUAT|QUAT)[A-Za-z0-9]*(?=\()",
    "EXI/HIO/MCC (dev hardware)": r"\b(?:EXI|HIO2?|MCC|AMC)[A-Z][A-Za-z0-9]*(?=\()",
    "WPAD/KPAD (Wii remote)": r"\b(?:WPAD|KPAD)[A-Z][A-Za-z0-9]*(?=\()",
    "NAND/SC (Wii storage/settings)": r"\b(?:NAND|SC)[A-Z][A-Za-z0-9]*(?=\()",
}

OS_CATEGORIES = [
    ("logging/panic", r"OS(?:Report|Panic|Warning|Attention|VAttention|Error)|OSReport"),
    ("threads, mutexes, message queues", r"OS(?:CreateThread|ResumeThread|SuspendThread|JoinThread|GetCurrentThread|SetThreadPriority|GetThreadPriority|CancelThread|Yield|Sleep|InitMutex|LockMutex|UnlockMutex|TryLockMutex|InitCond|WaitCond|SignalCond|InitMessageQueue|SendMessage|ReceiveMessage|JamMessage|EnableScheduler|DisableScheduler)"),
    ("interrupt masking (critical sections)", r"OS(?:DisableInterrupts|RestoreInterrupts|EnableInterrupts)"),
    ("time and stopwatches", r"OS(?:GetTime|GetTick|TicksTo\w+|\w+ToTicks|InitStopwatch|StartStopwatch|StopStopwatch|ResetStopwatch|DumpStopwatch)"),
    ("alarms", r"OS(?:CreateAlarm|SetAlarm|SetPeriodicAlarm|CancelAlarm)"),
    ("arena / memory", r"OS(?:Alloc|Free|GetArena\w*|SetArena\w*|InitAlloc|CreateHeap|RoundUp\w+|RoundDown\w+|PhysicalToCached|CachedToPhysical|InitFastCast)"),
    ("dynamic modules (RELs)", r"OS(?:Link\w*|Unlink|SetStringTable)"),
    ("reset / system settings", r"OS(?:Reset\w*|Restart|Shutdown|Return\w*|GetResetCode|GetSoundMode|GetLanguage|GetProgressiveMode|GetConsoleType)"),
]

# --------------------------------------------------------------------------- GX groups
GX_GROUPS = [
    ("TEV / texgen setup (fixed-function shading)", r"GXSetTev\w+|GXSetNumTevStages|GXSetTexCoordGen\w*|GXSetNumTexGens"),
    ("immediate-mode vertex submission", r"GXBegin|GXEnd|GXPosition\w+|GXNormal\w+|GXColor\w+|GXTexCoord\w+"),
    ("vertex formats / arrays", r"GXSetVtxDesc|GXSetVtxAttrFmt\w*|GXClearVtxDesc|GXSetArray|GXInvalidateVtxCache"),
    ("z / alpha / blend / cull state", r"GXSet(?:ZMode|ZCompLoc|BlendMode|AlphaCompare|AlphaUpdate|ColorUpdate|CullMode|Dither|PixelFmt|ClipMode)"),
    ("matrices / projection / viewport", r"GXLoad(?:Pos|Nrm|Tex)MtxImm|GXSetCurrentMtx|GXSetProjection\w*|GXSetViewport\w*|GXSetScissor\w*"),
    ("texture objects and palettes", r"GX(?:Init|Load)TexObj\w*|GXInitTlutObj|GXLoadTlut|GXInitTexCacheRegion|GXInvalidateTexAll"),
    ("lighting (per-channel)", r"GX(?:InitLight\w*|LoadLightObj\w*|SetChanCtrl|SetChanAmbColor|SetChanMatColor|SetNumChans)"),
    ("indirect texturing", r"GX(?:SetIndTex\w*|SetTevIndirect|SetNumIndStages|SetTevDirect|SetTevIndWarp|SetTevIndTile|SetTevIndBumpST)"),
    ("fog", r"GXSetFog\w*|GXInitFogAdjTable"),
    ("EFB copy / render-to-texture", r"GX(?:CopyTex|CopyDisp|SetTexCopySrc|SetTexCopyDst|SetCopyClear|SetCopyFilter|SetDispCopy\w*)"),
    ("display lists (prebuilt GX command streams)", r"GXCallDisplayList|GXBeginDisplayList|GXEndDisplayList"),
]

# --------------------------------------------------------------------------- parsers
# (format, component, files) – files are checked for existence when the report is written.
PARSERS = [
    ("RARC archives (`.arc`), Yaz0/Yay0", "JKernel", [
        "libs/JSystem/src/JKernel/JKRArchivePri.cpp", "libs/JSystem/src/JKernel/JKRMemArchive.cpp",
        "libs/JSystem/src/JKernel/JKRDvdArchive.cpp", "libs/JSystem/src/JKernel/JKRAramArchive.cpp",
        "libs/JSystem/src/JKernel/JKRCompArchive.cpp", "libs/JSystem/src/JKernel/JKRDecomp.cpp"]),
    ("J3D models `.bmd/.bdl` (incl. embedded GX display lists)", "J3DGraphLoader", [
        "libs/JSystem/src/J3DGraphLoader/J3DModelLoader.cpp", "libs/JSystem/src/J3DGraphLoader/J3DMaterialFactory.cpp",
        "libs/JSystem/src/J3DGraphLoader/J3DMaterialFactory_v21.cpp", "libs/JSystem/src/J3DGraphLoader/J3DShapeFactory.cpp",
        "libs/JSystem/src/J3DGraphLoader/J3DJointFactory.cpp", "libs/JSystem/src/J3DGraphLoader/J3DModelLoaderCalcSize.cpp"]),
    ("J3D animations `.bck/.btk/.brk/.btp/.bpk/.blk`", "J3DGraphLoader", [
        "libs/JSystem/src/J3DGraphLoader/J3DAnmLoader.cpp", "libs/JSystem/src/J3DGraphLoader/J3DClusterLoader.cpp"]),
    ("J2D layouts `.blo`, J2D animations, fonts, textures", "J2DGraph / JUtility", [
        "libs/JSystem/src/J2DGraph/J2DScreen.cpp", "libs/JSystem/src/J2DGraph/J2DAnmLoader.cpp",
        "libs/JSystem/src/J2DGraph/J2DMaterialFactory.cpp", "libs/JSystem/src/JUtility/JUTResFont.cpp",
        "libs/JSystem/src/JUtility/JUTTexture.cpp", "libs/JSystem/src/JUtility/JUTNameTab.cpp"]),
    ("Stage/room data `.dzs/.dzr` (tagged chunks)", "d_stage", ["src/d/d_stage.cpp"]),
    ("Collision meshes `.dzb`", "d_bg", ["src/d/d_bg_w.cpp", "src/d/d_bg_w_kcol.cpp", "src/d/d_bg_w_base.cpp"]),
    ("Event scripts (staff/cut data)", "d_event", ["src/d/d_event_data.cpp", "src/d/d_event_manager.cpp"]),
    ("Messages `.bmg` and message flow", "JMessage / d_msg", [
        "libs/JSystem/src/JMessage/resource.cpp", "libs/JSystem/src/JMessage/processor.cpp", "src/d/d_msg_flow.cpp",
        "src/d/d_msg_object.cpp"]),
    ("Particles `.jpc`", "JParticle", ["libs/JSystem/src/JParticle/JPAResourceLoader.cpp", "libs/JSystem/src/JParticle/JPAResourceManager.cpp"]),
    ("Cutscene timelines `.stb`", "JStudio", ["libs/JSystem/src/JStudio/JStudio/stb.cpp", "libs/JSystem/src/JStudio/JStudio/stb-data-parse.cpp"]),
    ("Audio banks/waves/sequences (`.baa`, BNK, WSYS, BMS)", "JAudio2", [
        "libs/JSystem/src/JAudio2/JASBNKParser.cpp", "libs/JSystem/src/JAudio2/JASWSParser.cpp",
        "libs/JSystem/src/JAudio2/JASSeqParser.cpp", "libs/JSystem/src/JAudio2/JASSeqReader.cpp",
        "libs/JSystem/src/JAudio2/JAUAudioArcInterpreter.cpp", "libs/JSystem/src/JAudio2/JASAramStream.cpp"]),
    ("Item tables, menu data (raw `.bin`/`.dat`)", "d_item / d_s_logo", ["src/d/d_item.cpp", "src/d/d_s_logo.cpp"]),
    ("Save data and memory-card images", "d_save / m_Do_MemCard", [
        "src/d/d_save.cpp", "src/m_Do/m_Do_MemCard.cpp", "src/m_Do/m_Do_MemCardRWmng.cpp"]),
    ("THP movies", "d_a_movie_player", ["src/d/actor/d_a_movie_player.cpp"]),
]


def loc(p):
    try:
        return sum(1 for _ in open(p, errors="replace"))
    except OSError:
        return None


def md_table(header, rows):
    out = ["| " + " | ".join(header) + " |", "|" + "|".join("---" for _ in header) + "|"]
    for r in rows:
        out.append("| " + " | ".join(str(c) for c in r) + " |")
    return out


def main():
    all_files = list(files(GAME_ROOTS))
    out = [
        "# Port survey data (generated)\n",
        "Generated by `python3 tools/utilities/port_survey.py`. Counts are regex-based estimates over `src/`, `include/` and `libs/JSystem/` (the SDK under `libs/dolphin` and `libs/revolution` is what a port *replaces*, so it is excluded). Use them to size work, not as exact figures.\n",
    ]

    # ---- lines of code
    out.append("## Lines of code by area\n")
    loc_by = collections.Counter()
    for p, t in all_files:
        if p.startswith("include/"):
            continue
        loc_by[area(p)] += t.count("\n")
    for extra in ("libs/dolphin", "libs/revolution", "libs/PowerPC_EABI_Support"):
        n = 0
        for p, t in files([extra]):
            n += t.count("\n")
        loc_by[extra + " (replaced, not ported)"] = n
    out += md_table(["Area", "Lines"], [(k, f"{v:,}") for k, v in sorted(loc_by.items(), key=lambda x: -x[1])])

    # ---- SDK family x area
    out.append("\n## SDK API calls by family and area\n")
    out.append("Call counts (`Name(`) per SDK family. Only areas with at least one call are shown.\n")
    fam_area = collections.defaultdict(collections.Counter)
    fam_files = collections.defaultdict(set)
    fam_names = collections.defaultdict(collections.Counter)
    for p, t in all_files:
        if p.startswith("include/") and not p.endswith(".h"):
            continue
        for fam, rx in FAMILIES.items():
            hits = re.findall(rx, t)
            if hits:
                fam_area[fam][area(p)] += len(hits)
                fam_files[fam].add(p)
                fam_names[fam].update(hits)
    rows = []
    for fam in FAMILIES:
        c = fam_area[fam]
        rows.append((fam, sum(c.values()), len(fam_names[fam]), len(fam_files[fam]),
                     ", ".join(f"{a} ({n})" for a, n in c.most_common(4))))
    out += md_table(["Family", "Calls", "Distinct functions", "Files", "Biggest users (calls)"], rows)

    out.append("\n### Most-called functions per family\n")
    for fam in FAMILIES:
        if fam_names[fam]:
            out.append(f"* **{fam}**: " + ", ".join(f"`{n}` ({c})" for n, c in fam_names[fam].most_common(10)))

    # ---- OS categories
    out.append("\n## `OS*` calls by purpose\n")
    os_names = fam_names["OS (threads, time, memory, reset, logging)"]
    cat_count = collections.Counter()
    for name, n in os_names.items():
        for cat, rx in OS_CATEGORIES:
            if re.fullmatch(rx, name):
                cat_count[cat] += n
                break
        else:
            cat_count["other"] += n
    out += md_table(["Purpose", "Calls"], cat_count.most_common())

    # ---- raw GX outside JSystem
    out.append("\n## GX (GPU) usage\n")
    gx_names = collections.Counter()
    gx_files_src = collections.Counter()
    for p, t in all_files:
        if not p.endswith((".cpp", ".c", ".inc")):
            continue
        hits = re.findall(r"\b(GX[A-Z][A-Za-z0-9]*|GD[A-Z][A-Za-z0-9]*)(?=\()", t)
        if hits:
            gx_names.update(hits)
            if p.startswith("src/"):
                gx_files_src[p] = len(hits)
    out.append(f"* {sum(gx_names.values()):,} GX/GD calls in total, {len(gx_names)} distinct functions.")
    out.append(f"* **{len(gx_files_src)} files under `src/`** call GX directly (everything else reaches the GPU through J3D/J2D/JUtility). Top of the list:\n")
    out += md_table(["File", "GX/GD calls"], [(f"`{p}`", n) for p, n in gx_files_src.most_common(25)])
    out.append("\nGX features by group (the fixed-function pipeline a shader layer must reproduce):\n")
    rows = []
    for name, rx in GX_GROUPS:
        n = sum(v for k, v in gx_names.items() if re.fullmatch(rx, k))
        rows.append((name, n))
    out += md_table(["Group", "Calls"], rows)

    # ---- PowerPC specifics
    out.append("\n## PowerPC-specific code\n")
    asm_files = collections.Counter()
    intr = collections.Counter()
    cache = collections.Counter()
    for p, t in all_files:
        n = len(re.findall(r"\basm\b|__asm|\bnofralloc\b", t))
        if n:
            asm_files[p] = n
        intr.update(re.findall(r"\b(__fabs|__frsqrte|__fres|__cntlzw|__abs|__lwbrx|__lhbrx|__stwbrx|__sthbrx|__lmw|__stmw|__fsel|__dcbz|__dcbf|__sync|__rlwimi|__psq_[ls]|__ps_\w+)\b", t))
        cache.update(re.findall(r"\b(DC[A-Z]\w*|IC[A-Z]\w*|LC[A-Z]\w*)(?=\()", t))
    out.append(f"* Inline assembly / `asm` functions: **{sum(asm_files.values())} occurrences in {len(asm_files)} files**:\n")
    out += md_table(["File", "Occurrences"], [(f"`{p}`", n) for p, n in asm_files.most_common(15)])
    out.append("\n* Compiler intrinsics: " + (", ".join(f"`{k}` ×{v}" for k, v in intr.most_common()) or "none"))
    out.append("* Data-cache / instruction-cache / locked-cache operations (`DC*`, `IC*`, `LC*`): "
               + (", ".join(f"`{k}` ×{v}" for k, v in cache.most_common()) or "none"))

    # ---- byte-order / ABI hazards
    out.append("\n## Byte-order and ABI hazards\n")
    bitfields = collections.Counter()
    multichar = collections.Counter()
    punning = collections.Counter()
    ptrint = collections.Counter()
    for p, t in all_files:
        n = len(re.findall(r"^\s*(?:u8|u16|u32|s8|s16|s32|int|unsigned(?: int)?|bool)\s+\w+\s*:\s*\d+\s*;", t, re.M))
        if n:
            bitfields[p] = n
        n = len(re.findall(r"'[A-Za-z0-9_]{2,8}'", t))
        if n:
            multichar[p] = n
        n = len(re.findall(r"\*\s*\(\s*(?:const\s+)?(?:u8|s8|u16|s16|char|unsigned char|short)\s*\*\s*\)\s*&", t))
        if n:
            punning[p] = n
        n = len(re.findall(r"\(\s*(?:u32|s32)\s*\)\s*(?:this|p[A-Z]\w*|i_\w*p\w*|\w*Ptr\w*|\w*_p\b)", t))
        if n:
            ptrint[p] = n
    out += md_table(["Hazard", "Occurrences", "Files", "Why it matters"], [
        ("Bit-fields (`u8 x : 3;`)", sum(bitfields.values()), len(bitfields),
         "Big-endian compilers allocate bits from the MSB, little-endian from the LSB. Bit-fields that overlay file/hardware data change meaning."),
        ("Multi-character constants (`'J3D2'`, `'n_all'`; approximate)", sum(multichar.values()), len(multichar),
         "Used as 32-bit tags. Compilers agree on the value (first char = most significant byte), which equals the big-endian file bytes only after the file word is byte-swapped."),
        ("Sub-word value punning (`*(u8*)&x`; plain byte-pointer arithmetic is not counted)", sum(punning.values()), len(punning),
         "Reads one byte/half of a wider value; picks the other end on a little-endian CPU."),
        ("Pointer cast to `u32`/`s32`", sum(ptrint.values()), len(ptrint),
         "Fine on 32-bit ARM; breaks on 64-bit hosts. Prefer `uintptr_t` (partly done upstream)."),
    ])
    out.append("\nTop files per hazard:\n")
    for name, c in (("bit-fields", bitfields), ("sub-word punning", punning), ("pointer-to-u32", ptrint), ("multi-char constants", multichar)):
        out.append(f"* {name}: " + ", ".join(f"`{p}` ({n})" for p, n in c.most_common(6)))
    static_asserts = sum(len(re.findall(r"STATIC_ASSERT\(", t)) for _, t in all_files)
    out.append(f"\n* `STATIC_ASSERT(sizeof(...) == 0x..)` layout checks: **{static_asserts}** (compiled in only for the MWCC GCN-USA build; see `include/global.h`).")
    out.append("* No byte-swap helper exists anywhere in the tree: every file format is read in native (big-endian) order.")

    # ---- parsers
    out.append("\n## Binary-format parser inventory\n")
    out.append("Files that interpret data authored for a big-endian machine. Line counts are per file (the total is the reading effort, not the rewrite effort).\n")
    rows = []
    for fmt, comp, fl in PARSERS:
        existing = [(f, loc(f)) for f in fl if os.path.exists(f)]
        total = sum(n for _, n in existing)
        rows.append((fmt, comp, ", ".join(f"`{os.path.basename(f)}`" for f, _ in existing) or "(not found)", f"{total:,}"))
    out += md_table(["Format", "Component", "Parser files", "Lines"], rows)

    Path("docs/port").mkdir(parents=True, exist_ok=True)
    Path("docs/port/survey-data.md").write_text("\n".join(out) + "\n")
    print("wrote docs/port/survey-data.md")


if __name__ == "__main__":
    main()
