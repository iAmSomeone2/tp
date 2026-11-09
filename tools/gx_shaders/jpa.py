"""JPA particle resources (.jpc, JPAC2-10) -> the GX TEV state JParticle sets for them.

JParticle builds its TEV setup in code from resource flag bits; this module reproduces that
code as synthetic j3d.Material objects so the same canonicaliser handles particles:

* JPAEmitterManager::draw: TEV stage 0 op = ADD, clamp, -> PREV; no colour channels.
* JPABaseShape::setGX: stage 0 colour/alpha inputs = st_ca[(flags >> 15) & 7] /
  st_aa[(flags >> 18) & 1]; alpha compare from mAlphaCompareCfg via st_c/st_ao.
* JPAResource::setPTev: stage 0 samples TEXMAP0 with TEXCOORD0; optional indirect
  (JPAExTexShape flag 0x1: ind stage 0 = TEXCOORD1 x TEXMAP2, ITF_8, ITB_STU, ITM_0) and
  optional second texture (flag 0x100: stage 1 = TEXMAP3 * prev).
* JPAResource::setCTev (child particles): one stage, TEXMAP1 with TEXCOORD0, direct. It does
  not set the colour/alpha inputs, so a child inherits the parent's stage-0 inputs from
  JPABaseShape::setGX (called once per emitter before drawing parent and children).
* Texgen 0 (JPAGenTexCrdMtx*): projected (MTX3x4 from POS, TEXMTX0) when isPrjTex,
  TEXMTX0 when isTexCrdAnm, identity otherwise.
* C0/C1 are the particle's prm/env colours (uniforms). Swap tables and K selectors are never
  set by JParticle (they keep whatever was set last); the identity is assumed here.
* Fog is not set by JParticle; the game sets it per draw group (dKy_GxFog_set in
  d_particle.cpp), so callers should consider both fog on and off.
"""

from __future__ import annotations

import struct

import j3d

CC_ZERO, CC_C0, CC_C1, CC_TEXC, CC_ONE, CC_CPREV = 15, 2, 4, 8, 12, 0
CA_ZERO, CA_A0, CA_TEXA, CA_APREV = 7, 1, 4, 0
ST_CA = [(CC_ZERO, CC_TEXC, CC_ONE, CC_ZERO), (CC_ZERO, CC_C0, CC_TEXC, CC_ZERO),
         (CC_C0, CC_ONE, CC_TEXC, CC_ZERO), (CC_C1, CC_C0, CC_TEXC, CC_ZERO),
         (CC_ZERO, CC_TEXC, CC_C0, CC_C1), (CC_ZERO, CC_ZERO, CC_ZERO, CC_C0)]
ST_AA = [(CA_ZERO, CA_TEXA, CA_A0, CA_ZERO), (CA_ZERO, CA_ZERO, CA_ZERO, CA_A0)]
ST_C = [0, 1, 3, 2, 5, 6, 4, 7]  # JPA index -> GXCompare
OP = (0, 0, 0, 1, 0)  # ADD, bias 0, scale 1, clamp, PREV


def _material(name, stages, texgens, ind_stages, alpha_comp, blend):
    return j3d.Material(name, 4, 0, [(0, 0, 0, 0, 2, 0)] * 4, texgens, [None] * 8, [0xFFFF] * 8,
                        stages, [(0, 1, 2, 3)] * 4, ind_stages, None, alpha_comp, blend,
                        (1, 3, 1), 0, [f"texmap{i}" for i in range(8)], [None] * 8)


def resources(data: bytes):
    """Yield (index, base material, child material or None, info) per JPA resource."""
    if data[:8] != b"JPAC2-10":
        raise ValueError(f"unsupported particle file {data[:8]!r}")
    res_num = struct.unpack_from(">H", data, 8)[0]
    off = 0x10
    for i in range(res_num):
        _usr, nblk = struct.unpack_from(">HH", data, off)
        off += 8
        bsp = ets = csp = None
        for _ in range(nblk):
            magic = data[off : off + 4]
            size = struct.unpack_from(">I", data, off + 4)[0]
            if magic == b"BSP1":
                bsp = off
            elif magic == b"ETX1":
                ets = off
            elif magic == b"SSP1":
                csp = off
            off += size
        if bsp is None:
            continue
        flags = struct.unpack_from(">I", data, bsp + 8)[0]
        acfg = data[bsp + 0x1A]
        bcfg = struct.unpack_from(">H", data, bsp + 0x18)[0]
        blend = ([0, 1, 2][bcfg & 3] if (bcfg & 3) < 3 else 0, (bcfg >> 2) & 0xF, (bcfg >> 6) & 0xF, (bcfg >> 10) & 0xF)
        ca = ST_CA[(flags >> 15) & 7] if ((flags >> 15) & 7) < len(ST_CA) else ST_CA[0]
        aa = ST_AA[(flags >> 18) & 1]
        alpha_comp = (ST_C[acfg & 7], data[bsp + 0x1B], (acfg >> 3) & 3, ST_C[(acfg >> 5) & 7], data[bsp + 0x1C])
        prj, crd_anm = bool(flags & 0x100000), bool(flags & 0x1000000)
        tg0 = (0, 0, 30) if prj else (1, 4, 30) if crd_anm else (1, 4, 60)
        texgens = [tg0]
        ind_on = sec_on = False
        if ets is not None:
            eflags = struct.unpack_from(">I", data, ets + 8)[0]
            ind_on, sec_on = bool(eflags & 0x1), bool(eflags & 0x100)
        ind = (0, 0, 7, 1, 0, 0, 0, 0, 0) if ind_on else (0,) * 9
        stages = [j3d.Stage(0, 0, 0xFF, ca + OP, aa + OP, 0, 0, 0x0C, 0x1C, ind)]
        ind_stages = []
        coord = 1
        if ind_on:
            ind_stages.append((1, 2, 0, 0))
            texgens.append((1, 4, 60))
            coord = 2
        if sec_on:
            stages.append(j3d.Stage(coord, 3, 0xFF, (CC_ZERO, CC_TEXC, CC_CPREV, CC_ZERO) + OP,
                                    (CA_ZERO, CA_TEXA, CA_APREV, CA_ZERO) + OP, 0, 0, 0x0C, 0x1C, (0,) * 9))
            texgens.append((1, 4, 60))
        base = _material("base", stages, texgens, ind_stages, alpha_comp, blend)
        child = None
        if csp is not None:
            child = _material("child", [j3d.Stage(0, 1, 0xFF, ca + OP, aa + OP, 0, 0, 0x0C, 0x1C, (0,) * 9)],
                              [(1, 4, 60)], [], alpha_comp, blend)
        yield i, base, child, dict(prj=prj, ind=ind_on, sec=sec_on)
