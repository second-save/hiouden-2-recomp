"""insert.py: write script/*.json English into a copy of the game (called by build.py; never on hio2/).

Per entry: English if it has some, else a twin's English (same `zh`), else the original bytes stay (and their
Chinese glyphs stay in the font). Fixed fields are re-padded by `align` with U+3000 cells; TM files become 4 slots
of 1024 bytes (D10 patch to HIO.EXE); ^-script blocks are laid out again, in place when they fit, else at the end of
the image up to each file's ceiling, and every `mov si` that pointed at a page is rewritten; MP unit names are also
copied into the save templates (MAPn/WINDn); A.NUM/A.CHA are rebuilt with the letter-pair glyphs (tools/bigram.py).
marker=True writes `#n` markers at full field width instead (ids in work/qa/marker_ids.tsv).
"""
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bigram  # noqa: E402
import codec  # noqa: E402
import extract  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
U3000 = b"\xa1\x40"
DESC_COLS = 50  # item/spell description line (25 cells)
TM_SLOT = 1024
TM_PATCH = (0x17AED, bytes.fromhex("80e40332c003c0"), bytes.fromhex("250003c1e00290"))  # and ax,0x300; shl ax,2; nop
# ^-scripts: DS base (image offset of DS:0) and the highest image offset the file may reach (docs/re/text_sites.md)
SCRIPT_FILES = {"VIS": (0x0000, 0x7E00),   # VIS's own buffer starts at DS:0x7E00
                "OP_E": (0x2090, 0x334B),  # HIOSYS reads its table at linear 0xE95B (image 0x334B)
                "ED_E": (0x2130, 0x334B)}
# data between a block and the image end that moves behind the new text: (start, end, opcode of its pointers)
SCRIPT_TAIL = {"ED_E": (0x306C, 0x30EA, 0xBA)}  # ED01.WLF.. ED14.WLF, each loaded by `mov dx, imm` (14 refs)
EXE_HDR = {"HIO": 0x1800, "VIS": 0x200, "OP_E": 0x200, "ED_E": 0x200}


def load_blocks():
    return {p.stem: json.loads(p.read_text(encoding="utf-8")) for p in sorted((ROOT / "script").glob("*.json"))}


def english(blocks):
    """id -> English (own or a twin's); missing = keep the original bytes."""
    by_zh = {}
    for es in blocks.values():
        for e in es:
            if e.get("en"):
                by_zh.setdefault(e["zh"], e["en"])
    return {e["id"]: e.get("en") or by_zh.get(e["zh"]) for es in blocks.values() for e in es
            if e.get("en") or by_zh.get(e["zh"])}


def filler(width, tag=""):
    s = (tag + "Lorem ipsum dolor sit amet consectetur adipiscing elit sed do eiusmod tempor") if tag else "=" * width
    return (s * 3)[:width]


def markers(blocks):
    """id -> marker text that fills each entry's room, keeping every control code."""
    out, ids = {}, []
    for es in blocks.values():
        for e in es:
            n = len(ids)
            ids.append(e["id"])
            tag = f"#{n} "
            if e.get("align") == "rows":
                out[e["id"]] = "\n".join(filler(DESC_COLS, tag if r == 0 else "-") for r in range(3))
            elif e.get("align"):
                out[e["id"]] = filler(min(46, e["field"][1] - e["field"][0]), tag)
            else:  # TM / script page: replace each text run by filler of the same width
                parts, first = [], True
                for is_code, s in bigram.parts(e["zh"]):
                    if is_code:
                        parts.append(s)
                    else:
                        w = 2 * bigram.cells(s)
                        parts.append(filler(w, tag if first else "x"))
                        first = False
                out[e["id"]] = "".join(parts)
    return out, ids


