Hiouden II - English translation patch
======================================

Unofficial English fan translation of Hiouden II (緋王傳Ⅱ), the Taiwanese DOS release by Softstar (1995) of Wolf
Team's strategy RPG. Unofficial: no game data is included and no pre-patched copies are provided.

What you need
- Your own copy of the Taiwanese DOS release, installed to a folder (the one with HIO.EXE, HIOSYS.EXE, A.CHA ...).
  Base files checked by SHA-1, e.g. HIO.EXE 828d53a3c41a9c3844a6e23d90d9574cc3e8fdc1 (all in checksums.txt).
- Python 3 (no extra packages).

How to patch
    python3 apply.py <your game folder> <new folder>
This copies the whole game folder and patches the copy; your original folder is not changed. It refuses files
that are not the original ones. Each patches/<FILE>.bps is a standard BPS patch, usable with Flips or RomPatcher.js.

How to play
Tested in the Hiouden II native recomp (https://github.com/second-save/hiouden2-en, build/hiouden2
--data-dir <new folder>), from a new game. The patch only changes game data in ways the original DOS engine reads,
but it has not been tested in DOSBox or on real hardware yet.

Translation status
- Translated: all event messages, cutscenes, opening, ending and staff roll; items, spells, monsters, unit and
  floor names, item and spell descriptions; menus, dialogs, toolbar help, help pages, battle and status messages.
- Not translated: the DOS installer and sound setup (not part of the game).

Known issues
- Old saves keep their original (Chinese) unit names, which the English font can no longer draw: start a new game.
- The status popup's weakness element shows 2-letter names (Fi = fire, Ic = ice, Th = thunder, St = star).

Credits
- Translation, tools and recomp: second-save. Made with extensive AI assistance (Claude by Anthropic):
  translation, review and visual QA by AI agents, from the Chinese text; names of the Hiouden 1 characters follow
  the series' established English spellings.
- Font: Spleen by Frederic Cambus, BSD 2-Clause licence (LICENSE-spleen.txt).
- Hiouden II (c) Wolf Team / Telenet Japan; Chinese edition (c) Softstar. Not affiliated.
Support: https://ko-fi.com/second_save
