"""GX material state -> canonical shader IR (the key) -> pseudocode.

The IR is the key: two materials share a program exactly when their IR (as a string) is
equal. Canonicalisation is done while building it:

* TEV stage expressions are simplified (lerp with c=0/1, zero operands, d=0, a==b), so
  differently-encoded but equivalent stages compare equal.
* Liveness from the output backwards drops stage halves (colour or alpha) and whole stages
  whose results are never read.
* TEV registers, texture maps, texture coordinates and vertex colour channels are renamed in
  order of first use. The run time keeps the mapping and binds uniforms/samplers accordingly.
* Unused texgens, colour channels, swizzles and K selectors disappear.

`Opts` switches further fields from "baked into the program" to "uniform"; the survey counts
programs under each so the cost/benefit of each choice is visible.

GX semantics follow the GX API docs and the J3D sources in this repo; see
docs/port/shader-analysis.md.
"""

from __future__ import annotations

from collections import Counter
from dataclasses import dataclass, field, replace

# --- GX enum names -----------------------------------------------------------------------

CMP_OPS = {8: "r8_gt", 9: "r8_eq", 10: "gr16_gt", 11: "gr16_eq", 12: "bgr24_gt", 13: "bgr24_eq",
           14: "rgb8_gt", 15: "rgb8_eq"}
COMPARE = ["never", "less", "equal", "lequal", "greater", "nequal", "gequal", "always"]
ALPHA_OP = ["and", "or", "xor", "xnor"]
FOG = {0: None, 2: "persp_lin", 4: "persp_exp", 5: "persp_exp2", 6: "persp_revexp", 7: "persp_revexp2",
       10: "ortho_lin", 12: "ortho_exp", 13: "ortho_exp2", 14: "ortho_revexp", 15: "ortho_revexp2"}
TG_TYPE = {0: "mtx3x4", 1: "mtx2x4", 10: "srtg", **{2 + i: f"bump{i}" for i in range(8)}}
TG_SRC = (["pos", "nrm", "binrm", "tangent"] + [f"tex{i}" for i in range(8)]
          + [f"texcoord{i}" for i in range(7)] + ["color0", "color1"])
DIFF = ["none", "sign", "clamp"]
ATTN = ["spec", "spot", "none"]
COMP = "rgba"

# GXAttr
VA_PNMTXIDX, VA_CLR0, VA_TEX0 = 0, 11, 13


@dataclass(frozen=True)
class Opts:
    """Which material fields become uniforms instead of program structure."""
    canonical: bool = True       # simplify/liveness/rename (False = raw field comparison)
    konst_uniform: bool = True   # KONST operand = per-stage uniform (CPU applies K selector)
    atest_uniform: bool = False  # alpha-compare functions as uniforms (key keeps "has discard")
    fog_uniform: bool = False    # fog function as uniform (key keeps on/off + range adjust)
    bs_uniform: bool = False     # per-stage bias/scale as uniforms (key keeps clamp)
    light_count: str = "count"   # "mask" (exact mask), "count" (popcount), "max" (fixed loop of 8)
    texmtx_uniform: bool = False # identity texgen matrices bound as uniforms (merge with "mtx")
    fog_always: bool = False     # every program applies fog; "no fog" is a uniform setting


# --- TEV as expression trees -----------------------------------------------------------
#
# The stages are executed symbolically: every TEV register lane holds an expression tree,
# starting as an uniform input ("regin"). After the last stage, out.rgb/out.a are trees whose
# leaves are samples, vertex colours, constants and uniforms. Register allocation, dead stages
# and pass-through copies disappear by construction; smart constructors fold identities, order
# commutative operands, and drop clamps/wraps that interval analysis proves are no-ops.
#
# Nodes are tuples: (kind, ...). Colour-lane nodes are vec3, alpha-lane nodes scalars;
# ("splat", x) replicates a scalar into a colour operand.

