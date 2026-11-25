# Fonts

## Game font (`hio2/A.NUM` + `A.CHA`)
- `A.NUM`: 3244 big-endian Big5 codes, unsorted. `A.CHA`: one 16x15 glyph per code, same order, 30 bytes (15 rows x 2 bytes, MSB = leftmost pixel).
- Loaded only by `HIOSYS.EXE`; the title menu draws through it (glyph test, 2026-11-25). The opening (`OP_E.EXE`,
  run as an INT 8Eh overlay inside HIOSYS) draws with it too: verified 2026-11-25 by `tools/poke_glyph.py` on 王國,
  shot at frame 1750 after clicking the third title entry (`--test-click 300,207,1450`).
- **Capacity: 4001 entries (slots 0-4000), counted from the file size.** Verified 2026-11-25 with
  `tools/append_glyph_test.py`: 王's code moved to an appended slot draws from that slot up to slot 4000; slot 4001+
  draws blank (no crash). So 757 slots can be appended to the original 3244, and any slot whose Chinese glyph the
  English build no longer uses can be reused. Lookup is by code, so A.NUM order doesn't matter.

## English plan (decision D4)
- Two 8 px Latin letters per 16 px glyph slot ("bigram" glyphs): English is encoded as Big5 codes whose glyphs show letter pairs, giving ~50 Latin characters per line with no engine change.
- The bigram set is built from the translated script (`tools/bigram.py`, `tools/insert.py`): the new A.NUM keeps
  every symbol/icon code (lead byte below 0xA4, 1,279 entries) and any Chinese glyph still drawn, then one slot per
  letter pair (~950 pairs; 2,197 of 4,001 slots used, 2026-11-25). Pair codes are Big5-shaped (lead A4-F9) and
  never reuse a code the original font knows; trails 5B/5C/5E are skipped.
- Latin source font: **Spleen 8x16** 2.3.0 by Frederic Cambus, BSD 2-Clause, `font/spleen-8x16.bdf` +
  `font/LICENSE-spleen` (from github.com/fcambus/spleen, 2026-11-25). Rows 1-15 of each glyph (row 0 is blank for
  every printable letter). The licence ships in the release zip.
