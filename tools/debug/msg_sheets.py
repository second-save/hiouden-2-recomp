"""Shot list: every TM message (0-127) from a cold boot, one contact sheet each.

usage: python3 tools/debug/msg_sheets.py [--out work/qa/msg] [--jobs 8] [--remap A=B ...] [N ...]
Runs warp.py --msg N in parallel (each on its own game copy) and writes OUT/mNNN.png: the box after the
right-click and after each page click (--pages, default 6).
Proof: all 128 ran on 2026-11-25 (~4 min at --jobs 10); sheets were in work/qa/msg. Extra --remap pairs pass through (e.g. a translated TM file).
"""
import argparse, glob, os, shutil, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ap = argparse.ArgumentParser()
ap.add_argument("--out", default=os.path.join(ROOT, "work/qa/msg"))
ap.add_argument("--jobs", type=int, default=8)
ap.add_argument("--remap", action="append", default=[])
ap.add_argument("--pages", type=int, default=6, help="page clicks (0, 4, 8, 49, 55, 118, 119 need ~20)")
ap.add_argument("msgs", nargs="*", type=int)
a = ap.parse_args()


def one(n):
    d = os.path.join(a.out, f"{n:03d}")
    cmd = [sys.executable, os.path.join(ROOT, "tools/debug/warp.py"), "--msg", str(n), "--out", d,
           "--every", "300", "--pages", str(a.pages), "--game", os.path.join(d, "game")] + [f"--remap={r}" for r in a.remap]
    rc = subprocess.run(cmd, stdout=subprocess.DEVNULL).returncode
    frames = [f for f in sorted(glob.glob(os.path.join(d, "f*.bmp"))) if int(os.path.basename(f)[1:7]) >= 8400]
    sheet = os.path.join(a.out, f"m{n:03d}.png")
    if frames:
        subprocess.run(["magick", "montage", *frames, "-tile", "4x", "-geometry", "320x200+2+2", sheet], check=True)
    shutil.rmtree(d)
    return n, rc, len(frames)


with ThreadPoolExecutor(a.jobs) as ex:
    for n, rc, k in ex.map(one, a.msgs or range(128)):
        if rc or not k:
            print(f"msg {n}: exit {rc}, {k} frames")
print("sheets in", a.out)
