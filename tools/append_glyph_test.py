"""Append test for the font: move a character's entry to a new slot past the original 3244, with a solid glyph.

usage: python3 -X utf8 tools/append_glyph_test.py GAME_DIR 王 [EXTRA]
The old A.NUM entry gets code 0xFEFE (unused), so the only way to draw the char is the appended slot. EXTRA pads
that many blank slots before it (to probe a size limit). Never point it at hio2/; restore A.NUM/A.CHA from hio2/.
"""
import os, sys

game, ch = sys.argv[1], sys.argv[2]
extra = int(sys.argv[3]) if len(sys.argv) > 3 else 0
assert os.path.realpath(game) != os.path.realpath("hio2"), "never write into hio2/"
code = ch.encode("big5")
num = bytearray(open(os.path.join(game, "A.NUM"), "rb").read())
cha = bytearray(open(os.path.join(game, "A.CHA"), "rb").read())
k = next(i for i in range(0, len(num), 2) if num[i:i + 2] == code)
num[k:k + 2] = b"\xfe\xfe"
num += b"\xfe\xfd" * extra + code
cha += b"\x00" * 30 * extra + b"\xff" * 30
open(os.path.join(game, "A.NUM"), "wb").write(num)
open(os.path.join(game, "A.CHA"), "wb").write(cha)
print(ch, "moved from slot", k // 2, "to slot", len(num) // 2 - 1)
