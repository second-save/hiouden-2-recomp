#!/usr/bin/env python3
"""
Static analyzer for hio2/ game files:
1. Parses the HIOSYS.EXE 4-bank File ID table (CS:2634) -> docs/re/file_table.md
2. Catalogs all translatable text sources across CVD, HMF, HIO/INI, and EXE files
   -> docs/re/text_inventory.md
"""

from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parent.parent
HIO2_DIR = ROOT / "hio2"
HIOSYS_PATH = HIO2_DIR / "HIOSYS.EXE"
OUT_FILE_TABLE = ROOT / "docs" / "re" / "file_table.md"
OUT_TEXT_INV = ROOT / "docs" / "re" / "text_inventory.md"

MZ_HEADER_DEFAULT = 0x200


def parse_hiosys_file_tables() -> list[tuple[int, int, str, int, int]]:
    """Return list of (bank, full_id, filename, cs_offset, file_offset)."""
    data = HIOSYS_PATH.read_bytes()
    ptrs = struct.unpack("<4H", data[0x2834:0x283C])
    # Boundaries between consecutive subtables in CS space (last ends at NUL at CS:31C6)
    bounds = list(ptrs) + [0x31C6]
    rows: list[tuple[int, int, str, int, int]] = []
    for bank in range(4):
        start_file = ptrs[bank] + MZ_HEADER_DEFAULT
        end_file = bounds[bank + 1] + MZ_HEADER_DEFAULT
        pos = start_file
        while pos < end_file:
            if data[pos] == 0:
                break
            nul = data.find(b"\x00", pos)
            if nul == -1 or nul >= end_file:
                break
            name = data[pos:nul].decode("ascii", errors="replace")
            sub_id = struct.unpack("<H", data[nul + 1 : nul + 3])[0]
            full_id = (bank << 9) | sub_id
            rows.append((bank, full_id, name, pos - MZ_HEADER_DEFAULT, pos))
            pos = nul + 3
    return rows


def extract_big5_runs(data: bytes, min_chars: int = 4) -> list[tuple[int, int, str]]:
    """Extract contiguous Big5 text runs (allowing ASCII control tags like ^0, \\n)."""
    runs: list[tuple[int, int, str]] = []
    i = 0
    n = len(data)
    while i < n - 1:
        b0, b1 = data[i], data[i + 1]
        if 0xA1 <= b0 <= 0xF9 and (0x40 <= b1 <= 0x7E or 0xA1 <= b1 <= 0xFE):
            j = i
            valid = 0
            while j < n:
                c0 = data[j]
                if j + 1 < n and 0xA1 <= c0 <= 0xF9 and (0x40 <= data[j + 1] <= 0x7E or 0xA1 <= data[j + 1] <= 0xFE):
                    valid += 1
                    j += 2
                elif c0 in (0x0A, 0x0D, 0x20, 0x5C, 0x5E) or (
                    0x20 <= c0 <= 0x7E and bytes([c0]) in b"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!?:;.,()[]/^\\ "
                ):
                    j += 1
                else:
                    break
            if valid >= min_chars:
                try:
                    decoded = data[i:j].decode("big5", errors="strict")
                    runs.append((i, j - i, decoded))
                    i = j
                    continue
                except UnicodeDecodeError:
                    pass
                decoded = data[i:j].decode("big5", errors="replace")
                if "\ufffd" not in decoded:
                    runs.append((i, j - i, decoded))
                    i = j
                    continue
        i += 1
    return runs


