#!/usr/bin/env python3

###
# Regenerates docs/actor-index.md: a table of every actor translation unit
# (src/d/actor/d_a_*.cpp) with its process name(s), whether it is a REL or lives
# in the DOL, and the human-readable name from its header's @brief.
#
# Usage (from anywhere; paths are resolved relative to the repository root):
#   python3 tools/utilities/gen_actor_index.py
###

import collections
import glob
import os
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
os.chdir(ROOT)

# Actors registered with ActorRel(...) in configure.py are separate RELs.
rels = set(re.findall(r'ActorRel\([^"]*"(d_a_[A-Za-z0-9_]+)"', open("configure.py").read()))


def brief(hdr):
    """First @brief in the actor's header, or ''."""
    if not os.path.exists(hdr):
        return ""
    m = re.search(r"@brief\s+(.+)", open(hdr, errors="replace").read())
    if not m:
        return ""
    b = re.sub(r"\s+", " ", m.group(1).strip())
    if b in ("???", "") or b.startswith("@"):
        return ""
    return b.replace("|", "/")


def category(name):
    s = name[len("d_a_"):]
    if s.startswith("b_"):
        return "Bosses"
    if s.startswith("e_"):
        return "Enemies"
    if s.startswith("npc"):
        return "NPCs and creatures"
    if s.startswith(("tag_", "kytag")):
        return "Tags (invisible triggers / controllers)"
    if s.startswith("obj_"):
        return "Objects"
    if s.startswith("door"):
        return "Doors"
    if s.startswith("mg_"):
        return "Minigames"
    if s.startswith("alink"):
        return "Player"
    return "Misc actors"


ORDER = [
    "Player",
    "Bosses",
    "Enemies",
    "NPCs and creatures",
    "Objects",
    "Doors",
    "Tags (invisible triggers / controllers)",
    "Minigames",
    "Misc actors",
]

groups = collections.defaultdict(list)
for f in sorted(glob.glob("src/d/actor/d_a_*.cpp")):
    name = os.path.basename(f)[:-4]
    src = open(f, errors="replace").read()

    profiles = []
    for _, prof, body in re.findall(
        r"(\w*process_profile_definition2?)\s+(g_profile_\w+)\s*=\s*\{(.*?)\n\};", src, re.S
    ):
        m = re.search(r"Proc Name\s*\*/\s*(fpcNm_\w+)", body)
        profiles.append((prof[len("g_profile_"):], m.group(1) if m else ""))

    desc = brief(f"include/d/actor/{name}.h")
    if not desc:  # fall back to the first descriptive comment line of the .cpp
        for line in src.splitlines()[:8]:
            m = re.match(r"^ \* (.+)$", line)
            if m and not m.group(1).startswith(("@", name)) and ".cpp" not in m.group(1):
                desc = m.group(1).strip().replace("|", "/")
                break

    groups[category(name)].append((name, profiles, desc, "REL" if name in rels else "DOL"))

out = [
    "# Actor index (generated)\n",
    "Every `src/d/actor/d_a_*.cpp` translation unit, with its process name(s), where it lives at runtime, and a human-readable name.\n",
    "- **Profile** – the `g_profile_*` symbol defined in the file; **Process name** – the `fpcNm_*_e` enum entry (see [`include/f_pc/f_pc_name.h`](../include/f_pc/f_pc_name.h)) used to spawn it.",
    "- **Runs from** – `REL` = separate relocatable module loaded on demand (`ActorRel(...)` in `configure.py`); `DOL` = part of the always-resident main executable.",
    "- **In-game name** – taken from the `@brief` in the actor's header when one exists. These are community-written and some are guesses (marked `?` in the source) or wrong. Blank means nobody has written one yet. Treat as a hint, then confirm in the code (resource names, sound IDs `Z2SE_*`, message text).",
    "- Regenerate with `python3 tools/utilities/gen_actor_index.py` (see [06-exploring.md](06-exploring.md#regenerating-the-actor-index)).\n",
    f"Total: {sum(len(v) for v in groups.values())} actor TUs.\n",
]
for g in ORDER:
    rows = groups.get(g, [])
    if not rows:
        continue
    out.append(f"\n## {g} ({len(rows)})\n")
    out.append("| Source | Profile → process name | Runs from | In-game name |")
    out.append("|---|---|---|---|")
    for name, profiles, desc, where in rows:
        p = "<br>".join(f"`{a}` → `{c}`" if c else f"`{a}`" for a, c in profiles) or "—"
        out.append(f"| [`{name}`](../src/d/actor/{name}.cpp) | {p} | {where} | {desc} |")

Path("docs/actor-index.md").write_text("\n".join(out) + "\n")
print("wrote docs/actor-index.md:", {g: len(v) for g, v in groups.items()})
