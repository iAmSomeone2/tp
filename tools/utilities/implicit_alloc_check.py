#!/usr/bin/env python3

###
# Lists the engine's implicit heap use: what still depends on the current heap, plus every delete.
# It is step 1 of docs/port/memory-design.md; its counts are the progress measure for step 2
# (explicit selection), which goes module by module until a module reports 0.
#
# What is counted (engine code only: tests, system headers and fetched dependencies are skipped):
#   new      `new T` / `new T[n]` whose operator new takes no JKRHeap*, so it allocates from the
#            current heap (placement new into a void* is not an allocation and is skipped), plus
#            JKRAlloc(...), JKRAllocFromHeap(nullptr, ...) and JKRHeap::alloc(size, align, nullptr)
#   delete   every delete expression (routed by address; step 3 sends them to the host or to the
#            heap that owns the address)
#   switch   every call that changes the current heap: JKRSetCurrentHeap, mDoExt_setCurrentHeap,
#            JKRHeap::setCurrentHeap, JKRHeap::becomeCurrentHeap
#
# How: every TU in a CMake compile_commands.json is run through clang-query with AST matchers; a
# site in a header counts once however many TUs include it. A TU that fails to parse may report
# too little, so such TUs are named and the script exits with status 1.
#
# Usage:
#   python3 tools/utilities/implicit_alloc_check.py                 # summary on stdout
#   python3 tools/utilities/implicit_alloc_check.py --report docs/port/implicit-allocations.md
#   python3 tools/utilities/implicit_alloc_check.py --sites src/d/d_stage.cpp   # list one file's sites
###

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
KINDS = ("new", "delete", "switch")

HEAP_PTR = 'hasType(pointerType(pointee(hasDeclaration(cxxRecordDecl(isSameOrDerivedFrom("JKRHeap"))))))'
NULL_ARG = "ignoringParenImpCasts(anyOf(cxxNullPtrLiteralExpr(), integerLiteral(equals(0)), gnuNullExpr()))"
ENGINE = "unless(isExpansionInSystemHeader())"
QUERY = f"""set output diag
set bind-root false
match cxxNewExpr({ENGINE}, hasDeclaration(functionDecl(unless(hasAnyParameter({HEAP_PTR})), unless(hasParameter(1, hasType(pointerType(pointee(voidType())))))))).bind("new")
match callExpr({ENGINE}, anyOf(callee(functionDecl(hasName("::JKRAlloc"))), allOf(callee(functionDecl(hasName("::JKRAllocFromHeap"))), hasArgument(0, {NULL_ARG})), allOf(callee(functionDecl(hasName("JKRHeap::alloc"))), argumentCountIs(3), hasArgument(2, {NULL_ARG})))).bind("new")
match cxxDeleteExpr({ENGINE}).bind("delete")
match callExpr({ENGINE}, callee(functionDecl(hasAnyName("::JKRSetCurrentHeap", "::mDoExt_setCurrentHeap", "JKRHeap::setCurrentHeap", "JKRHeap::becomeCurrentHeap")))).bind("switch")
"""
BINDS = re.compile(r'^(?P<file>[^:\s][^:]*):(?P<line>\d+):(?P<col>\d+): note: "(?P<kind>\w+)" binds here')
ERROR = re.compile(r"^[^:\s][^:]*:\d+:\d+: error: ")


def run_one(query_file, db_dir, tu):
    r = subprocess.run(["clang-query", "-p", db_dir, "-f", query_file, tu], capture_output=True, text=True)
    out = r.stdout + r.stderr
    sites = [(m["file"], int(m["line"]), int(m["col"]), m["kind"]) for m in map(BINDS.match, out.splitlines()) if m]
    failed = r.returncode != 0 or any(ERROR.match(l) for l in out.splitlines())
    return tu, failed, sites


def is_engine(path):
    return not path.startswith(("..", "/", "tests/", "build/")) and "_deps" not in path


