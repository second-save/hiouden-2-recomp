# Status

Updated at the end of every session. Keep it to what the next session needs; history is in git.

- **Phase:** released. v1.0 then v1.1 (2026-11-25) published at github.com/second-save/hiouden-2-recomp. v1.1:
  real-time DOS clock (v1.0 ran too fast), Esc quit box, hero renamed Mark (D13), how-to-play in README and
  HOW-TO-PLAY.txt, launcher asks to update when the shipped patches differ from the installed copy (install.ok holds
  the patch CRCs). v1.1 waived the visual-tour gate (only the shorter name changed; intro re-checked on screen).
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
  - Launcher + installer in the recomp (SDL3 port): pick your own game folder, it installs an English copy into the
    per-user folder (C++ BPS, refuses wrong/patched files, keeps saves), settings, Play. Packages:
    `tools/release_{mac,windows,linux}.sh` -> dist/HioudenII-{macOS.zip,Windows.zip,Linux-x86_64.tar.gz}.
    Published as github.com/second-save/hiouden-2-recomp (v1.0).
- **Next:** player feedback; Windows and Linux builds not played on real machines yet (Linux installer tested in a
  container, Windows untested). Possible improvements: a code patch for the
  2-letter weakness names (HIO:1E91A), test in DOSBox, check the window-setup and save dialogs in game.
- **Open bugs:** none known. Known limits in README.
- **Waiting on the user:** D12 (王兄 tag) was taken under D8 and may be overridden. Guide facts confirmed by the
  user: the Hourglass starts marching, fights are automatic, the v1.1 speed is right.
- Note: `hio2/MAP0.TMP`/`WIND0.TMP`/`stdout.txt`/`stderr.txt` were written by a run on hio2/ before the repo existed;
  harmless (live state, never patched).
