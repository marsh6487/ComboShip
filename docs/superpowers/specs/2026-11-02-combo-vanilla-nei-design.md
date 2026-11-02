# Combo Vanilla with NEI rewards — proposed design

Status: placement and behavior proposal for cor to review. No Combo Vanilla product code is implemented by this document.

## Intent and decisions

Play OoT and MM together with recognizable vanilla progression, cross-game travel, and NEI rewards distributed across both worlds. cor accepted fixed placements and asked Codex to propose the locations; cor does not need to author the placement list. Existing optional rewards, extra chests, shops, and supplementary quest rewards are all available approaches.

Codex's proposed defaults below are recommendations, not previously approved individual locations. Exact positions, collision clearance, actor parameters, and source-check IDs must be verified during implementation. Region names are intended destinations, not claims that a placed actor has been tested there.

## Approaches considered

| Approach | Benefit | Cost / decision |
| --- | --- | --- |
| Fixed NEI rewards over vanilla progression | Consistent discoveries, themed rewards, no seed generation required | Selected in conversation; Codex drafts and implements the placements after review |
| Shuffle NEI while protecting vanilla rewards | Different exploration each run | Possible later option; requires placement logic and enough eligible locations; excluded from first version |
| Distribute everything through a shop or starting inventory | Smallest integration | Does not satisfy the requested discoveries throughout both games |

Within the selected approach, prefer a safe existing low-value check when its identity is reliable. Use an additive reward when replacing a native reward would alter a quest, economy, or progression. Use a new chest only where no suitable check exists. Preserve vanilla heart pieces, bottles, masks, songs, dungeon equipment, keys, boss rewards, and quest exchanges. A supplementary reward occurs after the native reward finishes, never overwriting it.

## Source baseline and evidence

- Full upstream candidate: `marsh6487/ComboShip`, `candidate/crash-hints-market-magic-trace-20261102`, commit `46d57fd52152454388dea6d36c86d26c734d1615`, tree `0ec3b5f63ebb37753a353405ffa3880e901e2da2`.
- Local inspected subset at start: `c7dcac5`; this is a recovery history, not the full upstream history. Imported unchanged headers must not be uploaded as feature changes.
- `combo/ComboGameMode.h`: combined versus solo game ownership already exists. Keep Combo Vanilla inside combined play; do not make a fourth launcher game or change solo guards.
- `combo/ComboShip.cpp`, `Combo_OnOOTSaveInit`: seedless combined creation currently logs an error instead of creating a paired vanilla MM save. A menu label alone cannot implement this mode.
- `soh/soh/FleetShipCombo/FleetComboItems.h`, blob `a130a83388aa584095f7d9fe3e7e8903f477605c`: authoritative inspected cross-game item identities and progressive families. This list includes native items and randomizer-only capabilities; it is not itself an NEI checklist.
- `soh/mods/nei_save.h`: persistent ownership for cane skills, runes, seasons, upgrades, and independent travel tools already exists. Do not reinterpret inventory-cell contents as ownership of every item sharing that cell.
- `soh/mods/items/logic/item_cane_of_somaria.h`: six named skill IDs and explicit per-skill grants exist. Fixed rewards should use those identities rather than a random or order-dependent generic pickup.
- `combo/gameplay/MagicMirrorItem.h`: permanent, unshuffled Mirror identity. Preserve the existing stump acquisition; do not add a second copy.
- `soh/mods/house_rocs_feather.cpp`: an existing Link's House feather pickup exists. Its exact item identity must be resolved against the two separate feather identities before enabling a second feather reward.
- `combo/NeiSeasonsPolicy.h` and `NeiSeasons.cpp`: individual pickups and completion-gated seasons exist; outside rando the inspected OoT policy selects individual pickups. This proposal chooses individual season rewards, without a second completion-gated grant path.

Existing CI on the baseline failed formatting before the game-build matrix ran. This document does not change that status or establish a build/runtime result. Existing crash, hint, cursor, camera and seasonal source candidates remain inherited; magic-load tracing remains diagnostic only. DMT night improvement is cor's report about a separate overworld POC; day remains unverified.

