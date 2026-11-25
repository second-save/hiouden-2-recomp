import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class DumpTest(unittest.TestCase):
    def test_original_files_match_rom_sha1(self):
        r = subprocess.run([sys.executable, str(ROOT / "tools" / "check_rom.py")], capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stderr or r.stdout)


if __name__ == "__main__":
    unittest.main()
