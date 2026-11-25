# Script

One JSON file per block, written by `python3 tools/extract.py` (never by hand, except `en` and `notes`; `--force`
re-dumps and keeps them by id). `tests/test_extract.py` checks the dump against `hio2/`: every `zh` re-encodes to
the original bytes at its id, the rest of each field is padding, and fields never swallow a NUL/separator byte.

| File | What | Entries |
|---|---|---|
| `TM.json` | event messages, 4 x 512-byte slots per TM file (`\` codes, ends `\e`) | 128 |
| `IMMM.json` | item/spell popups: name, stat tag (right-aligned), description (3 rows, `\n` between rows) | 398 |
| `HIO.json` | HIO.EXE fixed fields: item/spell/unit/floor names, menus, dialogs, help, messages | 384 |
| `VIS.json`, `OP_E.json`, `ED_E.json` | cutscene, opening, ending + staff roll scripts, one entry per page | 82 / 9 / 32 |
| `MP.json` | per-map unit names (MPnn.HMF 0x1A40, 16-byte slots) | 241 |

Entry: `id` = `FILE:offset` (EXE image offset or file offset), `kind`, `zh` (source), `field` = [start, end) bytes the
English may fill, `align` for fixed fields, `section` on script pages a `mov si` points at, `en`, `notes`.
Budget: one Big5 cell = 2 bytes = 2 Latin letters with the bigram font, so `field` length in bytes = letters.

Text notation (`tools/codec.py`): Big5 as characters, `\x` and `^x` codes inline (each takes one byte), `[XX]` for a
byte that isn't text (e.g. `^[01]` = `^` + 0x01, a staff-roll position code). `№` is the game's C8D2 glyph.

Notes on content:
- TM31: Wolf Team staff notes by the planner MKG (easter egg). `淺沼` Asanuma, `稻毛` Inage, `小澤` Ozawa, `三宅`
  Miyake, `磯谷` Isogai, `櫻庭` Sakuraba (composer Motoi Sakuraba), `阿廣` Hiro; `狼組` = Wolf Team; honorifics
  `先生`/`叔叔` dropped (D6). Miyake's `聽寫繞跑的ＡＩ` is read as `聽說寫繞路的ＡＩ` ("writing the pathfinding AI").
- TM32: slot 0 sign `努力加奮鬥`; slots 1-2 "the floor to the north/west changed"; slot 3 = TM31 slot 3.
- TM30 slots 2-3 (not translated yet) are MKG's intro to TM31, and they give a cheat: on the first stage, hold CTRL and
  the right mouse button from choosing New Game until the game starts, and every monster joins your side.
- `HIO:1E91A` (attr) holds 2- and 4-byte attribute names whose boundaries are unknown: map them in the marker build.
- `WIND1-A.INI/HIO` and the unit records in `MAPn.HIO/INI`/`MAP0.TMP` (name at +0x70) repeat MP unit names (shipped
  save templates): insert maps them by source text, so they have no entries here.
- HIO.EXE spells: 22 in a 24-byte table, then 銀龍召喚術 and a second 死靈召喚術 packed without data bytes.
