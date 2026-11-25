"""check.py: the repo's one-command check (the pre-commit hook runs it). Runs every test in tests/."""
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
suite = unittest.defaultTestLoader.discover(str(ROOT / "tests"))
result = unittest.TextTestRunner(verbosity=1).run(suite)
sys.exit(0 if result.wasSuccessful() else 1)