## Product behavior

### File creation and identity

Offer `Vanilla + NEI` and `Randomizer` when creating a combined file. Default proposal: start a new Vanilla + NEI file in OoT, retaining native early story progression. Both game saves belong to the same slot and player name. Paired creation must complete before the slot becomes playable; on failure, retain the previous slot or show a recoverable creation error, rather than leaving half a pair.

Persist the mode and a placement-version number in the combined container. The file's identity governs subsequent loads, including launching directly into the last-played game. Do not infer the mode from global CVars, the currently loaded seed, a missing seed, or a missing placement table. Existing seeded saves stay Randomizer; existing solo saves stay solo. Never silently convert an existing file. The native per-game initialization must create legitimate vanilla quest/time/magic state without requiring a fabricated randomizer seed.

### Vanilla contract

Vanilla checks retain vanilla rewards except the explicitly reviewed minor-check substitutions listed below. Dungeon layouts, entrances, boss placements, native song teaching, quest chains, and MM's first-cycle progression remain native. Keep built-in Combo travel and its ordinary unlock rules. The route to the other game must be reachable with native items independently of the NEI reward list. Verify that claim before release; a new NEI travel item must never be needed to reach its own acquisition.

Native climbing, swimming, crawling, speaking, opening chests, clocks and instrument buttons remain available under their normal game rules. Do not import randomizer ability locks, enemy/boss souls, shuffled ocarina buttons, Triforce hunt, or Greg goals into Vanilla + NEI. Proposed ending: each game retains its native completion behavior; any optional shared-goal setting must be explicit and persisted, never inherited accidentally from a previous seed.

Keep native base capacities and vanilla magic progression. Do not import the last seed's 15-upgrade RPG rules, fractional magic settings, starting inventory or randomized settings. NEI abilities can expand the player's options; ordinary progression never requires an NEI item. Do not attempt to prevent intentional sequence breaks that an optional movement item permits.

### Per-file sharing profile

Proposed default: share NEI additions, retain each game's native progression. Persist this explicit profile in the file; never inherit the last seed's sharing configuration.

| State | Vanilla + NEI default |
| --- | --- |
| Added NEI ownership, cane skills, slate runes, seasons, NEI equipment/upgrades | Shared once through the paired slot |
| Native inventory and base equipment, magic/capacity levels, health, rupees, ammo and bottle contents | Local to each game; no generic native-item reconciliation |
| Native songs, instrument ownership/buttons, story/quest flags, masks, dungeon equipment/rewards/keys, time access and first-cycle state | Local; learned/earned at native sources; no remote write or unlock |
| Native items with a distinct foreign NEI representation | Import only into that dedicated foreign representation: MM Mirror Shield -> OoT Shield of Ikana; MM masks -> OoT imported mask page; MM Pendant -> OoT independent Pendant. Do not write the recipient's native story flags or consume home-game quest items |
| MM Razor/Gilded and Great Fairy's Sword represented as OoT weapon upgrades | Import the upgrade only after OoT already owns its native base sword (Kokiri or Biggoron respectively). Never grant the missing base sword or advance a progressive chain generically |
| OoT items playable as foreign equipment in MM | Deferred from the first version; this avoids replacing MM's native Bow/Hookshot/masks/songs or bypassing its first cycle |

Before the first MM cycle completes, hold all incoming NEI effects and remote receipts pending; release them after the native first-cycle completion event, with no added items during the Deku opening. OoT's opening remains native except accessible optional NEI pickups. Later native checks still grant their native rewards even if a foreign NEI representation already exists. These defaults make progression predictable; broader native-item sharing can be a separately designed option.

### Collection and sharing

Each added reward has a stable, namespaced check ID independent of scene actor index, coordinates, vanilla chest bits, day/setup, and installed archive. Persist collection and grant state in the paired slot. A grant must be acknowledged before a pending receipt is cleared; define and test recovery for closing the game during the receipt. Re-entry, a lost actor, canceling dialogue, resetting MM's cycle, changing age, switching games, copying a file, or reloading must not duplicate permanent rewards.

