#!/usr/bin/env python3
"""Numeric check of the canonicaliser against an independent TEV reference.

(a) tree correctness: for every distinct material, the canonical expression trees
    (shader.build_trees) evaluate to the same out.rgb/out.a as a direct execution of the
    material's TEV stages, on random inputs.
(b) no false merges: materials that share a fragment key produce identical reference outputs
    when their inputs are bound through their binding tables (canonical name -> material slot).

The reference executes stages from the material fields (not from shader.py's trees) with GX
semantics as documented by Dolphin's TEV implementation: a/b/c operands use the low 8 bits,
d the full signed 11-bit value; result = (d +- lerp(a, b, c) + bias) * scale, clamped or
wrapped; compare ops: R8/GR16/BGR24 compare the colour A/B operands in *both* lanes (A8 compares
the alpha operands, RGB8 compares per component); the last stage's destination registers are
the output. Rounding of the 8-bit datapath is not modelled (both sides use real arithmetic).

Usage: python3 tools/gx_shaders/check_equiv.py --disc <extracted files/res> [--trials 4]
"""

from __future__ import annotations

import argparse
import random
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import scan  # noqa: E402
import shader  # noqa: E402

OPTS = {n: o for n, _d, o in scan.LEVELS}[scan.RECOMMENDED]


# --- small vector helpers (rgb = 3-list, alpha = float) -----------------------------------

def _bin(f, x, y):
    if isinstance(x, list) or isinstance(y, list):
        x = x if isinstance(x, list) else [x] * 3
        y = y if isinstance(y, list) else [y] * 3
        return [f(p, q) for p, q in zip(x, y)]
    return f(x, y)


def _un(f, x):
    return [f(p) for p in x] if isinstance(x, list) else f(x)


def u8(v):
    return ((v * 255.0) % 256.0) / 255.0


def wrap11(v):
    return (((v * 255.0) + 1024.0) % 2048.0 - 1024.0) / 255.0


def sat(v):
    return min(max(v, 0.0), 1.0)


def _cmp(name, a, b, c):
    def val(x, n):
        q = [round(p * 255) for p in (x if isinstance(x, list) else [x] * 3)]
        return {"r8": q[0], "gr16": q[1] * 256 + q[0], "bgr24": q[2] * 65536 + q[1] * 256 + q[0]}[n]
    kind, rel = name.rsplit("_", 1)
    op = (lambda p, q: p > q) if rel == "gt" else (lambda p, q: p == q)
    if kind in ("rgb8",):
        aa, bb = a, b
        cc = c if isinstance(c, list) else [c] * 3
        return [cc[i] if op(round(aa[i] * 255), round(bb[i] * 255)) else 0.0 for i in range(3)]
    if kind == "a8":
        return c if op(round(a * 255), round(b * 255)) else _un(lambda _: 0.0, c)
    return c if op(val(a, kind), val(b, kind)) else _un(lambda _: 0.0, c)


# --- reference TEV --------------------------------------------------------------------------

def sample_id(mat, stg, proj):
    if stg.tex_map == 0xFF or stg.tex_coord == 0xFF:
        return None
    return (stg.tex_map, stg.tex_coord, bool(proj.get(stg.tex_coord)), shader._ind_desc(mat, stg.ind))