S10 = (-1024 / 255, 1023 / 255)  # TEV register range (11-bit signed)
ZERO, ONE, HALF = ("const", 0.0), ("const", 1.0), ("const", 0.5)
KSEL_VAL = [1.0, 7 / 8, 6 / 8, 5 / 8, 4 / 8, 3 / 8, 2 / 8, 1 / 8]
BIAS_VAL = [0.0, 0.5, -0.5, 0.0]
SCALE_VAL = [1.0, 2.0, 4.0, 0.5]
_RANK = {"const": 0, "regin": 1, "k": 2, "K": 3, "ras": 4, "tex": 5}


def rng(n):
    t = n[0]
    if t == "const":
        return (n[1], n[1])
    if t in ("tex", "ras", "k", "K", "u8"):
        return (0.0, 1.0)
    if t == "regin":
        return S10
    if t == "splat":
        return rng(n[1])
    if t in ("mul", "add", "sub"):
        (a0, a1), (b0, b1) = rng(n[1]), rng(n[2])
        if t == "add":
            return (a0 + b0, a1 + b1)
        if t == "sub":
            return (a0 - b1, a1 - b0)
        p = [a0 * b0, a0 * b1, a1 * b0, a1 * b1]
        return (min(p), max(p))
    if t == "neg":
        a0, a1 = rng(n[1])
        return (-a1, -a0)
    if t == "mix":
        (a0, a1), (b0, b1) = rng(n[1]), rng(n[2])
        return (min(a0, b0), max(a1, b1))
    if t == "bias":
        a0, a1 = rng(n[1])
        return (a0 + n[2], a1 + n[2])
    if t == "scale":
        a0, a1 = rng(n[1])
        return (a0 * n[2], a1 * n[2])
    if t == "bs":  # uniform bias in [-0.5, 0.5], scale in [0.5, 4]
        a0, a1 = rng(n[1])
        return (min((a0 - 0.5) * 4, (a0 - 0.5) * 0.5), max((a1 + 0.5) * 4, (a1 + 0.5) * 0.5))
    if t == "sat":
        a0, a1 = rng(n[1])
        return (min(max(a0, 0.0), 1.0), min(max(a1, 0.0), 1.0))
    if t == "wrap11":
        return rng(n[1]) if _within(rng(n[1]), S10) else S10
    if t == "cmp":
        (d0, d1), (c0, c1) = rng(n[5]), rng(n[4])
        return (min(d0, d0 + c0), max(d1, d1 + c1))
    raise ValueError(t)


def _within(r, lim):
    return r[0] >= lim[0] - 1e-9 and r[1] <= lim[1] + 1e-9


def _sortkey(n):
    return (_RANK.get(n[0], 9), repr(n))


def is_const(n, v=None):
    return n[0] == "const" and (v is None or abs(n[1] - v) < 1e-9)


def mul(a, b):
    if is_const(a, 0) or is_const(b, 0):
        return ZERO
    if is_const(a, 1):
        return b
    if is_const(b, 1):
        return a
    if is_const(a) and is_const(b):
        return ("const", a[1] * b[1])
    a, b = sorted((a, b), key=_sortkey)
    return ("mul", a, b)


def add(a, b):
    if is_const(a, 0):
        return b
    if is_const(b, 0):
        return a
    if is_const(a) and is_const(b):
        return ("const", a[1] + b[1])
    a, b = sorted((a, b), key=_sortkey)
    return ("add", a, b)


def sub(a, b):
    if is_const(b, 0):
        return a
    if a == b:
        return ZERO
    if is_const(a, 0):
        return ("neg", b)
    return ("sub", a, b)


def mix(a, b, c):
    if is_const(c, 0) or a == b:
        return a
    if is_const(c, 1):
        return b
    if is_const(a, 0):
        return mul(b, c)
    if is_const(b, 0):
        return mul(a, sub(ONE, c))
    return ("mix", a, b, c)


def u8(x):  # TEV a/b/c operands use the low 8 bits of a register
    return x if _within(rng(x), (0.0, 1.0)) else ("u8", x)


