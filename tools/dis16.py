"""16-bit x86 disassembly of an MZ file around file offsets (recon helper; needs .venv with capstone).

usage: .venv/bin/python3 tools/dis16.py hio2/HIO.EXE 0x1939 [0x1945 ...] [--before 40] [--after 8]
Prints image offsets (file offset minus the MZ header), which are the CS:IP offsets when the image loads at CS:0.
"""
import argparse
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

ap = argparse.ArgumentParser()
ap.add_argument("exe")
ap.add_argument("offs", nargs="+")
ap.add_argument("--before", type=int, default=40)
ap.add_argument("--after", type=int, default=8)
a = ap.parse_args()
d = open(a.exe, "rb").read()
hdr = int.from_bytes(d[8:10], "little") * 16
img = d[hdr:]
md = Cs(CS_ARCH_X86, CS_MODE_16)
for o in a.offs:
    at = int(o, 16) - hdr
    # decode from a few start points before `at` and keep the one that lands exactly on it
    for start in range(max(0, at - a.before), at + 1):
        ins = list(md.disasm(img[start:at + a.after * 4], start))
        if any(i.address == at for i in ins):
            break
    print(f"== file {o} (image {at:#x})")
    n_after = 0
    for i in ins:
        mark = ">>" if i.address == at else "  "
        print(f"{mark} {i.address:05x}: {i.mnemonic} {i.op_str}")
        if i.address > at:
            n_after += 1
            if n_after >= a.after:
                break
