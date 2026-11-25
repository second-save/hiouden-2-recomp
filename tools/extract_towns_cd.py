#!/usr/bin/env python3
"""
Extract raw files from Hioden-II_FM-Towns_JA/Hiouden II (Japan).bin,
decompress Wolf Team LZ5 / L5L archives, and dump Japanese Shift-JIS
unit/monster names from MAP00..20.LZ5 and hio2/WIND0.HIO.
"""

from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parent.parent
CD_BIN = ROOT / "Hioden-II_FM-Towns_JA" / "Hiouden II (Japan).bin"
WIND0_HIO = ROOT / "hio2" / "WIND0.HIO"
OUT_RAW = ROOT / "extracted" / "towns_cd"
OUT_DEC = ROOT / "extracted" / "towns_decompressed"
OUT_DOC = ROOT / "docs" / "re" / "japanese_names.md"

SECTOR_RAW = 2352
SECTOR_HDR = 16
SECTOR_DATA = 2048


def init_lz5_ring() -> bytearray:
    """Initialize the 4096-byte LZ5 ring buffer (matching VIS.EXE @ 0x115C)."""
    ring = bytearray(4096)
    pos = 0
    for val in range(256):
        ring[pos : pos + 13] = bytes([val]) * 13
        pos += 13
    for val in range(256):
        ring[pos] = val
        pos += 1
    for val in range(255, -1, -1):
        ring[pos] = val
        pos += 1
    ring[pos : pos + 128] = b"\x00" * 128
    pos += 128
    ring[pos : pos + 110] = b"\x20" * 110
    pos += 110
    assert pos == 0xFEE
    return ring


def decompress_lz5_chunk(chunk: bytes) -> tuple[bytes, int]:
    """Decompress one LZ5 chunk starting with <csize, usize>. Returns (data, consumed)."""
    csize, usize = struct.unpack("<2I", chunk[:8])
    src = chunk[8 : 8 + csize]
    if len(src) != csize:
        raise ValueError(f"Truncated LZ5 chunk: expected {csize}, got {len(src)}")
    out = bytearray()
    ring = init_lz5_ring()
    r = 0xFEE
    pos = 0
    while pos < len(src) and len(out) < usize:
        flags = src[pos]
        pos += 1
        for bit in range(8):
            if pos >= len(src) or len(out) >= usize:
                break
            if flags & (1 << bit):
                b = src[pos]
                pos += 1
                out.append(b)
                ring[r] = b
                r = (r + 1) & 0xFFF
            else:
                if pos + 1 >= len(src):
                    raise ValueError("Truncated LZ5 backreference")
                b1 = src[pos]
                b2 = src[pos + 1]
                pos += 2
                offset = b1 | ((b2 & 0xF0) << 4)
                length = (b2 & 0x0F) + 3
                for _ in range(length):
                    if len(out) >= usize:
                        break
                    b = ring[offset]
                    offset = (offset + 1) & 0xFFF
                    out.append(b)
                    ring[r] = b
                    r = (r + 1) & 0xFFF
    if len(out) != usize:
        raise ValueError(f"LZ5 size mismatch: expected {usize}, got {len(out)}")
    return bytes(out), 8 + csize


def decompress_l5l(data: bytes) -> list[bytes]:
    """Decompress all concatenated LZ5 chunks in an .L5L archive."""
    chunks: list[bytes] = []
    pos = 0
    while pos + 8 <= len(data):
        dec, consumed = decompress_lz5_chunk(data[pos:])
        chunks.append(dec)
        pos += consumed
    if pos != len(data):
        raise ValueError(f"L5L trailing bytes: consumed {pos}/{len(data)}")
    return chunks


def read_cd_bytes(cd_data: bytes, lba: int, size: int) -> bytes:
    n_sectors = (size + SECTOR_DATA - 1) // SECTOR_DATA
    buf = bytearray()
    for i in range(n_sectors):
        off = (lba + i) * SECTOR_RAW + SECTOR_HDR
        buf.extend(cd_data[off : off + SECTOR_DATA])
    return bytes(buf[:size])


