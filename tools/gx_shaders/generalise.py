"""How far the fragment-program set shrinks under generalisation, and where the programs come from.

Reads scan.py's outputs (build/gx_shaders/draws.csv and pseudo/fs_*.txt) and prints:

* origin: materials and programs in the 95 % head vs the tail, by source (J3D/J2D/JPA), and how
  many archives each program spans (engine-defined programs are broad, bespoke ones narrow);
* ladder: the program count after each generalisation step, cheapest first. Each step erases
  one kind of difference in the pseudocode text, so programs that then print the same could share
  one compiled program at the stated GPU cost.

Usage: python3 tools/gx_shaders/generalise.py [--out build/gx_shaders]
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

import pandas as pd

KEY = "fs:+fog"
STEPS = [  # name, GPU cost, regex substitutions on the pseudocode
    ("which register/konst slot", "free: binding table", [(r"\b(k[ca]|c)\d+\b", r"\1")]),
    ("numeric constants become uniforms", "free", [(r"(?<![\w.])\d+(\.\d+)?(?![\w])", "#")]),
    ("konst and register are one class", "free: both per-draw constants", [(r"\bk[ca]\b", "c")]),
    ("which texture map / uv set", "free: sampler binding, the VS routes uv", [(r"\bmap\d+", "map"), (r"\buv\d+", "uv")]),
    ("temporary names", "free", [(r"\b(t|e)\d+\b", r"\1")]),
    ("fog as a uniform switch", "~3 ALU when fog is off", [(r"\n?.*fog.*", "")]),
    ("vertex colour <-> uniform", "~1 ALU per input (mix by uniform)", [(r"\bv\d+\b", "c")]),
]


def coverage(counts: pd.Series, frac: float) -> int:
    c = counts.sort_values(ascending=False).cumsum() / counts.sum()
    return int((c < frac).sum()) + 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", type=Path, default=Path("build/gx_shaders"))
    out = ap.parse_args().out
    d = pd.read_csv(out / "draws.csv", dtype=str)

    # origin: base materials only (no load/run-time variants)
    m = d[d.variant.isna()].drop_duplicates(["source", "archive", "model", "material"])
    cnt = m[KEY].value_counts()
    head = set(cnt.index[: coverage(cnt, 0.95)])
    m = m.assign(in_head=m[KEY].isin(head))
    print(f"base materials {len(m)}, programs {len(cnt)}, {len(head)} cover 95 %\n")
    print("materials by source (head = the programs covering 95 %):")
    print(pd.crosstab(m.source, m.in_head, margins=True).rename(columns={True: "head", False: "tail"}), "\n")
    p = m.groupby(KEY).agg(archives=("archive", "nunique"), sources=("source", lambda s: ",".join(sorted(set(s)))))
    p["head"] = p.index.isin(head)
    print("programs by source:")
    print(pd.crosstab(p.sources, p["head"]).rename(columns={True: "head", False: "tail"}), "\n")
    print("archives per program:")
    print(p.groupby("head").archives.describe()[["count", "25%", "50%", "75%", "max"]], "\n")
    print(f"tail programs confined to one archive: {((~p['head']) & (p.archives == 1)).sum()} of {(~p['head']).sum()}")
    print("tail materials by archive group:", m[~m["in_head"]].archive.str.split("/").str[0].value_counts().to_dict(), "\n")

    # ladder: all materials and variants, as in the survey
    mats = d.drop_duplicates(["source", "archive", "model", "material", "variant"]).groupby(KEY).size()
    cur = {}
    for h in mats.index:
        lines = (out / f"pseudo/fs_{h}.txt").read_text().splitlines()
        cur[h] = "\n".join(l.split("//")[0].rstrip() for l in lines if l.strip() and not l.startswith("//"))

    def row(name, cost):
        n = pd.Series(cur).rename("k").to_frame().join(mats.rename("n")).groupby("k").n.sum()
        print(f"| {name} | {len(n)} | {coverage(n, .95)} | {coverage(n, .99)} | {cost} |")

    print("| step | programs | for 95 % | for 99 % | GPU cost |\n|---|---:|---:|---:|---|")
    row("recommended level", "-")
    for name, cost, subs in STEPS:
        for h in cur:
            for a, b in subs:
                cur[h] = re.sub(a, b, cur[h])
        row("+ " + name, cost)


if __name__ == "__main__":
    main()
