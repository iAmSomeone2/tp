"""J2D screen layouts (.blo, SCRNblo2) -> materials, via the MAT1 block.

Layout follows J2DMaterialFactory (libs/JSystem/src/J2DGraph/J2DMaterialFactory.cpp).
Materials are returned as j3d.Material so the same canonicaliser handles them. J2D colour
channels are never lit (J2DColorChan keeps only the material source). Indirect stages in
J2D materials are rare; they are counted (`ind_enabled`) but their per-stage settings are
not decoded yet (J2DIndInitData's per-stage layout is not pinned down in the headers).
"""

from __future__ import annotations

import struct

import j3d
from j3d import name_table

INIT_SIZE = 0xE8


def _u16(b, o):
    return struct.unpack_from(">H", b, o)[0]


def materials(data: bytes):
    """Yield (material, ind_enabled) for every material in a SCRNblo2 file."""
    if data[:8] != b"SCRNblo2":
        raise ValueError(f"unsupported layout {data[:8]!r}")
    p, end = 0x20, len(data)
    blk = None
    while p + 8 <= end:
        tag, size = data[p : p + 4], struct.unpack_from(">I", data, p + 4)[0]
        if tag == b"MAT1":
            blk = p
            break
        if size == 0:
            break
        p += size
    if blk is None:
        return
    b = data
    num = _u16(b, blk + 8)
    offs = struct.unpack_from(">23I", b, blk + 0x0C)
    off = [0 if v == 0 else blk + v for v in offs]
    (init, mid, names, ind, _cull, _matcol, chnum, chinfo, tgnum, tcinfo, _tmtx, _texno, _font,
     order, _tevcol, _kcol, stnum, stinfo, swap, swaptbl, acomp, blend, _dither) = off
    nm = name_table(b, names)
    has_ind = ind != 0 and (ind - names) > 4
    for i in range(num):
        d = init + _u16(b, mid + i * 2) * INIT_SIZE

        def idx(o, k):
            return _u16(b, d + o + k * 2)

        chn = b[d + 2]
        chan_num = b[chnum + chn] if chn != 0xFF else 0
        chans = []
        for c in range(4):
            j = idx(0x0C, c)
            src = 0 if j == 0xFFFF else b[chinfo + j * 4 + 1]
            chans.append((0, src, 0, 0, 2, 0))
        tgn = b[d + 3]
        tg_num = b[tgnum + tgn] if tgn != 0xFF else 0
        texgens = []
        for t in range(tg_num):
            j = idx(0x14, t)
            texgens.append((1, 4 + t, 60) if j == 0xFFFF else tuple(b[tcinfo + j * 4 : tcinfo + j * 4 + 3]))
        sn = b[d + 4]
        st_num = b[stnum + sn] if sn != 0xFF else 0
        stages = []
        for s in range(st_num):
            j = idx(0x72, s)
            o = (0xFF, 0xFF, 0xFF) if j == 0xFFFF else tuple(b[order + j * 4 : order + j * 4 + 3])
            j = idx(0x9A, s)
            si = j3d.TEV_STAGE_DEFAULT if j == 0xFFFF else tuple(b[stinfo + j * 20 : stinfo + j * 20 + 19])
            j = idx(0xBA, s)
            ras_sel, tex_sel = (0, 0) if j == 0xFFFF else (b[swap + j * 4], b[swap + j * 4 + 1])
            stages.append(j3d.Stage(o[0], o[1], o[2], si[1:10], si[10:19], ras_sel, tex_sel,
                                    b[d + 0x52 + s], b[d + 0x62 + s], (0,) * 9))
        swap_tables = []
        for t in range(4):
            j = idx(0xDA, t)
            swap_tables.append((0, 1, 2, 3) if j == 0xFFFF else tuple(b[swaptbl + j * 4 : swaptbl + j * 4 + 4]))
        j = _u16(b, d + 0xE2)
        ac = (7, 0, 0, 7, 0) if j == 0xFFFF else tuple(b[acomp + j * 8 : acomp + j * 8 + 5])
        j = _u16(b, d + 0xE4)
        bl = (1, 4, 5, 5) if j == 0xFFFF else tuple(b[blend + j * 4 : blend + j * 4 + 4])
        ind_enabled = has_ind and b[ind + i * 0x13C] == 1
        yield j3d.Material(nm[i] if i < len(nm) else f"mat{i}", b[d], chan_num, chans, texgens, [None] * 8,
                           [0xFFFF] * 8, stages, swap_tables, [], None, ac, bl, (1, 3, 1), 0,
                           [f"texmap{k}" for k in range(8)], [None] * 8), ind_enabled
