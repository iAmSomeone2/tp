#!/usr/bin/env python3
"""Generate the `assets/*.h` headers that a handful of translation units include.

The headers hold raw data (textures, display lists, message and font data) that is read out of
the user's own disc image, so they are not in the repository. This is the same pipeline the
Metrowerks build runs through ninja, minus the compile step:

  1. `dtk dol split config/<ver>/config.yml build/<ver>` reads the DOL and RELs from
     orig/<ver>, writes build/<ver>/bin/assets/*.bin and the plain headers in
     build/<ver>/include/assets/, and records everything in build/<ver>/config.json.
  2. Assets marked `custom_type: matDL` get no header from dtk; tools/converters/matDL_dis.py
     turns their .bin into a header (the `convert_matDL` rule in configure.py).

decomp-toolkit is downloaded to build/tools/ on first use (tag taken from configure.py), unless
--dtk points at an existing binary. Pass --stamp to touch a file once everything succeeded; the
CMake target uses it as the output of its custom command.
"""

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXE = ".exe" if sys.platform == "win32" else ""


def dtk_tag() -> str:
    """The decomp-toolkit tag configure.py pins, so both builds use the same release."""
    match = re.search(r'^config\.dtk_tag\s*=\s*"([^"]+)"', (ROOT / "configure.py").read_text(), re.M)
    if not match:
        sys.exit("could not find config.dtk_tag in configure.py")
    return match.group(1)


def find_dtk(build_dir: Path, explicit: Path | None) -> Path:
    if explicit is not None:
        if not explicit.is_file():
            sys.exit(f"--dtk {explicit} is not a file")
        return explicit
    dtk = build_dir / "tools" / f"dtk{EXE}"
    if not dtk.is_file():
        dtk.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [sys.executable, str(ROOT / "tools" / "download_tool.py"), "dtk", str(dtk), "--tag", dtk_tag()],
            check=True,
        )
    return dtk


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", default="GZ2E01", help="game version (default: GZ2E01)")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build", help="same as configure.py (default: build)")
    parser.add_argument("--dtk", type=Path, help="existing decomp-toolkit binary (default: download)")
    parser.add_argument("--stamp", type=Path, help="file to touch on success")
    args = parser.parse_args()

    config_yml = ROOT / "config" / args.version / "config.yml"
    if not config_yml.is_file():
        sys.exit(f"no such version: {config_yml} does not exist")
    disc_dir = ROOT / "orig" / args.version
    if not any(p for p in disc_dir.glob("*") if p.name != ".gitkeep"):
        sys.exit(f"no disc image in {disc_dir}; copy your game's image there (see README.md)")

    out_dir = args.build_dir / args.version
    dtk = find_dtk(args.build_dir, args.dtk)

    # Paths in config.yml (object_base, splits, symbols) are relative to the repository root.
    subprocess.run([str(dtk), "dol", "split", str(config_yml.relative_to(ROOT)), str(out_dir)], cwd=ROOT, check=True)

    config = json.loads((out_dir / "config.json").read_text())
    assets = list(config.get("extract", []))
    for module in config.get("modules", []):
        assets += module.get("extract", [])

    converted = 0
    for asset in assets:
        match asset.get("custom_type"):
            case None:
                continue
            case "matDL":
                scope = (asset.get("custom_data") or {}).get("scope", "local")
                header = out_dir / "include" / asset["header"]
                header.parent.mkdir(parents=True, exist_ok=True)
                subprocess.run(
                    [
                        sys.executable,
                        str(ROOT / "tools" / "converters" / "matDL_dis.py"),
                        str(out_dir / "bin" / asset["binary"]),
                        str(header),
                        "--symbol", asset["symbol"],
                        "--scope", scope,
                    ],
                    cwd=ROOT,
                    check=True,
                )
                converted += 1
            case other:
                sys.exit(f"unknown asset type {other!r} for {asset['symbol']}")

    headers = sum(1 for _ in (out_dir / "include" / "assets").glob("*.h"))
    print(f"{args.version}: {headers} asset headers in {out_dir / 'include' / 'assets'} ({converted} converted from matDL)")

    if args.stamp:
        args.stamp.parent.mkdir(parents=True, exist_ok=True)
        args.stamp.touch()


if __name__ == "__main__":
    main()
