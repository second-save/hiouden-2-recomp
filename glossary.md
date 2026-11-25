# Glossary - Hiouden II (Taiwanese DOS release)

Binding for every translator and reviewer. Agents propose additions in reports; only the controller edits this file.
Source text is Traditional Chinese (Big5); the JP column holds the Chinese form used in the script. Japanese forms
(FM Towns katakana) are in `docs/re/japanese_names.md`.

## Rulings (asked of the user once; options and reasons in docs/decisions.md)
- Honorifics: dropped. Name order: Western (given name first), also for the staff names in TM31/TM32 (D6).
- Romanisation: returning characters as in Hiouden 1 (Aeon Genesis patch; spellings checked against Wikipedia's plot
  summary, the AGTP page is offline); new ones from the Japanese katakana, made readable (D3, D9).
- Credits: English; TM31/TM32 dev notes translated (D7).
- Register: dark fantasy with gallows humour; Markfiend is curt and sarcastic, Mirlei gentle but stubborn. No
  modern slang. Sound effects in plain words.
- Abbreviations: avoid; when a fixed field is too small, use the short form listed here (max column), never invent
  a new abbreviation without reporting it.

## Style rules
- Punctuation map: 「」 -> "...", 『』 -> '...' (titles of books: '...'), …… -> ..., ！？ -> !?, ～/∼ -> drop or "~"
  only in the comic lines, ‧/． in names -> a space ("Lezard Valeth").
- ASCII only. No [ ] \ ^ in the English text: they mark control codes.
- Speaker tags in TM messages: 【name】 -> [Name] is NOT allowed (brackets): write the name followed by a colon,
  coloured as in the source (`\2Name:\F`).

## Characters
| JP | EN | max | notes | Avoid |
|---|---|---|---|---|
| 馬克芬德 / 馬克 | Markfiend / Mark | | hero (JP マルクフィーンド); 馬克 alone = Mark (Mirlei's and others' short form) | Macfinder / Marc |
| 蜜爾芮 | Mirlei | | heroine (JP ミィルレイ), healer, wears the spirit ring | Mirrei / Mireille |
| 理查‧Ａ‧馬金泰爾 / 理查國王 / 理查 | Richard A. MacIntyre / King Richard / Richard | | the Scarlet King of Hiouden 1 | McIntyre / Makintyre |
| 緋王 | the Scarlet King / Hiouden | | Richard's epithet; inside the title 緋王傳 it is the game name | Crimson King |
| 緋王傳 / 緋王傳Ⅱ | Hiouden / Hiouden II | | the game titles (keep romanised) | |
| 貝兒翠克絲 | Beatrix | | the dryad of Hiouden 1 | Beatrice |
| 馬金泰爾 | MacIntyre | | the royal house / kingdom | |
| 雷薩德．瓦雷斯 / 雷薩德 | Lezard Valeth / Lezard | | court sorcerer, final villain (JP レザードヴァレス) | Lesard / Valeth Lezard |
| 傑達恩 | Jadain | | warden of the dungeon, illusionist (JP ジェイダイン) | Jedan / Jaydain |
| 馬克利米里安 | Maclimilian | | unit name on stage 1 (JP マクリミリアン) | |
| 淺沼 / 稻毛 / 小澤 / 三宅 / 磯谷 / 櫻庭 / 阿廣 | Asanuma / Inage / Ozawa / Miyake / Isogai / Sakuraba / Hiro | | Wolf Team staff (TM31) | |
| 狼組 | Wolf Team | | the developer | Wolfteam |
| 王兄 | Royal Brother / brother | | late reveal (ending): Markfiend's royal elder brother, most likely the reigning king; speaker tag "Royal Brother:" (D12) | The King's Brother |
| 阿雄 / 阿祥 / 愛麗莎 / 庫諾翰 / 瑪麗 | Hsiung / Hsiang / Alisa / Kunohan / Marie | | prisoners and NPCs in TM messages (no Japanese source) | |
| 愛麗絲 / 肯恩 / 艾克 | Alice / Ken / Ike | | prisoners; Ike the grave robber (盜墓王艾克 = Ike the Grave King) | |
| 獄長 / 戴米托 / 芬格斯 | Warden / Demiter / Fungus | | | |
| 雪莉兒 / 木偶人 / 傀儡師 | Cheryl / Puppet / Doll Master | | the puppet's lost love, the puppet, their maker | |
| 魔精之王 | King of Fiends | | | |
| 比爾雷斯 | Biares | | the sorcerer of 比爾雷斯的魔藥 (Biares's Elixir) | |
| 克里斯特 | Krist | | author of 'Krist's Art of War' | |

## Places
| JP | EN | max | notes | Avoid |
|---|---|---|---|---|
| 魔界 | the Demon Realm | | | Hell / Makai |
| 地下牢 | the Dungeon / Dungeon | | the prison level names: 地下牢Ｂ６Ｆ -> Dungeon B6F | |
| 地下迷宮 / 地底湖 / 古代遺跡 / 溶岩地帶 | Underground Maze / Cavern Lake / Ancient Ruins / Lava Fields | | floor names | |
| 火燄結界 / 魔劍長眠之地 / 冰封的大廳 / 冰封的迷宮 / 地下神殿 | Flame Barrier / Demon Sword Tomb / Frozen Hall / Frozen Maze / Sunken Temple | | floor names | |
| 魔法陣 / 傳送用魔法陣 | magic circle / warp circle | | | |

## Items / skills / terms
| JP | EN | max | notes | Avoid |
|---|---|---|---|---|
| 妖魔 | demon | | the summoned monsters of the series | youkai |
| 精靈 | spirit | | | fairy / elf |
| 契約 | pact | | demons bound to the hero | contract |
| 召喚 | summon | | | |
| 火燄術 / 冰雪術 / 雷擊術 / 殞石術 / 妖殺術 / 混亂術 / 蠱毒術 / 石化術 | Fire / Blizzard / Thunder / Meteor / Demon Bane / Confuse / Poison / Petrify | | spells | |
| 熊熊變身術 / 治療術 / 還原術 / 復活術 / 隱身術 / 騰空術 / 區域禁咒術 / 魔法陷阱術 | Bear Form / Heal / Restore / Revive / Invisibility / Levitate / Anti-Magic Zone / Magic Trap | | spells | |
| 召喚術 | Summon X | | 精靈召喚術 Summon Spirit, 死靈召喚術 Summon Undead, 女戰神 Valkyrie, 獨角獸 Unicorn, 火精 Ifrit, 銀龍 Silver Wyrm | |
| 風精 / 火精 / 魔精 / 小惡魔 / 沙羅曼蛇 / 女戰神 / 獨角獸 / 銀龍 | Sylph / Ifrit / Fiend / Imp / Salamander / Valkyrie / Unicorn / Silver Wyrm | | demons | |
| 魅影冰魔 / 九犬妖女 / 三頭火獸 / 豬怪 / 地精 / 蛇妖 | Ice Phantom / Scylla / Chimera / Orc / Kobold / Lamia | | monsters | |
| 精靈項鍊 / 銀之鑰 / 紅寶石之鑰 / 生鏽的牢房鑰匙 / 蛇之鑰 | Spirit Necklace / Silver Key / Ruby Key / Rusty Cell Key / Serpent Key | | items (full list in script/IMMM.json en) | |
| 緋王劍 | Scarlet King's Sword | | | |

## Decisions from earlier batches
- (none yet)
