"""build.py: build the patched game into build/game/ (never into hio2/).

usage: python3 tools/build.py [--identity] [--out DIR]
Checks the originals against rom.sha1, copies hio2/ to OUT, then applies the edits. The DOS install keeps every
text file and EXE raw (no archives or compression; docs/re/text_sites.md), so the edit steps write in place.
--identity skips the edits: the result must equal the originals byte for byte (tests/test_identity.py).
"""
import argparse
import shutil
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_rom  # noqa: E402

import insert  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]


def build(out=ROOT / "build" / "game", identity=False, marker=False):
    check_rom.check()
    out = Path(out)
    shutil.rmtree(out, ignore_errors=True)
    shutil.copytree(ROOT / "hio2", out)
    if not identity:
        font = insert.insert(out, marker)
        print(f"font: {font.slots()} slots, {len(font.pair_code)} letter pairs")
    return out


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--identity", action="store_true")
    ap.add_argument("--marker", action="store_true", help="every entry shows #n + filler at full width")
    ap.add_argument("--out", default=None)
    a = ap.parse_args()
    out = a.out or str(ROOT / "build" / ("marker" if a.marker else "game"))
    print("built", build(out, a.identity, a.marker))
