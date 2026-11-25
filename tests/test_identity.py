import hashlib
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))


@unittest.skipUnless((ROOT / "hio2" / "HIO.EXE").is_file(), "original game not in hio2/")
class IdentityBuildTest(unittest.TestCase):
    def test_identity_build_equals_originals(self):
        import build
        with tempfile.TemporaryDirectory() as tmp:
            out = build.build(Path(tmp) / "game", identity=True)
            for line in (ROOT / "rom.sha1").read_text(encoding="utf-8").splitlines():
                if not line.strip() or line.startswith("#") or "hio2/" not in line:
                    continue
                want, name = line.split(None, 1)
                got = hashlib.sha1((out / Path(name.strip()).name).read_bytes()).hexdigest()
                self.assertEqual(got, want, name)
            # rom.sha1 pins only the files we read or change; every other file must match too (bytes, not mtime)
            names = sorted(p.name for p in (ROOT / "hio2").iterdir())
            self.assertEqual(sorted(p.name for p in out.iterdir()), names)
            diff = [n for n in names if (ROOT / "hio2" / n).read_bytes() != (out / n).read_bytes()]
            self.assertEqual(diff, [])


if __name__ == "__main__":
    unittest.main()
