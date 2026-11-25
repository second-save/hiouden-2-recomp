# Status

Updated at the end of every session. Keep it to what the next session needs; history is in git.

- **Phase:** 1 (recon). Setup (step 0) done 2026-11-25. No build yet.
- **Done:**
  - Repo set up from the fan-translation template; guard hook proven (blocks `sed -i`); pre-commit runs `tools/check.py`; originals pinned in `rom.sha1`.
  - Towns CD holds no script (only names, graphics, scenes; data ends at LBA 1452, rest is zero padding). Source = Chinese DOS (D5).
  - Font: `A.NUM` = 3244 Big5 codes (unsorted), `A.CHA` = 3244 glyphs, 16x15, 30 bytes, 2 bytes/row big-endian. Only `HIOSYS.EXE` opens it. Proven: editing the glyph for 重 changes the title menu (`work/fonttest.png`).
  - CVD text: `TM*.CVD` 2048-byte dialogue files, text ends at `\e` (max 480 bytes used; tail is junk). Codes: `\2..\8` colour, `\F` reset, `\n`, `\w \o \s \u` waits, `\k` key wait, `\c` clear, `\e` end. `IM*/MM*.CVD` are fixed-width records (name padded to 25 full-width cols, then description rows); `CD/EN/GR*.CVD` are graphics, not text.
- **Next:**
  1. Confirm the cutscene EXEs (`VIS.EXE`, `OP_E.EXE`, `ED_E.EXE`) draw with `A.CHA` too (warp or play to the opening, same glyph test).
  2. Learn how HIOSYS maps a Big5 code to a glyph (linear search of A.NUM? count from file size?) to know if glyphs can be appended or only reused.
  3. Map IM/MM record layout; list every text site (CVD, EXE string tables + their pointers/fixed fields, HMF/WIND/MAP name slots).
  4. Phase 2 debug tool: a way to reach any TM/IM/cutscene from a cold boot in the recomp (e.g. a `--warp` flag or env var that loads a given CVD into the message window).
- **Open bugs:** none.
- **Waiting on the user:** D6-D8 in `docs/decisions.md` (honorifics, credits, approval mode).
