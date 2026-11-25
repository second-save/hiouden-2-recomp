"""bigram.py: English as two 8 px letters per 16x15 glyph cell (decision D4, docs/fonts.md).

The game draws every 2-byte Big5 code as one 16 px cell from A.NUM/A.CHA, so English text is cut into letter pairs and
each distinct pair gets a code whose glyph shows both letters (Spleen 8x16, rows 1-15). Control codes (`\\x`, `^x`,
`[XX]`) stay as they are and restart the pairing, so an odd run before a code wastes half a cell. A non-ASCII
character in English (rare: kept symbols) is drawn with its original Big5 glyph, one cell.

    cols(line) -> visible width in Latin columns (2 per cell); lines(text, breaks) -> the visual lines
    Font(keep).add(text) collects pairs; .encode(text) -> bytes; .write(num, cha) builds A.NUM/A.CHA
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import codec  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
BDF = ROOT / "font" / "spleen-8x16.bdf"
CAPACITY = 4001  # A.NUM slots the engine reads (docs/fonts.md)
TOKEN = re.compile(r"\\[\s\S]|\^(?:\[[0-9A-F]{2}\]|[\s\S])|\[[0-9A-F]{2}(?:[0-9A-F]{2})?\]")


def parts(text):
    """[(is_code, str)] in order."""
    out, i = [], 0
    for m in TOKEN.finditer(text):
        if m.start() > i:
            out.append((False, text[i:m.start()]))
        out.append((True, m.group()))
        i = m.end()
    if i < len(text):
        out.append((False, text[i:]))
    return out


def cells(plain):
    """Cells used by a code-free run: ASCII in pairs, anything else one cell each."""
    n, run = 0, 0
    for c in plain:
        if " " <= c <= "~":
            run += 1
        else:
            n += (run + 1) // 2 + 1
            run = 0
    return n + (run + 1) // 2


def lines(text, breaks):
    """Split text into visual lines at the given codes (and at a literal newline); codes are dropped."""
    out, cur = [], []
    for is_code, s in parts(text):
        if is_code:
            if s in breaks:
                out.append(cur)
                cur = []
        else:
            for k, piece in enumerate(s.split("\n")):
                if k:
                    out.append(cur)
                    cur = []
                if piece:
                    cur.append(piece)
    out.append(cur)
    return out


def cols(line_runs):
    return 2 * sum(cells(r) for r in line_runs)


def wrap(text, width):
    """Greedy word wrap of a code-free paragraph to rows of at most `width` columns (a word never splits)."""
    rows, cur = [], ""
    for word in text.split():
        trial = f"{cur} {word}" if cur else word
        if cur and 2 * cells(trial) > width:
            rows.append(cur)
            cur = word
        else:
            cur = trial
    return rows + [cur] if cur else rows


def pairs(plain):
    """The pair strings a code-free run is drawn with (non-ASCII chars come back as themselves)."""
    out, run = [], ""
    for c in plain + "\0":
        if " " <= c <= "~":
            run += c
            continue
        if len(run) % 2:
            run += " "
        out += [run[i:i + 2] for i in range(0, len(run), 2)]
        run = ""
        if c != "\0":
            out.append(c)
    return out


def bdf_glyphs():
    """{char: [16 row bytes]} from the Spleen BDF (ASCII only)."""
    g, cur, rows = {}, None, None
    for line in BDF.read_text(encoding="ascii").splitlines():
        if line.startswith("ENCODING "):
            cur = int(line.split()[1])
        elif line == "BITMAP":
            rows = []
        elif line == "ENDCHAR":
            if cur is not None and 0x20 <= cur <= 0x7E:
                g[chr(cur)] = rows
            cur, rows = None, None
        elif rows is not None:
            rows.append(int(line, 16))
    return g


def free_codes(taken):
    """Big5-shaped codes for pairs: lead A4-F9, trail 40-7E/A1-FE, skipping trails that read as [ \\ ^ and taken codes."""
    for lead in range(0xA4, 0xFA):
        for trail in list(range(0x40, 0x7F)) + list(range(0xA1, 0xFF)):
            if trail in (0x5B, 0x5C, 0x5E):
                continue
            c = bytes([lead, trail])
            if c not in taken:
                yield c


class Font:
    def __init__(self, orig_num, orig_cha, keep_codes):
        """keep_codes: original codes whose glyphs must stay (symbols, icons, any Chinese still shown)."""
        self.orig = {}
        for i in range(0, len(orig_num), 2):
            self.orig.setdefault(orig_num[i:i + 2], orig_cha[15 * i:15 * i + 30])
        self.keep = [c for c in dict.fromkeys(keep_codes) if c in self.orig]
        self.pair_code = {}
        self._free = free_codes(set(self.orig))  # never reuse a code the original font knows: no stale meaning

    def add(self, text):
        for is_code, s in parts(text):
            if not is_code:
                for p in pairs(s):
                    if len(p) == 2 and p not in self.pair_code:
                        self.pair_code[p] = next(self._free)

    def encode(self, text):
        out = bytearray()
        for is_code, s in parts(text):
            if is_code:
                out += codec.encode(s)
                continue
            for p in pairs(s):
                out += self.pair_code[p] if len(p) == 2 else codec.encode(p)
        return bytes(out)

    def slots(self):
        return len(self.keep) + len(self.pair_code)

    def write(self, num_path, cha_path):
        if self.slots() > CAPACITY:
            raise SystemExit(f"font needs {self.slots()} slots, the engine reads {CAPACITY}")
        g = bdf_glyphs()
        num, cha = bytearray(), bytearray()
        for c in self.keep:
            num += c
            cha += self.orig[c]
        for p, c in self.pair_code.items():
            num += c
            for r in range(1, 16):  # BDF rows 1-15: row 0 is blank for every letter, descenders end by row 15
                cha += bytes([g[p[0]][r], g[p[1]][r]])
        Path(num_path).write_bytes(num)
        Path(cha_path).write_bytes(cha)


if __name__ == "__main__":
    assert cells("ab") == 1 and cells("abc") == 2 and cells("a■b") == 3
    assert pairs("Hey") == ["He", "y "] and pairs("a■") == ["a ", "■"]
    assert [cols(r) for r in lines(r"\2Hi\Fthere\nyou", {r"\n"})] == [8, 4]  # Hi | there: 1 + 3 cells
    print("selftest ok")
