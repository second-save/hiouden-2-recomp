# Hiouden II - style reference

Voice guide for translators. The Chinese comes from `script/*.json` (`zh`, markup removed). The English lines are
models of tone, not final text: fit them to the field and keep the control codes. Story context: `docs/synopsis.md`.

## General register
- Dark fantasy with gallows humour. People are waiting to be executed and joke about it. Keep death blunt
  ("die", "kill", "executed"), not "pass away".
- No modern slang (no "okay", "guys", "cool", "yeah right", "dude"). Mild period-neutral insults only: idiot, fool,
  bastard, swine, coward. 渾蛋/畜生 = "bastard"/"swine"; 白癡 = "idiots"; 卑鄙小人 = "coward"/"snake".
- Contractions are fine in speech. Narration and inscriptions use none.
- Punctuation (glossary): 「」 -> "...", 『』 -> '...', …… -> ..., ！？ -> !?, ～ dropped (keep "~" only in comic
  lines), names with ‧/． -> a space. ASCII only; no [ ] \ ^ in English. Speaker tags in TM: `Name:`, not brackets.
- Stammers and trailing dots carry tone: keep them, but at most one "..." per pause.
- Chuckles: 咈咈 -> "Heh heh." (Jadain, statue); 嘿嘿嘿 -> "Heh, heh, heh..." (guard).

## Markfiend
Curt, sarcastic, world-weary; insults as a cover for caring. Short sentences. Gets heated with villains.
- 快呀～快把我給殺了吧！ -> "Come on. Just kill me already."
- 妳想在那待到什麼時候？……妳想出去不是嗎？ -> "How long are you going to sit there? You wanted out, didn't you?"
- 好耶、這麼一來…我就不用大費周章召喚那些傢伙啦。真是萬歲萬歲萬萬歲。 -> "Good. Now I don't have to go to the
  trouble of summoning them myself. Hooray, hooray, three cheers."
- 你這畜生！ -> "You swine!" / 死？…說什麼傻話。 -> "Die? Don't be stupid."

## Mirlei
Polite, gentle, frightened at first; pleads; later quiet resolve. Stammers under stress. Never sarcastic.
- 求求你們、放我出去吧！好心的大人！ -> "Please, let me out! Please, kind sir!"
- 我們…還有路可走是嗎…？如果是這樣的話，我還是會跟著你一起走……。 -> "So there's... still a way out? Then I'll go
  with you..."
- 不要！不要走～～。 -> "No! Don't go!"

## Jadain
Smug warden, mocking and theatrical; contempt for everyone, including his own men. Fond of "my" phantoms.
- 你們這些白癡最好也趕快離開這裡吧！ -> "And you idiots had better clear out too!"
- 咈咈…好好想想這地牢的用意吧。原本就不需要有出口什麼的不是嗎？ -> "Heh heh... Think about what this dungeon is for.
  It never needed an exit, did it?"
- 那麼、你們兩個就陪著我一手造出來的幻影一起跳舞，直到死吧……。 -> "Now, the two of you can dance with my phantoms...
  until you drop dead."

## Lezard Valeth
Silky, amused, unhurried; drawn-out vowels (～) become a lingering "Oh..." or "Well, well". Grandiose when beaten.
- 你的直覺相當敏銳呀～。 -> "My, what sharp instincts you have."
- 原來如此～。但是……如果你想殺我…真的能夠辦得到嗎？ -> "I see. But... can you really kill me?"
- 可恨！沒想到我居然會失敗在最後這一步…… -> "Damn you! To fail at the very last step..."

## Guards, jailers, soldiers
Crude, bullying, low register; soldiers panicky and servile with Jadain.
- 這娘們每天可真吵！還不給我乖乖的老實一點！ -> "This wench never shuts up! Pipe down and behave!"
- 唷、妳這麼想離開這裡呀？好～我就把妳給弄出來！ -> "Oh, you want out that badly? Fine, I'll get you out!"
- 大事不好了！ -> "We've got trouble!" / 是、知…知道了。 -> "Y-yes, sir. Understood."

## The statue, demons, inscriptions
Statue: formal and ancient, slightly condescending, no contractions. Inscriptions (『』 with 爾/爾等) use archaic
"ye/thou" sparingly; keep them riddling.
- 我有話對你說。 -> "I would speak with you."
- 咈咈…看來能夠駕御統領我們魔族的那個人，恐怕不會是你這個不解世故的傢伙了… -> "Heh heh... The one who commands
  our kind will not be a green youth like you, it seems..."
- 爾等應穿越魔界之門 -> 'Ye must pass through the gate of the Demon Realm.'

## Prisoners and floor NPCs
Each has a quirk; keep it. Tags are descriptions (沾滿污泥的男子 -> "Mud-Caked Man"). The buck-toothed man (暴牙男)
mispronounces on purpose (穴穴 for 謝謝, 縮下 for 收下): render with a lisp-like slur, still readable.
- 唉呀呀、沾著泥巴好癢好癢。 -> "Ugh, this mud itches like mad."
- 穴穴你們救我區來。 -> "Thankth for gettin' me out."
- 對對對、你～也差不多快死了。 (madman) -> "Yes, yes, yes, and you'll be dead soon too~"
- Museum plaques (TM19-TM20): deadpan encyclopedia voice with a punchline. 這麼個幸運兒？？該不會就是你吧。 ->
  "And this lucky soul?? Surely not you."

## MKG, the developer (TM30-TM32)
Chatty, self-deprecating late-90s dev talking to "players"; jokes, mock sobbing, asides in brackets of the source
become commas or dashes (no [ ]). Staff names in Western order (glossary).
- 各位玩家們好。我是緋王傳Ⅱ’代中擔任企劃的ＭＫＧ先生。 -> "Hello, players. I'm MKG, the planner on Hiouden II'."
- 沒辦法每個人都介紹到…嗚咽！ -> "I can't introduce everyone... *sob*!"
- 能力強到可以用程式將現世跟魔界串聯在一起。呃、隨便說說可別當真。 -> "So good he could code a link between our world
  and the Demon Realm. Er, just kidding. Don't take that seriously."
