"""Translation batches (docs/translator-brief.md): split untranslated text, check translator output, merge it back.

    python3 -X utf8 tools/batches.py split [--size 3500] [--kinds k1,k2]   work/tl/bNNN.tsv + build/terms_bNNN.txt
    python3 -X utf8 tools/batches.py check [bNNN ...]      a batch's .out.tsv; with no batch, all English in script/
    python3 -X utf8 tools/batches.py merge bNNN ... [--overwrite]   check, then write the English into script/

Adapted from the skill's assets/batches.py for this game: source field `zh` (Traditional Chinese), codes from
tools/bigram.py, widths in bigram cells, and one batch row per distinct source string: merge copies the English to
every entry with the same `zh` (its twins), and the check applies every twin's limit.
Batch TSV: id, speaker, form, limit, source (` / ` between the rows of an item description). Output:
work/tl/bNNN.out.tsv, id<TAB>English, every id once, same order.
"""
import json
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import bigram  # noqa: E402

TL = ROOT / "work/tl"
SCRIPT = ROOT / "script"
GLOSSARY = ROOT / "glossary.md"
MAX_ENTRIES = 150

# ---- ADAPTER: this game's text format ----
TOKEN = bigram.TOKEN
FREE = {"\\n", "\\w", "\\o", "\\s", "\\u", "\\k", "\\c", "^0", "^$", "^[00]"}  # layout/pacing: may move or be added
HONORIFICS = {}  # dropped (D6)
SKIP_KINDS = {"attr"}  # field boundaries unknown until the marker build maps them
TM_BREAKS = {"\\n", "\\c", "\\u"}
SCRIPT_BREAKS = {"^0", "^[00]", "^@", "^\\", "^$", "\\n", "\\c", "\\u", "^[01]", "^[02]", "^[03]"}  # ^[00]: OP_E lines
# VIS: widest original line. OP_E/ED_E: the screen edge clips narration after 25 cells (visual QA 2026-11-25)
SCRIPT_COLS = {"VIS": 56, "OP_E": 50, "ED_E": 50}
TM_BYTES = 1024  # slot size after the D10 patch
NAME_KINDS = {"unit", "unit_name", "item", "item_name", "item_tag", "spell", "spell_name", "spell_tag", "floor"}
# OP_E/ED_E load inside HIOSYS's image; HIOSYS reads its own table at linear 0xE95B, so each image may grow only to
# 0x334B (loaded at 0xB610; --watch survey 2026-11-25). Text budget = the old block + the spare up to there - margin.
BLOCK_BUDGET = {"OP_E": (0x2DCB - 0x2661) + (0x334B - 0x2DCB) - 32, "ED_E": (0x306C - 0x25A4) + (0x334B - 0x30EA) - 32}


def docs():
    for path in sorted(SCRIPT.glob("*.json")):
        doc = json.loads(path.read_text(encoding="utf-8"))
        yield path, doc, doc


def block(entry):
    return entry["id"].split(":")[0]


def src(entry):
    return entry["zh"]


def twins_index():
    t = {}
    for _, _, entries in docs():
        for e in entries:
            t.setdefault(e["zh"], []).append(e)
    return t


TWINS = None


def twins(entry):
    global TWINS
    if TWINS is None:
        TWINS = twins_index()
    return TWINS.get(entry["zh"], [entry])


def needs_english(entry):
    if entry["kind"] in SKIP_KINDS or not entry["zh"].strip() or entry.get("en"):
        return False
    return twins(entry)[0]["id"] == entry["id"]  # the first twin carries the batch row


def group_of(entry):
    return entry["id"]


def speaker(entry):
    if entry["kind"] not in ("message", "script"):
        return ""
    m = re.search(r"【(.+?)】", entry["zh"]) or re.search(r"\^%　?(.+?)：", entry["zh"])
    return m[1] if m else ""


def rule(e):
    """(what, check(en) -> problem or None) for one entry."""
    b, k = block(e), e["kind"]
    field = e["field"][1] - e["field"][0]
    if b.startswith("TM"):
        def tm(en):
            w = max(bigram.cols(l) for l in bigram.lines(en, TM_BREAKS))
            if w > 46:
                return f"a line is {w} columns, the box shows 46"
            if not en.endswith("\\e"):
                return "must end with \\e"
            if est_bytes(en) > TM_BYTES:
                return f"{est_bytes(en)} bytes, the slot holds {TM_BYTES}"
            scrolled = bigram.tm_layout(bigram.slide_pauses(en))[1]
            if scrolled > 0:
                return (f"{scrolled} text row(s) scroll away before a pause (the box shows 4 rows; only a "
                        "speaker header may scroll off): add a \\k\\c page break at a sentence end")
        return "46/line", tm
    if k.endswith("_desc"):
        return "3 rows x 50 (auto-wrapped)", lambda en: (
            f"wraps to {len(bigram.wrap(en, 50))} rows, the popup shows 3" if len(bigram.wrap(en, 50)) > 3 else None)
    if b in SCRIPT_COLS:
        lim = SCRIPT_COLS[b]
        return f"{lim}/line", lambda en: next((f"a line is {w} columns, max {lim}" for w in
                                               (bigram.cols(l) for l in bigram.lines(en, SCRIPT_BREAKS)) if w > lim), None)
    lim = min(field, 46) if b[:2] in ("IM", "MM") else field  # 46 = the popup's line 1; HIO fields hold pieces
    if k in ("item_name", "spell_name") and field < 48:
        # the popup wraps every 23 cells: name, a space and the (English) stat tag share line 1's 46 columns
        tag = by_start().get((b, e["field"][1]))
        lim = 46 - 1 - (bigram.cols([tag["en"]]) if tag and tag.get("en") else 48 - field)
    return f"{lim}", lambda en: (f"{bigram.cols([en])} columns, the field holds {lim}"
                                 if bigram.cols([en]) > lim or "\n" in en else None)


