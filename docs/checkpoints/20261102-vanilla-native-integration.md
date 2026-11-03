# Combo Vanilla + NEI native integration candidate

This candidate completes the source integration following journal checkpoint `817e3cbf86132077bdd0a77f6d05b0bc5978b624`, full tree `71f019906732b0d3b0145e8d6e9958a2e180ddce`. It remains on `candidate/combo-vanilla-icons-20261102`, public fork `marsh6487/ComboShip`, draft PR #79. Source/native verification is separate from configured gameplay and acceptance; no accepted asset master or archive is changed.

## Implemented behavior

- A new `Vanilla + NEI` paired file starts in OoT without a seed. Its immutable mode/version and `nei-only-v1` sharing profile remain file-owned. Legacy seeded and solo behavior stays on its existing paths.
- MM keeps native introduction, Deku first cycle, first reset, story resources and native item/capacity progression. Pre-reset states cannot select the combined exploration scene, spawn overrides or return tunnel. Incoming paired NEI reconciliation waits until the first native reset.
- The fixed manifest binds **39 OoT and 30 MM checks**. Existing house Feather and authored Mirror sources are retained; supplemental rewards use native presentation/interaction services and independent collection IDs. Original major rewards, quest exchanges, prizes, songs, keys and dungeon equipment are preserved.
- Shops show their exact 40/60/80/90-rupee price, commit payment once with the exact reward and permanent check, and acknowledge already-owned aliases without payment. Interrupted/cancelled/stale receipts cannot fall through to generic progressive grants. OoT drains asynchronous save workers around commit/rollback; MM captures its final native/owl snapshot before committing.
- Shared state contains explicit NEI ownership and exact skills/runes/rods/seasons, full-width custom cells and approved foreign aliases. Native health, capacities, magic, currency, ammunition, bottle contents, inventory bases, songs, story flags, forms and age remain local. Generic FC materialization and native equip overlays are bypassed for Vanilla.
- MM masks import only into OoT's foreign page. Mirror Shield imports as Shield of Ikana; Pendant imports as independent ownership. Native Razor/Gilded and Great Fairy's Sword aliases require OoT's genuine native Kokiri/Biggoron bases. Native sword/shield ownership remains detectable while NEI equipment is selected.
- Source snapshots/deltas carry exact slot/version identity. Wrong-file or malformed payloads are rejected before applying or baselining. The legacy whole-snapshot hash comparison is bypassed because the two native views intentionally differ; seeded behavior retains that comparison.
- Earned inventory/equipment, Rito, exact wand modes, independent Block/Stone cane skills and Bomb Arrows work per file despite stale global randomizer/feature settings. User preferences are not globally rewritten. Both native endings remain independent.

## Placement decisions and variants

The complete check/payload/prerequisite/price list is `combo/vanilla/ComboVanillaPlacements.inc`. OoT binds named repeatable native return entrances or the native Temple pedestal. MM reads fixed named entrance/start-position lists from the loaded scene header, including packed native yaw and ordered alternate-header inheritance. Checks do not move because another reward was collected. Both adapters reject missing anchors, dynamic/steep/exit/submerged floors and blocked approaches.

One visible location adjustment from the proposal: **Deku Leaf is at the Sacred Forest Meadow south-entry ground approach, before the maze**, rather than beside Saria's live actor. Saria's native spawn requires Zelda's Letter; using her as the source would have added an unapproved story gate. The Four Sword remains a separate post-Forest Temple return/blue-warp reward. Named return entrances are repeatable, so already-completed quests retain a later collection opportunity.

Unverified minor-chest substitutions became additive interactions; no native rupee/ammo check identity was guessed. The bowling bonus records any of the six actual native prize IDs, preserving each original prize. Its local event history is not shared. Retained authored Mirror and stock swamp fallback share `vanilla.nei.magic_mirror`; native scene-owner/dependency selection permits one representation, and incomplete resources fail closed.

These source rules support loaded stock/authored variants without editing their archives. They do **not** prove that every proposed point is visible, reachable or collision-safe in the user's installed packs. Rejected geometry logs the affected check and suppresses that placement for runtime review.

## Explicit scope limits

| Capability | Version-one decision |
| --- | --- |
| Water Dragon Scale | Excluded explicitly: its legacy identity grants Sage's Tunic. No substitute item is awarded. |
| Foreign OoT native equipment in MM | Bases and gameplay effects remain deferred. Ultrashot, Rito Mask, Hammer Axe and True Master Sword entitlements persist in separate latent MM fields and round-trip without creating native gear, masks or forms. |
| MM Tri Rod behavior | Skill ownership is retained; MM's echo/Tri Rod behavior is unported. The fixed source is in OoT, where the existing echo system operates. No fake platform or replacement skill is granted. |
| Learned Tri Rod echoes | Existing local echo system is preserved; no per-echo chests or invented MM echo transfer. |
| Custom songs | Existing vanilla Command Melody learning is retained. Fugue of Home/Ballad of the Hero have no verified native teacher in the inspected source and their full non-rando behavior/teaching remains separate; no native song reward is replaced. |
| Capacity upgrades, Mario Mask | Additional native capacity upgrades are excluded; Mario Mask remains optional/off by default. |

## Source and native verification

