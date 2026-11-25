"""mkicon.py: the app icon (our own, not the game's art): "HIOUDEN / II" in the Spleen pixel font, gold on a dark
red rounded square. Writes packaging/icon_1024.png, packaging/hiouden2.ico and packaging/HioudenII.icns.

usage: python3 tools/mkicon.py      (needs ImageMagick `magick`; macOS `iconutil` for the .icns)
"""
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import bigram  # noqa: E402

S = 1024
BG, BORDER, GOLD = (90, 15, 12), (216, 176, 96), (232, 196, 112)
glyphs = bigram.bdf_glyphs()
px = [[None] * S for _ in range(S)]  # None = transparent


def rounded(x0, y0, x1, y1, r, col):
    for y in range(y0, y1):
        for x in range(x0, x1):
            dx = max(x0 + r - x, 0, x - (x1 - r - 1))
            dy = max(y0 + r - y, 0, y - (y1 - r - 1))
            if dx * dx + dy * dy <= r * r:
                px[y][x] = col


def text(s, scale, cy):
    w = len(s) * 8 * scale
    x0, y0 = (S - w) // 2, cy - 8 * scale
    for k, ch in enumerate(s):
        for row, bits in enumerate(glyphs[ch]):
            for col in range(8):
                if bits & (0x80 >> col):
                    for yy in range(scale):
                        for xx in range(scale):
                            px[y0 + row * scale + yy][x0 + (k * 8 + col) * scale + xx] = GOLD


rounded(64, 64, S - 64, S - 64, 180, BORDER)
rounded(84, 84, S - 84, S - 84, 162, BG)
text("HIOUDEN", 13, 330)
text("II", 34, 640)

with tempfile.TemporaryDirectory() as tmp:
    ppm = Path(tmp) / "icon.pam"
    with open(ppm, "wb") as f:  # PAM with alpha
        f.write(f"P7\nWIDTH {S}\nHEIGHT {S}\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n".encode())
        f.write(bytes(v for row in px for p in row for v in ((*p, 255) if p else (0, 0, 0, 0))))
    out = ROOT / "packaging"
    out.mkdir(exist_ok=True)
    png = out / "icon_1024.png"
    subprocess.run(["magick", str(ppm), str(png)], check=True)
    subprocess.run(["magick", str(png), "-define", "icon:auto-resize=256,128,64,48,32,16", str(out / "hiouden2.ico")],
                   check=True)
    iset = Path(tmp) / "HioudenII.iconset"
    iset.mkdir()
    for s in (16, 32, 128, 256, 512):
        subprocess.run(["magick", str(png), "-resize", f"{s}x{s}", str(iset / f"icon_{s}x{s}.png")], check=True)
        subprocess.run(["magick", str(png), "-resize", f"{2 * s}x{2 * s}", str(iset / f"icon_{s}x{s}@2x.png")], check=True)
    if shutil.which("iconutil"):
        subprocess.run(["iconutil", "-c", "icns", str(iset), "-o", str(out / "HioudenII.icns")], check=True)
print("wrote packaging/icon_1024.png, hiouden2.ico, HioudenII.icns")
