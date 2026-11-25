# Polish pass (Minor review findings; release refuses while any line is unticked)

Tick each `[x]` with "fixed" or "waived: reason".

## Items and names (review 2026-11-25)
- [ ] HIO:1E8FF - "Resist" lacks the colon the other popup labels have (6-col field)
- [ ] IM07:001B8 - 透明之咒 = the Invisibility spell: name it
- [ ] IM09:00404 - "Poison and Confuse spells" (use the spell names)
- [ ] IM08:0027C - 全恢復: "fully recover"
- [ ] IM14:004C8 - "Raises its owner's magic resist and INT by 10, and teaches Fire."
- [ ] IM04:00404 - comma splice
- [ ] IM06:000F4 - ": peach-pink" stilted
- [ ] IM16:0027C - 堅固但做工細膩 describes the work, not Gimli
- [ ] IM01:0058C - "the spirit world's people" awkward
- [ ] IM16:00268 - tag 【ＳＤ角色巨大化】: "SD sprite turns huge"
- [ ] MP03:01B90 - 獨眼男子 vs JP 狐目の男 (fox-eyed): check the portrait
- [ ] MP15:01A50 - 地精騎士 vs JP ゴールドナイト (Gold Knight): check the sprite
- [ ] MP18 names (King Markley?, Kirkwood?, Maximilian?) - check against Hiouden 1 spellings; Maximilian vs Maclimilian
## Dungeon messages
- [ ] TM18:00600 - "guarding the blue ram statue"
- [ ] TM13:00200 - 地下一層: B1F or "the floor below"? check in game
- [ ] TM02:00000 - "I mean, King Richard's sword"
- [ ] MM02:00030 - "can only sit there nicely"
- [ ] MM02:004C8 - "cross rubble and cliffs"
- [ ] TM12:00200 - "Skull Key" vs item "Eerie Skull Key"
- [ ] TM11:00600 - "They must... they must all have been executed..."
- [ ] TM01:00600 - "and you'll be dead soon too~" (no full stop after ~)
- [ ] TM29:00600 - same key-message template as TM07-TM09
- [ ] archaic pronouns: ye for 爾等 (TM24:00200, TM11:00200, TM28:00200)
- [ ] TM31:00200 - octopus joke: "Like an octopus, a hand in everything."
- [ ] TM30:00400 / TM31 - "Hiouden II'" prime: keep or drop (user ruling)
## Cutscenes
- [ ] VIS:018B0 - Markfiend's thought addresses her ("you")
- [ ] VIS:027F0 - "...Honestly, you! This is how you use the thing."
- [ ] VIS:02DAC - "Did it start when you were little?"
- [ ] six-dot ellipses (VIS:02294, 02AB4, 02B36, 027CC, 02DDE, 02EEA): one "..." per pause
- [ ] VIS:01F2C - "the demon I let loose on B2" if it fits
- [ ] VIS:020DE - "this Dungeon" capitalised as in the glossary
- [ ] VIS continuation-line indent: 2 spaces (scenes 0-1) vs 3 (scene 2 on): pick one
## Ending
- [ ] ED_E:02A64 - "Die? Don't be stupid." has no speaker tag (none in source either): check in game which name shows
## Engine limits (known issues)
- [ ] HIO:1E91A - weakness elements drawn from 2-byte slots: "Fi Ic Th St" (fire, ice, thunder, star); a wider table needs a code patch
- [ ] HIO:1E8BC - glued pieces at fixed offsets (corpse/spirit prompts): check in game
- [ ] HIO:15597, 155B5, 155D3, 15982, 18202/18222 - partial-field draws in the window-setup and save/load dialogs: check in game
