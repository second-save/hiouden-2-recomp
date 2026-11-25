# Text sites: where every string lives and how the game reaches it

Census of 2026-11-25 (Next item 1 in status). Offsets are **image** offsets (file minus the MZ header: HIO.EXE
0x1800, OP_E/ED_E/VIS 0x200). "In place" = rewrite inside the same bytes; with the bigram font one Big5 cell
(2 bytes) holds 2 Latin letters, so the byte budget of a field is also its letter budget.

## Already mapped elsewhere
| Site | Form | Addressing | Budget | Notes |
|---|---|---|---|---|
| `TM01-32.CVD` | 128 messages, `\`-codes | file + slot number (events.md) | 512 bytes/slot; 46 cols/line | width test exists |
| `IM01-16.CVD`, `MM01-03.CVD` | 196-byte records | file + record number | name 24 cells, desc 3x46 cols | text_inventory |
| `MPnn.HMF` 0x1A40, `WIND*.HIO/INI` 0x122C | 25 x 16-byte unit names | fixed slots | 8 cells = 16 letters | text_inventory s.2 |

## HIO.EXE (main game) — all fixed fields, no pointers found
A scan for `mov r16, imm16` / `push imm16` under every paragraph base found no consistent pointers to the
NUL-terminated pools (best base matched 5 of 14 strings). In-place edits work: swapping `重新開始遊戲` (image 0x18DC2)
for six other cells in a scratch copy showed on the title menu at frame 1450 (2026-11-25, work/qa/ip.png). The strings are padded with U+3000 to
fixed widths and read by index or offset arithmetic, so: **translate in place, same byte length, pad with
spaces**. Where a few have direct refs (DS = image 0x10000 base, e.g. `咒語：` 0x1E8F1 from code 0xF117,
`嗜食：` 0x1E914 from 0x6401, menu 0x18DBF from 0x9916) relocation is possible but not needed.

| Image offset | Content | Layout | Budget |
|---|---|---|---|
| 0x0E046 | spell names (24) | 24-byte records: 18-byte name + 6 data bytes | 9 cells = 18 letters |
| 0x0E9BB | `沒有` | fixed | |
| 0x0EAF6, 0x18AC3 | digit/underscore glyph strips | font-ish, likely keep | |
| 0x0FFA7 | `ＧＡＭＥ　ＯＶＥＲ` | full-width Latin | can become half-width |
| 0x12357 | `大召喚ＦＬＯＯＲ－` + digits | fixed | |
| 0x14152 | `無法留在魔界！` / `鑰匙不可以留在魔界。` | NUL-separated | |
| 0x15576-0x15B00 | window-layout save/load dialog, item settings (`物品環境設定`, 自動/手動), restart-scenario prompt | padded runs | |
| 0x18202 | save/load prompts (`要儲存目前的遊戲進度嗎？` 儲存 讀取 是 否) | padded | |
| 0x18DBF | title + record menu (`重新開始遊戲` `繼續上次進度` `玩者資料格式化` `開頭畫面展示` `記錄`), format confirm, `儲存進度中•••` | padded | |
| 0x192F2-0x19980 | toolbar help: 39 NUL-separated strings (titles 10 cells, lines 17 cells), some entries 2-3 lines | fixed 20/34-byte fields, `\0` between | 20 / 34 letters |
| 0x19FAF-0x1A4EE | `■魔界ＨＥＬＰ■` help pages (chest/scroll/crystal, unit formation, contract limit) | 50-cell (100-byte) rows | |
| 0x1A39C | 21 floor names `地下牢Ｂ６Ｆ` ... `地下神殿` (one per MPnn) | 8-cell slots | 16 letters |
| 0x1A4EF-0x1B4A0 | **bopomofo character picker**: initials row, `記號 第二 插入 削除`, then 1,425 hanzi grouped by initial, full-width symbols and A-Z/a-z | IME table | see open question |
| 0x1CF70 | unit/monster type names (48) | 16-byte slots | 8 cells = 16 letters |
| 0x1D538 | item names (128) | 36-byte records: 20-byte name + 16 data bytes | 10 cells = 20 letters |
| 0x1E740 | battle/status messages (`死亡了！` `和敵人接觸了！` `提昇到 等級了！！` `〔能力上昇〕` summon prompts, ...) | one 522-byte padded run | per field |
| 0x1E8F1-0x1E914 | status popup labels `咒語：物品：抵抗：特殊：弱點：嗜食：` + `沒有 毒 石化 混亂 火 冰 雷 星 全部` | 6-byte labels | 3 cells = 6 letters |
| 0x1E94B-0x1EA7F | 15 error popups (`ＭＰ不足！！` ... `沒有什麼效果！`) | NUL-separated, padded | per string |

## Cutscene scripts — pointer-addressed, relocatable
`^` codes (meanings read off the screenshots, not traced): `^0` newline, `^@` next page, `^\` end of section,
`^%` before a speaker name, `^$` page break.
Each section start is loaded by `mov si, imm16; call <draw>`; pages inside a section are read in sequence.
| File | Text block (image) | DS base | Section pointers |
|---|---|---|---|
| `VIS.EXE` | 0x15A2-0x325C (7,354 bytes; code follows) | 0 | 24 `mov si` sites, one per section (scenes 0-8, events.md) |
| `ED_E.EXE` | about 0x25A4-0x2B31 (+ staff roll) | 0x2130 | 23 matched `mov si` |
| `OP_E.EXE` | 0x2663 on: `^` narration, then `\`-coded story (TM-style `\e \c \s \n`) | 0x2090 | 2 matched; the `\` part is read like TM text |
Growing a block means moving it past the code that follows and patching these immediates (phase 4/5).

## Not translated
- `HIOSYS.EXE`, `SS.EXE`, `VIS4.EXE`, `VISM.EXE`: no Big5 text (VIS4 holds Japanese narration, unused).
- `SETUP.EXE`, `INSTALL.EXE`: DOS installers (CD install, sound card, joystick). The recomp never runs them;
  only matter for a DOS release (D4 says the patch should also run in DOS). Low priority, `$`-terminated strings.

## Open questions
1. Where is the bopomofo picker used? Probably nowhere: no naming/input wording in any text (名字 命名 輸入 名稱
   姓名 改名 all absent), no direct code ref found, and none of these opened it (2026-11-25, warp + clicks): name in
   the status popup (L/R), party list name, the bottom-right ornament on a 25 px grid (found: load book ~560,365,
   restart ~590,385, item settings 625,380), right-edge and top-bar icons. If a screen turns up, replace the table
   with A-Z in place.
2. Exact field boundaries inside the 0x1E740 run and the help pages: confirm in-game when inserting.
3. GR*.CVD / logo / window art: graphics-text survey (status Next).
