"""Parse the parts of J3D model files (.bmd/.bdl, J3D2bmd3/bdl4) that decide GX shader state.

Layouts follow the engine's own loader: J3DMaterialFactory (MAT3), J3DShapeFactory (SHP1),
J3DModelLoader (INF1/TEX1). Only MAT3 is read for materials; .bdl files also carry MDL3
(prebuilt material display lists), which the loader builds from the same data.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field


def _u8(b, o): return b[o]
def _u16(b, o): return struct.unpack_from(">H", b, o)[0]
def _u32(b, o): return struct.unpack_from(">I", b, o)[0]


def name_table(b: bytes, off: int) -> list[str]:
    n = _u16(b, off)
    out = []
    for i in range(n):
        s = off + _u16(b, off + 4 + i * 4 + 2)
        out.append(b[s : b.index(b"\0", s)].decode("shift_jis", "replace"))
    return out


@dataclass
class Stage:
    tex_coord: int
    tex_map: int
    chan: int
    color: tuple  # (a, b, c, d, op, bias, scale, clamp, dest)
    alpha: tuple
    ras_swap: int
    tex_swap: int
    kc_sel: int
    ka_sel: int
    ind: tuple  # (stage, fmt, bias, mtx, wrapS, wrapT, prev, lod, alphaSel)


@dataclass
class Material:
    name: str
    mode: int
    chan_num: int
    chans: list  # 4 x (enable, matSrc, lightMask, diffFn, attnFn, ambSrc): COLOR0, ALPHA0, COLOR1, ALPHA1
    texgens: list  # (type, src, mtx)
    tex_mtx_proj: list  # per texgen matrix slot: projection byte or None
    tex_no: list  # 8 texture indices (0xFFFF = none)
    stages: list
    swap_tables: list  # 4 x (r, g, b, a)
    ind_stages: list  # (coord, map, scaleS, scaleT)
    fog: tuple | None  # (type, adjEnable)
    alpha_comp: tuple  # (comp0, ref0, op, comp1, ref1)
    blend: tuple
    zmode: tuple
    zcomploc: int
    tex_names: list = field(default_factory=list)  # texture names by texmap slot (resolved)
    tex_formats: list = field(default_factory=list)


@dataclass
class Shape:
    mtx_type: int
    attrs: dict  # GXAttr -> GXAttrType


@dataclass
class Model:
    materials: list
    shapes: list
    draws: list  # (material index, shape index) in hierarchy order
    textures: list  # (name, format)


TEV_STAGE_DEFAULT = (0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0, 0, 0, 1, 0, 7, 7, 7, 7, 0, 0, 0, 1, 0)


def _read_mat3(b: bytes, blk: int, textures: list) -> list[Material]:
    hdr = struct.unpack_from(">H2x30I", b, blk + 8)
    num = hdr[0]
    off = [0 if v == 0 else blk + v for v in hdr[1:]]
    (init, mid, names, ind, cull, matcol, chnum, chinfo, amb, light, tgnum, tcinfo, tc2,
     tmtx, _f44, texno, order, tevcol, kcol, stnum, stinfo, swap, swaptbl, fog, acomp,
     blend, zmode, zcomp, dither, nbt) = off
    nm = name_table(b, names)
    has_ind = ind != 0 and (ind - names) > 4

    def idx16(base, i): return _u16(b, base + i * 2)

    mats = []
    for i in range(num):
        d = init + _u16(b, mid + i * 2) * 0x14C
        mode = _u8(b, d)
        chn = _u8(b, d + 2)
        chan_num = _u8(b, chnum + chn) if chn != 0xFF else 0
        chans = []
        for c in range(4):
            j = idx16(d + 0x0C, c)
            if j == 0xFFFF:
                chans.append((0, 0, 0, 0, 2, 0))  # J3DColorChan() default: unlit, REG
            else:
                e = chinfo + j * 8
                en, ms, lm, df, af, asrc = b[e : e + 6]
                chans.append((en, ms, lm, df, af, 0 if asrc == 0xFF else asrc))
        tgn = _u8(b, d + 3)
        tg_num = _u8(b, tgnum + tgn) if tgn != 0xFF else 0
        texgens = []
        for t in range(tg_num):
            j = idx16(d + 0x28, t)
            texgens.append((1, 4 + t, 60) if j == 0xFFFF else tuple(b[tcinfo + j * 4 : tcinfo + j * 4 + 3]))
        tex_mtx_proj = []
        for t in range(8):
            j = idx16(d + 0x48, t)
            tex_mtx_proj.append(None if j == 0xFFFF else (b[tmtx + j * 0x64], b[tmtx + j * 0x64 + 1]))
        tex_no = [0xFFFF if idx16(d + 0x84, t) == 0xFFFF else _u16(b, texno + idx16(d + 0x84, t) * 2) for t in range(8)]
        sn = _u8(b, d + 4)
        st_num = _u8(b, stnum + sn) if sn != 0xFF else 0
        ind_base = ind + i * 0x138 if has_ind else 0
        ind_on = has_ind and b[ind_base] == 1
        stages = []
        for s in range(st_num):
            j = idx16(d + 0xBC, s)
            o = (0xFF, 0xFF, 0xFF) if j == 0xFFFF else tuple(b[order + j * 4 : order + j * 4 + 3])
            j = idx16(d + 0xE4, s)
            si = TEV_STAGE_DEFAULT if j == 0xFFFF else tuple(b[stinfo + j * 20 : stinfo + j * 20 + 19])
            j = idx16(d + 0x104, s)
            ras_sel, tex_sel = (0, 0) if j == 0xFFFF else (b[swap + j * 4], b[swap + j * 4 + 1])
            kc = b[d + 0x9C + s]
            ka = b[d + 0xAC + s]
            indst = tuple(b[ind_base + 0x78 + s * 12 : ind_base + 0x78 + s * 12 + 9]) if ind_on else (0,) * 9
            stages.append(Stage(o[0], o[1], o[2], si[1:10], si[10:19], ras_sel, tex_sel,
                                0x0C if kc == 0xFF else kc, 0x1C if ka == 0xFF else ka, indst))
        swap_tables = []
        for t in range(4):
            j = idx16(d + 0x124, t)
            swap_tables.append((0, 1, 2, 3) if j == 0xFFFF else tuple(b[swaptbl + j * 4 : swaptbl + j * 4 + 4]))
        ind_stages = []
        if ind_on:
            for t in range(b[ind_base + 1]):
                ind_stages.append((b[ind_base + 4 + t * 4], b[ind_base + 5 + t * 4],
                                   b[ind_base + 0x68 + t * 4], b[ind_base + 0x69 + t * 4]))
        j = _u16(b, d + 0x144)
        fg = None if j == 0xFFFF else (b[fog + j * 0x2C], b[fog + j * 0x2C + 1])
        j = _u16(b, d + 0x146)
        ac = (7, 0, 0, 7, 0) if j == 0xFFFF else tuple(b[acomp + j * 8 : acomp + j * 8 + 5])
        j = _u16(b, d + 0x148)
        bl = (1, 4, 5, 5) if j == 0xFFFF else tuple(b[blend + j * 4 : blend + j * 4 + 4])
        j = _u8(b, d + 6)
        zm = (1, 3, 1) if j == 0xFF else tuple(b[zmode + j * 4 : zmode + j * 4 + 3])
        j = _u8(b, d + 5)
        zc = _u8(b, zcomp + j) if j != 0xFF else 0
        m = Material(nm[i], mode, chan_num, chans, texgens, tex_mtx_proj, tex_no, stages,
                     swap_tables, ind_stages, fg, ac, bl, zm, zc)
        for t in tex_no:
            if t != 0xFFFF and t < len(textures):
                m.tex_names.append(textures[t][0])
                m.tex_formats.append(textures[t][1])
            else:
                m.tex_names.append(None)
                m.tex_formats.append(None)
        mats.append(m)
    return mats


def _read_shp1(b: bytes, blk: int) -> list[Shape]:
    num = _u16(b, blk + 8)
    init, index, _names, vcd = (blk + _u32(b, blk + o) for o in (0x0C, 0x10, 0x14, 0x18))
    shapes = []
    for i in range(num):
        d = init + _u16(b, index + i * 2) * 0x28
        p = vcd + _u16(b, d + 4)
        attrs = {}
        while True:
            a, t = struct.unpack_from(">II", b, p)
            if a == 0xFF:
                break
            attrs[a] = t
            p += 8
        shapes.append(Shape(b[d], attrs))
    return shapes


def _read_tex1(b: bytes, blk: int) -> list:
    num = _u16(b, blk + 8)
    res = blk + _u32(b, blk + 0x0C)
    names = name_table(b, blk + _u32(b, blk + 0x10))
    return [(names[i], b[res + i * 0x20]) for i in range(num)]


def _read_inf1(b: bytes, blk: int) -> list:
    p = blk + _u32(b, blk + 0x14)
    draws, mat = [], None
    while True:
        t, v = struct.unpack_from(">HH", b, p)
        p += 4
        if t == 0:
            return draws
        if t == 0x11:
            mat = v
        elif t == 0x12:
            draws.append((mat, v))


def parse(b: bytes) -> Model:
    if b[:4] != b"J3D2" or b[4:8] not in (b"bmd3", b"bdl4", b"bmd2", b"bdl3"):
        raise ValueError(f"unsupported J3D file {b[:8]!r}")
    blocks = {}
    p = 0x20
    for _ in range(_u32(b, 0x0C)):
        tag = b[p : p + 4].decode()
        blocks[tag] = p
        p += _u32(b, p + 4)
    if "MAT3" not in blocks:
        raise ValueError("no MAT3 block (MAT2/v21 models are not handled)")
    textures = _read_tex1(b, blocks["TEX1"]) if "TEX1" in blocks else []
    return Model(
        materials=_read_mat3(b, blocks["MAT3"], textures),
        shapes=_read_shp1(b, blocks["SHP1"]),
        draws=_read_inf1(b, blocks["INF1"]),
        textures=textures,
    )