def main() -> None:
    rows = parse_hiosys_file_tables()
    OUT_FILE_TABLE.parent.mkdir(parents=True, exist_ok=True)

    ft_lines = [
        "# HIOSYS.EXE File-ID Table",
        "",
        "Parsed from `hio2/HIOSYS.EXE` (`CS:2634` pointer array -> `file 0x2834..0x33C6`).",
        "Lookup in `HIOSYS.EXE` (`CS:2471`): `bank = (DX >> 9) & 3`, `sub_id = DX & 0x01FF`, so `full_id = (bank << 9) | sub_id`.",
        "",
        "| Bank | Full ID (Hex) | Sub ID | Filename | CS Offset | File Offset | Exists in `hio2/` |",
        "|---:|---:|---:|---|---:|---:|---|",
    ]
    for bank, full_id, name, cs_off, file_off in rows:
        exists = "Yes" if (HIO2_DIR / name).exists() else "No"
        ft_lines.append(
            f"| {bank} | `0x{full_id:04X}` | `0x{full_id & 0x1FF:04X}` | `{name}` | `0x{cs_off:04X}` | `0x{file_off:04X}` | {exists} |"
        )
    OUT_FILE_TABLE.write_text("\n".join(ft_lines) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_FILE_TABLE} ({len(rows)} entries)")

    # Text inventory
    inv_lines = [
        "# Hiouden II Translatable Text Inventory",
        "",
        "Summary of all text-bearing files and executable segments in `hio2/`.",
        "",
        "## 1. Script & Description `.CVD` Files",
        "",
        "| File | Size (bytes) | Encoding | Preview (first 80 chars) |",
        "|---|---:|---|---|",
    ]

    for prefix, count in [("TM", 32), ("IM", 16), ("MM", 3)]:
        for idx in range(1, count + 1):
            fname = f"{prefix}{idx:02d}.CVD"
            p = HIO2_DIR / fname
            if not p.exists():
                continue
            raw = p.read_bytes()
            text = raw.split(b"\x00", 1)[0].decode("big5", errors="replace")
            preview = text.replace("\r", "").replace("\n", "\\n").replace("|", "\\|")[:80]
            inv_lines.append(f"| `{fname}` | {len(raw)} | Big5 | {preview} |")

    inv_lines.extend([
        "",
        "## 2. Stage Maps (`MP01..21.HMF`) & Window Name Tables (`WIND0.HIO`, `WIND1..A.INI`)",
        "",
        "- `MP01.HMF`..`MP21.HMF`: 7168 bytes each; 25 fixed 16-byte unit name slots at `0x1A40..0x1BD0` (Big5 in `hio2/`, Shift-JIS in FM Towns `H2MF/MAP00..20.LZ5`).",
        "- `WIND0.HIO`: 5248 bytes; 25 fixed 16-byte unit name slots at `0x122C..0x13BC` in **Shift-JIS**.",
        "- `WIND1.HIO`..`WINDA.HIO` and `WIND1.INI`..`WINDA.INI`: 5248 bytes each; same slot table in **Big5**.",
        "",
        "| File | Offset | Encoding | Non-empty Slots | First 3 Non-empty Entries |",
        "|---|---|---|---|---|",
    ])

    for idx in range(1, 22):
        fname = f"MP{idx:02d}.HMF"
        p = HIO2_DIR / fname
        if p.exists():
            raw = p.read_bytes()
            slots = [
                raw[0x1A40 + i * 16 : 0x1A40 + (i + 1) * 16]
                .split(b"\x00", 1)[0]
                .decode("big5", errors="replace")
                .rstrip("\u3000 ")
                for i in range(25)
            ]
            non_empty = [s for s in slots if s]
            sample = ", ".join(non_empty[:3]).replace("|", "\\|")
            inv_lines.append(f"| `{fname}` | `0x1A40` | Big5 | {len(non_empty)} | {sample} |")

    for fname, enc in [("WIND0.HIO", "cp932"), ("WIND1.INI", "big5")]:
        p = HIO2_DIR / fname
        if p.exists():
            raw = p.read_bytes()
            slots = [
                raw[0x122C + i * 16 : 0x122C + (i + 1) * 16]
                .split(b"\x00", 1)[0]
                .decode(enc, errors="replace")
                .rstrip("\u3000 ")
                for i in range(25)
            ]
            non_empty = [s for s in slots if s]
            sample = ", ".join(non_empty[:3]).replace("|", "\\|")
            inv_lines.append(f"| `{fname}` | `0x122C` | {enc} | {len(non_empty)} | {sample} |")

    inv_lines.extend([
        "",
        "## 3. Embedded Big5 Text in Executables",
        "",
        "| Executable | Size (bytes) | Big5 Runs (>=4 chars) | Total Big5 Bytes | Sample Offset Ranges |",
        "|---|---:|---:|---:|---|",
    ])

    for exe in ["HIO.EXE", "OP_E.EXE", "ED_E.EXE", "VIS.EXE", "VIS4.EXE"]:
        p = HIO2_DIR / exe
        raw = p.read_bytes()
        runs = extract_big5_runs(raw, min_chars=4)
        total_b = sum(ln for _, ln, _ in runs)
        samples = ", ".join(f"`0x{off:05X}` ({ln}B)" for off, ln, _ in runs[:5])
        inv_lines.append(f"| `{exe}` | {len(raw)} | {len(runs)} | {total_b} | {samples} |")

    inv_lines.append("")
    for exe in ["OP_E.EXE", "ED_E.EXE", "VIS.EXE", "HIO.EXE"]:
        p = HIO2_DIR / exe
        raw = p.read_bytes()
        runs = extract_big5_runs(raw, min_chars=4)
        inv_lines.append(f"### Sample Runs in `{exe}`")
        inv_lines.append("")
        shown = 0
        for off, ln, s in runs:
            # Filter to runs with clear Big5 punctuation/spaces or >= 16 bytes for HIO.EXE
            if exe == "HIO.EXE" and off < 0x19900:
                continue
            preview = s.replace("\r", "").replace("\n", "\\n").replace("|", "\\|")[:100]
            inv_lines.append(f"- `0x{off:05X}` ({ln}B): `{preview}`")
            shown += 1
            if shown >= 20:
                break
        inv_lines.append("")

    OUT_TEXT_INV.write_text("\n".join(inv_lines), encoding="utf-8")
    print(f"Wrote {OUT_TEXT_INV}")


if __name__ == "__main__":
    main()
