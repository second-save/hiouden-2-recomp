# Hiouden II English translation

Unofficial English fan translation of **Hiouden II** (緋王傳Ⅱ, the Taiwanese DOS release by Softstar of Wolf Team's
*Hiouden II: Mamono-tachi tono Chikai*, 1995): the source text, the tools that build the patch, and a native
recompilation (`src/`) that runs the DOS game without an emulator.

**Just want to play?** Download the patch from [Releases](../../releases) and follow its README. This repository holds
no game data; you need your own copy of the Taiwanese DOS release.

**Found a bug?** [Report it](../../issues/new/choose): untranslated text, typos, overflow, crashes.

## Translation status

| Part | Status |
|---|---|
| Story: event messages (128), cutscenes (9 scenes), opening and ending | Translated |
| Items, spells, monsters, map unit names, floors, stat tags, item and spell descriptions | Translated |
| Menus, dialogs, toolbar help, in-game help pages, battle and status messages | Translated |
| Staff roll and the developers' easter-egg notes | Translated |
| Title screen and logos | No text to translate (company logos only) |
| DOS installer and sound setup (`INSTALL.EXE`, `SETUP.EXE`) | Left in Chinese (not part of the game) |

Known issues:
- The status popup's weakness element uses 2-letter names (Fi, Ic, Th, St): the game keeps them in 2-byte slots.
- Saves made before patching keep their original unit names; start a new game after patching.

## How it works

The game draws text as 16x15 Big5 cells. English is encoded as letter pairs: each pair gets a Big5 code whose glyph
shows the two letters (Spleen 8x16), so a line holds 46-50 Latin characters with no change to the text engine. The
patch also widens the event-message slots (1024 bytes), relocates the cutscene/opening/ending scripts and their
pointers, and rebuilds the font files. Details: `docs/fonts.md`, `docs/re/`, `docs/decisions.md`.

## What's here

| Path | What it is |
|---|---|
| `script/` | Every text block: original Chinese (`zh`) and English (`en`). Translation fixes go here. |
| `glossary.md` | Binding names, terms and style rules. |
| `tools/` | Python build chain: extract, insert, batches, QA tour, release. |
| `tests/` | unittest suite (game-dependent tests skip when the game files are absent). |
| `src/` | The native recomp of the DOS game (CMake). |
| `docs/re/` | Reverse-engineering notes. |
| `docs/decisions.md`, `docs/status.md` | Rulings with their reasons; current progress and open bugs. |
| `docs/fonts.md` | Fonts used and their licences. |
| `AGENTS.md` | Instructions for coding agents (`CLAUDE.md` points to it). |

## Build your own patched game

Needs Python 3.12+, CMake and a C++ compiler, and your own copy of the Taiwanese DOS release.

```sh
git clone https://github.com/second-save/hiouden2-en
cd hiouden2-en
# copy the game folder to: hio2/   (the build refuses other versions, see rom.sha1)
cmake -B build && cmake --build build                  # -> build/hiouden2 (the recomp)
python3 tools/build.py                                 # -> build/game (patched game files)
python3 -X utf8 tools/check.py                         # tests + text lint
build/hiouden2 --data-dir <a copy of build/game>
```

## Support

If you enjoy this translation, you can support my work on Ko-fi:

[![Support me on Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/second_save)

## Licence

Tools and English translation: MIT. The font is Spleen by Frederic Cambus (BSD 2-Clause, `font/LICENSE-spleen`).
The original Chinese text in `script/` belongs to its publishers and is included only so the translation can be
checked against it. Hiouden II is (c) Wolf Team / Telenet Japan, Chinese edition (c) Softstar; this project is not
affiliated with them. Do not share patched game files.

The translation was made with extensive AI assistance (Claude by Anthropic): translation, review and visual QA by
AI agents, from the Chinese release. Corrections are very welcome.
