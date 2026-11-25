"""extract.py: dump every text site (docs/re/text_sites.md) to script/<block>.json.

usage: python3 tools/extract.py [--force]
Without --force it only writes missing files. With --force it rewrites them but keeps `en` and `notes` by id, so a
dump never loses English. Entry fields:
  id     "<FILE>:<image or file offset, hex>" (stable, address-based)
  kind   what it is (message, item, help, ...)
  zh     source text (tools/codec.py notation; padding stripped)
  field  [start, end): the bytes the English may use, in the same file and offset space as id
  align  left | right | center (fixed fields only: how insert pads)
  en     translation ("" = untranslated)
Offsets: CVD/HMF files are file offsets; EXEs are image offsets (file minus the MZ header).
blocks() is also what tests/test_extract.py checks: roundtrip, padding and coverage against hio2/.
"""
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import codec  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "hio2"
PAD = {b"\xa1\x40", b" ", b"\x00"}  # U+3000, ASCII space, NUL

# HIO.EXE, image offsets. Tables: (kind, start, count, stride, field bytes, align)
HIO_TABLES = [
    ("spell", 0x0E046, 22, 24, 18, "left"),   # + 6 data bytes per record; the last 2 break stride (pool below)
    ("floor", 0x1A39C, 21, 16, 16, "center"),  # one per MPnn
    ("unit", 0x1CF70, 48, 16, 16, "left"),
    ("item", 0x1D538, 128, 36, 20, "left"),   # + 16 data bytes per record
]
# Pools: (kind, start, end); split into entries at padding, each entry's field runs to the next entry
HIO_POOLS = [
    ("spell", 0x0E256, 0x0E27C),     # 銀龍召喚術 (18 bytes, no data), 死靈召喚術 (20)
    ("label", 0x0E9BC, 0x0E9CE),     # 沒有
    ("label", 0x0FFA9, 0x0FFBB),     # ＧＡＭＥ　ＯＶＥＲ
    ("label", 0x12357, 0x1236F),     # 大召喚ＦＬＯＯＲ－ (the full-width digits after it are a glyph strip)
    ("message", 0x14157, 0x1417B),
    ("dialog", 0x15577, 0x15627),    # window-layout save/load
    ("dialog", 0x1580D, 0x1581F),    # 物品環境設定
    ("dialog", 0x1594A, 0x159C2),    # item settings options
    ("dialog", 0x15AA0, 0x15ACE),    # restart scenario
    ("dialog", 0x18202, 0x1824C),    # save/load prompts
    ("menu", 0x18DC2, 0x18E76),      # title + record menu
    ("toolhelp", 0x192F3, 0x1996A),  # toolbar help
    ("help", 0x19FB0, 0x1A39C),      # 魔界ＨＥＬＰ pages
    ("message", 0x1E740, 0x1E91A),   # battle/status messages, then [colour byte] + 6-byte popup labels
    ("attr", 0x1E91A, 0x1E93A, "whole"),  # 沒有 毒 石化 混亂 火 冰 雷 星 全部 沒有: 2- and 4-byte names, split unknown
    ("message", 0x1E93A, 0x1EA8F),   # 距離太遠了。 and the error popups
]
WHOLE_NOTE = "field boundaries unknown: find which names the game draws (marker build) before translating"
# ^-scripts: (file, image start, image end); sections end at ^\ and are loaded by `mov si, imm` (relocatable)
SCRIPTS = [("VIS", 0x15A2, 0x325C),   # code follows
           ("OP_E", 0x2661, 0x2DCB),  # end of image
           ("ED_E", 0x25A4, 0x306C)]  # ED01.WLF.. file list follows
TM_SLOT = 512
REC = 196  # IM/MM record: 48-byte name row (name left, stat tag right), then rows of 50/50/48 bytes
DESC_ROWS = (50, 50, 48)


def image(name):
    d = (GAME / name).read_bytes()
    return d[int.from_bytes(d[8:10], "little") * 16:]


def tokens(data, a, b):
    """Split data[a:b] into codec units: (offset, bytes)."""
    i = a
    while i < b:
        n = 2 if (0xA1 <= data[i] <= 0xF9 or data[i] in (0x5C, 0x5E)) and i + 1 < b else 1
        yield i, data[i:i + n]
        i += n


def ctl(t):
    """A lone byte that can't be text: NUL terminator, 0x01 help separator, colour byte, the 0x81 in toolbar help.
    Never part of an entry or a pad."""
    return len(t) == 1 and not 0x20 <= t[0] < 0x7F


def runs(data, a, b):
    """Non-padding runs in data[a:b] as (start, end); padding and control bytes end a run."""
    out, cur = [], None
    for i, t in tokens(data, a, b):
        if t in PAD or ctl(t):
            if cur is not None:
                out.append((cur, i))
                cur = None
        elif cur is None:
            cur = i
    if cur is not None:
        out.append((cur, b))
    return out


