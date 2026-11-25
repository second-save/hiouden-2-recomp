import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "build" / "hiouden2"
sys.path.insert(0, str(ROOT / "tools"))


@unittest.skipUnless(BIN.is_file() and (ROOT / "hio2" / "HIO.EXE").is_file(), "needs build/hiouden2 and hio2/")
class LauncherInstallTest(unittest.TestCase):
    """The recomp's own installer (src/installer.cpp, C++ BPS) must produce exactly what tools/build.py builds."""

    def test_install_equals_build_and_refuses_patched_files(self):
        import build
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            built = build.build(tmp / "built")
            pdir = ROOT / "dist" / "patches"  # the installer's dev fallback for patches/
            pdir.mkdir(parents=True, exist_ok=True)
            import bps
            for f in sorted((ROOT / "hio2").iterdir()):
                new = (built / f.name).read_bytes()
                if f.read_bytes() != new:
                    (pdir / f"{f.name}.bps").write_bytes(bps.make(f.read_bytes(), new))
            env = dict(os.environ, HIOUDEN_HOME=str(tmp / "home"))
            r = subprocess.run([str(BIN), "--install", str(ROOT / "hio2")], env=env, capture_output=True, text=True)
            self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
            game = tmp / "home" / "game"
            diff = [f.name for f in built.iterdir() if (game / f.name).read_bytes() != f.read_bytes()]
            self.assertEqual(diff, [])
            self.assertTrue((game / "install.ok").is_file())
            r = subprocess.run([str(BIN), "--install", str(built)], env=env, capture_output=True, text=True)
            self.assertEqual(r.returncode, 1)
            self.assertIn("isn't the original", r.stdout)


if __name__ == "__main__":
    unittest.main()