def sat(x):
    return x if _within(rng(x), (0.0, 1.0)) else ("sat", x)


def wrap11(x):
    return x if _within(rng(x), S10) else ("wrap11", x)


def splat(x):
    return x if x[0] == "const" else ("splat", x)


@dataclass
class FragIR:
    lines: list = field(default_factory=list)       # pseudocode (this is the key)
    tex_maps: list = field(default_factory=list)    # canonical sampler slot -> original texmap
    coords: list = field(default_factory=list)      # canonical uv slot -> original texcoord
    chans: list = field(default_factory=list)       # canonical varying -> original channel (0/1)
    chan_parts: dict = field(default_factory=dict)  # original channel -> {"rgb", "a"} read
    discard: bool = False
    tex_names: list = field(default_factory=list)
    ops: int = 0                                     # arithmetic nodes in the DAG
    samples: int = 0
    trees: tuple = ()                                # (out.rgb, out.a) expression trees, not part of the key
    binding: str = ""                                # canonical name -> material slot, for the run time


    def key(self) -> str:
        return "\n".join(self.lines)


def _ind_desc(mat, ind):
    """Indirect lookup for a stage's indirect settings, or None for a direct stage."""
    if not any(ind):
        return None
    st = ind[0] & 3
    look = mat.ind_stages[st] if st < len(mat.ind_stages) else (0xFF, 0xFF, 0, 0)
    return (ind[1:], look)  # (fmt, bias, mtx, wrapS, wrapT, add_prev, lod, alphaSel), (coord, map, sS, sT)


def build_trees(mat, opts: Opts, proj: dict):
    state = {(r, lane): ("regin", r, lane) for r in range(4) for lane in ("rgb", "a")}
    last = None
    for s, stg in enumerate(mat.stages):
        ind = _ind_desc(mat, stg.ind)
        if stg.tex_map == 0xFF or stg.tex_coord == 0xFF:
            sample = None
        else:
            sample = (stg.tex_map, stg.tex_coord, bool(proj.get(stg.tex_coord)), ind)
        tex_tbl = mat.swap_tables[stg.tex_swap & 3]
        ras_tbl = mat.swap_tables[stg.ras_swap & 3]
        ch = {0: 0, 2: 0, 4: 0, 1: 1, 3: 1, 5: 1}.get(stg.chan)

        def tex(comp):
            return ZERO if sample is None else ("tex", sample, comp)

        def ras(comp):
            if ch is None:
                return ZERO if stg.chan in (6, 0xFF) else ("ras", f"bump{stg.chan}", comp)
            return ("ras", ch, comp)

        def konst(half):
            if opts.konst_uniform:
                return ("k", s, half)
            sel = stg.kc_sel if half == "c" else stg.ka_sel
            if sel < 8:
                return ("const", KSEL_VAL[sel])
            if half == "c" and sel < 0x10:
                return ("K", sel - 0x0C, "rgb")
            return ("K", (sel - 0x10) & 3, COMP[(sel - 0x10) >> 2])

        def kc(x):
            if x < 8:
                r, lane = x >> 1, ("rgb" if x % 2 == 0 else "a")
                v = state[(r, lane)]
                return v if lane == "rgb" else splat(v)
            return {8: lambda: tex("".join(COMP[i] for i in tex_tbl[:3])),
                    9: lambda: splat(tex(COMP[tex_tbl[3]])),
                    10: lambda: ras("".join(COMP[i] for i in ras_tbl[:3])),
                    11: lambda: splat(ras(COMP[ras_tbl[3]])),
                    12: lambda: ONE, 13: lambda: HALF, 14: lambda: konst("c"), 15: lambda: ZERO}[x]()

        def ka(x):
            if x < 4:
                return state[(x, "a")]
            return {4: lambda: tex(COMP[tex_tbl[3]]), 5: lambda: ras(COMP[ras_tbl[3]]),
                    6: lambda: konst("a"), 7: lambda: ZERO}[x]()

        out = {}
        col_ab = [u8(kc(x)) for x in stg.color[:2]]
        for half, cfg, getter, lane in (("c", stg.color, kc, "rgb"), ("a", stg.alpha, ka, "a")):
            a, b, c, d = (getter(x) for x in cfg[:4])
            op, bias, scale, clamp, dest = cfg[4:9]
            a, b, c = u8(a), u8(b), u8(c)
            if op >= 8:
                name = CMP_OPS.get(op, f"cmp{op}")
                if lane == "a" and op >= 14:
                    name = "a8_gt" if op == 14 else "a8_eq"
                elif lane == "a":
                    # R8/GR16/BGR24 compares in the alpha lane read the stage's *colour* A/B operands
                    # (only the A8 compare reads the alpha operands); C and D stay alpha.
                    a, b = col_ab
                e = d if is_const(c, 0) else add(d, ("cmp", name, a, b, c, ZERO))
            else:
                m = mix(a, b, c)
                e = add(d, m) if op == 0 else sub(d, m)
                if opts.bs_uniform:
                    if (bias, scale) != (0, 0):
                        e = ("bs", e)
                else:
                    if BIAS_VAL[bias & 3]:
                        e = ("bias", e, BIAS_VAL[bias & 3])
                    if SCALE_VAL[scale & 3] != 1.0:
                        e = ("scale", e, SCALE_VAL[scale & 3])
            out[(dest & 3, lane)] = sat(e) if clamp else wrap11(e)
        state.update(out)
        last = (stg.color[8] & 3, stg.alpha[8] & 3)
    if last is None:
        return None, None
    return state[(last[0], "rgb")], state[(last[1], "a")]


