# Map events and message loading (HIO.EXE)

HIO.EXE image = file minus its 0x1800-byte MZ header; at runtime it sits at 0CC1:0000, so image offset X runs at
linear 0xCC10 + X (CS 1CC1 covers image 0x10000+). `tools/dis16.py hio2/HIO.EXE <file offset>` disassembles.

## Message files (verified 2026-11-25: `warp.py --msg 0` shows TM01 slot 0 in-game, work/qa/m0)
- Message number N (one byte) -> file ID `0x100 + N>>2` (TM01..TM32), slot `N & 3`; each TM file is 4 slots of
  512 bytes, each ending at `\e`. 128 messages. Code: image 0x17AB8 (`mov al,[di+3]; shr ax,2; add ax,0x100`).
- Items: number n (1-based) -> file `0xB0 + (n-1)>>3` (IM01..IM16), record `(n-1) & 7`; spells add 0x10 (MM01..03).
  Code: image 0x0EF64.
- Box: lines break only at `\n`; original lines are at most 23 cells (368 px) wide; longer messages scroll.

## Event records (verified by trace, 2026-11-25)
- In MPnn.HMF at 0x1200 (MP01), 22-byte records until a type-0 record. Copied into MAP0.TMP state (segment 4000).
- `[0]` type: 4 = one tile, entered/walked onto (scan at image 0x179F7); 5 = 2x2 area, examined with a right-click
  (scan at image 0x17A5D). `[1]` 0xFF. `[2],[3]` tile (a, b). `[4..5]` filled at load: `(a+16)*96 + (b+16)`.
- `[6..21]`: four actions of 4 bytes, `[0]` opcode (0xFF = none; jump table CS:7A99), opcode 0 = message:
  `00 cond 0xFF N`. Opcodes 1/2 look like tile changes (`01 xx yy 00`), not mapped yet.
- A right-click at screen (600,140) on a cold-boot stage 1 examines tile (0x32,0x0C) (BX=0x18DC); `warp.py --msg`
  moves MP01's area event 0x12B0 there.
