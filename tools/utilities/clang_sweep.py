#!/usr/bin/env python3

###
# Runs a syntax-only pass of a modern compiler (default: clang++ -std=c++20)
# over every game/JSystem translation unit, WITHOUT the Metrowerks headers, and
# summarises what still fails. This measures how far the tree is from building
# with a stock toolchain (the first milestone of a port), and can be re-run to
# track progress.
#
# The pass is deliberately lenient: -fsyntax-only, warnings off, GameCube USA
# (VERSION 0), DEBUG off. It cannot see link errors, ABI/layout differences,
# or anything that only shows up at run time.
#
# By default it checks the translation units that the CMake build compiles for
# the chosen GameCube version (configure.py + config/<version>/splits.txt, via
# gen_cmake_sources.py). Files that exist only in other versions (Wii, Shield,
# ShieldD's HostIO/debug code) are not part of that build and are skipped; pass
# --all-files to sweep every .cpp under src/ and libs/JSystem/src instead.
#
# Usage (from anywhere):
#   python3 tools/utilities/clang_sweep.py                       # summary on stdout
#   python3 tools/utilities/clang_sweep.py --report docs/port/compile-sweep.md
#   python3 tools/utilities/clang_sweep.py --cxx arm-vita-eabi-g++ --std gnu++20
#   python3 tools/utilities/clang_sweep.py --all-files --define DEBUG=1 --version 12
###

import argparse
import collections
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))

# Classification of compiler messages, first match wins.
CATEGORIES = [
    ("Missing generated asset header (`assets/<ver>/…` or `build/<ver>/include`, produced from your disc image)",
     r"'assets/[^']+' file not found"),
    ("Wii/Shield-only SDK header (`revolution/…`) pulled in on a GameCube configuration",
     r"'revolution/[^']+' file not found"),
    ("Metrowerks math macro missing (`DEG_TO_RAD`, `RAD_TO_DEG`) – provided by MSL `<cmath>`",
     r"undeclared identifier '(?:DEG_TO_RAD|RAD_TO_DEG)'"),
    ("Non-standard C library extension (`stricmp`, `strnicmp`, …)", r"undeclared identifier '(?:stricmp|strnicmp)'"),
    ("`va_start` / `va_end` without `<cstdarg>`", r"undeclared identifier 'va_(?:start|end)'"),
    ("`goto`-like jump past initialisation in `switch` (MWCC accepts, standard C++ rejects)", r"cannot jump from switch statement"),
    ("Narrowing in constant expressions / template arguments (`-Wc++11-narrowing`)", r"narrowed"),
    ("Pointer cast to a smaller integer (only on 64-bit hosts; fine on 32-bit ARM)", r"cast from pointer to smaller type"),
    ("Other missing header", r"file not found"),
    ("Other undeclared identifier", r"undeclared identifier"),
]


def classify(msg):
    for name, rx in CATEGORIES:
        if re.search(rx, msg):
            return name
    return "Other"


def run_one(args, path):
    cmd = [
        args.cxx, f"-std={args.std}", "-fsyntax-only", "-fdeclspec", "-w", "-fno-caret-diagnostics",
        "-ferror-limit=0", f"-DVERSION={args.version}",
        "-I", "include", "-I", "src", "-I", "libs/JSystem/include",
        "-I", "libs/dolphin/include", "-I", "libs/dolphin/include/dolphin",
        "-I", f"assets/{args.asset_version}",
    ]
    cmd += [f"-D{d}" for d in args.define]
    cmd.append(path)
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    errs = [l for l in (r.stdout + r.stderr).splitlines() if "error:" in l]
    return path, errs


