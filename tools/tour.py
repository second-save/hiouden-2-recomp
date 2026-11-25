"""tour.py: QA tour of a build from cold boots; one contact sheet per scene for the visual-QA gate.

usage: python3 tools/tour.py [--base build/game] [--out work/qa/tour] [--jobs 10] [--only msg|vis|misc]
Sheets: msg/mNNN.png (every TM message, its box cropped at full size, one tile per page click), vis/vN.png (VIS
cutscenes 0-8), misc/*.png (title menu, opening, ending + staff roll, status / item / spell popups, load, restart and
item-settings dialogs). Original-game sheets for comparison: --base hio2 --out work/qa/tour_zh.
"""
import argparse
import glob
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WARP = os.path.join(ROOT, "tools/debug/warp.py")
LONG_MSGS = {0, 4, 8, 49, 55, 118, 119}  # need ~20 page clicks
STATUS = ["--click", "305,335,8100,2"]
MISC = {  # name: (warp args, first frame kept, crop w,h,x,y or None)
    "title": (["--frames", "1445"], 1440, None),
    "status": (STATUS + ["--frames", "8600"], 8400, None),
    "item": (STATUS + ["--click", "234,265,8700,2", "--frames", "9000"], 8800, None),
    "spell": (STATUS + ["--click", "232,247,8700,2", "--frames", "9000"], 8800, None),
    "load": (["--click", "560,365,8100", "--frames", "8500"], 8300, None),
    "restart": (["--click", "590,385,8100", "--frames", "8500"], 8300, None),
    "itemset": (["--click", "625,380,8100", "--frames", "8500"], 8300, None),
    "opening": (["--opening", "--frames", "4200"], 1500, None),
    "ending": (["--ending", "--frames", "12000"], 1500, None),
}


def run(name, args, out_dir, every, keep_from, crop, cols=4, base=None):
    d = os.path.join(out_dir, "_" + name)
    shutil.rmtree(d, ignore_errors=True)
    cmd = [sys.executable, WARP, "--base", base, "--game", os.path.join(d, "game"), "--out", d,
           "--every", str(every)] + args
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    shutil.rmtree(os.path.join(d, "game"), ignore_errors=True)
    frames = [f for f in sorted(glob.glob(os.path.join(d, "f*.bmp"))) if int(os.path.basename(f)[1:7]) >= keep_from]
    frames += [os.path.join(d, "last.bmp")]
    sheet = os.path.join(out_dir, name + ".png")
    os.makedirs(os.path.dirname(sheet), exist_ok=True)
    src = [f + (f"[{crop}]" if crop else "") for f in frames if os.path.exists(f)]
    geo = "+2+2" if crop else "320x200+2+2"
    subprocess.run(["magick", "montage", *src, "-tile", f"{cols}x", "-geometry", geo, sheet], check=True)
    shutil.rmtree(d)
    return sheet


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", default=os.path.join(ROOT, "build/game"))
    ap.add_argument("--out", default=os.path.join(ROOT, "work/qa/tour"))
    ap.add_argument("--jobs", type=int, default=10)
    ap.add_argument("--only")
    a = ap.parse_args()
    jobs = []
    if a.only in (None, "msg"):
        for n in range(128):
            pages = 20 if n in LONG_MSGS else 6
            jobs.append((f"msg/m{n:03d}", ["--msg", str(n), "--pages", str(pages)], 300, 8400, "640x200+0+60", 2))
    if a.only in (None, "vis"):
        for n in range(9):
            jobs.append((f"vis/v{n}", ["--vis", str(n), "--frames", "14001"], 300, 2400, None, 4))
    if a.only in (None, "misc"):
        for name, (args, keep, crop) in MISC.items():
            every = 250 if name in ("opening", "ending") else 100
            jobs.append((f"misc/{name}", args, every, keep, crop, 4))
    os.makedirs(a.out, exist_ok=True)
    with ThreadPoolExecutor(a.jobs) as ex:
        for s in ex.map(lambda j: run(*j, base=a.base) if True else None,
                        [(n, args, a.out, every, keep, crop, cols) for n, args, every, keep, crop, cols in jobs]):
            pass
    print(f"{len(jobs)} sheets in {a.out}")


if __name__ == "__main__":
    main()
