import json
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAX_COLS = 46  # TM message box: 23 cells of 16 px, two Latin letters per cell (measured 2026-11-25, work/qa/m0)
CODE = re.compile(r"\\[0-9A-Za-z]")


def line_widths(slot):
    """Visible width of each box line in one CVD slot: full-width chars count 2, control codes 0."""
    for part in re.split(r"\\n|\\c", slot):
        text = CODE.sub("", part)
        yield sum(2 if ord(c) > 0x2000 else 1 for c in text), text


class ScriptWidthTest(unittest.TestCase):
    def test_widths(self):
        self.assertEqual(list(line_widths(r"\2ab\Fc\nd")), [(3, "abc"), (1, "d")])

    def test_tm_lines_fit_the_box(self):
        over = []
        for e in json.loads((ROOT / "script" / "TM.json").read_text(encoding="utf-8")):
            if not e["en"]:
                continue
            self.assertTrue(e["en"].endswith(r"\e"), f"{e['id']} lacks \\e")
            over += [f"{e['id']}: {w} cols: {t!r}" for w, t in line_widths(e["en"]) if w > MAX_COLS]
        self.assertEqual(over, [])


if __name__ == "__main__":
    unittest.main()