class _Printer:
    def __init__(self, mat, ir: FragIR):
        self.mat, self.ir = mat, ir
        self.names, self.lets, self.uses = {}, [], Counter()
        self.maps, self.coords, self.samples, self.kn, self.regs = {}, {}, {}, {}, {}

    def count(self, n):
        self.uses[n] += 1
        if self.uses[n] == 1 and n[0] not in ("const", "regin", "k", "K", "ras", "tex"):
            for c in n[1:]:
                if isinstance(c, tuple) and c and isinstance(c[0], str) and c[0] in _KINDS:
                    self.count(c)

    def sample_name(self, smp):
        if smp not in self.samples:
            m, c, proj, ind = smp
            uv = self.coord(c)
            uve = f"{uv}.xy / {uv}.z" if proj else f"{uv}.xy"
            if ind:
                (fmt, bias, mtx, ws, wt, prev, lod, asel), (icoord, imap, ss, st) = ind
                itex = f"sample({self.tmap(imap)}, {self.coord(icoord)}.xy * scale({ss},{st}))" if imap != 0xFF else "0"
                uve = (f"indirect({uve}, {itex}, fmt={fmt}, bias={bias}, mtx={mtx}, wrap=({ws},{wt}), "
                       f"add_prev={prev}, lod={lod}, alpha={asel})")
            name = f"t{len(self.samples)}"
            self.samples[smp] = name
            self.lets.append(f"{name} = sample({self.tmap(m)}, {uve})")
        return self.samples[smp]

    def tmap(self, m):
        if m not in self.maps:
            self.maps[m] = len(self.maps)
            self.ir.tex_maps.append(m)
            self.ir.tex_names.append(self.mat.tex_names[m] if m < len(self.mat.tex_names) else None)
        return f"map{self.maps[m]}"

    def coord(self, c):
        if c not in self.coords:
            self.coords[c] = len(self.coords)
            self.ir.coords.append(c)
        return f"uv{self.coords[c]}"

    def chan(self, ch, comp):
        self.ir.chan_parts.setdefault(ch, set()).add("a" if comp in ("a",) or len(set(comp)) == 1 and comp[0] == "a" else "rgb")
        if ch not in self.ir.chans:
            self.ir.chans.append(ch)
        return f"v{self.ir.chans.index(ch)}"

    def expr(self, n, top=False):
        if n in self.names:
            return self.names[n]
        t = n[0]
        if t == "const":
            v = n[1]
            s = ("%g" % v)
        elif t == "regin":
            if n[1] not in self.regs:
                self.regs[n[1]] = len(self.regs)
            s = f"c{self.regs[n[1]]}.{n[2]}"
        elif t == "k":
            key = (n[1], n[2])
            if key not in self.kn:
                self.kn[key] = sum(1 for k in self.kn if k[1] == n[2])
            s = f"k{n[2]}{self.kn[key]}"
        elif t == "K":
            s = f"K{n[1]}.{n[2]}"
        elif t == "tex":
            nm = self.sample_name(n[1])
            s = nm if n[2] == "rgb" else f"{nm}.{n[2]}"
        elif t == "ras":
            if isinstance(n[1], str):
                s = n[1]
            else:
                nm = self.chan(n[1], n[2])
                s = nm if n[2] == "rgb" else f"{nm}.{n[2]}"
        elif t == "splat":
            inner = self.expr(n[1])
            s = f"{inner}{''.join(inner[-1] for _ in range(2))}" if inner[-2:-1] == "." else f"vec3({inner})"
        elif t == "mul":
            s = f"{self.sub(n[1])} * {self.sub(n[2])}"
        elif t == "add":
            s = f"{self.expr(n[1])} + {self.sub(n[2])}"
        elif t == "sub":
            s = f"{self.expr(n[1])} - {self.sub(n[2])}"
        elif t == "neg":
            s = f"-{self.sub(n[1])}"
        elif t == "mix":
            s = f"mix({self.expr(n[1])}, {self.expr(n[2])}, {self.expr(n[3])})"
        elif t == "bias":
            s = f"{self.sub(n[1])} {'+' if n[2] > 0 else '-'} {abs(n[2]):g}"
        elif t == "scale":
            s = f"{self.sub(n[1])} * {n[2]:g}"
        elif t == "bs":
            s = f"({self.expr(n[1])} + bias) * scale"
        elif t in ("sat", "wrap11", "u8"):
            s = f"{t}({self.expr(n[1])})"
        elif t == "cmp":
            s = f"({n[1]}({self.expr(n[2])}, {self.expr(n[3])}) ? {self.expr(n[4])} : 0)"
        else:
            raise ValueError(t)
        trivial = t == "splat" and n[1][0] in ("const", "regin", "k", "K", "tex", "ras")
        if t not in ("const", "regin", "k", "K", "tex", "ras") and not trivial and self.uses[n] > 1 and not top:
            name = f"e{sum(1 for v in self.names.values() if v.startswith('e'))}"
            self.lets.append(f"{name} = {s}")
            self.names[n] = name
            self.ir.ops += 1
            return name
        if t not in ("const", "regin", "k", "K", "tex", "ras", "splat"):
            self.ir.ops += 1
        return s

    def sub(self, n):
        s = self.expr(n)
        return f"({s})" if " " in s and not s.startswith(("mix(", "sat(", "wrap11(", "u8(")) or s.startswith("-") else s


