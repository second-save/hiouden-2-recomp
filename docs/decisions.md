# Decisions

The user's rulings on scope, style and trade-offs, newest last. Read before asking the user anything; if a later
decision replaces one, mark the old one "replaced by D<n>" instead of deleting it. Binding term spellings live in
`glossary.md`; this file holds the why.

| # | Date | Question | Options offered | Decision and reason |
|---|---|---|---|---|
| D1 | 2026-11-25 | Source script | Japanese Towns; Chinese DOS; both with JP primary | Japanese Towns chosen, **replaced by D5** |
| D2 | 2026-11-25 | Target language | English; French | English |
| D3 | 2026-11-25 | Romanization source for names | Aeon Genesis Hiouden 1 patch (SNES, 2005); fresh romanization | Aeon Genesis spellings for returning characters, Hepburn from the Japanese katakana for new ones, so the two games match |
| D4 | 2026-11-25 | English font | 8 px half-width / VWF; full-width glyphs | 8 px half-width. Implemented as two Latin letters per 16x15 glyph slot in `A.CHA` (see `docs/fonts.md`), so no engine change and the patch also runs in the original DOS game |
| D5 | 2026-11-25 | Source script, after the Towns CD turned out to hold no dialogue (only names, graphics, scenes; the script was on the HDD install / PC-98 disks, which we don't have) | find a PC-98/floppy image; translate from Chinese | Translate from the Taiwanese DOS Big5 script. Use the Japanese wherever it survives: unit/monster names (Towns `H2MF/MAP*.LZ5`, `hio2/WIND0.HIO`), opening narration (`hio2/VIS4.EXE`) |
| D6 | 2026-11-25 | Honorifics and name order | keep, drop; JP or Western order | No honorifics, Western name order (given name first) |
| D7 | 2026-11-25 | Credits (TM31/TM32 Taiwanese dev notes, staff roll) | keep, translate | Translate TM31 and TM32 into English |
| D8 | 2026-11-25 | Specs and plans | need approval, pre-approved | Pre-approved: keep going through checkpoints, report at the end |
| D9 | 2026-11-25 | Main names (taken under D8, user may override) | literal Hepburn (Marukufiindo, Miirurei); readable forms | Readable forms of the katakana: Markfiend (マルクフィーンド, short 馬克 = Mark), Mirlei (ミィルレイ), Lezard Valeth (レザードヴァレス, as Valkyrie Profile spells the same katakana), Jadain (ジェイダイン); returning: Richard A. MacIntyre, Beatrix (Wikipedia; the AGTP page is offline) |
| D10 | 2026-11-25 | Long TM messages (English ~1.75x the Big5 bytes) | cut to 512 B; patch slots | Patch HIO.EXE (image 0x17AED: `and ax,0x300; shl ax,2`) for 1024-byte slots, TM files become 4 KB; verified in-game on slots 2 and 3 |
| D12 | 2026-11-25 | 王兄 in the ending (taken under D8, user may override) | "The King's Brother"; "The King"; neutral | "Royal Brother:" as the speaker tag: the source never says he is the king, and "The King's Brother" would deny it; keeps the hint |
| D13 | 2026-11-25 | Hero's name (overrides D9) | Markfiend; Mark | User: "just Mark is enough". Mark everywhere, speaker tags and unit name included |
| D11 | 2026-11-25 | Latin font | macOS system font; game glyphs; free bitmap font | Spleen 8x16 (BSD-2-Clause), rows 1-15, two letters per cell; licence in `font/LICENSE-spleen`, ships with the patch |