BY_START = None


def by_start():
    global BY_START
    if BY_START is None:
        BY_START = {(block(e), e["field"][0]): e for _, _, es in docs() for e in es}
    return BY_START


def est_bytes(en):
    return sum(len(s.encode()) if c else 2 * bigram.cells(s) for c, s in bigram.parts(en))


def limit_label(entry):
    return " ; ".join(sorted({rule(t)[0] for t in twins(entry)}))


def width_problems(entry, en):
    out = []
    for t in twins(entry):
        p = rule(t)[1](en)
        if p and p not in out:
            out.append(p + (f" (twin {t['id']})" if t["id"] != entry["id"] else ""))
    return out


def encodable(char):
    return " " <= char <= "~" and char not in "[]\\^"
# ---- end of ADAPTER ----


def glossary():
    """[(source forms, EN forms, avoid forms)] from glossary.md's | JP | EN | max | notes | Avoid | tables."""
    rows = []
    if not GLOSSARY.exists():
        return rows
    for line in GLOSSARY.read_text(encoding="utf-8").splitlines():
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if line.startswith("|") and len(cells) >= 2 and cells[0] not in ("JP", "") and not set(cells[0]) <= set("-: "):
            avoid = [x.strip() for x in cells[4].split("/") if x.strip()] if len(cells) > 4 else []
            rows.append(([x.strip() for x in cells[0].split("/")], [x.strip() for x in cells[1].split("/")], avoid))
    return rows


def problems(entry, en, terms):
    if not en.strip():
        return ["empty English"]
    out = []
    plain = TOKEN.sub("", en)
    if bad := sorted({c for c in plain if not encodable(c) and c != "\n"}):
        out.append(f"unencodable characters {bad} (ASCII only; no [ ] \\ ^ outside codes)")
    if re.search(r"\\u[0-9a-fA-F]{4}", en) or "\t" in en:
        out.append("escape sequence or tab in the English")
    free = FREE
    if block(entry) == "ED_E" and "^0" not in src(entry) and "^[01]" in src(entry):
        # ED_E narration: ^[nn] codes drive a scrolling crawl; with any other line structure than the original's
        # the last line stays on screen under the staff roll (verified in game 2026-11-25). Keep every ^[nn]
        # exactly (same count, so the same number of lines, each <= 50 columns); ^0 there garbles the line.
        free = FREE - {"^0"}
        if "^0" in en:
            out.append("this narration page breaks lines with ^[01], not ^0")
    want = Counter(t for t in TOKEN.findall(src(entry)) if t not in free)
    have = Counter(t for t in TOKEN.findall(en) if t not in free)
    if want != have:
        out.append(f"control codes differ: source {dict(want)} EN {dict(have)}")
    for zhs, ens, avoid in terms:
        if entry["kind"] in NAME_KINDS:
            break  # a name that contains a term is a different word (魔界巨蠍 is a scorpion, not "the Demon Realm")
        if entry.get("align") and entry["field"][1] - entry["field"][0] < 24:
            break  # tiny fixed field (a 16-column popup): the hard limit wins over the full glossary form
        if any(z in src(entry) for z in zhs) and not any(e.lower() in en.lower() for e in ens):
            out.append(f"glossary: {zhs[0]} -> {ens[0]}")
        out += [f"glossary Avoid: {a}" for a in avoid if re.search(rf"\b{re.escape(a)}\b", en)]
    return out + width_problems(entry, en)


def index():
    return {e["id"]: e for _, _, entries in docs() for e in entries}