def reference(mat, env, proj):
    regs = {r: [list(env["reg"][r][:3]), env["reg"][r][3]] for r in range(4)}
    last = None
    CMP = shader.CMP_OPS
    for s, stg in enumerate(mat.stages):
        sid = sample_id(mat, stg, proj)
        tex = env["tex"](sid) if sid is not None else [0.0] * 4
        ttb = mat.swap_tables[stg.tex_swap & 3]
        tex = [tex[ttb[0]], tex[ttb[1]], tex[ttb[2]], tex[ttb[3]]]
        ch = {0: 0, 2: 0, 4: 0, 1: 1, 3: 1, 5: 1}.get(stg.chan)
        if ch is not None:
            ras = env["ras"][ch]
        elif stg.chan in (6, 0xFF):
            ras = [0.0] * 4
        else:
            ras = env["bump"]
        rtb = mat.swap_tables[stg.ras_swap & 3]
        ras = [ras[rtb[0]], ras[rtb[1]], ras[rtb[2]], ras[rtb[3]]]
        kc, ka = env["k"](s)

        def cin(x):
            if x < 8:
                r = regs[x >> 1]
                return list(r[0]) if x % 2 == 0 else [r[1]] * 3
            return {8: tex[:3], 9: [tex[3]] * 3, 10: ras[:3], 11: [ras[3]] * 3, 12: [1.0] * 3, 13: [0.5] * 3,
                    14: list(kc), 15: [0.0] * 3}[x]

        def ain(x):
            if x < 4:
                return regs[x][1]
            return {4: tex[3], 5: ras[3], 6: ka, 7: 0.0}[x]

        col = [cin(x) for x in stg.color[:4]]
        alp = [ain(x) for x in stg.alpha[:4]]
        res = {}
        for lane, ops, cfg in (("c", col, stg.color), ("a", alp, stg.alpha)):
            a, b, c, d = ops
            a, b, c = _un(u8, a), _un(u8, b), _un(u8, c)
            op, bias, scale, clamp, dest = cfg[4:9]
            if op >= 8:
                name = CMP.get(op, f"cmp{op}")
                if lane == "a":
                    if op >= 14:
                        name = "a8_gt" if op == 14 else "a8_eq"
                    else:
                        a, b = _un(u8, col[0]), _un(u8, col[1])
                v = _bin(lambda p, q: p + q, d, _cmp(name, a, b, c))
            else:
                lerp = _bin(lambda p, q: p + q, _bin(lambda p, q: p * (1.0 - q), a, c), _bin(lambda p, q: p * q, b, c))
                v = _bin(lambda p, q: p + q, d, lerp) if op == 0 else _bin(lambda p, q: p - q, d, lerp)
                v = _un(lambda p: (p + shader.BIAS_VAL[bias & 3]) * shader.SCALE_VAL[scale & 3], v)
            res[(dest & 3, lane)] = _un(sat if clamp else wrap11, v)
        for (r, lane), v in res.items():
            if lane == "c":
                regs[r][0] = v
            else:
                regs[r][1] = v
        last = (stg.color[8] & 3, stg.alpha[8] & 3)
    if last is None:
        return None
    return regs[last[0]][0], regs[last[1]][1]


# --- tree evaluation ------------------------------------------------------------------------

def eval_tree(n, env):
    t = n[0]
    if t == "const":
        return n[1]
    if t == "tex":
        v = env["tex"](n[1])
        return _pick(v, n[2])
    if t == "ras":
        v = env["ras"][n[1]] if not isinstance(n[1], str) else env["bump"]
        return _pick(v, n[2])
    if t == "k":
        kc, ka = env["k"](n[1])
        return list(kc) if n[2] == "c" else ka
    if t == "regin":
        r = env["reg"][n[1]]
        return list(r[:3]) if n[2] == "rgb" else r[3]
    if t == "splat":
        v = eval_tree(n[1], env)
        return [v] * 3
    a = [eval_tree(c, env) if isinstance(c, tuple) and c and isinstance(c[0], str) and c[0] in shader._KINDS else c
         for c in n[1:]]
    if t == "mul":
        return _bin(lambda p, q: p * q, a[0], a[1])
    if t == "add":
        return _bin(lambda p, q: p + q, a[0], a[1])
    if t == "sub":
        return _bin(lambda p, q: p - q, a[0], a[1])
    if t == "neg":
        return _un(lambda p: -p, a[0])
    if t == "mix":
        return _bin(lambda p, q: p + q, _bin(lambda p, q: p * (1 - q), a[0], a[2]), _bin(lambda p, q: p * q, a[1], a[2]))
    if t == "bias":
        return _un(lambda p: p + a[1], a[0])
    if t == "scale":
        return _un(lambda p: p * a[1], a[0])
    if t in ("sat", "wrap11", "u8"):
        return _un({"sat": sat, "wrap11": wrap11, "u8": u8}[t], a[0])
    if t == "cmp":
        return _cmp(a[0], a[1], a[2], a[3])
    raise ValueError(t)


def _pick(v, comp):
    idx = ["rgba".index(ch) for ch in comp]
    return [v[i] for i in idx] if len(idx) == 3 else v[idx[0]]


# --- environments ----------------------------------------------------------------------------