_KINDS = {"const", "regin", "k", "K", "ras", "tex", "splat", "mul", "add", "sub", "neg", "mix",
          "bias", "scale", "bs", "sat", "wrap11", "u8", "cmp"}


def fragment(mat, opts: Opts, proj=None) -> FragIR:
    """Canonical fragment IR (pseudocode) for a J3D material (j3d.Material)."""
    if not opts.canonical:
        return _raw_fragment(mat)
    ir = FragIR()
    rgb, a = build_trees(mat, opts, proj or {})
    if rgb is None:
        ir.lines = ["out = undefined  // no TEV stages"]
        return ir
    p = _Printer(mat, ir)
    p.count(rgb)
    p.count(a)
    srgb = p.expr(rgb, top=True)
    sa = p.expr(a, top=True)
    body = p.lets + [f"out.rgb = {srgb}", f"out.a = {sa}"]
    c0, _r0, aop, c1, _r1 = mat.alpha_comp

    def passes(x, y):
        return [x and y, x or y, x != y, x == y][aop & 3]
    outcomes = {passes(x, y) for x in ((False, True) if c0 not in (0, 7) else (c0 == 7,))
                for y in ((False, True) if c1 not in (0, 7) else (c1 == 7,))}
    if outcomes != {True}:
        ir.discard = True
        if outcomes == {False}:
            body.append("discard")
        elif opts.atest_uniform:
            body.append("if (!alpha_test(out.a)) discard  // functions + refs are uniforms")
        else:
            body.append(f"if (!({COMPARE[c0]}(out.a, aref0) {ALPHA_OP[aop & 3]} {COMPARE[c1]}(out.a, aref1))) discard")
    if mat.fog and FOG.get(mat.fog[0]):
        adj = ", range_adjust" if mat.fog[1] else ""
        fn = "fog_fn" if opts.fog_uniform else FOG[mat.fog[0]]
        body.append(f"out.rgb = mix(out.rgb, fog_color, {fn}(depth{adj}))")
    elif opts.fog_always:
        body.append("out.rgb = mix(out.rgb, fog_color, fog_fn(depth))")
    ir.samples = len(p.samples)
    ir.trees = (rgb, a)
    regn = ["prev", "reg0", "reg1", "reg2"]
    bind = [f"c{v}={regn[k]}" for k, v in p.regs.items()]
    bind += [f"k{half}{v}=stage{st}" for (st, half), v in p.kn.items()]
    bind += [f"map{v}=texmap{k}" for k, v in p.maps.items()]
    bind += [f"uv{v}=texcoord{k}" for k, v in p.coords.items()]
    bind += [f"v{i}=chan{ch}" for i, ch in enumerate(ir.chans)]
    ir.binding = " ".join(bind)
    ir.lines = body
    return ir