def collect_files(args):
    if not args.all_files:
        import gen_cmake_sources

        if 0 <= args.version < len(gen_cmake_sources.GCN_VERSIONS):
            return gen_cmake_sources.sources_for_version(gen_cmake_sources.GCN_VERSIONS[args.version])
        sys.exit(f"--version {args.version} is not a GameCube version, so there is no CMake file set "
                 "for it; add --all-files to sweep every source file")
    skip = ("NdevExi2A", "odemuexi2", "odenotstub", "amcstubs", "lingcod")
    out = []
    for base in ("src", "libs/JSystem/src"):
        for dp, _, fns in os.walk(ROOT / base):
            for fn in sorted(fns):
                if fn.endswith(".cpp") and not any(s in dp for s in skip):
                    out.append(os.path.relpath(os.path.join(dp, fn), ROOT))
    return sorted(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--cxx", default="clang++")
    ap.add_argument("--std", default="c++20")
    ap.add_argument("--version", type=int, default=0, help="VERSION define (0 = GCN USA)")
    ap.add_argument("--asset-version", default="GZ2E01")
    ap.add_argument("--define", action="append", default=[], help="extra -D (repeatable), e.g. DEBUG=1")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--all-files", action="store_true",
                    help="sweep every .cpp under src/ and libs/JSystem/src, not just the CMake build's set")
    ap.add_argument("--report", help="write a markdown report to this path")
    args = ap.parse_args()

    files = collect_files(args)
    print(f"checking {len(files)} translation units with {args.cxx} -std={args.std} …", file=sys.stderr)
    with ThreadPoolExecutor(args.jobs) as ex:
        results = list(ex.map(lambda f: run_one(args, f), files))

    failing = [(f, e) for f, e in results if e]
    by_cat = collections.defaultdict(lambda: collections.defaultdict(list))
    for f, errs in failing:
        for e in errs:
            by_cat[classify(e)][f].append(e)

    ver = subprocess.run([args.cxx, "--version"], capture_output=True, text=True).stdout.splitlines()[0]
    lines = [
        "# Compile sweep (generated)\n",
        f"Generated by `python3 tools/utilities/clang_sweep.py`. Compiler: `{ver}`; flags: `-std={args.std} -fsyntax-only`, "
        f"`VERSION={args.version}`, `DEBUG` {'defined' if any(d.startswith('DEBUG') for d in args.define) else 'off'}, "
        "**no Metrowerks/MSL headers**. "
        + ("Files: every `.cpp` under `src/` and `libs/JSystem/src`."
           if args.all_files else
           "Files: the translation units the CMake build compiles for this version (`configure.py` + splits); "
           "Wii/Shield-only and ShieldD debug files are not included.")
        + "\n",
        f"**{len(files) - len(failing)} of {len(files)} translation units parse cleanly** ({100 * (len(files) - len(failing)) / len(files):.1f}%); "
        f"{len(failing)} still fail.\n",
        "This is a syntax check on the host's own pointer size (usually 64-bit). It cannot see link errors, struct-layout drift, or run-time behaviour.\n",
        "## Failures by cause\n",
        "| Cause | Files | Errors |",
        "|---|---|---|",
    ]
    for cat, per in sorted(by_cat.items(), key=lambda kv: -sum(len(v) for v in kv[1].values())):
        lines.append(f"| {cat} | {len(per)} | {sum(len(v) for v in per.values())} |")
    lines.append("\n## Failing files\n")
    for cat, per in sorted(by_cat.items(), key=lambda kv: -sum(len(v) for v in kv[1].values())):
        lines.append(f"### {cat}\n")
        for f, errs in sorted(per.items()):
            first = re.sub(r"^\S+?:\d+:\d+: (?:fatal )?error: ", "", errs[0])[:140]
            lines.append(f"* `{f}` ({len(errs)}) – {first}")
        lines.append("")
    text = "\n".join(lines) + "\n"

    print("\n".join(lines[:4 + 4 + len(by_cat)]))
    if args.report:
        (ROOT / args.report).parent.mkdir(parents=True, exist_ok=True)
        (ROOT / args.report).write_text(text)
        print(f"wrote {args.report}", file=sys.stderr)


if __name__ == "__main__":
    main()
