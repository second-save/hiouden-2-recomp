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
ap.add_argument("--click", action="append", default=[], help="extra x,y,frame[,btn] clicks after landing (btn 2=right)")
ap.add_argument("--msg", type=lambda s: int(s, 0), help="show TM message N (file TM{N//4+1:02}, slot N%%4) on stage 1")
ap.add_argument("--pages", type=int, default=6, help="with --msg: left clicks to page through it")
a, passthrough = ap.parse_known_args()  # anything else (e.g. --trace-ip 24607) goes to the recomp

game = os.path.join(ROOT, "work/qa/game_warp")
shutil.rmtree(game, ignore_errors=True)
shutil.copytree(os.path.join(ROOT, "hio2"), game)
os.makedirs(a.out, exist_ok=True)
if a.msg is not None:
    # MP01 event record 0x12B0 is a type-5 (2x2, examine with right-click) message event: move it onto tile
    # (0x32,0x0C), which a right-click at (600,140) checks on a cold boot (BX=0x18DC = (0x32+16)*96 + 0x0C+16),
    # and point its action at message N. Then page through with left clicks. See docs/re/events.md.
    assert a.stage == "01", "--msg works on stage 1 only"
    mp = os.path.join(game, "MP01.HMF")
    d = bytearray(open(mp, "rb").read())
    assert d[0x12B0] == 5 and d[0x12B6] == 0, "unexpected MP01 event record"
    d[0x12B2:0x12B4] = bytes([0x32, 0x0C])
    d[0x12B9] = a.msg
    open(mp, "wb").write(d)
    a.click.append("600,140,8100,2")
    a.click += [f"320,300,{8400 + 300 * k}" for k in range(a.pages)]
    a.frames = max(a.frames, 8400 + 300 * a.pages + 100)
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
cmd += passthrough
r = subprocess.run(cmd, stderr=None if passthrough else subprocess.DEVNULL)
print("exit", r.returncode, "->", os.path.join(a.out, "last.bmp"))
sys.exit(r.returncode)