Use existing cross-game ownership/grant paths with explicit item identities; one shared pickup must not be applied twice on reconciliation. Fixed rewards must not randomly select an unowned rune or cane skill. Never truncate extended item IDs to eight bits. Shared ownership does not create duplicate replacements for protected native quest rewards: e.g. Ikana Shield/MM Mirror Shield, Pendant of Memories, MM masks, native songs, Razor/Gilded swords and Great Fairy's Sword retain their original native sources and story interactions.

For a chest substitution, persist a distinct replacement-check identity and keep the original vanilla chest identity for native behavior. For additive NPC rewards, separate native quest completion from the NEI collection flag. Handle already-completed quests by offering the pending bonus on a later interaction, provided the player still meets the recorded completion condition. Shops show the item and price, check wallet capacity, charge once on successful purchase, and become ordinary restock or sold-out entries after collection. No permanent NEI reward requires an irreplaceable trade item or a permanently missable day/time window.

## Proposed location list

These are draft placements to review together. `Extra` means a new optional pickup in existing walkable space; `Bonus` means an additional reward following the named native interaction; `Minor` means replace an existing low-value rupee/ammo chest only after its exact check is verified. A minor check that turns out to be protected becomes an Extra nearby. No new geometry, mandatory quest, dungeon route, or additional heavy NPC model is required.

Access is deliberately expressed in native capabilities. Validate specific room/setup/day and safe return with the real game and authored scenes before selecting final coordinates. Added pickups must also exist with stock assets; authored archive layouts need safe placements or a guarded variant, not raw coordinate assumptions.

### Exploration and travel tools

| Item / identity | Proposed home | Reward type and native access |
| --- | --- | --- |
| Lantern — `FCI_LANTERN` | OoT Market bazaar | New shop offer, 40 rupees; early child access; affordable under native starting wallet |
| Shovel — `FCI_SHOVEL` | OoT Graveyard, Dampe's hut exterior | Extra ground-level chest; ordinary graveyard access, no digging requirement |
| Net — `FCI_NET` | MM Southern Swamp tourist area | Extra chest by a walkable bank; ordinary swamp access after the first cycle |
| Beetle — `FCI_BEETLE` | OoT Kokiri shop | New shop offer, 60 rupees; no Beetle puzzle to buy it |
| Gust Jar — `FCI_GUST_JAR` | MM Deku Palace outer gardens | Minor low-value chest or Extra; native Deku access; no palace bean shortcut required |
| Deku Leaf — `FCI_DEKU_LEAF` | OoT Sacred Forest Meadow | Extra at the ground-level approach to Saria's clearing; native maze route |
| Whip — `FCI_WHIP` | MM Woodfall approach | Extra on a native Deku-accessible platform with an ordinary return route |
| Mogma Mitts — `FCI_MOGMA_MITTS` | OoT Goron City | Bonus after Darunia grants the Bracelet; do not replace the Bracelet |
| Ball and Chain — `FCI_BALL_AND_CHAIN` | MM mountain village after Goht | Extra in ordinary thawed ground; no new ice or chain puzzle |
| Spinner — `FCI_SPINNER` | OoT Gerudo Fortress | Bonus after membership; keep card and prisoner rewards |
| Switch Hook — `FCI_SWITCH_HOOK` | MM Great Bay, Pirates' Fortress exterior | Minor or Extra reached with native Zora/Hookshot capabilities |
| Clawshot — `FCI_CLAWSHOT` | OoT Gerudo Training Ground | Extra in the entry-side walkable area after admission; no extra key spend |
| Roc's Feather / Cape — two `FCI_SKIJER_ROC` copies | Feather: OoT Link's House if the current pickup is this identity, otherwise an Extra at the Kokiri-to-Lost-Woods approach; Cape: MM Milk Road after Epona | Exact Feather grant at the first source; explicit Cape ownership at the latter, independent of collection order; both optional and reachable without jumping |
| Separate SoH Roc's Feather — `FCI_ROCS_FEATHER` | Retain existing Link's House source if it owns this identity; otherwise OoT Lake Hylia | One Extra on the accessible fishing-pond approach; do not duplicate the house pickup; exact existing owner is a required implementation audit |
| Rito Mask — explicit native grant required | OoT Death Mountain summit | Extra at safe ground near the Great Fairy approach; native bomb route, no flying requirement; not a new MM transformation-mask reward |
| Minish Cap — `FCI_MINISH_CAP` | OoT Kokiri Forest after the Deku Tree | Extra beside existing ground-level village scenery; no small-form access gate |
| Poke Ball — `FCI_POKEBALL` | MM Milk Road | Extra ground-level chest after first-cycle access; no companion required |
| Shadow Crystal — `FCI_SHADOW_CRYSTAL` | OoT Graveyard after learning Sun's Song | Bonus on leaving the Royal Family's Tomb; native tomb reward remains intact; entry/exit and animation-skip regression required |
| Time Gate — `FCI_TIME_GATE` | OoT Temple of Time after native adulthood | Bonus at a separate pedestal interaction after the Master Sword event; no sword replacement or automatic cutscene interruption |
| Magic Mirror — `FCI_MAGIC_MIRROR` | Existing MM Lost Woods stump | Retain authored Yes/No interaction and persistent pickup; stock-scene fallback: a ground-level reward at the Southern Swamp tourist-center exterior, with the same Yes/No interaction and shared collection ID. Activate only one representation for the supported scene configuration; no second grant |
| Phantom Hourglass — `FCI_PHANTOM_HOURGLASS` | MM Clock Town clock-tower base after first cycle | Extra outside the interior story path; no hourglass/time-stop gate |
| Zonai Timer — `FCI_ZONAI_PERMAFROST` | MM Snowhead approach | Extra on ground accessible through the native Goron route |
| Hylia's Grace — `FCI_HYLIAS_GRACE` | OoT Temple of Time after Forest Temple | Supplementary altar interaction; require native Forest completion, no Chamber archive dependency; retain any existing permanent source if found |
| Demise's Destruction — `FCI_DEMISE_DESTRUCTION` | MM Ikana Castle after the native king encounter | Supplementary room-exit reward; preserve Elegy and all native progression |
| Bottomless Bottle — `FCI_BOTTOMLESS_BOTTLE` | MM swamp potion shop after Koume's rescue | New shop offer, 90 rupees, after native rescue completion; protect all original bottle rewards and contents |