def _raw_fragment(mat) -> FragIR:
    """No canonicalisation: compare the material fields as stored (after truncation)."""
    ir = FragIR()
    for s in mat.stages:
        ir.lines.append(repr((s.tex_coord, s.tex_map, s.chan, s.color, s.alpha, s.ras_swap,
                              s.tex_swap, s.kc_sel, s.ka_sel, s.ind)))
    ir.lines.append(repr((mat.swap_tables, mat.ind_stages, mat.alpha_comp, mat.fog)))
    return ir


# --- vertex IR ---------------------------------------------------------------------------

def vertex(mat, shape, frag: FragIR, opts: Opts, post_texmtx=False) -> list:
    """Canonical vertex IR lines for one (material, shape) draw, given its fragment IR."""
    attrs = shape.attrs if shape else {}
    lines = []
    skinned = VA_PNMTXIDX in attrs
    lines.append("pos = " + ("skin(pnmtx[idx], pos)" if skinned else "mtx * pos"))
    need_nrm = False
    if not opts.canonical:
        lines.append(repr((mat.chan_num, mat.chans, mat.texgens, sorted(attrs.items()), shape.mtx_type if shape else None)))
        return lines
    for out_idx, ch in enumerate(frag.chans):
        for part in sorted(frag.chan_parts.get(ch, ())):
            cfg = mat.chans[ch * 2 + (1 if part == "a" else 0)]
            en, ms, lm, df, af, asrc = cfg
            mat_src = "vtx_clr" + str(ch) if ms else "matcol"
            if ms and (VA_CLR0 + ch) not in attrs:
                mat_src += "(missing->default)"
            if not en:
                lines.append(f"v{out_idx}.{part} = {mat_src}.{part}")
                continue
            need_nrm = True
            amb = ("vtx_clr" + str(ch)) if asrc else "ambcol"
            if opts.light_count == "mask":
                lights = f"mask={lm:#04x}"
            elif opts.light_count == "count":
                lights = f"n={bin(lm).count('1')}"
            else:
                lights = "n=8(masked)" if lm else "n=0"
            dfn = "none" if af == 0 else DIFF[df] if df < 3 else f"df{df}"
            lines.append(f"v{out_idx}.{part} = {mat_src}.{part} * sat({amb}.{part} + lights({lights}, diffuse={dfn}, attn={ATTN[af] if af < 3 else af}))")
    for slot, c in enumerate(frag.coords):
        if c >= len(mat.texgens):
            lines.append(f"uv{slot} = 0  // texcoord{c} not generated")
            continue
        ty, src, mtx = mat.texgens[c]
        s = TG_SRC[src] if src < len(TG_SRC) else f"src{src}"
        if s in ("nrm", "binrm", "tangent"):
            need_nrm = True
        if s.startswith("tex") and not s.startswith("texcoord") and (VA_TEX0 + src - 4) not in attrs:
            s += "(missing)"
        m = "texmtx" if (mtx != 60 or opts.texmtx_uniform) else "identity"
        tys = TG_TYPE.get(ty, f"type{ty}")
        post = " then post_mtx" if post_texmtx else ""
        lines.append(f"uv{slot} = {tys}({m}, {s}){post}")
    if need_nrm:
        lines.insert(1, "nrm = " + ("skin_n(pnmtx[idx], nrm)" if skinned else "nmtx * nrm"))
    return lines


