# Polish pass (Minor review findings; release refuses while any line is unticked)

Tick each `[x]` with "fixed" or "waived: reason".

## Items and names (review 2026-11-25)
- [x] HIO:1E8FF - "Resist" lacks the colon - waived: the field is exactly 6 columns
- [x] IM07:001B8 - Invisibility named - fixed (b014)
- [x] IM09:00404 - spell names - fixed (b014)
- [x] IM08:0027C - "fully recover" - fixed (b014)
- [x] IM14:004C8 - fixed (b014)
- [x] IM04:00404 - comma splice - fixed (b014)
- [x] IM06:000F4 - fixed (b014)
- [x] IM16:0027C - fixed (b014)
- [x] IM01:0058C - fixed (b014)
- [x] IM16:00268 - SD tag - fixed (b014)
- [x] MP03:01B90 - 獨眼男子 vs JP 狐目の男 - waived: the English follows the Chinese release it patches
- [x] MP15:01A50 - 地精騎士 vs JP ゴールドナイト - waived: follows the Chinese release
- [x] MP18 names (King Markley?, Kirkwood?, Maximilian?) - waived: phantom impostors named only in the Chinese map list; no Hiouden 1 English source reachable (AGTP page offline); recorded in glossary as provisional
## Dungeon messages
- [x] TM18:00600 - fixed (b014)
- [x] TM13:00200 - 地下一層 - waived: "the floor below" is the natural reading in context; not reachable to verify the floor
- [x] TM02:00000 - fixed (b014)
- [x] MM02:00030 - fixed (b014)
- [x] MM02:004C8 - fixed (b014)
- [x] TM12:00200 - "Skull Key" - waived: dialogue short form of "Eerie Skull Key", unambiguous
- [x] TM11:00600 - fixed (b014)
- [x] TM01:00600 - fixed (b014)
- [x] TM29:00600 - fixed (b014)
- [x] archaic pronouns (TM24:00200, TM11:00200, TM28:00200) - fixed (b014)
- [x] TM31:00200 - octopus joke - waived: "Quite an octopus" kept (the joke is the developer's; clear enough with "all sorts of little jobs")
- [x] TM30:00400 / TM31 - "Hiouden II'" prime - fixed: dropped in TM30 (b013); the prime meant the hard-disk edition and reads as a typo in English
## Cutscenes
- [x] VIS:018B0 - fixed (b014)
- [x] VIS:027F0 - fixed (b014)
- [x] VIS:02DAC - fixed (b014)
- [x] six-dot ellipses - fixed (b014)
- [x] VIS:01F2C - fixed (b014)
- [x] VIS:020DE - fixed (b014)
- [x] VIS continuation-line indent 2 vs 3 spaces - waived: both read fine in the cutscene sheets (visual QA ok)
## Ending
- [x] ED_E:02A64 - no speaker tag (none in source either) - waived: same as the original
## Engine limits (known issues, in README)
- [x] HIO:1E91A - weakness elements in 2-byte slots ("Fi Ic Th St") - waived: needs a code patch to a wider table; listed in README known issues
- [x] HIO:1E8BC - corpse/spirit prompt pieces - waived: each English piece fills its original offset exactly; the screen is not reachable with the debug tool
- [x] HIO:15597, 155B5, 155D3 (window-setup dialog), 18202 (save prompt) - waived: same offsets as the original pieces; the load prompt (18222) and item settings (15982) were checked in game and are correct