The stock Mirror fallback is an implementation target, not an existing tested actor. It uses native post-first-cycle swamp access and cannot require the Mirror itself. The authored stump and fallback share `vanilla.nei.magic_mirror`; loading a different archive never restores collection. Verify the scene-owner detection before choosing which representation spawns.

### Magic rods, cane skills and slate runes

| Item / identity | Proposed home | Reward type and native access |
| --- | --- | --- |
| Fire Rod — `FCI_FIRE_ROD` | OoT Dodongo's Cavern exit | Bonus after King Dodongo; preserve Heart Container, Ruby and warp |
| Ice Rod — `FCI_ICE_ROD` | MM mountain village after Goht | Bonus at a separate post-thaw interaction; distinct check from Ball and Chain |
| Light Rod — `FCI_LIGHT_ROD` | OoT Desert Colossus fairy approach | Extra outside the fountain on safely reachable ground; native wasteland access |
| Dominion Rod — `FCI_DOMINION_ROD` | MM Ikana Castle courtyard | Extra reached with native Ikana access; no statue-moving gate |
| Sand Rod — `FCI_WAND_SAND_ROD` | OoT Desert Colossus | Extra beside the Spirit Temple approach; separate from fairy and Light Rod |
| Tornado Rod — `FCI_WAND_TORNADO_ROD` | MM Woodfall after Odolwa | Bonus outside the temple after normal return; no wind gate |
| Water Rod — `FCI_WAND_WATER_ROD` | OoT Zora's Fountain | Extra at a dry, native-accessible shore; no Water Rod/advanced swimming gate |
| Meteor Rod — `FCI_WAND_METEOR_ROD` | OoT Death Mountain Crater after Volvagia | Bonus at the safe Fire Temple return point; preserve Fire Medallion |
| Storm Rod — `FCI_WAND_STORM_ROD` | MM Great Bay after Gyorg | Extra on the normal return shore; no storm requirement |
| Shadow Scepter — `FCI_WAND_SHADOW_SCEPTER` | MM Ikana Graveyard | Extra reachable with native Captain's Hat route; make availability repeatable across cycles |
| Somaria: Statue — skill 0 | OoT Goron City after Bracelet | Extra; separate from Mitts; no Cane requirement |
| Somaria: Block — skill 1 | MM swamp tourist area | Extra; distinct from Net; native post-first-cycle swamp access |
| Somaria: Platform / Tri Rod entry — skill 2 | OoT Lake Hylia after Water Temple | Supplementary shore reward; verify actual skill-to-Tri Rod behavior before writing receipt text |
| Pacci: Flip — skill 3 | MM Clock Town curiosity shop | New shop offer, 90 rupees; ordinary night shop access; repeatable window |
| Pacci: Stone — skill 4 | OoT Gerudo Training Ground | Minor in an optional native-accessible room; no Cane puzzle or additional key gate |
| Pacci: Ultrahand — skill 5 | MM Stone Tower approach | Extra reached with native Elegy and Hookshot; no Ultrahand gate |
| Slate: Remote Bomb — `FCI_SLATE_RUNE_BOMB` | OoT Bombchu Bowling | Bonus after first native prize won; preserve original prize and ammo |
| Slate: Stasis — `FCI_SLATE_RUNE_STASIS` | MM mountain smithy after thaw | Extra outside shop; preserve Razor/Gilded transaction |
| Slate: Cryonis — `FCI_SLATE_RUNE_CRYONIS` | OoT Ice Cavern | Bonus after the native final reward/Sheik sequence; preserve boots, song and bottle contents |
| Slate: Master Cycle — `FCI_SLATE_RUNE_MASTER_CYCLE` | MM Romani Ranch after Epona | Separate ranch interaction; keep horse/song access and the alien-defense quest unchanged |
| Slate: Sensor — `FCI_DESIRE_SENSOR` | OoT Market bazaar | New shop offer, 80 rupees; verify this grants the sensor rune under the current layout, not a dead legacy item |