def entry(fid, off, kind, data, s, e, field, align=None):
    d = {"id": f"{fid}:{off:05X}", "kind": kind, "zh": codec.decode(data[s:e]), "field": list(field)}
    if align:
        d["align"] = align
    d["en"] = ""
    return d


def pool(fid, kind, data, a, b, whole=False):
    """Entries for the runs in data[a:b]; a field runs on over padding up to the next entry or control byte."""
    rs = runs(data, a, b)
    if whole:
        rs = [(rs[0][0], rs[-1][1])]
    out = []
    for k, (s, e) in enumerate(rs):
        end = rs[k + 1][0] if k + 1 < len(rs) else b
        end = next((i for i, t in tokens(data, e, end) if ctl(t)), end)
        out.append(entry(fid, s, kind, data, s, e, (s, end), "left"))
        if whole:
            out[-1]["notes"] = WHOLE_NOTE
    return out


def table(fid, kind, data, start, count, stride, width, align):
    out = []
    for k in range(count):
        f0 = start + k * stride
        rs = runs(data, f0, f0 + width)
        assert len(rs) == 1, (fid, kind, k, rs)  # one name per field
        out.append(entry(fid, rs[0][0], kind, data, *rs[0], (f0, f0 + width), align))
    return out


def blocks():
    b = {}
    tm = []
    for n in range(1, 33):
        d = (GAME / f"TM{n:02d}.CVD").read_bytes()
        for s in range(4):
            a = s * TM_SLOT
            e = d.index(b"\\e", a) + 2
            assert e <= a + TM_SLOT
            tm.append(entry(f"TM{n:02d}", a, "message", d, a, e, (a, a + TM_SLOT)))
    b["TM"] = tm
    im = []
    for name in [f"IM{n:02d}" for n in range(1, 17)] + [f"MM{n:02d}" for n in range(1, 4)]:
        d = (GAME / f"{name}.CVD").read_bytes()
        kind = "item" if name[0] == "I" else "spell"
        for r in range(8):
            a = r * REC
            row = runs(d, a, a + 48)  # name, then the stat tag (which may hold padding: 【殺傷力　１２】)
            im.append(entry(name, a, kind + "_name", d, *row[0], (a, row[1][0] if len(row) > 1 else a + 48), "left"))
            if len(row) > 1:
                im.append(entry(name, row[1][0], kind + "_tag", d, row[1][0], row[-1][1], (row[1][0], a + 48), "right"))
            rows, p = [], a + 48
            for w in DESC_ROWS:
                rr = runs(d, p, p + w)
                rows.append(codec.decode(d[p:rr[-1][1]]) if rr else "")
                p += w
            im.append({"id": f"{name}:{a + 48:05X}", "kind": kind + "_desc", "zh": "\n".join(rows),
                       "field": [a + 48, a + REC], "align": "rows", "en": ""})
    b["IMMM"] = im
    d = image("HIO.EXE")
    hio = []
    for kind, start, count, stride, width, align in HIO_TABLES:
        hio += table("HIO", kind, d, start, count, stride, width, align)
    for kind, a, e, *whole in HIO_POOLS:
        hio += pool("HIO", kind, d, a, e, bool(whole))
    b["HIO"] = sorted(hio, key=lambda x: x["field"][0])
    for name, start, end in SCRIPTS:
        d = image(f"{name}.EXE")
        # one entry per page (ends ^@, ^\ or \e); pages of a section are read in sequence, so insert concatenates them
        pages, a, new_section = [], start, True
        for i, t in tokens(d, start, end):
            if t in (b"^@", b"^\\", b"\\e"):
                pages.append(entry(name, a, "script", d, a, i + 2, (a, i + 2)))
                if new_section:
                    pages[-1]["section"] = True  # a `mov si, imm` points here
                new_section, a = t == b"^\\", i + 2
        assert a == end, (name, hex(a), hex(end))
        b[name] = pages
    mp = []
    for n in range(1, 22):
        d = (GAME / f"MP{n:02d}.HMF").read_bytes()
        for k in range(25):
            f0 = 0x1A40 + 16 * k
            rs = runs(d, f0, f0 + 16)
            if rs:
                assert len(rs) == 1, (n, k, rs)
                mp.append(entry(f"MP{n:02d}", rs[0][0], "unit_name", d, *rs[0], (f0, f0 + 16), "left"))
    b["MP"] = mp
    return b


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--force", action="store_true")
    a = ap.parse_args()
    out = ROOT / "script"
    for name, entries in blocks().items():
        path = out / f"{name}.json"
        if path.exists():
            if not a.force:
                print("kept", path.name)
                continue
            old = {e["id"]: e for e in json.loads(path.read_text(encoding="utf-8"))}
            for e in entries:
                for k in ("en", "notes"):
                    if old.get(e["id"], {}).get(k):
                        e[k] = old[e["id"]][k]
        path.write_text(json.dumps(entries, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
        print("wrote", path.name, len(entries))


if __name__ == "__main__":
    main()
