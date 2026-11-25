import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import batches  # noqa: E402
import bigram  # noqa: E402


class ScriptWidthTest(unittest.TestCase):
    def test_widths(self):
        """Columns in bigram cells: an odd run before a code wastes half a cell (TM box: 46 columns, measured
        2026-11-25 work/qa/m0; `\\u` starts a new line too)."""
        self.assertEqual([bigram.cols(l) for l in bigram.lines(r"\2ab\Fc\nd", batches.TM_BREAKS)], [4, 2])

    def test_all_english_passes_the_batch_check(self):
        """Every shipped English line: codes matched, ASCII, glossary, widths and byte budgets (tools/batches.py)."""
        entries, terms = batches.index(), batches.glossary()
        bad = [f"{i}: {p}" for i, e in entries.items() if e.get("en") for p in batches.problems(e, e["en"], terms)]
        bad += batches.budget_problems({}, entries)
        self.assertEqual(bad, [])


if __name__ == "__main__":
    unittest.main()
