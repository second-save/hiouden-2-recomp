# Translator brief - Hiouden II (DOS, Taiwanese release, 1995)

You translate part of the Traditional Chinese script of Hiouden II into English for a fan patch. It is a dark
real-time strategy RPG by Wolf Team: Markfiend, a condemned prisoner who can bind demons, escapes a dungeon with
Mirlei, a girl jailed as a witch, fighting through a dungeon of 21 floors to the court sorcerer Lezard Valeth. The
Chinese is itself a translation of the Japanese original, so Japanese-flavoured phrasing is normal.
Work only on the batches named in your dispatch, one at a time in the order given, each to a passing check before
the next. Do all translation yourself; do not dispatch subagents. Work from the repo root.

## Read first
1. `docs/style-reference.md` - character voices and example pairs (if it exists yet).
2. `build/terms_<batch>.txt` - binding glossary rows for this batch; the full table is `glossary.md`.
3. `docs/synopsis.md` - plot and who is who (if it exists yet). Don't give later reveals away early.

## Input: `work/tl/<batch>.tsv`
`id<TAB>speaker<TAB>form<TAB>limit<TAB>source`. The source is Traditional Chinese. `form` is the entry kind:
- `message` (TM*): event dialogue in a 46-column box. Keep the codes: `\2`..`\8` start a colour, `\F` ends it,
  `\e` ends the message (must be last). You may move or add the layout codes `\n` (new line), `\u` (new line),
  `\k` (wait for a click), `\c` (clear box), `\w \o \s` (short pauses): put `\n` where the English line must break
  so no line exceeds 46 columns. A line starting with a full-width space in the source is indented: use 1-2 ASCII
  spaces. Speaker headers `\2【name】\F` become `\2Name:\F` (no brackets).
- `script` (VIS/OP_E/ED_E): cutscene, opening and ending pages. Codes: `^0` new line (movable), `^$` page break
  (movable), `^@` next page and `^\` end of scene (both must stay, at the end), `^%` before a speaker name (keep it
  where it is: `^%Markfiend:^0  "Text"`), `^[01]` `^[02]` `^[03]` staff-roll positions (keep exactly). Line limit is
  in the `limit` column.
- `item_desc` / `spell_desc`: write one plain paragraph; the tool wraps it into the popup's 3 rows of 46 columns.
  The source shows its rows joined by ` / `; do not copy the slashes.
- names and labels (`item`, `item_name`, `item_tag`, `spell*`, `unit`, `unit_name`, `floor`, `label`, `dialog`,
  `menu`, `toolhelp`, `help`, `message` from HIO): fixed fields; `limit` is the maximum in columns. Many share one
  row for all their copies in the game; the limit shown is the tightest of them.
The `limit` is hard (the text is drawn in fixed cells). Columns = letters and spaces, ASCII only.

## Output: `work/tl/<batch>.out.tsv`, written with your file-write tool (not a shell heredoc)
`id<TAB>English`, every input id exactly once, same order. Plain ASCII: straight quotes, `...`, `--`; no tabs, no
Chinese or full-width characters, no `[ ] \ ^` except inside the codes listed above, no `\uXXXX`.

## Validate
Run `python3 -X utf8 tools/batches.py check <batch>` and fix every reported id until it prints OK.

## Style
- Faithful: keep the information and order of ideas; adapt idiom, not meaning. Don't add details the source doesn't
  state; don't summarise or skip. Adapt wordplay and the developers' jokes so they still land (no translator notes).
- Keep the speaker audible: Markfiend curt, sarcastic, world-weary; Mirlei gentle, earnest, stubborn; guards and
  jailers coarse; Jadain sneering and theatrical; Lezard cold and grandiose; old men folksy. A line any speaker
  could have said is a mistake.
- Names and terms exactly as in the glossary. No honorifics; Western name order. Never abbreviate to fit unless the
  glossary gives a short form: if a fixed field can't hold the meaning, use the most natural short wording and report
  `LONG: id`.
- Item, spell and monster names: Title Case, concise, classic fantasy RPG register (Fireball-style spell names are
  fine: 火燄術 -> Fire, or Flame; pick one pattern and keep it for the whole set). Stat tags like
  `【殺傷力　１２】` -> `[ATK 12]` is NOT allowed (brackets): use `ATK 12`, `DEF 7`, `MP 5`, `Vigor+50+a`.
- Punctuation: 「」 -> "...", 『』 -> '...', …… -> ..., ！？ -> !?.

## Consistency
Never edit `glossary.md`. For a recurring name or term not in it, pick a form, use it consistently, and report it.

## Report (reply with only this)
Files written and entry count; then any lines: `NEW TERM: source -> en`, `UNSURE: id - note`, `LONG: id - note`.
