"""pairs.py: review files for reviewer agents (docs: the skill's reviewer brief).

usage: python3 -X utf8 tools/pairs.py OUT.txt bNNN [bNNN ...]     entries of those batches, +-2 neighbours as context
       python3 -X utf8 tools/pairs.py OUT.txt --blocks TM VIS ...  whole blocks in file order (story pass)
Format: `>> id | kind | source` then `EN: ...`; context lines start with `   ` and are not to be judged.
Twins (same source text) are listed once, under the batch row.
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main(argv):
    out, args = Path(argv[0]), argv[1:]
    blocks = {p.stem: json.loads(p.read_text(encoding="utf-8")) for p in sorted((ROOT / "script").glob("*.json"))}
    order = [e for es in blocks.values() for e in es]
    pos = {e["id"]: k for k, e in enumerate(order)}
    if args[0] == "--blocks":
        want = [e["id"] for b in args[1:] for e in blocks[b]]
        ctx = 0
    else:
        want = [line.split("\t")[0] for b in args for line in
                (ROOT / f"work/tl/{b}.tsv").read_text(encoding="utf-8").splitlines() if line.strip()]
        ctx = 2
    lines = []
    for i in want:
        k = pos[i]
        for j in range(max(0, k - ctx), k):
            lines.append(f"   ({order[j]['id']}) {order[j]['zh']} => {order[j]['en']}")
        e = order[k]
        lines.append(f">> {e['id']} | {e['kind']} | {e['zh']}")
        lines.append(f"EN: {e['en']}")
        for j in range(k + 1, min(len(order), k + 1 + ctx)):
            lines.append(f"   ({order[j]['id']}) {order[j]['zh']} => {order[j]['en']}")
        lines.append("")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines), encoding="utf-8")
    print(f"{out}: {len(want)} entries")


if __name__ == "__main__":
    main(sys.argv[1:])
