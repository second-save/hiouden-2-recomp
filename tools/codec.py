"""Big5 text codec for Hiouden II's script bytes, roundtrip-exact.

Byte grammar (docs/re/text_sites.md): a lead byte 0xA1-0xF9 starts a 2-byte Big5 cell (the trail may be 0x5B `[`,
0x5C `\\` or 0x5E `^`, so cells are taken first); `\\` and `^` are control codes that take exactly one argument byte
(`\\n`, `\\e`, `^0`, `^\\`, and `^` + 0x01..0x14 positioning in OP_E/ED_E); other bytes 0x20-0x7E are ASCII.
Anything else, or a cell cp950 can't map both ways, is written `[XX]` / `[XXYY]` and encodes back to the same bytes.

    decode(b) -> str, encode(str) -> bytes, with encode(decode(b)) == b for every b.
"""
import re

ESC = re.compile(r"\[([0-9A-F]{2}|[0-9A-F]{4})\]")
EXTRA = {b"\xc8\xd2": "№"}  # glyphs in A.NUM that cp950 lacks (C8D2 drawn as № in MP05/MP10 unit names)
EXTRA_ENC = {v: k for k, v in EXTRA.items()}


def _cell(b):
    if b in EXTRA:
        return EXTRA[b]
    try:
        c = b.decode("cp950")
        return c if len(c) == 1 and c.encode("cp950") == b else None
    except UnicodeError:
        return None


def decode(data):
    out, i, n = [], 0, len(data)
    while i < n:
        b = data[i]
        if 0xA1 <= b <= 0xF9 and i + 1 < n:
            c = _cell(data[i:i + 2])
            out.append(c if c else f"[{data[i:i + 2].hex().upper()}]")
            i += 2
        elif b in (0x5C, 0x5E) and i + 1 < n:  # code + one argument byte
            a = data[i + 1]
            out.append(chr(b) + (chr(a) if 0x20 <= a <= 0x7E and a != 0x5B else f"[{a:02X}]"))
            i += 2
        elif 0x20 <= b <= 0x7E and b != 0x5B:
            out.append(chr(b))
            i += 1
        else:
            out.append(f"[{b:02X}]")
            i += 1
    return "".join(out)


def encode(text):
    out, i = bytearray(), 0
    while i < len(text):
        m = ESC.match(text, i)
        if m:
            out += bytes.fromhex(m[1])
            i = m.end()
            continue
        c = text[i]
        out += c.encode("ascii") if ord(c) < 0x80 else EXTRA_ENC.get(c) or c.encode("cp950")
        i += 1
    return bytes(out)


if __name__ == "__main__":
    import sys
    data = open(sys.argv[1], "rb").read()
    print(decode(data[int(sys.argv[2], 0):int(sys.argv[3], 0)]))