def extract_iso_tree(cd_data: bytes, lba: int, length: int, rel_prefix: Path) -> list[Path]:
    extracted: list[Path] = []
    dir_bytes = read_cd_bytes(cd_data, lba, length)
    pos = 0
    while pos < length:
        rec_len = dir_bytes[pos]
        if rec_len == 0:
            pos = ((pos // SECTOR_DATA) + 1) * SECTOR_DATA
            continue
        rec = dir_bytes[pos : pos + rec_len]
        ext_lba = struct.unpack("<I", rec[2:6])[0]
        ext_len = struct.unpack("<I", rec[10:14])[0]
        flags = rec[25]
        name_len = rec[32]
        name = rec[33 : 33 + name_len].decode("ascii", errors="replace").split(";")[0]
        if name not in ("\x00", "\x01"):
            rel_path = rel_prefix / name
            if flags & 2:
                extracted.extend(extract_iso_tree(cd_data, ext_lba, ext_len, rel_path))
            else:
                out_path = OUT_RAW / rel_path
                out_path.parent.mkdir(parents=True, exist_ok=True)
                out_path.write_bytes(read_cd_bytes(cd_data, ext_lba, ext_len))
                extracted.append(rel_path)
        pos += rec_len
    return extracted


def decode_sjis_slots(blob: bytes, count: int = 25) -> list[str]:
    names: list[str] = []
    for i in range(count):
        slot = blob[i * 16 : (i + 1) * 16]
        if len(slot) < 16:
            break
        # Strip trailing full-width spaces (0x81 0x40) and NULs
        text = slot.split(b"\x00", 1)[0].decode("cp932", errors="replace").rstrip("\u3000 ")
        names.append(text)
    return names


def main() -> None:
    cd_data = CD_BIN.read_bytes()
    pvd = read_cd_bytes(cd_data, 16, SECTOR_DATA)
    if pvd[1:6] != b"CD001":
        raise RuntimeError("Invalid ISO9660 PVD in CD image")

    root_rec = pvd[156 : 156 + 34]
    root_lba = struct.unpack("<I", root_rec[2:6])[0]
    root_len = struct.unpack("<I", root_rec[10:14])[0]

    # Locate HIO2/ in root directory
    root_dir = read_cd_bytes(cd_data, root_lba, root_len)
    pos = 0
    hio2_lba = None
    hio2_len = 0
    while pos < root_len:
        rlen = root_dir[pos]
        if rlen == 0:
            break
        rec = root_dir[pos : pos + rlen]
        nlen = rec[32]
        name = rec[33 : 33 + nlen].decode("ascii", errors="replace").split(";")[0]
        if name == "HIO2":
            hio2_lba = struct.unpack("<I", rec[2:6])[0]
            hio2_len = struct.unpack("<I", rec[10:14])[0]
            break
        pos += rlen
    if hio2_lba is None:
        raise RuntimeError("HIO2 directory not found on CD image")

    raw_files = extract_iso_tree(cd_data, hio2_lba, hio2_len, Path("HIO2"))
    print(f"Extracted {len(raw_files)} files from HIO2/ to {OUT_RAW}")

    lz5_count = 0
    l5l_chunk_count = 0
    map_names: dict[str, list[str]] = {}

    for rel in sorted(raw_files):
        src_path = OUT_RAW / rel
        raw_bytes = src_path.read_bytes()
        ext = rel.suffix.upper()
        if ext == ".LZ5":
            dec, consumed = decompress_lz5_chunk(raw_bytes)
            if consumed != len(raw_bytes):
                raise ValueError(f"{rel}: consumed {consumed}/{len(raw_bytes)}")
            out_path = OUT_DEC / rel.with_suffix(".bin")
            out_path.parent.mkdir(parents=True, exist_ok=True)
            out_path.write_bytes(dec)
            lz5_count += 1
            if rel.parent.name == "H2MF" and rel.name.startswith("MAP"):
                map_names[rel.name] = decode_sjis_slots(dec[0x1A40:0x1BD0], 25)
        elif ext == ".L5L":
            chunks = decompress_l5l(raw_bytes)
            chunk_dir = OUT_DEC / rel.with_suffix("")
            chunk_dir.mkdir(parents=True, exist_ok=True)
            for idx, dec in enumerate(chunks):
                (chunk_dir / f"chunk_{idx:02d}.bin").write_bytes(dec)
            l5l_chunk_count += len(chunks)

    print(f"Decompressed {lz5_count} .LZ5 files and {l5l_chunk_count} .L5L chunks to {OUT_DEC}")

    # Generate docs/re/japanese_names.md
    OUT_DOC.parent.mkdir(parents=True, exist_ok=True)
    lines: list[str] = [
        "# Japanese Shift-JIS Unit & Monster Names",
        "",
        "Extracted from FM Towns CD `HIO2/H2MF/MAP00..20.LZ5` (`0x1A40..0x1BD0`) and `hio2/WIND0.HIO` (`0x122C..0x13BC`).",
        "",
    ]

    if WIND0_HIO.exists():
        w_bytes = WIND0_HIO.read_bytes()
        w_names = decode_sjis_slots(w_bytes[0x122C:0x13BC], 25)
        lines.append("## `hio2/WIND0.HIO` (25 slots at `0x122C`)")
        lines.append("")
        lines.append("| Slot | Japanese (Shift-JIS) |")
        lines.append("|---:|---|")
        for idx, nm in enumerate(w_names):
            if nm:
                lines.append(f"| {idx} | {nm} |")
        lines.append("")

    lines.append("## FM Towns Stage Maps (`H2MF/MAP00..20.LZ5` at `0x1A40`)")
    lines.append("")
    for mname in sorted(map_names):
        lines.append(f"### `{mname}`")
        lines.append("")
        lines.append("| Slot | Japanese (Shift-JIS) |")
        lines.append("|---:|---|")
        for idx, nm in enumerate(map_names[mname]):
            if nm:
                lines.append(f"| {idx} | {nm} |")
        lines.append("")

    OUT_DOC.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {OUT_DOC}")


if __name__ == "__main__":
    main()
