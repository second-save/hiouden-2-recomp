"""Apply the Hiouden II English patch to your copy of the Taiwanese DOS release (shipped in the release zip).

usage: python3 apply.py ORIGINAL_GAME_DIR OUTPUT_DIR

Copies the whole original game folder to OUTPUT_DIR (the original is never touched), checks each file the patch
changes against its SHA-1, then applies patches/<FILE>.bps to it. Needs only Python 3 (no packages). Each .bps is a
standard BPS patch, so Flips or RomPatcher.js work per file too.
"""
import hashlib
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import bps  # noqa: E402


def main(argv):
    if len(argv) != 2 or argv[0] in ("-h", "--help"):
        sys.exit(__doc__)
    src, out = Path(argv[0]), Path(argv[1])
    if out.exists():
        sys.exit(f"{out} exists: pick a new folder (nothing written)")
    if src.resolve() == out.resolve():
        sys.exit("output must be a different folder")
    want = {}
    for line in (HERE / "checksums.txt").read_text(encoding="utf-8").splitlines():
        if line.strip():
            sha, name = line.split(None, 1)
            want[name.strip()] = sha
    bad = []
    for name, sha in want.items():
        f = src / name
        if not f.is_file():
            bad.append(f"missing {name}")
        elif hashlib.sha1(f.read_bytes()).hexdigest() != sha:
            bad.append(f"{name}: not the original file (already patched, or a different release?)")
    if bad:
        sys.exit("cannot patch:\n  " + "\n  ".join(bad))
    shutil.copytree(src, out)
    for name in want:
        patched = bps.apply((src / name).read_bytes(), (HERE / "patches" / f"{name}.bps").read_bytes())
        (out / name).write_bytes(patched)
    print(f"patched {len(want)} files into {out}")


if __name__ == "__main__":
    main(sys.argv[1:])