def pack(raw, size, align):
    pad = size - len(raw)
    assert pad >= 0, (raw, size)
    pads = U3000 * (pad // 2) + b" " * (pad % 2)
    if align == "right":
        return pads + raw
    if align == "center":
        k = (pad // 4) * 2  # whole cells on the left
        return pads[:k] + raw + pads[k:]
    return raw + pads


def write(path, data):
    Path(path).write_bytes(bytes(data))


def insert(out, marker=False):
    out = Path(out)
    assert out.resolve() != (ROOT / "hio2").resolve(), "never write into hio2/"
    blocks = load_blocks()
    if marker:
        en, ids = markers(blocks)
        (ROOT / "work" / "qa").mkdir(parents=True, exist_ok=True)
        (ROOT / "work/qa/marker_ids.tsv").write_text("".join(f"#{n}\t{i}\n" for n, i in enumerate(ids)), encoding="utf-8")
    else:
        en = english(blocks)

    def text_of(e):
        t = en.get(e["id"])
        if t is not None and e.get("align") == "rows" and "\n" not in t:
            t = "\n".join(bigram.wrap(t, DESC_COLS))
        if t is not None and e["id"].startswith("TM"):
            t = bigram.tm_layout(bigram.slide_pauses(t))[0]  # codes only: the original's scroll rules, no gaps
        return t

    # font: keep symbols/icons (leads below 0xA4) and every glyph of text that stays Chinese or is non-ASCII
    num, cha = (ROOT / "hio2/A.NUM").read_bytes(), (ROOT / "hio2/A.CHA").read_bytes()
    keep = [num[i:i + 2] for i in range(0, len(num), 2) if num[i] < 0xA4]
    for es in blocks.values():
        for e in es:
            t = text_of(e)
            src = e["zh"] if t is None else t
            keep += [codec.encode(c) for is_code, s in bigram.parts(src) if not is_code for c in s if ord(c) > 0x7E]
    font = bigram.Font(num, cha, keep)
    for es in blocks.values():
        for e in es:
            t = text_of(e)
            if t is not None:
                font.add(t)

    files = {}

    def data(name):
        if name not in files:
            files[name] = bytearray((out / name).read_bytes())
        return files[name]

    def put(name, off, raw):
        d = data(name)
        d[off:off + len(raw)] = raw

    # IM/MM records: the popup draws the 98-cell record as a stream: line 1 has 23 cells (the icon takes 2), the
    # next lines 25 (verified in-game 2026-11-25, work/qa/rb). English: line 1 = name + tag right-aligned in 46
    # columns, then 3 description lines of 50: 46 + 150 = 196 bytes, the whole record.
    recs = {}
    for e in blocks["IMMM"]:
        fid = e["id"].split(":")[0]
        a = e["field"][0] - e["field"][0] % extract.REC
        recs.setdefault((fid, a), {})[e["kind"].split("_")[1]] = e
    for (fid, a), parts_ in recs.items():
        texts = {k: text_of(e) for k, e in parts_.items()}
        if any(t is None for t in texts.values()):
            continue  # untranslated record: keep the original bytes and layout
        name, tag = font.encode(texts["name"]), font.encode(texts.get("tag", ""))
        assert len(name) + len(tag) + (2 if tag else 0) <= 46, (fid, a, texts)
        raw = name + pack(b"", 46 - len(name) - len(tag), "left") + tag
        rows = texts["desc"].split("\n")
        assert len(rows) <= 3, (fid, a, rows)
        for row in rows + [""] * (3 - len(rows)):
            raw += pack(font.encode(row), DESC_COLS, "left")
        put(f"{fid}.CVD", a, pack(raw, extract.REC, "left"))

    # fixed fields: HIO.EXE, MP
    for b in ("HIO", "MP"):
        for e in blocks[b]:
            t = text_of(e)
            if t is None:
                continue
            fid = e["id"].split(":")[0]
            name, base = (f"{fid}.EXE", EXE_HDR[fid]) if fid == "HIO" else (f"{fid}{'.HMF' if fid[:2] == 'MP' else '.CVD'}", 0)
            a, z = e["field"]
            put(name, base + a, pack(font.encode(t), z - a, e["align"]))

    # TM: 4 slots of 1024 bytes
    for n in range(1, 33):
        name = f"TM{n:02d}.CVD"
        old = (ROOT / "hio2" / name).read_bytes()
        new = bytearray()
        for s in range(4):
            e = blocks["TM"][(n - 1) * 4 + s]
            t = text_of(e)
            raw = font.encode(t) if t is not None else old[s * 512:old.index(b"\\e", s * 512) + 2]
            assert len(raw) <= TM_SLOT, (e["id"], len(raw))
            new += raw + b"\0" * (TM_SLOT - len(raw))
        files[name] = new
    off, want, patch = TM_PATCH
    hio = data("HIO.EXE")
    assert hio[0x1800 + off:0x1800 + off + 7] == want
    hio[0x1800 + off:0x1800 + off + 7] = patch

    # ^-scripts
    for b, (ds, ceiling) in SCRIPT_FILES.items():
        relayout(b, blocks[b], text_of, font, data(f"{b}.EXE"), ds, ceiling)

    # save templates: unit names copied from the maps
    names = {}
    for e in blocks["MP"]:
        t = text_of(e)
        if t is not None:
            names[codec.encode(e["zh"])] = pack(font.encode(t), 16, "left")
    # only the blank slot templates (.INI): .HIO are the player's save slots and .TMP the live state, never patched
    for f in sorted((ROOT / "hio2").iterdir()):
        if f.name.startswith("MAP") and f.suffix == ".INI":
            slots = [0x8000 + 0x100 * k + 0x70 for k in range(0x80)]
        elif f.name.startswith("WIND") and f.suffix == ".INI":
            slots = [0x122C + 16 * k for k in range(25)]
        else:
            continue
        d = data(f.name)
        for s in slots:
            key = bytes(d[s:s + 16]).rstrip(b"\0").removesuffix(U3000 * 8)
            while key.endswith(U3000):
                key = key[:-2]
            if key in names:
                d[s:s + 16] = names[key]

    for name, d in files.items():
        write(out / name, d)
    font.write(out / "A.NUM", out / "A.CHA")
    return font


def relayout(b, pages, text_of, font, d, ds, ceiling):
    hdr = EXE_HDR[b]
    lo, hi = pages[0]["field"][0], pages[-1]["field"][1]
    raws = []
    for e in pages:
        t = text_of(e)
        raws.append(font.encode(t) if t is not None else bytes(d[hdr + e["field"][0]:hdr + e["field"][1]]))
    total = sum(map(len, raws))
    img_len = len(d) - hdr
    tail = SCRIPT_TAIL.get(b)
    tail_bytes = bytes(d[hdr + tail[0]:hdr + tail[1]]) if tail else b""
    grows_at_end = (tail[1] if tail else hi) == img_len  # nothing but (movable) tail data after the block
    if total <= hi - lo and not tail:
        start = lo  # fits where it was
    elif grows_at_end:
        start = lo  # extend from the old start; the tail (if any) follows the new text
    else:
        start = (img_len + 15) & ~15  # code follows the block: move it to the end of the image
    end_all = start + total + len(tail_bytes)
    assert end_all <= ceiling, f"{b}: text ends at {end_all:#x}, past the ceiling {ceiling:#x}"
    # pointers: every `mov si, imm16` (BE) outside the old block that names a page start
    starts = {e["field"][0] - ds: k for k, e in enumerate(pages)}
    refs = [(i, starts[d[i + 1] | d[i + 2] << 8]) for i in range(hdr, hdr + lo)
            if d[i] == 0xBE and (d[i + 1] | d[i + 2] << 8) in starts]
    refs += [(i, starts[d[i + 1] | d[i + 2] << 8]) for i in range(hdr + hi, len(d) - 2)
             if d[i] == 0xBE and (d[i + 1] | d[i + 2] << 8) in starts and not (tail and hdr + tail[0] <= i)]
    assert {k for _, k in refs} >= {k for k, e in enumerate(pages) if e.get("section")}, f"{b}: a section lost its pointer"
    tail_refs = [i for i in range(hdr, hdr + lo) if tail and d[i] == tail[2]
                 and tail[0] - ds <= (d[i + 1] | d[i + 2] << 8) < tail[1] - ds]
    # clear the old block (and tail) and grow the file as needed
    d[hdr + lo:hdr + (tail[1] if tail else hi)] = b"\0" * ((tail[1] if tail else hi) - lo)
    if len(d) < hdr + end_all:
        d += b"\0" * (hdr + end_all - len(d))
    pos, new_start = start, []
    for raw in raws:
        new_start.append(pos)
        d[hdr + pos:hdr + pos + len(raw)] = raw
        pos += len(raw)
    if tail:
        d[hdr + pos:hdr + pos + len(tail_bytes)] = tail_bytes
        for i in tail_refs:
            v = d[i + 1] | d[i + 2] << 8
            d[i + 1:i + 3] = struct.pack("<H", v + pos - tail[0])
        assert len(tail_refs) == 14, (b, len(tail_refs))
    for i, k in refs:
        d[i + 1:i + 3] = struct.pack("<H", new_start[k] - ds)
    size = len(d)
    d[2:6] = struct.pack("<HH", size % 512, (size + 511) // 512)  # MZ: bytes in last page, pages


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--marker", action="store_true")
    a = ap.parse_args()
    f = insert(a.out, a.marker)
    print(f"inserted into {a.out}: {f.slots()} font slots ({len(f.pair_code)} letter pairs)")