Each rune establishes the slate's usable ownership as its native grant requires. Do not also place a generic randomly-selected Sheikah Slate pickup. The six named wand rods establish the wand family; do not additionally grant the all-rods/medallion generic wand. Each cane skill is a distinct reward, not six calls to a generic progressive/random grant. Tri Rod learned echoes remain governed by the existing echo system, without inventing a chest for each echo.

### Seasons and equipment

| Item / identity | Proposed home | Reward type and native access |
| --- | --- | --- |
| Rod of Seasons — `FCI_ROD_OF_SEASONS` | OoT Lost Woods, Kokiri-side entrance | Extra on ordinary walkable ground; usable ownership independent of season unlocks |
| Spring — `FCI_SEASON_SPRING` | OoT Zora's River | Extra on native walkable lower-bank terrain; accessible without spring/weather changes |
| Summer — `FCI_SEASON_SUMMER` | MM Great Bay coast | Extra on dry shore reachable through normal Zora progression |
| Autumn — `FCI_SEASON_AUTUMN` | MM Southern Swamp after Odolwa | Supplementary tourist-area reward; separate from Tornado Rod |
| Winter — `FCI_SEASON_WINTER` | OoT Ice Cavern approach | Extra on native-accessible solid ground; no winter/ice creation gate |
| Cane of Byrna — `FCI_EXT_CANE_OF_BYRNA` | MM Ikana Canyon | Extra reached through native canyon progression; no invincibility gate |
| Four Sword — `FCI_EXT_FOUR_SWORD` | OoT Sacred Forest Meadow after Forest Temple | Supplementary clearing interaction; preserve Saria/Sheik/song behavior |
| Trident — `FCI_EXT_TRIDENT` | MM Ikana Castle after the king | Separate optional courtyard cache; no duplication of Demise reward check |
| Goddess Shield — `FCI_EXT_DIVINE_SHIELD` | OoT Hyrule Castle fairy approach | Extra outside the fountain; native bomb access; do not replace Din's Fire |
| Kite Shield — `FCI_EXT_SHEIKAH_SHIELD` | OoT Kakariko bazaar | New shop offer, 80 rupees; distinct from Lantern |
| Magic Cape — `FCI_EXT_MAGIC_CAPE` | MM Ikana Graveyard | Extra in a repeatably accessible native grave route; no invisibility gate |
| Spirit Breastplate — `FCI_EXT_SPIRIT_BREASTPLATE` | OoT Spirit Temple after Twinrova | Bonus at safe return, preserving native reward and Mirror Shield |
| Champion's Tunic — `FCI_EXT_CHAMPIONS_TUNIC` | MM Zora Cape after Gyorg | Extra on shore; separate from Great Bay Storm Rod |
| Pegasus Boots — `FCI_EXT_PEGASUS_ANKLET` | OoT Lon Lon Ranch | Bonus after an ordinary ranch minigame win; preserve heart piece/bottle/song and avoid changing race difficulty |
| Climb Boots — `FCI_EXT_CLIMB_BOOTS` | MM Snowhead mountain route | Extra reachable with native Goron traversal; no climb-boot surface required |
| Roc's Boots — `FCI_EXT_ROC_BOOTS` | OoT Gerudo Valley | Extra reached via the native bridge/Longshot route; no enhanced jump gate |
| Water Dragon Scale — `FCI_EXT_WATER_DRAGON_SCALE` | MM Great Bay coast after Gyorg | Extra at a dry native-accessible approach; no deep-diving gate |
| Ultrashot — third `FCI_HOOKSHOT` level, explicit upgrade grant | OoT Lake Hylia after Water Temple | Bonus once native Longshot is owned; never advance a missing base Hookshot |
| Hammer Axe — second `FCI_HAMMER` level, explicit upgrade grant | OoT Goron City after Fire Temple | Bonus only with native Hammer and Fire completion; preserve Darunia/story rewards |
| True Master Sword — second `FCI_MASTER_SWORD` level, explicit upgrade grant | OoT Temple of Time after all six native medallions | Optional altar reward, native Master Sword prerequisite; preserve time travel and pedestal behavior |
| Twilight Upgrade — explicit native upgrade grant | MM Ikana after Stone Tower | Extra on safe ground after native clear; reuse its existing Bomb Arrow behavior without granting random bow levels |
| Bomb Arrows — `FCI_BOMB_ARROWS` | OoT Kakariko bazaar, adult optional offer | New 80-rupee upgrade offer only after native Bow and Bomb Bag; if Twilight already owns it, mark this check satisfied without charging or giving another major reward |