def texgen_projective(mat) -> dict:
    return {i: tg[0] == 0 for i, tg in enumerate(mat.texgens)}


def apply_warp(mat):
    """d_resorce.cpp addWarpMaterial: +1 texgen (POS via mtx, 3x4) and +1 stage (tex * prev)."""
    import j3d  # local import keeps shader.py usable without j3d
    n = len(mat.texgens)
    if n >= 4 or len(mat.stages) >= 4:
        return None
    stg = j3d.Stage(n, 3, 0xFF, (0x0F, 0x08, 0x00, 0x0F, 0, 0, 0, 1, 0),
                    (0x07, 0x04, 0x00, 0x07, 0, 0, 0, 1, 0), 0, 0, 0x0C, 0x1C, (0,) * 9)
    names = list(mat.tex_names) + [None] * 8
    names[3] = "warp(Always:0x5d)"
    return replace(mat, texgens=list(mat.texgens) + [(0x00, 0x00, n * 3 + 0x1E)],
                   stages=list(mat.stages) + [stg], alpha_comp=(0x04, 0x80, 0x00, 0x03, 0xFF),
                   zcomploc=0, tex_names=names[:8])


if __name__ == "__main__":
    import j3d

    def mk(stages):
        return j3d.Material("t", 1, 1, [(0, 1, 0, 0, 2, 0)] * 4, [(1, 4, 60), (1, 5, 60)], [None] * 8,
                            [0, 1] + [0xFFFF] * 6, stages, [(0, 1, 2, 3)] * 4, [], None,
                            (7, 0, 0, 7, 0), (0, 0, 0, 0), (1, 3, 1), 1, ["x", "y"] + [None] * 6)
    D = (0,) * 9
    # tex * ras, written as lerp(0, TEXC, RASC) and as lerp(0, RASC, TEXC)
    m1 = j3d.Stage(0, 0, 4, (15, 8, 10, 15, 0, 0, 0, 1, 0), (7, 4, 5, 7, 0, 0, 0, 1, 0), 0, 0, 12, 28, D)
    m2 = j3d.Stage(0, 0, 4, (15, 10, 8, 15, 0, 0, 0, 1, 0), (7, 5, 4, 7, 0, 0, 0, 1, 0), 0, 0, 12, 28, D)
    # a dead stage writing r1, and a pass-through copy prev -> prev
    dead = j3d.Stage(1, 1, 4, (15, 15, 15, 8, 0, 0, 0, 1, 2), (7, 7, 7, 4, 0, 0, 0, 1, 2), 0, 0, 12, 28, D)
    copy = j3d.Stage(0xFF, 0xFF, 0xFF, (15, 15, 15, 0, 0, 0, 0, 0, 0), (7, 7, 7, 0, 0, 0, 0, 0, 0), 0, 0, 12, 28, D)
    # same as m1 but computed in r0 and moved to prev by a second stage
    via_r0 = j3d.Stage(0, 0, 4, (15, 8, 10, 15, 0, 0, 0, 1, 1), (7, 4, 5, 7, 0, 0, 0, 1, 1), 0, 0, 12, 28, D)
    mov = j3d.Stage(0xFF, 0xFF, 0xFF, (15, 15, 15, 2, 0, 0, 0, 1, 0), (7, 7, 7, 1, 0, 0, 0, 1, 0), 0, 0, 12, 28, D)
    ref = fragment(mk([m1]), Opts()).key()
    for variant in ([m2], [dead, m1], [m1, copy], [via_r0, mov]):
        got = fragment(mk(variant), Opts()).key()
        assert got == ref, (got, ref)
    assert ref.splitlines() == ["t0 = sample(map0, uv0.xy)", "out.rgb = v0 * t0", "out.a = v0.a * t0.a"], ref
    # register inputs are S10 uniforms: reading one as operand a/b/c must keep the u8() wrap
    k = fragment(mk([j3d.Stage(0, 0, 4, (15, 8, 2, 15, 0, 0, 0, 1, 0), (7, 7, 7, 1, 0, 0, 0, 1, 0), 0, 0, 12, 28, D)]), Opts()).key()
    assert "u8(c0.rgb)" in k and "out.a = sat(c0.a)" in k, k  # C0 and A0 are one register
    print("ok")


