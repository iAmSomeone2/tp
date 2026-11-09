"""Minimal RARC (JKRArchive) reader, plus Yaz0 for archives or members that are still compressed."""

from __future__ import annotations

import struct
from typing import Iterator, NamedTuple


def yaz0_decompress(src: bytes) -> bytes:
    if src[:4] != b"Yaz0":
        return src
    size = struct.unpack_from(">I", src, 4)[0]
    dst = bytearray(size)
    s, d = 16, 0
    while d < size:
        code = src[s]
        s += 1
        for bit in range(7, -1, -1):
            if d >= size:
                break
            if code & (1 << bit):
                dst[d] = src[s]
                d += 1
                s += 1
            else:
                b1, b2 = src[s], src[s + 1]
                s += 2
                dist = ((b1 & 0x0F) << 8 | b2) + 1
                n = b1 >> 4
                if n == 0:
                    n = src[s] + 0x12
                    s += 1
                else:
                    n += 2
                for _ in range(n):
                    dst[d] = dst[d - dist]
                    d += 1
    return bytes(dst)


class Member(NamedTuple):
    path: str  # "dir/sub/file.ext" relative to the archive root (root dir name dropped)
    dir_tag: str  # 4-char node type of the containing directory, e.g. "BMDR"
    data: bytes


def members(raw: bytes) -> Iterator[Member]:
    raw = yaz0_decompress(raw)
    if raw[:4] != b"RARC":
        raise ValueError("not a RARC archive")
    data_off = struct.unpack_from(">I", raw, 0x0C)[0] + 0x20
    info = 0x20
    n_nodes, node_off, n_entries, entry_off, _strlen, str_off = struct.unpack_from(">6I", raw, info)
    node_off += info
    entry_off += info
    str_off += info

    def name(off: int) -> str:
        end = raw.index(b"\0", str_off + off)
        return raw[str_off + off : end].decode("shift_jis", "replace")

    def walk(node_idx: int, prefix: str) -> Iterator[Member]:
        ntype, _nname, _hash, count, first = struct.unpack_from(">4sIHHI", raw, node_off + node_idx * 0x10)
        tag = ntype.decode("ascii", "replace").strip()
        for i in range(first, first + count):
            e = entry_off + i * 0x14
            _id, _h, attr, _pad, noff, doff, size = struct.unpack_from(">HHBBHII", raw, e)
            nm = name(noff)
            if attr & 0x02:  # directory
                if nm in (".", ".."):
                    continue
                yield from walk(doff, f"{prefix}{nm}/")
            else:
                blob = raw[data_off + doff : data_off + doff + size]
                yield Member(prefix + nm, tag, yaz0_decompress(blob))

    yield from walk(0, "")
