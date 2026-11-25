# Script

`en/<FILE>.txt`: English for `hio2/<FILE>.CVD`. TM files hold 4 messages in 512-byte slots (HIO.EXE loads TM file
`0x100 + msg>>2`); the .txt has one line per slot, slot 0 first, with the CVD control codes as written in
`docs/status.md` (`\n \k \c \e \2..\8 \F \w \o \s \u`). Drafts until the extract format and width gate (phases 4-5)
exist; then they move into that format. Line budget assumed for now: ~46 Latin columns (23 cells x 2), 3 lines a page.

- TM31: Wolf Team staff notes by the planner MKG (easter egg). `淺沼` Asanuma, `稻毛` Inage, `小澤` Ozawa, `三宅`
  Miyake, `磯谷` Isogai, `櫻庭` Sakuraba (composer Motoi Sakuraba), `阿廣` Hiro; `狼組` = Wolf Team; honorifics
  `先生`/`叔叔` dropped (D6). Miyake's `聽寫繞跑的ＡＩ` is read as `聽說寫繞路的ＡＩ` ("writing the pathfinding AI").
- TM32: slot 0 sign `努力加奮鬥`; slots 1-2 "the floor to the north/west changed"; slot 3 = TM31 slot 3.
- TM30 slots 2-3 (not translated yet) are MKG's intro to TM31, and they give a cheat: on the first stage, hold CTRL and
  the right mouse button from choosing New Game until the game starts, and every monster joins your side.
