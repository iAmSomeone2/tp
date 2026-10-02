# Twilight Princess decompilation – orientation guide

High-level documentation for people (and coding agents) who are new to this codebase. It explains *what is where*, *how the engine hangs together*, and *what the internal names mean*. It deliberately does not try to document every class; it aims to get you to the right file, and to the right question, quickly.

> These pages were written by reading the source. Where a claim is a community guess rather than something the code proves, it says so. If something here disagrees with the code, the code wins – please fix the doc.

## Reading order

| # | Page | Read it when you want to… |
|---|------|---------------------------|
| 1 | [01-project-overview.md](01-project-overview.md) | Understand what the repo is, how it builds, what "matching" means, and what the version codes (`GZ2E01`, `RZDE01_00`, `ShieldD`…) are. |
| 2 | [02-source-layout.md](02-source-layout.md) | Find your way around the directory tree, the file-name prefixes (`f_pc_`, `d_a_`, `m_Do_`, `J3D…`), and which code is in the DOL vs. in RELs. |
| 3 | [03-engine-architecture.md](03-engine-architecture.md) | See how the game boots, what a frame looks like, and how processes, scenes, actors, layers and dynamic modules work. **This is the most important page.** |
| 4 | [04-game-systems.md](04-game-systems.md) | Get a one-section overview of each subsystem: player, collision, camera, targeting, events/cutscenes, stages/rooms, save data, environment, rendering, UI, messages, audio, input… |
| 5 | [05-glossary.md](05-glossary.md) | Decode an internal name: prefixes and suffixes, Japanese/romaji words, actor abbreviations (`e_ba`, `npc_ins`, `lv4…`), stage codes (`F_SP103`), file extensions. |
| 6 | [06-exploring.md](06-exploring.md) | Follow worked examples ("how do I find where X happens?"), grep recipes, and a list of good questions to ask next. |
| 7 | [port/README.md](port/README.md) | Plan a native PS Vita port of this fork: SDK/hardware surface, endianness, graphics (GX → GXM), toolchain, roadmap and risks. Includes generated survey data. |
| – | [actor-index.md](actor-index.md) | Look up any of the ~770 actor source files: process name, DOL/REL, in-game name. (Generated.) |

Existing notes that pre-date this guide: [re_notes.md](re_notes.md) (class sizes and naming TODOs) and [rels_sha1.md](rels_sha1.md) (hashes of the original RELs). [mainpage.h](mainpage.h) feeds the Doxygen site.

## The 60-second version

* The repo turns C++ source into a **byte-identical rebuild** of the GameCube/Wii game binaries. It contains no game data; you supply your own disc image.
* The game is a **process-based engine**. Everything alive (scenes, actors, the camera, the HUD, even the message box) is a *process* created from a *profile*, scheduled by the `f_pc` framework, and typed by the `f_op` layer as scene / actor / camera / kankyo (environment) / message / overlap (screen wipe).
* Game code lives in namespaced-by-prefix files: `f_` framework, `m_Do_` machine-dependent glue ("Do" = the Zelda engine layer over the SDK), `d_` "Dolzel" game logic (`d_a_*` are actors), `c_` SComponent utilities, `J*` Nintendo's JSystem middleware, `Z2*` sound, plus the Dolphin/Revolution SDK and Metrowerks runtime.
* About 770 actor files are compiled as separate **RELs** (relocatable modules) loaded on demand; the player (`d_a_alink`) and the core systems are in the main **DOL**.
* One source tree builds **all** versions (GCN/Wii/Shield, USA/PAL/JPN, retail/debug) using `#if VERSION == …` / `PLATFORM_*` / `DEBUG` conditionals.

## Conventions used in these docs

* Paths are relative to the repository root and clickable; `src/d/…` is the implementation, `include/d/…` the header. `libs/` mirrors this for SDK/middleware (`libs/JSystem/src`, `libs/JSystem/include`).
* "TU" = translation unit (one `.cpp`/`.c` file that compiles to one object).
* Numbers such as file counts were measured on the checkout these docs were written against and will drift.