The Shield of Ikana uses MM's native Mirror Shield acquisition; the Pendant of Memories uses its native quest; Great Fairy's Sword uses its native fairy source; Razor and Gilded swords use the native smithy. Their allowed foreign effects follow the explicit profile above, with native interactions intact; do not enable generic native-item sharing to achieve this. Other native MM masks, bottles, Powder Keg, Pictograph Box and OoT items likewise keep their sources. Do not fill every menu slot by duplicating those items.

Quartz of Motion is a second Stone of Agony identity (`FCI_STONE_OF_AGONY` chain length 2). Proposed extra: OoT House of Skulltula supplementary interaction after 30 tokens and native Stone of Agony ownership; retain the native 30-token reward. Verify exact grant rather than awarding the first-level stone again. New native-capacity upgrades (fourth Bow/Bomb Bag, extra Wallet/Scale) are excluded from the initial placement set until their non-rando grant and capacity policies are audited; they must not appear accidentally from a generic progressive grant.

## Existing features and deliberately separate options

Command Melody retains the earlier vanilla-learning contract: available when its applicable instrument/companion feature becomes usable, rather than another randomly withheld ability. Keep companion enablement and natural enemy encounters under their existing options. Do not add mandatory enemies or change actor models as part of the reward pass. Fugue of Home and Ballad of the Hero need their existing native-teaching audit before a placement is added; preserve any existing source, and propose an additive teacher interaction only if none exists. Mario Mask remains an explicit optional extra, disabled by default for this first Vanilla + NEI proposal; if cor wants it included, a Clock Town minigame bonus can provide it without replacing a native prize.

