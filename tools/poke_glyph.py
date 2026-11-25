"""Fill the A.CHA glyphs of the given characters with solid ink, in a QA copy of the game (glyph tests).

usage: python3 -X utf8 tools/poke_glyph.py GAME_DIR 王國...
Never point it at hio2/; restore with `cp hio2/A.CHA GAME_DIR/`.
"""
import os, sys

game, chars = sys.argv[1], sys.argv[2]
assert os.path.realpath(game) != os.path.realpath("hio2"), "never write into hio2/"
num = open(os.path.join(game, "A.NUM"), "rb").read()
codes = [int.from_bytes(num[i:i + 2], "big") for i in range(0, len(num), 2)]
path = os.path.join(game, "A.CHA")
cha = bytearray(open(path, "rb").read())
for ch in chars:
    k = codes.index(int.from_bytes(ch.encode("big5"), "big"))
    cha[k * 30:k * 30 + 30] = b"\xff" * 30
    print(ch, "slot", k)
open(path, "wb").write(cha)
