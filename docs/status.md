# Status

Updated at the end of every session. Keep it to what the next session needs; history is in git.

- **Phase:** all 9 done. v1.0 built locally: `dist/Hiouden2_EN_v1.0.zip` (92 KB, 98 per-file BPS patches + `apply.py`,
  sha1 4239ab1f4fd305d2a0c97a8552941d30ef811425, 2026-11-25). Every release gate passed: tests, smoke, no Chinese
  left, visual QA (146 sheets, 4 rounds), story pass (3 chapters), polish list, README, re-apply check.
  **Not published**: publishing under second-save waits for the user.
- **Done:**
  - Recon + debug tool: `docs/re/` (text_sites.md maps every string), `tools/debug/warp.py` (`--stage`, `--msg N`,
    `--vis N`, `--opening`, `--ending`, `--base DIR`; skips the start-up intro unless `--with-intro`), `tools/tour.py`
    (146 contact sheets: all TM messages, VIS 0-8, menus, popups, dialogs, opening, ending).
  - Text pipeline: `tools/extract.py` -> `script/*.json` (zh/en), `tools/codec.py`, `tools/batches.py` (every rule:
    codes, ASCII, glossary, widths, TM page scroll, OP_E/ED_E byte budgets, ED_E narration codes), `tools/insert.py`
    via `tools/build.py`, `tools/bigram.py` (letter-pair font from Spleen 8x16, TM layout normaliser, pause slide).
  - Engine facts the insert relies on (verified in game): TM slots 1024 bytes (D10); TM box 4 rows, `\u` always
    scrolls one row, the wait arrow needs its own row, only a header/blank row may scroll off unread; item popup =
    23-cell line then 25-cell lines; VIS text moved to the image end (ceiling DS:0x7E00); OP_E/ED_E grow to image
    0x334B (HIOSYS table at linear 0xE95B), ED_E's ED01..14.WLF list moves behind the text (14 `mov dx`);
    narration clips at 50 cols; ED_E narration must keep the original ^[nn] structure or the last line stays under
    the staff roll.
  - Release: `python3 tools/release.py --version X [--force]`.
- **Next (only if the user asks):** publish (public repo second-save/hiouden2-en, README screenshots/GIFs in
  `docs/media/` from the patched build, GitHub release with the zip). Possible improvements: a code patch for the
  2-letter weakness names (HIO:1E91A), test in DOSBox, check the window-setup and save dialogs in game.
- **Open bugs:** none known. Known limits in README.
- **Waiting on the user:** D9/D12 (names, 王兄 tag) were taken under D8 and may be overridden; publishing.
- Note: `hio2/MAP0.TMP`/`WIND0.TMP`/`stdout.txt`/`stderr.txt` were written by a run on hio2/ before the repo existed;
  harmless (live state, never patched).
