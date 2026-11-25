# Status

Updated at the end of every session. Keep it to what the next session needs; history is in git.

- **Phase:** 1 (recon) and 2 (debug tool) done; phase 3 (identity build) next. Rulings D1-D8 all answered (`docs/decisions.md`).
- **Done:**
  - Repo set up from the fan-translation template; guard hook proven (blocks `sed -i`); pre-commit runs `tools/check.py`; originals pinned in `rom.sha1`.
  - Towns CD holds no script (only names, graphics, scenes). Source = Chinese DOS (D5).
  - Font: `A.NUM` + `A.CHA`, 16x15, 30 bytes/glyph. **Capacity 4001 entries** (757 appendable), lookup by code, count from file size. HIOSYS title menu, OP_E opening and VIS intro all draw with it (glyph tests). Details: `docs/fonts.md`.
  - CVD text: `TM*.CVD` dialogue, ends at `\e` (max 480 bytes used). Codes: `\2..\8` colour, `\F` reset, `\n`, `\w \o \s \u` waits, `\k` key wait, `\c` clear, `\e` end. IM/MM: 8 records x 196 bytes per file (128 items, 24 spells); see `docs/re/text_inventory.md`.
  - Saves: `MAPn.HIO`/`WINDn.HIO` slots, all at the start; game writes `MAP0.TMP`/`WIND0.TMP` (smoke command now uses `work/qa/game`).
  - Recomp flags added: `--dump-every N DIR`, `--remap A=B` (served in `resolve_path`).
  - Debug warp: `tools/debug/warp.py --stage NN` boots cold, starts New Game, clicks through the VIS intro and lands on stage NN (remaps MP01.HMF). Verified on 01, 02, 08, 14.
  - **TM files hold 4 messages each (512-byte slots), 128 in all**; message N -> TM(N>>2 + 1) slot N&3. Event records and loaders: `docs/re/events.md`.
  - Debug tool (`tools/debug/warp.py`, all from a cold boot): `--stage NN`; `--msg N` (any TM message, verified on N=0); `--opening`; `--ending` (ED_E + staff roll via `--remap OP_E.EXE=ED_E.EXE`); item popup by clicks (see text_inventory). Recomp flags added: `--trace-ip`, right-click in `--test-click x,y,f,2`.
  - Width gate measured: TM box and IM/MM descriptions both 23 visible cells = **46 Latin columns** a line; `tests/test_script_width.py` enforces it on `script/en/TM*.txt`.
  - TM31/TM32 English drafts, all 4 slots each, in `script/en/` (D7), Western order, no honorifics (D6), fit 46 cols. Not inserted yet (no insert tool or bigram font yet).
  - Debug tool complete for text QA: spell popup by clicks (text_inventory); `tools/debug/msg_sheets.py` shot-lists all 128 messages in ~4 min (0, 4, 8, 49, 55, 118, 119 need `--pages 20`); `warp.py --vis N` plays VIS.EXE cutscene N (0-8) in the intro's place; `--game DIR` for parallel runs. How scenes are chosen: `docs/re/events.md`.
  - Text-site census: `docs/re/text_sites.md`. HIO.EXE text is all fixed-width padded fields (items 128x36 B, spells 24x24 B, units 48x16 B, help, menus, messages), no pointers: translate in place (verified on the title menu). VIS/OP_E/ED_E scripts: each section loaded by `mov si, imm`, so blocks can be relocated. Installers skipped.
  - Graphics survey: no Chinese drawn into any art seen (only company logos and Latin UI labels); details in text_sites.md. Phase 1 recon complete.
- **Next:**
  1. Phase 3 (identity build into `build/game/`), phase 4 (extract), phase 5 (bigram font + marker build).
- **Open bugs:** none.
- **Waiting on the user:** nothing.