The six new CI gates run through the actual regression driver with the combined graphics ABI. They cover the exact journal, actual host callbacks/disk restart, native sharing/serialization, first-cycle resources/endings, both reward adapters, native resource traversal/geometry policy, receipt lifecycle and Mirror owner selection. Existing paired creation/serializer/opening suites remain registered.

Independent reviewers verified the host journal, native sharing and both native adapters. Concrete defects reproduced and corrected during review include accepted MM talk ordering, packed entrance yaw, inherited alternate lists, stale SW97 gates, native text-ID collisions, arrival-dependent anchors, independent cane collection order, latent ownership families and sword aliases under NEI equips. Fresh sanitizer suites pass with leak detection disabled for this host. Fixture boundaries include collision services, portions of native grants/presentation, saving and network transport; complete network delta assembly and configured gameplay are not established by these checks.

The launcher and modified full game translation units pass local syntax checks. The preceding journal checkpoint's public CI passed its full Linux build and five regression groups; the entrance group exposed a missing extraction dependency, now repaired without deleting assertions. The final candidate's platform jobs must be read separately before claiming its actual game build passes.

### CI dependency follow-up

The complete `2e4edd9250929205d35d8adfa281040b6a702bc7` source checkpoint restored independently from the public fork: all 84 changed blobs match the reviewed hashes. Its Actions run `41543875980` passed source/format, world and entrance checks but exposed a missing production `ComboVanillaMode.h` include in `OTRGlobals.cpp`. The actual full translation unit reproduces the undeclared `Combo_IsVanilla` error before that include and passes syntax verification with it.

The same run exposed legacy extraction fixtures missing the newly required mode state or native helper bodies. The bounded follow-up supplies production headers, explicit seeded mode state where combined saves are exercised, the real Rito predicate and the real queued peer-pull handler. It also repairs the corresponding editor, pause-wheel, equipment, wand and paired-cane extraction boundaries. Existing behavioral assertions remain intact; reward bindings, sharing policy and placement behavior are unchanged.

The six affected registered CI entries pass locally through the actual combined-ABI regression driver, including complete native weather/winter and MM editor syntax checks. Focused wand, equipment-access and paired-cane checks also pass. A fresh platform Actions run is still required for the follow-up revision; local syntax and fixture results do not establish a complete game build or configured runtime behavior.

The follow-up run `41547590152` passed source/format and entrance checks, and built the complete OoT engine on Linux before continuing to MM. Its later fixture failures reproduced missing dependencies in mask routing, native seasonal weather and both pause-page prefixes. Those fixtures now supply the real Rito predicate, production mode header and explicit seeded mode globals as appropriate; both C and C++ consumers retain their original assertions. All 58 equipment entries pass across the full local run and its repaired continuations. Platform builds and the other regression groups remain separately tracked in PR #79.

The final item continuation exposed the same missing mode dependency in the legacy native Magic Mirror grant fixture. Both ports now use the production mode header and explicit seeded combined state. All 51 item entries and all 58 equipment entries passed locally; the 42 system entries cleared through the public CI prefix and local continuations. Rendering continuation and actual platform results remain separately tracked in PR #79. These fixture-only changes preserve the game sources from `2dfac243a8cef8847f560006f4d467df845939f6`.

## Preserved Shadow Crystal and dialogue/font fixes

The inherited fixes from `33359ffce2a79e80d7f490d64c1462bfcaad6b14` remain part of this same source candidate. Wolf/Shadow Crystal entry and removal retain the flash path and outgoing-form capture instead of updating an unused MM skeleton callback. New Rito availability edits are outside those crash-fix hunks; the eight focused Wolf transition controls pass again.

Dialogue wrapping retains the 8192-byte scratch/destination fix in `ComboItemReceiptPresentation.h`. Native MM glyph/resource bounds remain in `z_kanfont.c`; optional GUI fonts still load after archives in `BenPort.cpp`. Receipt layout, native decoder/font bounds, mod-font selection and location-title suites pass. External font-pack overrides and visual acceptance remain runtime checks. The inherited magic-refill trace is still diagnostic only.

## Decisive configured runtime checks

1. Create a seedless paired file, start OoT, and verify old seeded/solo files remain distinct.
2. Enter MM for the first time; complete the native introduction, Deku cycle and first reset with no imported NEI effects beforehand.
3. Travel/save/reload both ways. Verify shared earned NEI and unchanged local native inventory, capacities, bottles, songs and story.
4. Collect representative early/late rewards, Block before Statue, Stone before Flip, Cape before house Feather, exact rods/runes/seasons and imported aliases with/without native sword bases.
5. Buy each price tier, cancel/retry a receipt, interrupt/reload, copy the paired file, reset MM and revisit already-completed quests. Check one permanent grant/payment.
6. Inspect every supported stock/authored anchor for visibility, floor clearance, ordinary native access and safe return; investigate any suppressed-check log.
7. Finish either game while the peer is unfinished and confirm its ordinary ending.
8. Repeat Shadow Crystal entry/removal/rapid input in the reported scene and inspect long dialogue/map text with the actual font-pack load order.

Configured game runtime and user acceptance are pending. Source publication is a recoverable candidate checkpoint, not promotion to an accepted master.
