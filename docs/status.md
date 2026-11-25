# Status

Updated at the end of every session. Keep it to what the next session needs; history is in git.

- **Phase:** 1 (recon) mostly done; phase 2 (debug tool) started. Rulings D1-D8 all answered (`docs/decisions.md`).
- **Done:**
  - Repo set up from the fan-translation template; guard hook proven (blocks `sed -i`); pre-commit runs `tools/check.py`; originals pinned in `rom.sha1`.
  - Towns CD holds no script (only names, graphics, scenes). Source = Chinese DOS (D5).
  - Font: `A.NUM` + `A.CHA`, 16x15, 30 bytes/glyph. **Capacity 4001 entries** (757 appendable), lookup by code, count from file size. HIOSYS title menu, OP_E opening and VIS intro all draw with it (glyph tests). Details: `docs/fonts.md`.
  - CVD text: `TM*.CVD` dialogue, ends at `\e` (max 480 bytes used). Codes: `\2..\8` colour, `\F` reset, `\n`, `\w \o \s \u` waits, `\k` key wait, `\c` clear, `\e` end. IM/MM: 8 records x 196 bytes per file (128 items, 24 spells); see `docs/re/text_inventory.md`.
  - Saves: `MAPn.HIO`/`WINDn.HIO` slots, all at the start; game writes `MAP0.TMP`/`WIND0.TMP` (smoke command now uses `work/qa/game`).
  - Recomp flags added: `--dump-every N DIR`, `--remap A=B` (served in `resolve_path`).
  - Debug warp: `tools/debug/warp.py --stage NN` boots cold, starts New Game, clicks through the VIS intro and lands on stage NN (remaps MP01.HMF). Verified on 01, 02, 08, 14.
  - TM31/TM32 English drafts in `script/en/` (D7), Western order, no honorifics (D6). Not inserted yet.
- **Next:**
  1. Debug tool, TM messages: find a TM trigger close to a stage start (walk by `--click` on the map) or the TM display call in HIO.EXE, then `--remap TMxx.CVD=TMyy.CVD` shows any TM. Same for the item window (IM) and spell window (MM), and the ending (ED_E.EXE: find how it is launched; overlay ID like OP_E's 0x4410).
  2. Item window: measure IM/MM field widths in-game (layout is still a guess).
  3. List every remaining text site: HIO.EXE strings (279 runs; menus, window help, save list `記錄`/`地下牢B6F-1`) and how they are addressed (fixed fields or pointers); OP_E/ED_E/VIS `^`-coded scripts; HMF/WIND name slots.
  4. Graphics-text survey (GR*.CVD, title logo, window art).
  5. Then phase 3 (identity build into `build/game/`) and phase 4 (extract).
- **Open bugs:** none.
- **Waiting on the user:** nothing.
