"""Cold-boot warp: New Game, click through the intro, land on stage NN (MPNN.HMF) and save a frame.

usage: python3 tools/debug/warp.py --stage 08 --out work/qa/warp08 [--frames 8001] [--every 0] [--remap A=B ...]
         [--click x,y,frame ...]
Runs on a fresh copy of hio2/ in work/qa/game_warp (never hio2/ itself: the game writes MAP0.TMP/WIND0.TMP).
Works by remapping MP01.HMF, which New Game loads, to the chosen stage. Tiles stay CD01's (fine for text QA).
Extra --remap pairs swap any other file, e.g. TM01.CVD=TM31.CVD to show TM31 where TM01 would appear.
Proof: work/qa/warp_sheet.png (stages 01, 02, 08) from 2026-11-25.
"""
import argparse, os, shutil, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TITLE_NEW_GAME = "300,167,1450"  # first title entry; the menu is up by frame ~1400
INTRO_CLICKS = range(2400, 7600, 300)  # VIS.EXE intro advances one page per click; map is up by ~7600

ap = argparse.ArgumentParser()
ap.add_argument("--stage", default="01")
ap.add_argument("--out", required=True)
ap.add_argument("--frames", type=int, default=8001)
ap.add_argument("--every", type=int, default=0, help="also dump every Nth frame")
ap.add_argument("--remap", action="append", default=[])
ap.add_argument("--click", action="append", default=[], help="extra x,y,frame clicks after landing")
a = ap.parse_args()

game = os.path.join(ROOT, "work/qa/game_warp")
shutil.rmtree(game, ignore_errors=True)
shutil.copytree(os.path.join(ROOT, "hio2"), game)
os.makedirs(a.out, exist_ok=True)
cmd = [os.path.join(ROOT, "build/hiouden2"), "--data-dir", game, "--headless-test", str(a.frames),
       "--test-click", TITLE_NEW_GAME, "--dump-frame", os.path.join(a.out, "last.bmp")]
for f in INTRO_CLICKS:
    cmd += ["--test-click", f"320,300,{f}"]
for c in a.click:
    cmd += ["--test-click", c]
if a.stage != "01":
    cmd += ["--remap", f"MP01.HMF=MP{int(a.stage):02d}.HMF"]
for r in a.remap:
    cmd += ["--remap", r]
if a.every:
    cmd += ["--dump-every", str(a.every), a.out]
r = subprocess.run(cmd, stderr=subprocess.DEVNULL)
print("exit", r.returncode, "->", os.path.join(a.out, "last.bmp"))
sys.exit(r.returncode)
