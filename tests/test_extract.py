import json
import re
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import codec  # noqa: E402
import extract  # noqa: E402

EXES = {"HIO", "VIS", "OP_E", "ED_E"}
EXT = {"TM": ".CVD", "IM": ".CVD", "MM": ".CVD", "MP": ".HMF"}


RUN = re.compile(rb"(?:[\xa1-\xc5][\x40-\x7e\xa1-\xfe])+")  # Big5 punctuation + level-1 hanzi
WAIVED_FILES = (
    ".WLF", ".WL2", ".VOC", ".FMU", ".SDF", ".PCX",  # images, sound, music: byte noise
    "A.NUM", "A.CHA", "T_WK1",                      # font code lists and glyphs
    "INSTALL.EXE", "SETUP.EXE", "INST-",            # DOS installers and their art, not run by the recomp
    "old-games.nfo",                                # release info file, not game text
    "MAP",                                          # save templates/state: unit names copied from MP*.HMF
    "WIND",                                         # save templates: MP01's unit names again
    "GR", "CD", "EN",                               # CVD graphics
)
WAIVED_RANGES = [  # (file, start, end) image offsets, why
    ("HIO.EXE", 0x1A4EC, 0x1B4B0),  # bopomofo character picker tables: data, no screen uses it
    ("HIO.EXE", 0x0B000, 0x0D000),  # code bytes that happen to decode (簽高散啣, 橇凝醋瘸螢, 瓚顯灞霹)
    ("HIO.EXE", 0x14400, 0x14420),  # code: 屨垂垂垂
    ("HIO.EXE", 0x15B40, 0x15BE0),  # code: 壎孕孕孕市垂徉
]


def source(fid):
    if fid in EXES:
        return extract.image(f"{fid}.EXE")
    return (extract.GAME / f"{fid}{EXT[fid[:2]]}").read_bytes()


def is_pad(data, a, b):
    return all(t in extract.PAD for _, t in extract.tokens(data, a, b))


class CodecTest(unittest.TestCase):
    def test_codes_take_one_byte_and_trails_win(self):
        raw = b"\\n\xa5\x5c^\x01[\xff"  # \n, a cell whose trail is `\`, ^ + 0x01, a bare [, a stray byte
        self.assertEqual(codec.encode(codec.decode(raw)), raw)
        self.assertEqual(codec.decode(raw)[:2], "\\n")


@unittest.skipUnless((ROOT / "hio2" / "HIO.EXE").is_file(), "original game not in hio2/")
class ExtractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.blocks = extract.blocks()
        cls.data = {}

    def get(self, fid):
        if fid not in self.data:
            self.data[fid] = source(fid)
        return self.data[fid]

    def test_roundtrip_and_padding(self):
        bad = []
        for name, entries in self.blocks.items():
            for e in entries:
                fid, off = e["id"].split(":")
                d, off = self.get(fid), int(off, 16)
                a, b = e["field"]
                if e.get("align") == "rows":
                    p = a
                    for w, row in zip(extract.DESC_ROWS, e["zh"].split("\n")):
                        raw = codec.encode(row)
                        if d[p:p + len(raw)] != raw or not is_pad(d, p + len(raw), p + w):
                            bad.append(e["id"])
                        p += w
                    continue
                raw = codec.encode(e["zh"])
                if d[off:off + len(raw)] != raw or not a <= off <= off + len(raw) <= b:
                    bad.append(e["id"])
                elif e.get("align") and not (is_pad(d, a, off) and is_pad(d, off + len(raw), b)):
                    bad.append(e["id"] + " pad")
        self.assertEqual(bad, [])

    def test_fields_tile_without_overlap(self):
        """Within a file, fields never overlap; script pages and HIO pools leave no gap inside their range."""
        bad = []
        for name, entries in self.blocks.items():
            by = {}
            for e in entries:
                by.setdefault(e["id"].split(":")[0], []).append(e["field"])
            for fid, fs in by.items():
                fs.sort()
                bad += [f"{fid} overlap {x} {y}" for x, y in zip(fs, fs[1:]) if x[1] > y[0]]
                if name in ("VIS", "OP_E", "ED_E"):
                    bad += [f"{fid} gap {x} {y}" for x, y in zip(fs, fs[1:]) if x[1] != y[0]]
        hio = self.get("HIO")
        between = lambda x, y: all(t in extract.PAD or extract.ctl(t) for _, t in extract.tokens(hio, x, y))  # noqa: E731
        for kind, a, b, *_ in extract.HIO_POOLS:
            fs = sorted(e["field"] for e in self.blocks["HIO"] if a <= e["field"][0] < b)
            if not between(a, fs[0][0]):
                bad.append(f"HIO pool {a:#x} starts with text outside an entry")
            bad += [f"HIO pool {a:#x} gap {x}" for x, y in zip(fs, fs[1:]) if not between(x[1], y[0])]
            # a field never swallows a NUL terminator, separator or colour byte
            bad += [f"HIO {f} holds a control byte" for f in fs if any(extract.ctl(t) for _, t in extract.tokens(hio, *f))]
            # pool ends are hand-checked: the bytes after each end are code/data (docs/re/text_sites.md)
        self.assertEqual(bad, [])

    def test_no_text_left_out(self):
        """Coverage against the originals: every run of 4+ Big5 hanzi in a game file is inside an entry's field,
        or in a waived file/range below. A new hit means text the extractor misses (or data to waive, with a why)."""
        fields = {}
        for es in self.blocks.values():
            for e in es:
                fields.setdefault(e["id"].split(":")[0], []).append(e["field"])
        missed = []
        for f in sorted(extract.GAME.iterdir()):
            stem = f.stem
            if any(f.name.startswith(p) or f.suffix == p for p in WAIVED_FILES):
                continue
            d = extract.image(f.name) if f.suffix == ".EXE" else f.read_bytes()
            for m in RUN.finditer(d):
                s = m.start()
                while d[s:s + 2] == b"\xa1\x40":  # skip leading U+3000 (pairs, not bytes)
                    s += 2
                t = codec.decode(m.group())
                if sum(0x4E00 <= ord(c) <= 0x9FFF for c in t) < 4:
                    continue
                if any(a <= s < b for a, b in fields.get(stem, [])):
                    continue
                if f.name[:2] in ("IM", "MM") and s >= 8 * extract.REC:
                    continue  # stale buffer after the 8 records (docs/re/text_inventory.md)
                if not any(w[0] == f.name and w[1] <= s < w[2] for w in WAIVED_RANGES):
                    missed.append(f"{f.name} {s:#x} {t[:20]}")
        self.assertEqual(missed, [])

    def test_counts(self):
        n = {k: len(v) for k, v in self.blocks.items()}
        self.assertEqual(n["TM"], 128)
        self.assertEqual(sum(e["kind"] == "item_name" for e in self.blocks["IMMM"]), 128)
        self.assertEqual(sum(e["kind"] == "spell_name" for e in self.blocks["IMMM"]), 24)
        self.assertEqual(sum(e.get("section", False) for e in self.blocks["VIS"]), 24)  # one per `mov si`

    def test_dump_matches_extractor(self):
        """script/*.json holds exactly what blocks() extracts (zh and fields); only en/notes are hand-edited."""
        for name, entries in self.blocks.items():
            saved = json.loads((ROOT / "script" / f"{name}.json").read_text(encoding="utf-8"))
            strip = lambda es: [{k: v for k, v in e.items() if k not in ("en", "notes")} for e in es]  # noqa: E731
            self.assertEqual(strip(saved), strip(entries), name)


if __name__ == "__main__":
    unittest.main()
