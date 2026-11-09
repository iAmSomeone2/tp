#!/usr/bin/env python3
"""Static extraction of GX state set by *code* (not data), and the shader programs it implies.

For every function in the game/JSystem sources that calls GX, list the GX calls in source
order with their arguments resolved by clang (enumerators and constant expressions), then
replay each function through a GX state machine: at every draw call (GXBegin,
GXCallDisplayList) the TEV/texgen/channel/alpha-test state set so far becomes a synthetic
material that goes through the same canonicaliser as the disc materials (shader.py).

Callees that are themselves analysed (e.g. JPABaseShape::setGX called from
JPAResource::draw) are inlined, up to a small depth. State a function never sets is
*inherited*: it is filled from GXInit's defaults (libs/dolphin/src/gx/GXInit.c) so a program
can still be printed, and listed per draw site so it can be audited. Arguments clang cannot
fold to a constant make the site *unresolved* (typically loops over stages or values that come
from data); those sites need a hand audit or a data-driven model (as jpa.py does for JParticle).

Control flow is not modelled: calls are replayed in source order and calls inside a branch
or loop are flagged `cond`. This over-approximates (both arms of an if are applied).

Usage:
  python3 tools/gx_shaders/gxcalls.py [--out build/gx_shaders] [--report docs/port/shader-code-paths.md] [files...]
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import re
import subprocess
import sys
from collections import Counter, defaultdict
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import clang_c as cc  # noqa: E402
import j3d  # noqa: E402
import shader  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
SRC_DIRS = ("src/", "libs/JSystem/src/")
HEADER_DIRS = ("include/", "libs/JSystem/include/", "libs/dolphin/include/")  # inline helpers: inlined, never roots


def _main_checkout() -> Path:
    """The main checkout (generated assets/ live there when this is a git worktree)."""
    try:
        common = subprocess.run(["git", "rev-parse", "--git-common-dir"], cwd=ROOT, capture_output=True,
                                text=True).stdout.strip()
        return (ROOT / common).resolve().parent if common else ROOT
    except OSError:
        return ROOT
FLAGS = ["-x", "c++", "-std=c++20", "-fdeclspec", "-w", "-DVERSION=0", "-fno-short-enums", "-fsigned-char",
         "-I", "include", "-I", "src", "-I", "libs/JSystem/include", "-I", "libs/dolphin/include",
         "-I", "libs/dolphin/include/dolphin", "-I", "assets/GZ2E01",
         "-I", str(_main_checkout() / "assets" / "GZ2E01"),
         "-I", str(_main_checkout() / "build" / "GZ2E01" / "include"),  # generated assets/*.h (needs the disc)
         "-ferror-limit=0"]


def _resource_dir():
    """libclang does not find its builtin headers (stddef.h, ...) on its own; ask the driver."""
    try:
        d = subprocess.run(["clang", "-print-resource-dir"], capture_output=True, text=True).stdout.strip()
        return ["-resource-dir", d] if d else []
    except OSError:
        return []


FLAGS += _resource_dir()
STATE_CALLS = re.compile(r"\bGX(SetTev\w*|SetNumTevStages|SetNumTexGens|SetTexCoordGen2?|SetNumChans|SetChanCtrl|"
                         r"SetAlphaCompare|SetFog|SetNumIndStages|SetIndTex\w*|Begin|CallDisplayList)\s*\(")
DRAWS = {"GXBegin", "GXCallDisplayList"}
INLINE_DEPTH = 3
# Draw sites whose TEV state comes from data that scan.py already enumerates: J3D material display
# lists (J3DMaterial::load / J3DGD*), JParticle resource flags (jpa.py), J2D materials in .blo
# files (blo.py). Their "inherited" state is set by the data path, not missing.
DATA_DRIVEN = (("libs/JSystem/src/J3DGraphBase/", "J3D material display list (scan.py: J3D)"),
               ("libs/JSystem/src/JParticle/", "JParticle resource flags (scan.py: JPA)"),
               ("libs/JSystem/src/J2DGraph/J2DPictureEx.cpp", "J2D material (scan.py: J2D)"),
               ("libs/JSystem/src/J2DGraph/J2DWindowEx.cpp", "J2D material (scan.py: J2D)"),
               ("libs/JSystem/src/J2DGraph/J2DTextBoxEx.cpp", "J2D material (scan.py: J2D)"))


def data_source(path):
    for prefix, what in DATA_DRIVEN:
        if path.startswith(prefix):
            return what
    return ""
OPTS = shader.Opts(light_count="max", texmtx_uniform=True, atest_uniform=True, fog_uniform=True)  # = scan.py "+fog"


# --- extraction (runs in worker processes) ---------------------------------------------

def _arg(tu, c):
    """(value or None, enumerator name or None, source text)."""
    val = cc.evaluate_int(c)
    name, cur = None, c
    for _ in range(6):
        if cur.kind == cc.DECL_REF:
            ref = cc.referenced(cur)
            if ref is not None and ref.kind == cc.ENUM_CONST:
                name = cc.spelling(ref)
                if val is None:
                    val = cc.enum_value(ref)
            break
        ch = cc.children(cur)
        if len(ch) != 1:
            break
        cur = ch[0]
    return val, name, cc.tokens(tu, c)


def _walk_body(tu, node, depth, events):
    for ch in cc.children(node):
        k = ch.kind
        if k == cc.CALL:
            name = cc.spelling(ch)
            _, line = cc.location(ch)
            if name.startswith(("GX", "GD")):
                events.append(dict(kind="gx", name=name, line=line, cond=depth > 0,
                                   args=[_arg(tu, a) for a in cc.call_args(ch)]))
            else:
                ref = cc.referenced(ch)
                if ref is not None:
                    events.append(dict(kind="call", name=name, usr=cc.usr(ref), line=line, cond=depth > 0))
        _walk_body(tu, ch, depth + (1 if k in cc.BRANCH_KINDS else 0), events)


def _qualname(scope, c):
    return "::".join(scope + [cc.spelling(c)])


def _walk_decls(tu, node, scope, out):
    for ch in cc.children(node):
        path, _line = cc.location(ch)
        try:
            rel = str(Path(path).resolve().relative_to(ROOT))
        except ValueError:
            continue
        header = rel.startswith(HEADER_DIRS)
        if not (header or rel.startswith(SRC_DIRS)):
            continue
        if ch.kind in cc.SCOPE_KINDS:
            _walk_decls(tu, ch, scope + [cc.spelling(ch)], out)
        elif ch.kind in cc.FUNC_KINDS and cc.is_definition(ch):
            events = []
            _walk_body(tu, ch, 0, events)
            if events and (not header or any(e["kind"] == "gx" for e in events)):
                out.append(dict(file=rel, function=_qualname(scope, ch), usr=cc.usr(ch),
                                line=cc.location(ch)[1], events=events, header=header))


def extract(tu_path: str):
    tu, cur = cc.parse(str(ROOT / tu_path), FLAGS)
    out = []
    _walk_decls(tu, cur, [], out)
    return tu_path, out, cc.errors(tu)


# --- GX state machine --------------------------------------------------------------------

TEVOP_C = {0: [(15, 8, 10, 15), (15, 8, 0, 15)], 1: [(10, 8, 9, 15), (0, 8, 9, 15)],
           2: [(10, 12, 8, 15), (0, 12, 8, 15)], 3: [(15, 15, 15, 8)] * 2, 4: [(15, 15, 15, 10), (15, 15, 15, 0)]}
TEVOP_A = {0: [(7, 4, 5, 7), (7, 4, 0, 7)], 1: [(7, 7, 7, 5), (7, 7, 7, 0)], 2: [(7, 4, 5, 7), (7, 4, 0, 7)],
           3: [(7, 7, 7, 4)] * 2, 4: [(7, 7, 7, 5), (7, 7, 7, 0)]}


class GXState:
    """Tracks what a code path has set; everything else is 'inherited' (GXInit default assumed)."""

    def __init__(self):
        self.v = {}            # field -> value
        self.unresolved = set()

    def set(self, field, value, src):
        self.unresolved = {u for u in self.unresolved if not u.startswith(f"{field} <- ")}
        if value is None:
            self.unresolved.add(f"{field} <- {src}")
            self.v.pop(field, None)
        else:
            self.v[field] = value

    def get(self, field, default, inherited):
        if field in self.v:
            return self.v[field]
        inherited.add(field.split("[")[0])
        return default

    def apply(self, name, args):
        a = [x[0] for x in args]
        t = [x[2] for x in args]

        if name == "GXSetNumTevStages":
            self.set("nstages", a[0], f"{name}({t[0]})")
        elif name in ("GXSetTevOrder", "GXSetTevColorIn", "GXSetTevAlphaIn", "GXSetTevColorOp",
                      "GXSetTevAlphaOp", "GXSetTevSwapMode", "GXSetTevKColorSel", "GXSetTevKAlphaSel",
                      "GXSetTevDirect", "GXSetTevIndirect", "GXSetTevOp"):
            st = a[0]
            if st is None:
                self.unresolved.add(f"{name}[?] <- {name}({', '.join(t)})")
                return
            rest = tuple(a[1:])
            src = f"{name}({', '.join(t)})"
            if name == "GXSetTevOp":
                mode = a[1]
                if mode is None:
                    self.unresolved.add(f"tevop[{st}] <- {src}")
                    return
                self.set(f"cin[{st}]", TEVOP_C[mode][0 if st == 0 else 1], src)
                self.set(f"ain[{st}]", TEVOP_A[mode][0 if st == 0 else 1], src)
                self.set(f"cop[{st}]", (0, 0, 0, 1, 0), src)
                self.set(f"aop[{st}]", (0, 0, 0, 1, 0), src)
                return
            field = {"GXSetTevOrder": "order", "GXSetTevColorIn": "cin", "GXSetTevAlphaIn": "ain",
                     "GXSetTevColorOp": "cop", "GXSetTevAlphaOp": "aop", "GXSetTevSwapMode": "swap",
                     "GXSetTevKColorSel": "kc", "GXSetTevKAlphaSel": "ka", "GXSetTevDirect": "ind",
                     "GXSetTevIndirect": "ind"}[name]
            if name == "GXSetTevDirect":
                rest = (0,) * 9
            elif name == "GXSetTevIndirect":  # (ind_stage, fmt, bias, mtx, wrap_s, wrap_t, add_prev, utc_lod, alpha)
                rest = tuple(rest)
            if name == "GXSetTevOrder" and len(rest) == 3 and rest[1] is not None:
                rest = (rest[0], rest[1] & 0xFF if rest[1] != 0xFF else 0xFF, rest[2])
            self.set(f"{field}[{st}]", None if any(x is None for x in rest) else rest, src)
        elif name == "GXSetTevSwapModeTable":
            if a[0] is None:
                self.unresolved.add(f"swaptbl[?] <- {name}")
                return
            self.set(f"swaptbl[{a[0]}]", None if any(x is None for x in a[1:]) else tuple(a[1:]), name)
        elif name == "GXSetNumTexGens":
            self.set("ntexgens", a[0], f"{name}({t[0]})")
        elif name in ("GXSetTexCoordGen", "GXSetTexCoordGen2"):
            if a[0] is None:
                self.unresolved.add(f"texgen[?] <- {name}({', '.join(t)})")
                return
            self.set(f"texgen[{a[0]}]", None if any(x is None for x in a[1:4]) else tuple(a[1:4]), f"{name}({', '.join(t)})")
        elif name == "GXSetNumChans":
            self.set("nchans", a[0], f"{name}({t[0]})")
        elif name == "GXSetChanCtrl":  # (chan, enable, amb_src, mat_src, light_mask, diff_fn, attn_fn)
            ch = a[0]
            vals = a[1:]
            cfg = None if any(x is None for x in vals) else (vals[0], vals[2], vals[3], vals[4], vals[5], vals[1])
            src = f"{name}({', '.join(t)})"
            if ch is None:
                self.unresolved.add(f"chan[?] <- {src}")
                return
            for idx in {0: [0], 1: [2], 2: [1], 3: [3], 4: [0, 1], 5: [2, 3]}.get(ch, []):
                self.set(f"chan[{idx}]", cfg, src)
        elif name == "GXSetAlphaCompare":
            self.set("acomp", None if None in a else tuple(a), f"{name}({', '.join(t)})")
        elif name == "GXSetFog":
            self.set("fog", a[0], f"{name}({t[0]})")
        elif name == "GXSetNumIndStages":
            self.set("nind", a[0], f"{name}({t[0]})")
        elif name == "GXSetIndTexOrder":
            if a[0] is not None:
                self.set(f"indorder[{a[0]}]", None if None in a[1:3] else tuple(a[1:3]), name)
        elif name == "GXSetIndTexCoordScale":
            if a[0] is not None:
                self.set(f"indscale[{a[0]}]", None if None in a[1:3] else tuple(a[1:3]), name)

    def material(self):
        """Synthetic j3d.Material from the current state, plus the inherited fields it relies on.

        A field counts as inherited only if the program reads it: K selectors when KONST is an
        input, swap modes/tables when a stage reads its texture or raster colour, colour channels
        and texgens when a stage uses them, the indirect setup of every active stage.
        """
        inh = set()
        v = self.v

        def get(field, default, relevant=True):
            if field in v:
                return v[field]
            if relevant:
                inh.add(field.split("[")[0])
            return default

        n = get("nstages", 1) or 1
        stages = []
        for s in range(min(n, 16)):
            if s == 0 and "cin[0]" not in v and "ain[0]" not in v:
                cin, ain = TEVOP_C[3][0], TEVOP_A[3][0]  # GXInit: GXSetTevOp(STAGE0, GX_REPLACE)
                inh.update(("cin", "ain"))
            else:
                cin = get(f"cin[{s}]", (15, 15, 15, 0) if s else TEVOP_C[3][0])
                ain = get(f"ain[{s}]", (7, 7, 7, 0) if s else TEVOP_A[3][0])
            reads_tex = 8 in cin or 9 in cin or 4 in ain
            reads_ras = 10 in cin or 11 in cin or 5 in ain
            order = get(f"order[{s}]", (s, s, 4) if s < 8 else (0xFF, 0xFF, 0xFF), reads_tex or reads_ras)
            cop = get(f"cop[{s}]", (0, 0, 0, 1, 0))
            aop = get(f"aop[{s}]", (0, 0, 0, 1, 0))
            swap = get(f"swap[{s}]", (0, 0), reads_tex or reads_ras)
            ind = get(f"ind[{s}]", (0,) * 9, reads_tex)
            kc = get(f"kc[{s}]", 6, 14 in cin)
            ka = get(f"ka[{s}]", 0, 6 in ain)
            stages.append(j3d.Stage(order[0], order[1], order[2], tuple(cin) + tuple(cop), tuple(ain) + tuple(aop),
                                    swap[0], swap[1], kc, ka, tuple(ind)))
            stages[-1].reads = (reads_tex, reads_ras)
        dflt_swap = [(0, 1, 2, 3), (0, 0, 0, 3), (1, 1, 1, 3), (2, 2, 2, 3)]
        used_tbl = {st.tex_swap for st in stages if st.reads[0]} | {st.ras_swap for st in stages if st.reads[1]}
        swaps = [get(f"swaptbl[{i}]", dflt_swap[i], i in used_tbl) for i in range(4)]
        used_coords = {st.tex_coord for st in stages if st.reads[0] and st.tex_coord != 0xFF}
        ntg = get("ntexgens", 1, bool(used_coords)) or 0
        texgens = [get(f"texgen[{i}]", (1, 4 + i, 60), i in used_coords) for i in range(min(max(ntg, max(used_coords, default=-1) + 1), 8))]
        used_ch = set()
        for st in stages:
            if st.reads[1] and st.chan in (0, 2, 4):
                used_ch.update((0, 1))
            elif st.reads[1] and st.chan in (1, 3, 5):
                used_ch.update((2, 3))
        chans = [get(f"chan[{i}]", (0, 1, 0, 0, 2, 0), i in used_ch) for i in range(4)]  # GXInit: off, mat=VTX
        ind_used = any(any(st.ind) for st in stages)
        nind = get("nind", 0, ind_used) if ind_used else 0
        ind_stages = []
        for i in range(nind or 0):
            o = get(f"indorder[{i}]", (0, 0))
            sc = get(f"indscale[{i}]", (0, 0))
            ind_stages.append((o[0], o[1], sc[0], sc[1]))
        acomp = get("acomp", (7, 0, 0, 7, 0))
        fog = get("fog", None)
        for st in stages:
            del st.reads
        mat = j3d.Material("code", 0, v.get("nchans", 0), chans, texgens, [None] * 8, [0xFFFF] * 8, stages,
                           swaps, ind_stages, (fog, 0) if fog else None, tuple(acomp), (0, 0, 0, 0), (1, 3, 1), 0,
                           [f"texmap{i}" for i in range(8)], [None] * 8)
        return mat, inh


TEV_FIELDS = ("nstages", "cin[", "ain[", "order[", "cop[", "aop[")


def replay(fn, table, state, sites, depth=0, stack=(), root=None):
    """Replay `fn` into `state`; draw sites are attributed to the root of the replay, with the
    function that issues the draw kept as `at`."""
    root = root or fn
    for ev in fn["events"]:
        if ev["kind"] == "call":
            callee = table.get(ev["usr"])
            if callee and depth < INLINE_DEPTH and ev["usr"] not in stack:
                replay(callee, table, state, sites, depth + 1, stack + (fn["usr"],), root)
            continue
        if ev["name"] in DRAWS:
            mat, inh = state.material()
            set_tev = any(k.startswith(TEV_FIELDS) for k in state.v)
            sites.append(dict(file=root["file"], function=root["function"], line=ev["line"], draw=ev["name"],
                              at=f"{fn['file']}:{ev['line']} {fn['function']}", at_file=fn["file"],
                              set_tev=set_tev, cond=ev["cond"], inherited=sorted(inh),
                              unresolved=sorted(state.unresolved), mat=mat))
        else:
            state.apply(ev["name"], ev["args"])


# --- driver ---------------------------------------------------------------------------------

# Not part of the GameCube CMake build (see CLAUDE.md / docs/port/cmake.md)
EXCLUDE = {"src/d/d_home_button.cpp": "Wii only",
           "src/d/actor/d_a_movie_player.cpp": "removed from tp_actors; replaced by the nf_movie stub"}


def candidate_tus():
    out = subprocess.run(["grep", "-rlE", STATE_CALLS.pattern.replace("\\b", "\\b"), "src", "libs/JSystem/src",
                          "--include=*.cpp", "--include=*.inc"], cwd=ROOT, capture_output=True, text=True).stdout.split()
    tus = set()
    for f in out:
        if f.endswith(".inc"):
            inc = Path(f).name
            hits = subprocess.run(["grep", "-rl", f'#include ".*{inc}"', "src", "libs/JSystem/src", "--include=*.cpp"],
                                  cwd=ROOT, capture_output=True, text=True).stdout.split()
            tus.update(hits)
        else:
            tus.add(f)
    return sorted(tus - set(EXCLUDE))


def h(s):
    return hashlib.sha1(s.encode()).hexdigest()[:10]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("files", nargs="*")
    ap.add_argument("--out", type=Path, default=ROOT / "build" / "gx_shaders")
    ap.add_argument("--report", type=Path)
    ap.add_argument("--data-programs", type=Path, help="draws.csv from scan.py, to mark programs the data already covers")
    args = ap.parse_args()
    tus = args.files or candidate_tus()
    funcs, tu_errors = [], {}
    with ProcessPoolExecutor() as ex:
        for tu, out, errs in ex.map(extract, tus):
            funcs.extend(out)
            if errs:
                tu_errors[tu] = errs
    # header-inline functions are seen by many TUs: keep one copy each
    seen_usr, uniq = set(), []
    for f in funcs:
        if f["usr"] in seen_usr:
            continue
        seen_usr.add(f["usr"])
        uniq.append(f)
    funcs = uniq
    table = {}
    for f in funcs:
        table.setdefault(f["usr"], f)
    args.out.mkdir(parents=True, exist_ok=True)
    with open(args.out / "gx_calls.csv", "w", newline="") as fp:
        w = csv.writer(fp)
        w.writerow(["file", "function", "line", "call", "cond", "args"])
        for f in funcs:
            for ev in f["events"]:
                if ev["kind"] == "gx":
                    w.writerow([f["file"], f["function"], ev["line"], ev["name"], int(ev["cond"]),
                                "; ".join(x[1] or (str(x[0]) if x[0] is not None else x[2]) for x in ev["args"])])
    # replay every function that draws (directly or through inlined callees) from a fresh state
    sites = []
    callers = defaultdict(set)
    for f in table.values():
        for ev in f["events"]:
            if ev["kind"] == "call" and ev["usr"] in table:
                callers[ev["usr"]].add(f["usr"])
    # replay from roots only: a function that an analysed caller inlines is covered there, with
    # the caller's state, instead of a second time from a fresh state
    for f in table.values():
        if not callers.get(f["usr"]) and not f.get("header"):
            replay(f, table, GXState(), sites)
    roots = sites
    data_fs = set()
    if args.data_programs and args.data_programs.exists():
        import pandas as pd
        data_fs = set(pd.read_csv(args.data_programs, usecols=["fs:+fog"], low_memory=False)["fs:+fog"])
    pdir = args.out / "pseudo"
    pdir.mkdir(parents=True, exist_ok=True)
    rows = []
    progs = {}
    for s in roots:
        mat = s.pop("mat")
        f = shader.fragment(mat, OPTS, shader.texgen_projective(mat))
        fh = h(f.key())
        progs.setdefault(fh, (f.key(), []))[1].append(s)
        # data-driven: the draw is issued by a data-driven library *and* the path set no TEV state first
        rows.append(dict(s, fs=fh, in_data=fh in data_fs,
                         data="" if s["set_tev"] else data_source(s["at_file"])))
    for fh, (text, ss) in progs.items():
        hdr = [f"// fs {fh}  from code  sites={len(ss)}"]
        for s in ss[:10]:
            hdr.append(f"//   {s['file']}:{s['line']} {s['function']} {s['draw']}{' (conditional)' if s['cond'] else ''}")
            if s["inherited"]:
                hdr.append(f"//     inherited (GXInit default assumed): {', '.join(s['inherited'])}")
            if s["unresolved"]:
                hdr.append(f"//     unresolved: {'; '.join(s['unresolved'][:6])}")
        (pdir / f"fs_{fh}.txt").write_text("\n".join(hdr) + "\n\n" + text + "\n") if not (pdir / f"fs_{fh}.txt").exists() else None
    with open(args.out / "code_sites.csv", "w", newline="") as fp:
        w = csv.DictWriter(fp, fieldnames=["file", "function", "line", "draw", "at", "cond", "fs", "in_data", "data",
                                           "inherited", "unresolved"], extrasaction="ignore")
        w.writeheader()
        for r in rows:
            w.writerow({**r, "inherited": " ".join(r["inherited"]), "unresolved": " | ".join(r["unresolved"])})
    text = report(funcs, rows, progs, data_fs, tu_errors, tus)
    if args.report:
        args.report.write_text(text)
    print(text)


def grep_counts(tus):
    """Textual GX/GD call count per source file (the TUs and the .inc files they include)."""
    pat = re.compile(r"\bG[XD][A-Z]\w*\s*\(")
    files = set(tus)
    for t in tus:
        for line in (ROOT / t).read_text(errors="replace").splitlines():
            m = re.match(r'\s*#include\s+"(.+\.inc)"', line)
            if m:
                inc = (ROOT / t).parent / m.group(1)
                if not inc.exists():
                    inc = ROOT / "src" / m.group(1)
                if inc.exists():
                    files.add(str(inc.relative_to(ROOT)))
    out = {}
    for f in files:
        text = re.sub(r"//[^\n]*|/\*.*?\*/", "", (ROOT / f).read_text(errors="replace"), flags=re.S)
        out[f] = len(pat.findall(text))
    return out


def report(funcs, rows, progs, data_fs, tu_errors, tus):
    src_funcs = [f for f in funcs if not f.get("header")]
    n_calls = sum(1 for f in src_funcs for e in f["events"] if e["kind"] == "gx")
    gx_funcs = [f for f in src_funcs if any(e["kind"] == "gx" for e in f["events"])]
    files = sorted({f["file"] for f in gx_funcs})
    data_rows = [r for r in rows if r["data"]]
    rows = [r for r in rows if not r["data"]]
    L = ["# Shader code paths (generated)", "",
         "*Generated by `tools/gx_shaders/gxcalls.py` from the source tree (no disc data needed); do not edit by hand.* "
         "Method: [shader-analysis.md](shader-analysis.md#6-code-driven-configurations).", "",
         f"{n_calls} GX/GD calls in {len(gx_funcs)} functions across {len(files)} source files; "
         f"{len(rows) + len(data_rows)} draw sites (`GXBegin`/`GXCallDisplayList`, after inlining analysed callees). "
         f"{len(data_rows)} of them draw with state that comes from data already covered by `scan.py` "
         f"({', '.join(sorted({r['data'] for r in data_rows}))}); the tables below are about the other {len(rows)}, "
         "whose state is set by code.", ""]
    full = [r for r in rows if not r["unresolved"] and not r["inherited"]]
    inh_only = [r for r in rows if not r["unresolved"] and r["inherited"]]
    unres = [r for r in rows if r["unresolved"]]
    L += ["| draw sites | count |", "|---|---:|",
          f"| state fully set by the code path | {len(full)} |",
          f"| constant, but some state inherited (GXInit default assumed) | {len(inh_only)} |",
          f"| some state not constant (needs data model or hand audit) | {len(unres)} |",
          f"| distinct fragment programs (as printed) | {len({r['fs'] for r in rows})} |",
          f"| ... of which also produced by disc data | {len({r['fs'] for r in rows if r['fs'] in data_fs})} |", ""]
    inh = Counter(x for r in rows for x in r["inherited"])
    L += ["Most often inherited state (what draw sites rely on someone else having set):", "",
          "| field | sites |", "|---|---:|"] + [f"| `{k}` | {v} |" for k, v in inh.most_common(15)] + [""]
    by_file = defaultdict(list)
    for r in rows:
        by_file[r["file"]].append(r)
    L += ["## Per file", "", "| file | draw sites | resolved | programs | functions |", "|---|---:|---:|---:|---|"]
    for f, rs in sorted(by_file.items(), key=lambda kv: -len(kv[1])):
        fns = sorted({r["function"].split("::")[-1] for r in rs})
        L.append(f"| `{f}` | {len(rs)} | {sum(1 for r in rs if not r['unresolved'])} | {len({r['fs'] for r in rs})} | "
                 f"{', '.join(fns[:6])}{' …' if len(fns) > 6 else ''} |")
    why = Counter()
    for r in unres:
        for u in r["unresolved"]:
            why[re.sub(r"\(.*", "", u.split(" <- ")[-1])] += 1
    L += ["", "## Unresolved sites", "",
          "Calls whose arguments clang could not fold to a constant, by GX function (a site can have several). "
          "These are loops over stages/texgens and values passed in from data; each needs a hand audit or a data "
          "model like `jpa.py`. Full list: `build/gx_shaders/code_sites.csv`.", "",
          "| call | sites |", "|---|---:|"] + [f"| `{k}` | {v} |" for k, v in why.most_common(12)]
    # completeness: extracted calls vs a textual count, and parse errors
    got = Counter()
    for f in src_funcs:
        got[f["file"]] += sum(1 for e in f["events"] if e["kind"] == "gx")
    want = grep_counts(tus)
    L += ["", "## Completeness", "",
          f"Text search finds {sum(want.values())} `GX*(`/`GD*(` call sites in these {len(want)} files; clang extracted "
          f"{sum(got.values())} from function bodies. The difference is code the GameCube-USA (release) "
          "preprocessor pass drops, i.e. `#if DEBUG` debug drawing (most of the `m_Do_ext.cpp` and `d_map.cpp` "
          "gap) and other versions' arms, plus enum casts that look like calls to a text search "
          "(`GXTevColorArg(x)` in `J2DMatBlock.cpp`). Largest differences:", "",
          "| file | text | extracted |", "|---|---:|---:|"]
    diffs = sorted(want, key=lambda f: -(want[f] - got.get(f, 0)))[:12]
    L += [f"| `{f}` | {want[f]} | {got.get(f, 0)} |" for f in diffs]
    L += ["", f"Translation units with clang errors: {len(tu_errors)} of {len(tus)}."]
    for tu, errs in sorted(tu_errors.items()):
        L.append(f"- `{tu}`: {len(errs)} errors, first: `{errs[0][:120]}`")
    L += ["", "## Programs", "", "Fully resolved programs from code, most draw sites first:", ""]
    cnt = Counter(r["fs"] for r in rows if not r["unresolved"])
    for fh, c in cnt.most_common(25):
        text = progs[fh][0]
        L.append(f"- `fs_{fh}` ({c} sites{', also in data' if fh in data_fs else ''}): " +
                 " ; ".join(f"`{x}`" for x in text.splitlines()))
    L.append("")
    return "\n".join(L)


if __name__ == "__main__":
    main()
