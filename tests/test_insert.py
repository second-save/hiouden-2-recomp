import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))


@unittest.skipUnless((ROOT / "hio2" / "HIO.EXE").is_file(), "original game not in hio2/")
class InsertTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import build
        import insert
        cls.insert = insert
        cls.tmp = tempfile.TemporaryDirectory()
        cls.out = build.build(Path(cls.tmp.name) / "game")
        cls.blocks = insert.load_blocks()
        cls.en = insert.english(cls.blocks)
        import bigram
        num, cha = (ROOT / "hio2/A.NUM").read_bytes(), (ROOT / "hio2/A.CHA").read_bytes()
        # rebuild the same pair table the build made: same keep list order and same add order
        cls.num = (cls.out / "A.NUM").read_bytes()
        cls.codes = {cls.num[i:i + 2] for i in range(0, len(cls.num), 2)}

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def image(self, name, hdr):
        return (self.out / name).read_bytes()[hdr:]

    def test_font_fits_and_covers_every_cell(self):
        self.assertLessEqual(len(self.num) // 2, 4001)
        self.assertEqual(len((self.out / "A.CHA").read_bytes()), 15 * len(self.num))
        # every 2-byte cell in the built TM files decodes to a glyph the font has
        missing = set()
        for n in range(1, 33):
            d = (self.out / f"TM{n:02d}.CVD").read_bytes()
            for s in range(4):
                slot = d[s * 1024:(s + 1) * 1024]
                i, end = 0, slot.index(b"\\e")
                while i < end:
                    if slot[i] in (0x5C, 0x5E):
                        i += 2
                    elif slot[i] >= 0x81:
                        if slot[i:i + 2] not in self.codes:
                            missing.add(slot[i:i + 2].hex())
                        i += 2
                    else:
                        i += 1
        self.assertEqual(missing, set())

    def test_tm_slots_are_1024_and_end_with_e(self):
        for n in range(1, 33):
            d = (self.out / f"TM{n:02d}.CVD").read_bytes()
            self.assertEqual(len(d), 4096)
            for s in range(4):
                self.assertIn(b"\\e", d[s * 1024:(s + 1) * 1024], f"TM{n:02d} slot {s}")
        hio = (self.out / "HIO.EXE").read_bytes()
        self.assertEqual(hio[0x1800 + 0x17AED:0x1800 + 0x17AF4], bytes.fromhex("250003c1e00290"))

    def test_script_pointers_land_on_their_pages(self):
        """Every `mov si` that pointed at a page in the original now points at a page start in the build, in the same
        order; and the built page bytes there end with that page's terminator."""
        for b, (ds, ceiling) in self.insert.SCRIPT_FILES.items():
            orig = (ROOT / "hio2" / f"{b}.EXE").read_bytes()[0x200:]
            new = self.image(f"{b}.EXE", 0x200)
            pages = self.blocks[b]
            starts = {e["field"][0] - ds: e for e in pages}
            lo, hi = pages[0]["field"][0], pages[-1]["field"][1]
            sites = [i for i in range(lo) if orig[i] == 0xBE and (orig[i + 1] | orig[i + 2] << 8) in starts]
            sites += [i for i in range(hi, len(orig) - 2) if orig[i] == 0xBE and (orig[i + 1] | orig[i + 2] << 8) in starts
                      and not (b == "ED_E" and i >= 0x306C)]
            self.assertTrue(sites, b)
            targets = {}
            for i in sites:
                e = starts[orig[i + 1] | orig[i + 2] << 8]
                targets[e["field"][0]] = ds + (new[i + 1] | new[i + 2] << 8)
            first = min(targets.values())
            for old, target in targets.items():
                self.assertLessEqual(target, ceiling, (b, hex(old)))
                self.assertNotEqual(new[target], 0, (b, hex(old), "points at padding"))
                self.assertTrue(target == first or new[target - 2:target] in (b"^@", b"^\\", b"\\e"),
                                (b, hex(old), "not at a page start", new[target - 2:target]))
            # order kept: page starts that were in order stay in order
            olds = sorted(targets)
            self.assertEqual([targets[o] for o in olds], sorted(targets.values()), b)

    def test_ed_e_file_list_moved_with_its_pointers(self):
        new = self.image("ED_E.EXE", 0x200)
        orig = (ROOT / "hio2/ED_E.EXE").read_bytes()[0x200:]
        refs = [i for i in range(0x25A4) if orig[i] == 0xBA and 0xF3C <= (orig[i + 1] | orig[i + 2] << 8) < 0xFBA]
        self.assertEqual(len(refs), 14)
        for k, i in enumerate(sorted(refs, key=lambda i: orig[i + 1] | orig[i + 2] << 8)):
            off = 0x2130 + struct.unpack_from("<H", new, i + 1)[0]
            self.assertEqual(new[off:off + 9], f"ED{k + 1:02d}.WLF\0".encode(), hex(i))

    def test_fixed_fields_hold_their_english(self):
        import bigram
        bad = []
        hio = (self.out / "HIO.EXE").read_bytes()[0x1800:]
        for e in self.blocks["HIO"]:
            t = self.en.get(e["id"])
            if t is None:
                continue
            a, z = e["field"]
            self.assertEqual(len(hio[a:z]), z - a)
            if hio[a:z].count(b"\xa1\x40") * 2 + 2 * bigram.cells(t) < z - a - 1:
                bad.append(e["id"])
        self.assertEqual(bad, [])


if __name__ == "__main__":
    unittest.main()