# --- pattern mining helpers ------------------------------------------------------------

_LEAF_CLASS = {"tex": "T", "ras": "V", "k": "K", "K": "K", "regin": "C"}


def general(n):
    """Tree with leaves replaced by their class (T sample, V vertex colour, K konst, C register input)."""
    t = n[0]
    if t in _LEAF_CLASS:
        lane = n[2]
        alpha = (t == "k" and lane == "a") or (t != "k" and lane != "rgb" and len(lane) == 1)
        return (_LEAF_CLASS[t], "a" if alpha else "")
    if t == "const":
        return n
    return (t,) + tuple(general(c) if isinstance(c, tuple) and c and c[0] in _KINDS else c for c in n[1:])


def _is_node(c):
    return isinstance(c, tuple) and c and isinstance(c[0], str)


def subtrees(n, out):
    """Collect all internal subtrees of a generalised tree into `out`."""
    if n[0] in ("T", "V", "K", "C", "const"):
        return out
    out.add(n)
    for c in n[1:]:
        if _is_node(c):
            subtrees(c, out)
    return out


def size(n):
    if n[0] in ("T", "V", "K", "C", "const"):
        return 0
    return 1 + sum(size(c) for c in n[1:] if _is_node(c))


def show(n):
    t = n[0]
    if t in ("T", "V", "K", "C"):
        return t + ("." + n[1] if n[1] else "")
    if t == "const":
        return "%g" % n[1]
    a = [show(c) if _is_node(c) else ("%g" % c if isinstance(c, float) else str(c)) for c in n[1:]]
    if t == "mul":
        return f"{a[0]} * {a[1]}"
    if t in ("add", "sub"):
        return f"({a[0]} {'+' if t == 'add' else '-'} {a[1]})"
    if t == "scale":
        return f"{a[0]} * {a[1]}" if a[0].startswith("(") else f"({a[0]}) * {a[1]}"
    if t == "bias":
        return f"({a[0]} + {a[1]})"
    if t == "splat":
        return a[0][:-2] + ".aaa" if a[0].endswith(".a") else f"vec3({a[0]})"
    if t == "cmp":
        return f"({a[0]}({a[1]}, {a[2]}) ? {a[3]} : 0)"
    return f"{t}({', '.join(a)})"
