# Translation reviewer brief - Hiouden II (DOS, Taiwanese release)

You review part of a Traditional Chinese -> English fan translation of Hiouden II (Wolf Team dark fantasy strategy
RPG; the Chinese is itself a translation of the Japanese original). Read-only: no edits, no git, no subagents.
Mechanical validity (ids, control codes, ASCII, widths, byte budgets) is already enforced by
`tools/batches.py check` - judge translation quality. Return the report as your final message.

## Inputs (repo /Users/antoineclerc-renaud/develop/hioden-2)
- Pairs file named in your dispatch: `>> id | kind | source` then `EN: ...`; lines starting with spaces are context
  (`(id) source => English`), not to be judged. Read ALL of it, in passes if large.
- `glossary.md` (rulings at the top; binding names), `docs/style-reference.md`, `docs/synopsis.md`.
- Codes: `\2`..`\8` colour on, `\F` off, `\n` `\u` new line, `\k` wait, `\c` clear, `\w \o \s` pauses, `\e` end;
  `^0` / `^[00]` new line, `^@` next page, `^\` end of scene, `^%` before a speaker name, `^[01]`.. staff positions.
  Fixed fields are short on purpose (hard column limits): don't flag brevity that the limit forces, only meaning loss.

## Judge
- Accuracy: mistranslation, omission, addition, wrong speaker or addressee, reversed meaning. Quote the Chinese and
  say what it means.
- Consistency: glossary names/terms exactly (a one-letter slip is what players notice); the same item/monster/key
  named the same way in names, descriptions and dialogue.
- Jokes and wordplay (the developers' gags, the buck-toothed prisoner's mispronunciations) must still land.
- Voice: Markfiend curt and sarcastic, Mirlei gentle but stubborn, guards coarse, Jadain theatrical, Lezard cold and
  grandiose, MKG chatty. Register and emotional beats must survive, not flatten into neutral prose.
- Naturalness: would an English reader stumble? Calques, stilted word order, odd stock phrases.

## Report
### Issues
#### Critical (meaning reversed, wrong speaker, broken sense)
#### Important (clear mistranslation, glossary inconsistency, voice flattened, joke dead, unnatural enough to stumble)
#### Minor (style - deferred to one polish pass)
Each: `id - source gist - problem - suggested EN` (the suggested EN must keep the same codes and roughly the same
length; fixed fields keep within the same column count).
### Assessment
Approved | Needs fixes - one sentence why.