def module_of(path):
    parts = Path(path).parts
    if parts[:2] == ("libs", "JSystem"):
        # libs/JSystem/src/<M>/... or libs/JSystem/include/JSystem/<M>/...
        rest = parts[3:] if parts[2] == "src" else parts[4:]
        return f"JSystem/{rest[0]}" if len(rest) > 1 else "JSystem"
    if parts[0] == "libs":
        return f"libs/{parts[1]}"
    if parts[0] in ("src", "include") and len(parts) > 2:
        if parts[1] == "d" and len(parts) > 3 and parts[2] == "actor":
            return "d/actor"
        return parts[1]
    return parts[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--db", default=str(ROOT / "build/cmake/default/compile_commands.json"))
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--report", help="write a markdown report to this path")
    ap.add_argument("--sites", metavar="PATH", help="print every site in this file (repo-relative) and exit")
    args = ap.parse_args()

    db_dir = str(Path(args.db).parent)
    tus = sorted({e["file"] for e in json.load(open(args.db))})
    tus = [t for t in tus if is_engine(os.path.relpath(t, ROOT))]
    print(f"checking {len(tus)} translation units …", file=sys.stderr)

    sites = set()  # (relpath, line, col, kind); a header site counts once
    failed = []
    with tempfile.NamedTemporaryFile("w", suffix=".query") as q:
        q.write(QUERY)
        q.flush()
        with ThreadPoolExecutor(args.jobs) as ex:
            for tu, bad, found in ex.map(lambda t: run_one(q.name, db_dir, t), tus):
                if bad:
                    failed.append(os.path.relpath(tu, ROOT))
                for f, line, col, kind in found:
                    rel = os.path.relpath(os.path.realpath(f), ROOT)
                    if is_engine(rel):
                        sites.add((rel, line, col, kind))

    if args.sites:
        for rel, line, col, kind in sorted(s for s in sites if s[0] == args.sites):
            text = (ROOT / rel).read_text(errors="replace").splitlines()[line - 1].strip()
            print(f"{rel}:{line}:{col}: {kind}: {text}")
        return 0

    per_file = defaultdict(Counter)
    for rel, _, _, kind in sites:
        per_file[rel][kind] += 1
    per_module = defaultdict(Counter)
    for rel, counts in per_file.items():
        per_module[module_of(rel)].update(counts)
    total = Counter()
    for c in per_module.values():
        total.update(c)

    summary = f"{total['new']} implicit allocations, {total['delete']} deletes, {total['switch']} current-heap switches"
    print(summary)
    for m in sorted(per_module, key=lambda m: -sum(per_module[m].values())):
        c = per_module[m]
        print(f"  {m:28} new {c['new']:5}  delete {c['delete']:5}  switch {c['switch']:4}")
    if failed:
        print(f"{len(failed)} TUs did not parse cleanly (their counts may be incomplete): {', '.join(sorted(failed))}")

    if args.report:
        with open(args.report, "w") as out:
            out.write("# Implicit allocations (generated)\n\n")
            out.write("Generated by `python3 tools/utilities/implicit_alloc_check.py` (`clang-query` over the CMake "
                      "build's compile database). Step 1 of [memory-design.md](memory-design.md#order-of-work): "
                      "these counts are the progress measure for step 2, which works module by module until a "
                      "module reports 0. `--sites <file>` lists one file's sites.\n\n")
            out.write("* **new**: `new` whose `operator new` takes no `JKRHeap*` (it allocates from the current heap), "
                      "plus `JKRAlloc`, `JKRAllocFromHeap(nullptr, …)` and `JKRHeap::alloc(…, nullptr)`. Placement "
                      "new into a `void*` is not counted.\n")
            out.write("* **delete**: every `delete` expression.\n")
            out.write("* **switch**: every call that changes the current heap (`JKRSetCurrentHeap`, "
                      "`mDoExt_setCurrentHeap`, `JKRHeap::setCurrentHeap`, `JKRHeap::becomeCurrentHeap`).\n\n")
            out.write("A site in a header counts once. Tests, system headers and fetched dependencies are skipped.\n\n")
            out.write(f"**Total: {summary}.**\n\n")
            if failed:
                out.write(f"{len(failed)} TUs did not parse cleanly, so their counts may be incomplete: "
                          + ", ".join(f"`{f}`" for f in sorted(failed)) + ".\n\n")
            out.write("## By module\n\n| Module | new | delete | switch |\n|---|--:|--:|--:|\n")
            for m in sorted(per_module):
                c = per_module[m]
                out.write(f"| `{m}` | {c['new']} | {c['delete']} | {c['switch']} |\n")
            out.write("\n## By file\n\n| File | new | delete | switch |\n|---|--:|--:|--:|\n")
            for rel in sorted(per_file):
                c = per_file[rel]
                out.write(f"| `{rel}` | {c['new']} | {c['delete']} | {c['switch']} |\n")
        print(f"wrote {args.report}", file=sys.stderr)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
