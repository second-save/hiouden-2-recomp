# Visual-QA reviewer brief - Hiouden II English (DOS, recomp)

You review contact sheets from the English build's QA tour and give each one a verdict. Read-only on everything
except your one verdict file: no edits to the repo, no git, no subagents. Repo root:
/Users/antoineclerc-renaud/develop/hioden-2.

## Inputs (in your dispatch)
- A chunk file: one `id<TAB>path` line per sheet. Read every sheet in it; none may be skipped.
- Your verdict file: `work/qa/verdicts/<chunk>.tsv`.
- The original (Traditional Chinese) game's sheets: `work/qa/tour_zh/<id>.png` (same ids), and `glossary.md`.
- Sheets: `msg/mNNN` = one event message, its box cropped at full size, one tile per click (the box types text out,
  then pages); `vis/vN` = cutscene N; `misc/*` = title menu, popups, dialogs, opening, ending + staff roll.

## How to look
- Read each sheet whole first. Where text is small, zoom with
  `python3 -X utf8 ~/.claude/skills/translate-retro-game/scripts/sheet.py SHEET.png --crop x,y,w,h --zoom 3 --out /private/tmp/claude-501/<name>.png`
  and read that.
- When something looks wrong, open the same id in `work/qa/tour_zh/`. If the original shows the same thing, it is
  the game's own behaviour: `ok`, with a note.

## Judge
An `issue` is any of:
- Chinese still visible (except in the original-only comparison sheets);
- text overflowing a box, clipped by a frame, or running off screen;
- garbage glyphs, wrong or missing letters, letters with stray pixels, a cell showing a wrong pair;
- broken line breaks: a word split mid-line, a page ending mid-sentence, a one-word last line;
- an empty box where text belongs (compare with the original);
- a name that differs from `glossary.md`.

Standing non-bugs (`ok`):
- text still typing out, a box mid-page, the same page repeated after the message ended (the tour keeps clicking);
- the message box scrolls when a page has more lines than fit (the original does too);
- lines in the ORIGINAL that run onto the border (several Chinese messages are 24-27 cells wide);
- two letters per 16 px cell, so word spacing can look slightly uneven after a colour code (half-cell gaps);
- `+-` in level-up stats stands for the original's ± sign.
- Second round (after fixes): a page may scroll ONE row while typing before it pauses (the original does the same);
  more than one row lost before a pause is an issue. A line hidden under the wait arrow is an issue. Opening and
  ending narration must not be cut at the right edge.

## Verdicts
Write your verdict file once you have read every sheet, one line per id in the chunk:
`id<TAB>ok|issue<TAB>what and where` (for an issue: which tile, which text, what is wrong).
Then return a short summary: counts, and the issues one per line.