These are proposed scope decisions, visible for review rather than silent omissions. No generic "grant all NEI" or cheat ownership toggle should be required to use items actually earned in this mode. Audit any existing global feature toggles that currently disable earned equipment; solve availability per file without permanently changing the user's solo/randomizer settings.

## Implementation boundaries and builder decisions

1. Introduce per-file mode/version and native paired-save initialization first. Validate save creation/load/copy/delete, game switching, and absence of stale seed settings before adding rewards.
2. Audit existing permanent NEI sources in both games, particularly feather identities, Hylia's Grace, seasons, custom songs, Rito Mask and Mirror fallback. Retain a working source and remove its proposed duplicate from the placement manifest; record the resolution visibly.
3. Add a neutral, versioned reward manifest with exact native grant payloads, collected-check IDs, prerequisites and per-scene variants. Reuse existing receipt/GI pathways. New rewards must remain distinct from randomizer check IDs and leave existing seeded behavior unchanged.
4. Bind a small early set in both games (Lantern/Shovel/Net plus retained Mirror/feather), then implement the remaining table by region. Choose actual minor checks from source, not coordinate proximity. Use safe existing NPCs/chests rather than new heavy actors.
5. Audit stock and authored scene geometry for final coordinates. If the authored archive is unavailable, ship the supported stock placement and leave that authored variant explicitly unverified; do not claim compatibility from source checks alone. No modification or consolidation of accepted `.o2r` archives is authorized by this design.
6. Keep future shuffle outside the first implementation. Persist a placement version now so later additions do not revive already-collected rewards or reinterpret old file IDs.

The builder owns exact source modules, serialization keys, internal mode enum values, check ID strings, native actor/chest parameters, final coordinates, and minor-check substitutions within the preservation contract. If the actual grant behavior differs from the label (e.g. randomized rune, missing full-width ID, no non-rando usable behavior), repair or defer that reward explicitly; do not silently replace it with a different item. Show any consequential location or progression change to cor in the updated manifest.

## Verification and recovery

Required source/native checks cover paired initialization and rollback, per-file identity versus global settings, legacy seeded/solo loads, one-time reward receipts and interrupted grants, shared reconciliation without double-progressive upgrades, explicit cane/rune grants, full-width IDs, no vanilla reward replacement beyond the approved minor list, and MM cycle resets. Include negative controls for Randomizer and Solo. Run the repository's formatting and required checks before publishing code checkpoints.

Required configured runtime evidence covers new file creation, first travel both ways, save/reload in each game, age and day/setup variants, normal MM first cycle and Song of Time reset, shop charging/capacity, native quest followed by bonus, collecting after a quest was already completed, interrupted/canceled receipts, and stock versus authored placement collision/visibility. Test Shadow Crystal animation skip separately from enabling its fixed reward. Validate that all rewards are reachable using native abilities and that the return path is safe; a thematic location is not proof of reachability.

Candidate/master status stays separate: this proposal is not implemented, built, runtime-proven or promoted. Preserve the baseline above. Future product changes go on a separate feature candidate inheriting the verified bugfix checkpoint, not directly onto develop or an accepted scene master. GitHub is the requested durable checkpoint destination; source publishing does not mean runtime acceptance.

## Review requested

The selected overall approach is already agreed. cor should review the actual locations and visible proposed defaults: individual seasons/runes/rods, native story and quest rewards retained, optional late upgrades, OoT start, NEI-only sharing with dedicated foreign-item exceptions, first-cycle isolation, native endings, and Mario Mask off by default. The most useful response is corrections to these choices. Approval of this document enables the implementation plan; it does not imply any in-game acceptance.

Builder review: a separate read-only review identified native shop age restrictions, incomplete Feather fallback, missing stock Mirror fallback, and unspecified sharing defaults. This revision corrects those four issues before user review. Exact placements remain proposed and runtime-unverified.
