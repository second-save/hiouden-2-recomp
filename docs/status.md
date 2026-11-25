# Status

Updated at the end of every session. Keep it to what the next session needs; history is in git.

- **Phase:** 1-4 done (recon, debug tool, identity build, extract); phase 5 (bigram font + marker build) next. Rulings D1-D8 all answered (`docs/decisions.md`).
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
  - Extract: `tools/extract.py` -> `script/*.json` (TM 128, IMMM 398, HIO 384, VIS 82 / OP_E 9 / ED_E 32 pages, MP 241); `tools/codec.py` (Big5, `\`/`^` + one byte, `[XX]`, C8D2 = №). `tests/test_extract.py`: roundtrip, padding, fields never cover a NUL/separator, no 4+ hanzi run left out of any file (waivers with reasons). TM31/TM32 drafts moved into `TM.json` en.
- **Next:**
  1. Phase 5: bigram font (A.NUM/A.CHA, 757 appendable slots + reused CJK slots) and the insert step (`tools/build.py` EDITS: pack fields by align, re-pad, copy MP names into MAP/WIND templates), then a marker build (`[id]` in every entry) and look at it with warp/msg_sheets/--vis. Map the `HIO:1E91A` attribute names there.
  2. Then phase 6 translation batches (`batches.py` from the skill).
- **Open bugs:** none. Note: `hio2/MAP0.TMP`/`WIND0.TMP` were rewritten at 10:57 on 2026-11-25, before the repo existed (a run on hio2/ itself). They are live state the game rewrites on New Game and aren't pinned; harmless, but restore them from the original install if a clean copy is ever needed.
- **Waiting on the user:** nothing.
