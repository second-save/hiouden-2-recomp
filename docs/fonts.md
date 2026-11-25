# Fonts

## Game font (`hio2/A.NUM` + `A.CHA`)
- `A.NUM`: 3244 big-endian Big5 codes, unsorted. `A.CHA`: one 16x15 glyph per code, same order, 30 bytes (15 rows x 2 bytes, MSB = leftmost pixel).
- Loaded only by `HIOSYS.EXE`; the title menu draws through it (glyph test, 2026-11-25).

## English plan (decision D4)
- Two 8 px Latin letters per 16 px glyph slot ("bigram" glyphs): English is encoded as Big5 codes whose glyphs show letter pairs, giving ~50 Latin characters per line with no engine change.
- The bigram set is built from the translated script; slots come from glyphs the English build no longer needs.
- Latin source font: not chosen yet. Record the font, its source and licence here before shipping.
