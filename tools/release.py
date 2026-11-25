"""release.py: gate, then package the English patch (local only; publishing is the user's call).

usage: python3 tools/release.py --version 1.0 [--force]
Stops at the first failure: build -> tests -> smoke boot -> no Chinese left in any translated field -> visual and
story QA gates (qa_gate.py check) -> no unticked line in docs/polish.md -> no template placeholder in README.md ->
per-file BPS patches for every changed file -> re-apply them to a fresh copy of hio2/ and compare with the build ->
dist/Hiouden2_EN_v<version>.zip (patches, apply.py, bps.py, checksums, README, font licence). Never overwrites a
released version without --force.
"""
import argparse
import hashlib
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
QA_GATE = Path.home() / ".claude/skills/translate-retro-game/scripts/qa_gate.py"
MANIFESTS = [("work/qa/manifest.tsv", "work/qa/verdicts"), ("work/qa/story/manifest.tsv", "work/qa/story/verdicts")]


def step(name, ok, detail=""):
    print(f"[{'ok' if ok else 'FAIL'}] {name}" + (f": {detail}" if detail else ""))
    if not ok:
        sys.exit(1)


def zh_left(out):
    """Ids whose built bytes still hold a Chinese character (an untranslated entry or a missed field)."""
    import codec
    import insert
    blocks = insert.load_blocks()
    en = insert.english(blocks)
    missing = [e["id"] for es in blocks.values() for e in es if e["zh"].strip() and e["id"] not in en
               and re.search(r"[一-鿿]", e["zh"])]
    return missing


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--version", required=True)
    ap.add_argument("--force", action="store_true")
    a = ap.parse_args()
    name = f"Hiouden2_EN_v{a.version}"
    dist = ROOT / "dist" / name
    zpath = ROOT / "dist" / f"{name}.zip"
    if (dist.exists() or zpath.exists()) and not a.force:
        sys.exit(f"{zpath} exists: pass --force to rebuild it")

    import build
    out = build.build()
    step("build", out.is_dir(), str(out))
    r = subprocess.run([sys.executable, "-X", "utf8", str(ROOT / "tools/check.py")], capture_output=True, text=True)
    step("tests", r.returncode == 0, r.stderr.strip().splitlines()[-1] if r.stderr.strip() else "")
    with tempfile.TemporaryDirectory() as tmp:
        g = Path(tmp) / "game"
        shutil.copytree(out, g)
        frame = Path(tmp) / "smoke.bmp"
        subprocess.run([str(ROOT / "build/hiouden2"), "--data-dir", str(g), "--headless-test", "1500",
                        "--dump-frame", str(frame)], capture_output=True)
        step("smoke boot to the title menu", frame.is_file() and frame.stat().st_size > 1000)
    left = zh_left(out)
    step("no Chinese left in translated text", not left, ", ".join(left[:8]))
    for manifest, verdicts in MANIFESTS:
        r = subprocess.run([sys.executable, "-X", "utf8", str(QA_GATE), "check", str(ROOT / manifest), str(ROOT / verdicts)],
                           capture_output=True, text=True)
        step(f"qa gate {manifest}", r.returncode == 0, (r.stdout + r.stderr).strip().splitlines()[-1] if r.returncode else "")
    polish = ROOT / "docs/polish.md"
    open_lines = [l for l in polish.read_text(encoding="utf-8").splitlines() if l.startswith("- [ ]")] if polish.exists() else []
    step("docs/polish.md all ticked", not open_lines, f"{len(open_lines)} open")
    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    step("README.md has no template placeholder", not re.search(r"<(Game title|platform|publisher|year)[^>]*>", readme))

    import bps
    shutil.rmtree(dist, ignore_errors=True)
    (dist / "patches").mkdir(parents=True)
    changed = []
    for f in sorted((ROOT / "hio2").iterdir()):
        new = (out / f.name).read_bytes()
        if f.read_bytes() != new:
            changed.append(f.name)
            (dist / "patches" / f"{f.name}.bps").write_bytes(bps.make(f.read_bytes(), new))
    (dist / "checksums.txt").write_text("".join(f"{hashlib.sha1((ROOT / 'hio2' / n).read_bytes()).hexdigest()}  {n}\n"
                                                for n in changed), encoding="utf-8")
    shutil.copy(ROOT / "tools/apply_patch.py", dist / "apply.py")
    shutil.copy(ROOT / "tools/bps.py", dist / "bps.py")
    shutil.copy(ROOT / "font/LICENSE-spleen", dist / "LICENSE-spleen.txt")
    shutil.copy(ROOT / "docs/release-readme.txt", dist / "README.txt")
    step("patches", bool(changed), f"{len(changed)} files")
    with tempfile.TemporaryDirectory() as tmp:
        o = Path(tmp) / "patched"
        r = subprocess.run([sys.executable, "-B", str(dist / "apply.py"), str(ROOT / "hio2"), str(o)],
                           capture_output=True, text=True)  # -B: no __pycache__ in the release folder
        same = r.returncode == 0 and all((o / f.name).read_bytes() == (out / f.name).read_bytes()
                                         for f in out.iterdir() if f.is_file())
        step("re-applied patch equals the build", same, r.stderr.strip())
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(dist.rglob("*")):
            if f.is_file() and "__pycache__" not in f.parts:
                z.write(f, f"{name}/{f.relative_to(dist)}")
    print(f"released {zpath} ({zpath.stat().st_size // 1024} KB, {len(changed)} patched files)")


if __name__ == "__main__":
    main()