def read_tsv(path):
    return [line.split("\t") for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def existing():
    return sorted(TL.glob("b[0-9][0-9][0-9].tsv"))


def split(size, kinds=None):
    """Numbers on from the last batch and leaves out ids already batched, so a repeat run can't duplicate work."""
    TL.mkdir(parents=True, exist_ok=True)
    (ROOT / "build").mkdir(exist_ok=True)
    taken = {r[0] for p in existing() for r in read_tsv(p)}
    n = max((int(p.stem[1:]) for p in existing()), default=0)
    todo = [e for _, _, entries in docs() for e in entries
            if needs_english(e) and e["id"] not in taken and (not kinds or e["kind"] in kinds)]
    batches, cur, chars = [], [], 0
    for e in todo:
        c = len(TOKEN.sub("", src(e)))
        if cur and (chars + c > size or len(cur) >= MAX_ENTRIES):
            batches.append(cur)
            cur, chars = [], 0
        cur.append(e)
        chars += c
    if cur:
        batches.append(cur)
    terms = glossary()
    for batch in batches:
        n += 1
        name = f"b{n:03d}"
        path = TL / f"{name}.tsv"
        if path.exists():
            sys.exit(f"{path} already exists")
        rows = [f"{e['id']}\t{speaker(e)}\t{e['kind']}\t{limit_label(e)}\t{src(e).replace(chr(10), ' / ')}" for e in batch]
        path.write_text("\n".join(rows) + "\n", encoding="utf-8", newline="\n")
        text = " ".join(src(e) for e in batch)
        used = [(z, ens[min(k, len(ens) - 1)]) for zl, ens, _ in terms for k, z in enumerate(zl) if z in text]
        (ROOT / f"build/terms_{name}.txt").write_text("".join(f"{z}\t{e}\n" for z, e in used)
                                                      or "(no glossary terms in this batch)\n", encoding="utf-8", newline="\n")
    print(f"{len(batches)} new batches in {TL}, {len(taken)} ids already batched")


def check_batch(batch, entries, terms):
    want = [r[0] for r in read_tsv(TL / f"{batch}.tsv")]
    rows = [(r[0], "\t".join(r[1:])) for r in read_tsv(TL / f"{batch}.out.tsv")]
    bad = []
    if [r[0] for r in rows] != want:
        got = [r[0] for r in rows]
        dup = sorted(i for i, c in Counter(got).items() if c > 1)
        bad.append(f"ids differ: missing {sorted(set(want) - set(got))[:8]} extra {sorted(set(got) - set(want))[:8]} "
                   f"duplicate {dup[:8]} (order must match too)")
    for eid, en in rows:
        if eid in entries:
            bad += [f"{eid}: {p}: {en!r}" for p in problems(entries[eid], en, terms)]
    bad += budget_problems(dict(rows), entries)
    return bad, dict(rows)


def budget_problems(new, entries):
    """OP_E/ED_E: all pages together must fit the block budget (untranslated pages count at their Chinese bytes)."""
    out = []
    for b, budget in BLOCK_BUDGET.items():
        pages = [e for e in entries.values() if block(e) == b]
        if not any(e["id"] in new for e in pages):
            continue
        total = sum(est_bytes(new.get(e["id"]) or e.get("en") or "") if (new.get(e["id"]) or e.get("en"))
                    else e["field"][1] - e["field"][0] for e in pages)
        if total > budget:
            out.append(f"{b}: all pages together take {total} bytes, the budget is {budget}: shorten by {total - budget}")
    return out


def merge(new, overwrite):
    """Writes each English to the entry and its twins; refuses to replace different English unless told to."""
    by_id = index()
    fan = {}
    for eid, en in new.items():
        for t in twins(by_id[eid]):
            fan[t["id"]] = en
    if not overwrite:
        for _, _, entries in docs():
            for e in entries:
                if e["id"] in fan and e.get("en") and e["en"] != fan[e["id"]]:
                    sys.exit(f"{e['id']} would lose {e['en']!r}: pass --overwrite if that is meant")
    changed = 0
    for path, doc, entries in docs():
        touched = False
        for e in entries:
            if e["id"] in fan and e.get("en") != fan[e["id"]]:
                e["en"] = fan[e["id"]]
                touched = True
                changed += 1
        if touched:
            path.write_text(json.dumps(doc, ensure_ascii=False, indent=1) + "\n", encoding="utf-8", newline="\n")
    print(f"merged: {changed} entries changed")


def main(argv):
    if not argv:
        sys.exit(__doc__)
    cmd, args = argv[0], argv[1:]
    if cmd == "split":
        kinds = set(args[args.index("--kinds") + 1].split(",")) if "--kinds" in args else None
        return split(int(args[args.index("--size") + 1]) if "--size" in args else 3500, kinds)
    overwrite = "--overwrite" in args
    names = [a for a in args if a != "--overwrite"]
    entries, terms = index(), glossary()
    if cmd == "check" and not names:
        bad = [f"{i}: {p}: {e['en']!r}" for i, e in entries.items() if e.get("en") for p in problems(e, e["en"], terms)]
        print(f"{len(entries)} entries, {len(bad)} problems")
        print("\n".join(bad[:200]))
        return 1 if bad else 0
    if cmd not in ("check", "merge") or not names:
        sys.exit(__doc__)
    new, failed = {}, 0
    for batch in names:
        bad, rows = check_batch(batch, entries, terms)
        new.update(rows)
        failed += bool(bad)
        print(f"{batch}: " + ("OK" if not bad else f"{len(bad)} problems"))
        for line in bad:
            print("  " + line)
    if failed:
        return 1
    if cmd == "merge":
        merge(new, overwrite)
    return 0


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.exit(main(sys.argv[1:]))
