#!/usr/bin/env python3
"""Survey every J3D material on the disc and count distinct GX shader programs.

Usage:
  python3 tools/gx_shaders/scan.py --disc <dir with extracted files/res> \
      [--out build/gx_shaders] [--report docs/port/shader-survey.md]

Extract the disc first (dtk decompresses Yaz0 on the way out):
  build/tools/dtk vfs cp orig/GZ2E01/GZ2E01.iso:files/res <dir>

Writes:
  <out>/draws.csv                one row per (archive, model, material, shape) draw variant
  <out>/pseudo/fs_<hash>.txt     pseudocode of every distinct fragment program (recommended level)
  <out>/pseudo/vs_<hash>.txt     pseudocode of every distinct vertex program
  <report>                       summary page with the counts and the shared patterns

Nothing under <out> is committed: it is derived from the user's disc.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import sys
from collections import Counter
from dataclasses import replace
from pathlib import Path

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parent))
import j3d  # noqa: E402
import jpa  # noqa: E402
import blo  # noqa: E402
import rarc  # noqa: E402
import shader  # noqa: E402
from shader import Opts  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]

# Cumulative canonicalisation levels: (name, description, Opts)
LEVELS = [
    ("raw", "material fields as stored (truncated to the used stage/texgen counts)", Opts(canonical=False)),
    ("canonical", "TEV run symbolically: simplified, dead code removed, registers/maps/coords renamed",
     Opts(konst_uniform=False, light_count="mask")),
    ("+konst", "KONST operand is a per-stage uniform (CPU applies the K selector)", Opts(light_count="mask")),
    ("+lightcount", "vertex lighting keyed by light count, not mask", Opts(light_count="count")),
    ("+maxlights", "fixed 8-light loop, masked lights zeroed", Opts(light_count="max")),
    ("+texmtx", "identity texgen matrices bound as uniforms", Opts(light_count="max", texmtx_uniform=True)),
    ("+atest", "alpha-compare functions as uniforms (key keeps 'can discard')",
     Opts(light_count="max", texmtx_uniform=True, atest_uniform=True)),
    ("+fog", "fog function as uniform (key keeps on/off, range adjust)",
     Opts(light_count="max", texmtx_uniform=True, atest_uniform=True, fog_uniform=True)),
    ("+biasscale", "per-stage bias/scale as uniforms",
     Opts(light_count="max", texmtx_uniform=True, atest_uniform=True, fog_uniform=True, bs_uniform=True)),
    ("+fogalways", "every program applies fog (\"off\" is a uniform)",
     Opts(light_count="max", texmtx_uniform=True, atest_uniform=True, fog_uniform=True, bs_uniform=True,
          fog_always=True)),
]
RECOMMENDED = "+fog"

# dRes_info_c::loaderBasicBmd: tags whose load path sets J3DMLF_UsePostTexMtx (0x20)
# (BMDE/BMDV: flags |= 0x20; BMWR/BMWE: flags ^= 0x60020; BMDP 0x59020030; BMDL 0x29020030)
POST_TEXMTX_TAGS = {"BMDE", "BMDV", "BMDP", "BMDL", "BMWR", "BMWE"}
WARP_TAGS = {"BMWR", "BMWE"}
# loaderBasicBmd masks channel 0's light mask by g_env_light.light_mask_type (set per stage/room)
BASIC_TAGS = {"BMDR", "BMDV", "BMDE", "BMWR", "BMWE"}
LIGHT_MASK_TYPES = {1: 0x04, 2: 0x0C, 3: 0x0D, 4: 0x0F, 5: 0x1F, 6: 0x3F, 7: 0x7F}
TEXTURE_REBINDS = ("fbtex_dummy", "dummy", "Zbuffer")  # d_resorce.cpp setIndirectTex


def h(s: str) -> str:
    return hashlib.sha1(s.encode()).hexdigest()[:10]


def walk_archives(disc: Path):
    for d, _, files in os.walk(disc):
        for f in sorted(files):
            if f.endswith(".arc"):
                p = Path(d) / f
                yield str(p.relative_to(disc)), p.read_bytes()


def files_in(arc_name: str, raw: bytes, exts):
    """(archive, member path, directory tag, data) for members with one of `exts`, recursing into nested archives."""
    for m in rarc.members(raw):
        if m.path.endswith(".arc"):
            yield from files_in(f"{arc_name}/{m.path}", m.data, exts)
        elif m.path.endswith(exts):
            yield arc_name, m.path, m.dir_tag, m.data


def material_variants(mat, tag: str, arc: str):
    """Yield (variant label, material) for load-time and run-time structural rewrites."""
    yield "", mat
    if tag in WARP_TAGS:
        w = shader.apply_warp(mat)
        if w is not None:
            yield "warp", w
    n = mat.name
    if arc.startswith("Stage/") and len(n) >= 7 and n[3:5] == "MA":
        code = n[3:7]
        if code in ("MA03", "MA09", "MA17", "MA19") and mat.fog is not None:
            yield "bgfog", replace(mat, fog=(6 if n[5:7] == "09" else 7, mat.fog[1]))
        if code in ("MA00", "MA01", "MA04", "MA16") and n[6] == "1":
            yield "bg_underwater", replace(mat, alpha_comp=(4, 0x80, 0, 3, 0xFF), zmode=(1, 3, 1))
            yield "bg_opa", replace(mat, alpha_comp=(7, 0, 1, 7, 0), zmode=(1, 3, 0))


class Survey:
    def __init__(self):
        self.rows = []
        self.programs = {}  # (kind, hash) -> text
        self.trees = {}
        self.runtime_vs = {l[0]: set() for l in LEVELS}  # vertex keys incl. light_mask_type variants
        self.errors = Counter()

    def emit(self, row, mat, shape, tag, light_masks=False):
        proj = shader.texgen_projective(mat)
        row.update(material=mat.name, mode=mat.mode, stages=len(mat.stages), texgens=len(mat.texgens),
                   blend=["none", "blend", "logic", "subtract"][mat.blend[0]] if mat.blend[0] < 4 else mat.blend[0],
                   ind=len(mat.ind_stages), fog=mat.fog[0] if mat.fog else 0,
                   rebind=",".join(sorted({n for n in mat.tex_names if n and n.startswith(TEXTURE_REBINDS)})))
        for lname, _desc, opts in LEVELS:
            f = shader.fragment(mat, opts, proj)
            v = shader.vertex(mat, shape, f, opts, post_texmtx=tag in POST_TEXMTX_TAGS)
            fk, vk = f.key(), "\n".join(v)
            fh, vh = h(fk), h(vk)
            row[f"fs:{lname}"] = fh
            row[f"vs:{lname}"] = vh
            self.runtime_vs[lname].add(vh)
            if light_masks:
                for lmask in LIGHT_MASK_TYPES.values():
                    ch = list(mat.chans)
                    ch[0] = ch[0][:2] + (ch[0][2] & lmask,) + ch[0][3:]
                    v2 = shader.vertex(replace(mat, chans=ch), shape, f, opts, post_texmtx=tag in POST_TEXMTX_TAGS)
                    self.runtime_vs[lname].add(h("\n".join(v2)))
            if lname == RECOMMENDED:
                self.trees[fh] = f.trees
                self.programs[("fs", fh)] = fk
                self.programs[("vs", vh)] = vk
                row.update(fs_ops=f.ops, fs_samples=f.samples, discard=f.discard, fs_bind=f.binding)
        self.rows.append(row)

    def j3d_models(self, disc: Path):
        for arc, raw in walk_archives(disc):
            try:
                entries = list(files_in(arc, raw, (".bmd", ".bdl", ".blo")))
            except Exception as e:  # noqa: BLE001
                self.errors[f"archive: {e}"] += 1
                continue
            for arc_name, path, tag, data in entries:
                if path.endswith(".blo"):
                    self.layout(arc_name, path, data)
                    continue
                try:
                    mdl = j3d.parse(data)
                except Exception as e:  # noqa: BLE001
                    self.errors[f"model: {e}"] += 1
                    continue
                shapes_of = {}
                for mi, si in mdl.draws:
                    shapes_of.setdefault(mi, []).append(si)
                for mi, mat0 in enumerate(mdl.materials):
                    for variant, mat in material_variants(mat0, tag, arc_name):
                        for si in shapes_of.get(mi, [None]):
                            shape = mdl.shapes[si] if si is not None and si < len(mdl.shapes) else None
                            self.emit(dict(source="J3D", archive=arc_name, model=path, tag=tag, variant=variant,
                                           shape=si), mat, shape, tag,
                                      light_masks=tag in BASIC_TAGS and variant == "")

    def layout(self, arc_name, path, data):
        try:
            mats = list(blo.materials(data))
        except Exception as e:  # noqa: BLE001
            self.errors[f"layout: {e}"] += 1
            return
        for mat, ind_enabled in mats:
            if ind_enabled:
                self.errors["layout: material with indirect stages (ind settings not decoded)"] += 1
            self.emit(dict(source="J2D", archive=arc_name, model=path, tag="BLO", variant="", shape=None),
                      mat, None, "BLO")

    def particles(self, disc: Path):
        for p in sorted(disc.rglob("*.jpc")):
            try:
                res = list(jpa.resources(p.read_bytes()))
            except Exception as e:  # noqa: BLE001
                self.errors[f"particle: {e}"] += 1
                continue
            for idx, base, child, _info in res:
                for mat in (base, child):
                    if mat is None:
                        continue
                    for variant, m in (("", mat), ("fog", replace(mat, fog=(2, 0)))):
                        self.emit(dict(source="JPA", archive=str(p.relative_to(disc)), model=f"res{idx:04}",
                                       tag="JPA", variant=variant, shape=None), m, None, "JPA")


def scan(disc: Path):
    sv = Survey()
    sv.j3d_models(disc)
    sv.particles(disc)
    return pd.DataFrame(sv.rows), sv.programs, sv.errors, sv.runtime_vs, sv.trees


def outputs(text: str):
    """(out.rgb expression, out.a expression, tail) of a fragment program, with let-bound
    temporaries substituted back so the expressions are self-contained."""
    import re
    env, rgb, a, tail = {}, "", "", []
    for line in text.splitlines():
        k, _, v = line.partition(" = ")
        v = re.sub(r"\be\d+\b", lambda m: f"({env[m.group(0)]})", v)
        if re.fullmatch(r"e\d+", k):
            env[k] = v
        elif k == "out.rgb" and not rgb:
            rgb = v
        elif k == "out.a":
            a = v
        elif not k.startswith("t"):
            tail.append("discard" if "discard" in line else "fog" if "fog" in line else line)
    return rgb, a, tuple(tail)


def shape_of(expr: str) -> str:
    """Expression with uniforms generalised: which *kind* of value sits where, not which one."""
    import re
    expr = re.sub(r"\bk([ca])\d+\b", r"k\1", expr)
    expr = re.sub(r"\bc\d+\.", "c.", expr)
    return expr


def write_programs(out: Path, df: pd.DataFrame, programs: dict):
    pdir = out / "pseudo"
    pdir.mkdir(parents=True, exist_ok=True)
    for old in pdir.glob("*.txt"):
        old.unlink()
    for kind in ("fs", "vs"):
        col = f"{kind}:{RECOMMENDED}"
        for hsh, grp in df.groupby(col):
            srcs = grp[["archive", "model", "material", "variant"]].drop_duplicates().head(8)
            hdr = [f"// {kind} {hsh}  level={RECOMMENDED}  draws={len(grp)}  "
                   f"materials={grp[['archive', 'model', 'material']].drop_duplicates().shape[0]}",
                   "// examples:"]
            hdr += [f"//   {r.archive} {r.model} {r.material} {r.variant}".rstrip() for r in srcs.itertuples()]
            (pdir / f"{kind}_{hsh}.txt").write_text("\n".join(hdr) + "\n\n" + programs[(kind, hsh)] + "\n")
    df.to_csv(out / "draws.csv", index=False)


def md_table(df: pd.DataFrame) -> str:
    cols = list(df.columns)
    out = ["| " + " | ".join(cols) + " |", "|" + "|".join("---:" if pd.api.types.is_numeric_dtype(df[c]) else "---" for c in cols) + "|"]
    for r in df.itertuples(index=False):
        out.append("| " + " | ".join(str(x) for x in r) + " |")
    return "\n".join(out)


def report(df: pd.DataFrame, programs: dict, errors: Counter, runtime_vs: dict, trees: dict) -> str:
    mats = df.drop_duplicates(["archive", "model", "material", "variant"])
    base = df[df.variant == ""]
    L = []
    L.append("# Shader survey (generated)")
    L.append("")
    L.append("*Generated by `tools/gx_shaders/scan.py` from the user's disc (GZ2E01); do not edit by hand.* "
             "Method and terms: [shader-analysis.md](shader-analysis.md).")
    L.append("")
    unit = {"J3D": "models", "J2D": "layouts", "JPA": "particle resources"}
    per = {k: f"{g[['archive', 'model']].drop_duplicates().shape[0]} {unit.get(k, 'files')} / "
              f"{g.drop_duplicates(['archive', 'model', 'material']).shape[0]} materials"
           for k, g in base.groupby("source")}
    L.append("Input: " + "; ".join(f"**{k}** {v}" for k, v in per.items()) +
             f". {len(base)} material/shape draws, +{len(df) - len(base)} variants from load-time and run-time rewrites "
             "(warp material, BG material-name rules, light masks, particle fog).")
    if errors:
        L.append("")
        L.append("Skipped: " + "; ".join(f"{k} ×{v}" for k, v in errors.most_common()))
    L.append("")
    L.append("## Distinct programs per canonicalisation level")
    L.append("")
    L.append("Each level includes the ones above it. *Pairs* = distinct (vertex, fragment) combinations, i.e. linked "
             "programs. *vertex_rt* also counts the vertex programs that appear when `light_mask_type` (set per "
             "stage/room, applied by `dRes_info_c::loaderBasicBmd`) masks channel 0's lights at load.")
    L.append("")
    t = []
    for lname, desc, _ in LEVELS:
        t.append(dict(level=f"`{lname}`", fragment=df[f"fs:{lname}"].nunique(), vertex=df[f"vs:{lname}"].nunique(),
                      vertex_rt=len(runtime_vs[lname]),
                      pairs=df[[f"fs:{lname}", f"vs:{lname}"]].drop_duplicates().shape[0], meaning=desc))
    L.append(md_table(pd.DataFrame(t)))
    L.append("")
    L.append(f"Pseudocode is emitted at level `{RECOMMENDED}`.")
    fcol, vcol = f"fs:{RECOMMENDED}", f"vs:{RECOMMENDED}"

    L.append("")
    L.append("## Coverage")
    L.append("")
    L.append("How many of the most common fragment programs cover a given share of materials (each material/variant counted once).")
    L.append("")
    counts = mats[fcol].value_counts()
    tot = counts.sum()
    cum = counts.cumsum() / tot
    cov = []
    for p in (0.5, 0.8, 0.9, 0.95, 0.99, 1.0):
        cov.append(dict(share=f"{p:.0%}", programs=int((cum < p - 1e-9).sum() + 1)))
    L.append(md_table(pd.DataFrame(cov)))
    L.append("")
    L.append(f"Programs used by exactly one material: {(counts == 1).sum()} of {len(counts)}.")

    L.append("")
    L.append("## Program shape")
    L.append("")
    L.append("Arithmetic operations (DAG nodes) × texture samples, per distinct fragment program:")
    L.append("")
    shp = mats.drop_duplicates(fcol).assign(ops=lambda d: pd.cut(d.fs_ops, [-1, 0, 1, 2, 4, 8, 16, 1000],
                                                                  labels=["0", "1", "2", "3-4", "5-8", "9-16", ">16"]))
    tab = shp.groupby(["ops", "fs_samples"], observed=False).size().unstack(fill_value=0)
    tab.columns = [f"{c} samples" for c in tab.columns]
    L.append(md_table(tab.reset_index()))
    L.append("")
    L.append(f"Programs that can `discard`: {mats.drop_duplicates(fcol).discard.sum()}. "
             f"Materials with indirect stages: {mats[mats.ind > 0].shape[0]}. "
             f"Materials whose textures are re-bound to the framebuffer/Z buffer at load: {mats[mats.rebind != ''].shape[0]}.")

    L.append("")
    L.append("## Shared patterns")
    L.append("")
    outs = {hsh: outputs(text) for (kind, hsh), text in programs.items() if kind == "fs"}
    prog_mats = mats[fcol].value_counts()
    rgbs, alphas, tails, shapes = Counter(), Counter(), Counter(), Counter()
    for hsh, (rgb, a, tail) in outs.items():
        n = prog_mats.get(hsh, 0)
        rgbs[rgb] += n
        alphas[a] += n
        tails[tail] += n
        shapes[(shape_of(rgb), shape_of(a), tail)] += 1
    L.append("### Colour and alpha are nearly independent")
    L.append("")
    L.append(f"The {len(outs)} programs combine {len(rgbs)} distinct `out.rgb` expressions, {len(alphas)} distinct `out.a` "
             f"expressions and {len(tails)} distinct tails (alpha test / fog). A generator that emits the colour and "
             f"alpha expressions independently needs only those {len(rgbs)} + {len(alphas)} expression templates.")
    L.append("")
    L.append("Most used `out.rgb` expressions (weighted by materials):")
    L.append("")
    L.append(md_table(pd.DataFrame([dict(materials=c, expression=f"`{e}`") for e, c in rgbs.most_common(20)])))
    L.append("")
    L.append("Most used `out.a` expressions:")
    L.append("")
    L.append(md_table(pd.DataFrame([dict(materials=c, expression=f"`{e}`") for e, c in alphas.most_common(12)])))
    L.append("")
    L.append("### Families: programs that differ only in their inputs")
    L.append("")
    L.append(f"Ignoring which uniform (`kc0` vs `kc1`, `c0` vs `c1`) and which texture map / uv set a program reads collapses the {len(outs)} programs into "
             f"{len(shapes)} families; programs in one family can share code with a different uniform binding table.")
    L.append("")
    L.append("### Shared sub-expressions")
    L.append("")
    L.append("Internal sub-expressions (at least 2 operations) inside the distinct fragment programs, with sample/"
             "vertex-colour/konst/register leaves generalised to `T`/`V`/`K`/`C`. These are the candidates for a "
             "shared function library in the generated shaders (and for hand-written fast paths). Ranked by "
             "programs × operations, i.e. code shared.")
    L.append("")
    sub_progs, sub_mats = Counter(), Counter()
    for hsh, tr in trees.items():
        found = set()
        for root in tr:
            if root:
                shader.subtrees(shader.general(root), found)
        for st in found:
            if shader.size(st) >= 2:
                sub_progs[st] += 1
                sub_mats[st] += prog_mats.get(hsh, 0)
    top = sorted(sub_progs, key=lambda st: (-sub_progs[st] * shader.size(st), -sub_mats[st]))[:20]
    L.append(md_table(pd.DataFrame([dict(programs=sub_progs[st], materials=sub_mats[st], ops=shader.size(st),
                                         expression=f"`{shader.show(st)}`") for st in top])))
    L.append("")
    L.append("### Most used programs")
    L.append("")
    top = counts.head(15)
    rows = []
    for hsh, c in top.items():
        body = programs[("fs", hsh)].splitlines()
        rows.append(dict(materials=c, id=f"`fs_{hsh}`", program="<br>".join(f"`{x}`" for x in body)))
    L.append(md_table(pd.DataFrame(rows)))

    L.append("")
    L.append("### Vertex programs")
    L.append("")
    vc = mats[vcol].value_counts()
    rows = []
    for hsh, c in vc.head(10).items():
        rows.append(dict(materials=c, id=f"`vs_{hsh}`", program="<br>".join(f"`{x}`" for x in programs[("vs", hsh)].splitlines())))
    L.append(md_table(pd.DataFrame(rows)))

    L.append("")
    L.append("## By source")
    L.append("")
    src = mats.assign(dir=mats.source + " " + mats.archive.str.split("/").str[0]).groupby("dir").agg(
        materials=("material", "size"), programs=(fcol, "nunique")).reset_index()
    key = mats.source + " " + mats.archive.str.split("/").str[0]
    src["only_here"] = [mats[(key == d) & ~mats[fcol].isin(mats[key != d][fcol])][fcol].nunique() for d in src.dir]
    L.append(md_table(src))
    L.append("")
    L.append("## Blend modes")
    L.append("")
    L.append("Blend is applied by GXM's shader patcher when a program is instantiated, so it is not in the key, "
             "except that GXM has no logic ops: `logic` draws need emulation (a program variant reading the "
             "framebuffer, or a different blend). Materials per blend type:")
    L.append("")
    L.append(md_table(mats.groupby(["source", "blend"]).size().reset_index(name="materials")))
    L.append("")
    L.append("## Variants from code rewrites")
    L.append("")
    var = df[df.variant != ""].groupby("variant").agg(draws=("material", "size"),
                                                     new_fs=(fcol, lambda s: (~s.isin(base[fcol])).pipe(lambda m: s[m].nunique()))).reset_index()
    L.append(md_table(var) if len(var) else "(none)")
    L.append("")
    return "\n".join(L)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--disc", type=Path, required=True)
    ap.add_argument("--out", type=Path, default=ROOT / "build" / "gx_shaders")
    ap.add_argument("--report", type=Path)
    a = ap.parse_args()
    df, programs, errors, runtime_vs, trees = scan(a.disc)
    a.out.mkdir(parents=True, exist_ok=True)
    write_programs(a.out, df, programs)
    text = report(df, programs, errors, runtime_vs, trees)
    if a.report:
        a.report.write_text(text)
    print(text)


if __name__ == "__main__":
    main()
