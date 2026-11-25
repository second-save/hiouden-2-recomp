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

ROOT = Path(__file__).resolve().parents[1]
EDITS = []  # phase 4/5 add steps here: each a function(out_dir: Path)


def build(out=ROOT / "build" / "game", identity=False):
    check_rom.check()
    out = Path(out)
    shutil.rmtree(out, ignore_errors=True)
    shutil.copytree(ROOT / "hio2", out)
    for step in [] if identity else EDITS:
        step(out)
    return out


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--identity", action="store_true")
    ap.add_argument("--out", default=str(ROOT / "build" / "game"))
    a = ap.parse_args()
    print("built", build(a.out, a.identity))