def rand_env(rng: random.Random, keyfn=lambda x: x):
    """Random inputs. keyfn maps an original identity to the identity values are drawn by
    (identity for check (a); the canonical name for check (b))."""
    cache = {}
    seed = rng.random()
    big = rng.random() < 0.5  # half the trials use register inputs outside [0, 1] (exercises u8/wrap11)

    def draw(key, n, lo=0, hi=255):
        k = keyfn(key)
        if k not in cache:
            r = random.Random(repr((k, seed)))
            cache[k] = [r.randint(lo, hi) / 255.0 for _ in range(n)]
        return cache[k]

    return {
        "tex": lambda sid: draw(("tex", sid), 4),
        "ras": {0: draw(("ras", 0), 4), 1: draw(("ras", 1), 4)},
        "bump": draw(("bump",), 4),
        "k": lambda s: (draw(("kc", s), 3), draw(("ka", s), 1)[0]),
        "reg": {r: draw(("reg", r), 4, -1024 if big else 0, 1023 if big else 255) for r in range(4)},
    }


def close(x, y):
    if x is None or y is None:
        return x is y
    fx = (x[0] if isinstance(x[0], list) else [x[0]] * 3) + [x[1]]
    fy = (y[0] if isinstance(y[0], list) else [y[0]] * 3) + [y[1]]
    return all(abs(p - q) < 1e-6 for p, q in zip(fx, fy))


def binding_maps(ir):
    m = {}
    for item in ir.binding.split():
        k, v = item.split("=")
        m[k[:2] + v if k.startswith(("kc", "ka")) else v] = k
    return m


def canon_key(bind, ident):
    """Map an original identity to its canonical name via the binding table."""
    kind = ident[0]
    if kind == "tex":
        sid = ident[1]
        if sid is None:
            return ident
        mp, crd, proj, ind = sid
        cind = None
        if ind:
            fields, (icoord, imap, ss, st) = ind
            cind = (fields, (bind.get(f"texcoord{icoord}", icoord), bind.get(f"texmap{imap}", imap), ss, st))
        return ("tex", (bind.get(f"texmap{mp}", ("unbound", mp)), bind.get(f"texcoord{crd}", ("unbound", crd)), proj, cind))
    if kind == "ras":
        return ("ras", bind.get(f"chan{ident[1]}", ("unbound", ident[1])))
    if kind in ("kc", "ka"):
        return (kind, bind.get(f"{kind}stage{ident[1]}", ("unbound", ident[1])))
    if kind == "reg":
        return ("reg", bind.get(["prev", "reg0", "reg1", "reg2"][ident[1]], ("unbound", ident[1])))
    return ident


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--disc", type=Path, required=True)
    ap.add_argument("--trials", type=int, default=4)
    a = ap.parse_args()
    mats = {}
    orig_emit = scan.Survey.emit

    def collect(self, row, mat, shape, tag, light_masks=False):
        k = shader.fragment(mat, shader.Opts(canonical=False)).key()
        mats.setdefault(k, (mat, row["archive"], row.get("model"), mat.name))
    scan.Survey.emit = collect
    sv = scan.Survey()
    sv.j3d_models(a.disc)
    sv.particles(a.disc)
    scan.Survey.emit = orig_emit
    rng = random.Random(1)
    bad_tree, groups = [], defaultdict(list)
    for mat, arc, model, name in mats.values():
        proj = shader.texgen_projective(mat)
        rgb, al = shader.build_trees(mat, OPTS, proj)
        ir = shader.fragment(mat, OPTS, proj)
        groups[ir.key()].append((mat, proj, binding_maps(ir), (arc, model, name)))
        for _ in range(a.trials):
            env = rand_env(rng)
            ref = reference(mat, env, proj)
            got = None if rgb is None else (eval_tree(rgb, env), eval_tree(al, env))
            if not close(ref, got):
                bad_tree.append(((arc, model, name), ref, got))
                break
    bad_merge = []
    for key, members in groups.items():
        if len(members) < 2:
            continue
        for _ in range(a.trials):
            seed = rng.random()
            outs = []
            for mat, proj, bind, src in members:
                env = rand_env(random.Random(seed), keyfn=lambda ident, b=bind: canon_key(b, ident))
                outs.append((reference(mat, env, proj), src))
            base = outs[0][0]
            diff = [src for o, src in outs[1:] if not close(base, o)]
            if diff:
                bad_merge.append((key.splitlines()[:3], outs[0][1], diff[:3]))
                break
    print(f"distinct materials: {len(mats)}; fragment keys: {len(groups)}")
    print(f"(a) tree != reference: {len(bad_tree)}")
    for src, ref, got in bad_tree[:10]:
        print("   ", src, "\n      ref", ref, "\n      got", got)
    print(f"(b) false merges: {len(bad_merge)}")
    for key, first, diff in bad_merge[:10]:
        print("   ", key, first, diff)
    return 1 if bad_tree or bad_merge else 0


if __name__ == "__main__":
    sys.exit(main())
