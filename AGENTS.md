# Hiouden II (Taiwanese DOS release, via the native recomp) English translation

Skill: translate-retro-game (`~/.claude/skills/translate-retro-game/SKILL.md`; `python3 tools/skill.py` prints it). Read it before text or QA work.
The game runs in this repo's own recomp (`src/`, CMake, binary `build/hiouden2`); there is no emulator.

## Commands (run from repo root)
- Recomp:   `cmake --build build` -> `build/hiouden2`
- Check:    `python3 -X utf8 tools/check.py` (all tests in `tests/`; the pre-commit hook runs it; `git config core.hooksPath tools/hooks`)
- Dump:     `python3 tools/check_rom.py` (original files vs `rom.sha1`)
- Smoke:    `cp -R hio2 work/qa/game` once, then `build/hiouden2 --data-dir work/qa/game --headless-test 1500 --dump-frame work/qa/smoke.bmp` (title menu ~frame 1400; the game writes MAP0.TMP/WIND0.TMP, so never `--data-dir hio2`)
- Click:    add `--test-click x,y,frame` (640x400 coords); `--trace-overlays` logs file I/O; `--dump-every N DIR`; `--remap A=B` serves file B when the game opens A
- Warp:     `python3 tools/debug/warp.py --stage NN --out DIR` (cold boot -> New Game on MPNN.HMF; extra `--remap`, `--click x,y,f[,btn]`); `--msg N --every 150` shows TM message N; unknown flags (`--trace-ip LINEAR`) pass to the recomp
- Disasm:   `.venv/bin/python3 tools/dis16.py hio2/HIO.EXE <file offset>` (`python3 -m venv .venv && .venv/bin/pip install capstone`)
- Images:   `magick` / `montage` (no PIL here)
- Build/Release: not written yet (phase 3+)

## Paths
- Original game: `hio2/` (Taiwanese DOS install, Big5; gitignored, pinned in `rom.sha1`). Never write into it: builds go to `build/game/`.
- FM Towns disc (Japanese names only): `Hioden-II_FM-Towns_JA/`; `tools/extract_towns_cd.py` -> `extracted/` (gitignored).
- Font: `hio2/A.NUM` (Big5 code list) + `A.CHA` (16x15 glyphs, 30 bytes each, same order). Small set: `T_WK1.*`.
- RE notes: `docs/re/` (`text_inventory.md`, `file_table.md`, `events.md`, `japanese_names.md`). Glossary: `glossary.md`. Fonts: `docs/fonts.md`.

## Rulings
- Edit source files only with the file-edit tool; scripts read and analyse, never `.replace()` and write back.
- Python is `python3` here (no `python` on this Mac).
- Test runs must never touch the user's saves: run QA on a copy of the game dir, never `hio2/` itself.
- Subagents: use the skill's translator / reviewer / visual-qa briefs (`$SKILL/assets/*-brief.md`).
- User decisions with options and reasons: `docs/decisions.md`. Read before asking; add new rulings the same session.

## Status
`docs/status.md`: done, next, open bugs. Update it at the end of every session.
