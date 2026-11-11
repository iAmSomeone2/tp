#!/usr/bin/env python3
"""Build the vita-bench test corpus from the user's disc image.

Each corpus entry stands in for one baked archive: the archive's member files,
fully decompressed (dtk undoes the outer and per-member Yaz0), concatenated at
32-byte alignment. That is close to what a baked .nfa holds, minus its header.
Textures are still GX formats; CMPR and BC1 are the same size, so the effect on
compression ratios is small.

Output (default build/vita-bench/corpus/):
  files/<name>.bin   one loose file per entry (the "one open per archive" case)
  pack.bin           all entries concatenated, 32-byte aligned
  pack.idx           one line per entry: "<name> <offset> <size>"

Copy the corpus directory to ux0:data/nfbench/corpus/ on the Vita. To also test
app0:, configure the benchmark with -DNFBENCH_CORPUS=<this directory>.
"""

import argparse
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# The logo scene's ARAM-resident set (d_s_logo.cpp:1443-1501), a stage, and
# common object archives.
ARCHIVES = [
    "Object/AlAnm.arc", "FieldMap/Field0.arc",
    "Layout/fmapres.arc", "Layout/dmapres.arc", "Layout/clctres.arc",
    "Layout/itemicon.arc", "Layout/ringres.arc", "Layout/playerName.arc",
    "Layout/itmInfRes.arc", "Layout/button.arc", "CardIcon/cardicon.arc",
    "Layout/msgcom.arc", "Layout/msgres00.arc", "Layout/msgres01.arc",
    "Layout/msgres02.arc", "Layout/msgres03.arc", "Layout/msgres04.arc",
    "Layout/msgres05.arc", "Layout/msgres06.arc",
    "Stage/F_SP103/STG_00.arc", "Stage/F_SP103/R00_00.arc", "Stage/F_SP103/R01_00.arc",
    "Object/Alink.arc", "Object/Always.arc", "Object/Kmdl.arc",
    "Object/Wmdl.arc", "Object/Midna.arc", "Object/Npc_ks.arc",
]

ALIGN = 32


def pad(data: bytearray) -> None:
    data.extend(b"\0" * (-len(data) % ALIGN))


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--iso", type=Path, default=ROOT / "orig/GZ2E01/GZ2E01.iso")
    ap.add_argument("--dtk", type=Path, default=ROOT / "build/tools/dtk")
    ap.add_argument("--out", type=Path, default=ROOT / "build/vita-bench/corpus")
    args = ap.parse_args()

    files_dir = args.out / "files"
    files_dir.mkdir(parents=True, exist_ok=True)
    pack = bytearray()
    index = []

    with tempfile.TemporaryDirectory() as tmp:
        for arc in ARCHIVES:
            name = arc.replace("/", "_").removesuffix(".arc")
            dest = Path(tmp) / name
            subprocess.run([str(args.dtk), "vfs", "cp", "-q", f"{args.iso}:files/res/{arc}:", str(dest)], check=True)
            entry = bytearray()
            for member in sorted(p for p in dest.rglob("*") if p.is_file()):
                entry += member.read_bytes()
                pad(entry)
            (files_dir / f"{name}.bin").write_bytes(entry)
            index.append(f"{name} {len(pack)} {len(entry)}")
            pack += entry
            print(f"{name:28} {len(entry) / 1024:9.1f} KiB")

    (args.out / "pack.bin").write_bytes(pack)
    (args.out / "pack.idx").write_text("\n".join(index) + "\n")
    print(f"{len(index)} entries, {len(pack) / 2**20:.2f} MiB -> {args.out}")


if __name__ == "__main__":
    main()
