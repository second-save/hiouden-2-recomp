# Status

Updated at the end of every session. Keep it to what the next session needs; history is in git.

- **Phase:** 1-7 done (recon, debug tool, identity build, extract, font + insert, translation, review); phase 8
  (in-game QA) in its fix rounds; phase 9 (release) scripted (`tools/release.py`), not yet passing its gates.
- **Done:**
  - Recon + debug tool: `docs/re/` (text_sites.md is the map of every string), `tools/debug/warp.py` (`--stage`,
    `--msg N`, `--vis N`, `--opening`, `--ending`, `--base DIR`; skips the start-up intro unless `--with-intro`),
    `tools/tour.py` (146 contact sheets of a build: all TM messages, VIS 0-8, menus, popups, dialogs, opening, ending).
  - Text pipeline: `tools/extract.py` -> `script/*.json` (zh/en, fields), `tools/codec.py`, `tools/batches.py`
    (split/check/merge with every rule: codes, ASCII, glossary, widths, TM page scroll, OP_E/ED_E byte budgets),
    `tools/insert.py` (via `tools/build.py`), `tools/bigram.py` (letter-pair font from Spleen 8x16).
  - Engine facts that shaped the insert (all verified in game): TM slots widened to 1024 bytes (D10, HIO.EXE
    0x17AED); TM box = 4 rows, `\u` scrolls, the wait arrow needs its own row (insert normalises codes); item popup
    wraps the 98-cell record at 23 then 25 cells; VIS text relocated to the image end (ceiling DS:0x7E00, HIO reloads
    CD01 after each cutscene); OP_E/ED_E load inside HIOSYS and may grow to image 0x334B (HIOSYS table at linear
    0xE95B), ED_E's ED01..14.WLF list moves behind the text with its 14 `mov dx` pointers; narration clips at 50 cols.
  - Translation: every entry has English (1,281). Names first (b001-b003), then 7 parallel batches, review by 4
    reviewers + 3 story reviewers, fix rounds b012 (review) and b013 (layout + story, in progress).
  - Recomp: `--watch LO-HI` logs CPU access to a memory range (used to find free memory for the scripts).
  - Release: `tools/release.py --version X` (gates, per-file BPS patches + `apply.py`, re-apply check, zip in dist/).
    `README.md` and `docs/release-readme.txt` written. Publishing (second-save GitHub) waits for the user.
- **Next:**
  1. Merge b013, rebuild, re-run `tools/tour.py`, `qa_gate.py init`, re-dispatch visual QA on changed sheets
     (`qa_gate.py todo`), fix what they find.
  2. Polish pass over `docs/polish.md` (agent), tick every line; story verdicts in `work/qa/story/verdicts/`.
  3. `python3 tools/release.py --version 1.0`; then ask the user before publishing.
- **Open bugs:** see visual QA verdicts in `work/qa/verdicts/` and `docs/polish.md`.
- **Waiting on the user:** D9/D12 (names, 王兄 tag) taken under D8, may be overridden; "Hiouden II'" prime; publishing.
- Note: `hio2/MAP0.TMP`/`WIND0.TMP`/`stdout.txt`/`stderr.txt` were written by a run on hio2/ before the repo existed;
  harmless (live state, not patched).
