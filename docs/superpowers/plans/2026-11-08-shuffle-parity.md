# ComboShip shuffle parity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the complete user-authorized parity scope with Glitchless correctness and recoverable public checkpoints.

**Architecture:** Extend native item/check/option catalogs and each game's actual runtime and solver adapters. Shared catalogs express identities while native adapters own engine behavior. Keep existing MM soul immunity and all old enum values; append new identities before sentinel values.

**Tech Stack:** C/C++ native SoH/2Ship, Python native regression fixtures, CMake and GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-11-08-shuffle-parity-design.md`

## Global Constraints

- New shuffles default off; disabled options add neither items nor checks.
- Preserve baseline numeric IDs, old saves, setting identities, solo/combined guards and all cumulative integrations.
- Glitchless never assumes a disabled trick; all runtime restrictions need corresponding reachability requirements.
- Excluded checks never receive required progression.
- Checkpoints may be WIP but must identify unimplemented/failed/untested paths; no master or merge.
- Every new shuffle and completed item-pool control uses the existing What's New accent: gold for new controls,
  cyan for updated controls. Register its real CVar and use a presentation scope for direct widgets; preserve IDs,
  layout, disabled states and the global accent toggle. Add remaining families as their actual settings land.
- User authorization 2026-11-09 supersedes the old single-worker restriction. Run six independent implementation workers concurrently in isolated worktrees. Controller owns shared integration, append-only ID allocation, publication and overall verification; serialize only genuine dependencies/conflicts.

## Current execution state — November 9, 2026

R22 follow-up integration is underway on the same authorized candidate: Termina
collision capacity, reviewed authored-key GIs and complete enemy/NPC actor-model
soul GIs are integrated through394353da. The completed Mirror follow-up binds both
existing Combo checkboxes to one effective setting (including OFF), retaining
independent standalone preferences; final original/Combo native-card verification
passes and the reviewed donor08bae2d8 is integrated. The optional rotating key
GIF is excluded at the user's request; all other uploaded assets are retained.
No additional user-facing global control is part of that task. The controller
owns merged files, exact asset blobs/tree verification, checkpoints and PR89
publication. Workstream owners and current evidence are recorded at the top of
`docs/SHUFFLE_PARITY_PROGRESS_20261108.md`; older status text below is historical.

- [x] Preserve/review/integrate Termina collision, authored key resources and full
  enemy/NPC soul models with both receiving-game renderers.
- [x] Close R21 real Windows native CLI build/link requirement; full job SUCCESS.
- [x] Repair the R21 Desired Items provider's pinned ImGui include dependency,
  retaining assertions and diagnostics; exact full row PASS without ambient paths.
- [x] Finish and integrate final shared Mirror native-card/settings/preset tests.
- [ ] Publish cumulative checkpoint with raw base64 asset blobs, exact Git hashes,
  final tree equality and fetched-byte verification.
- [ ] Complete fresh integrated Linux/Windows resources/modules/CLI and native
  generation/replay checks; repair actual failures while independent work continues.

The source implementation and independent native boundary reviews for Tasks 1–11
are integrated. Checked steps mean their stated source/boundary work has evidence;
they do not imply configured gameplay acceptance. The detailed owner, source
checkpoint and evidence ledger is `docs/SHUFFLE_PARITY_PROGRESS_20261108.md`.

R11 checkpoint `3eb1649eebd24fb9b7994c788cb268b52f7932f8`
(tree `900fd9e2c024110a26fb6cb1d5336de30745f6ef`) passed full Linux/Windows
native builds, both resource archives/smokes and first-drop Vanilla native
Off/On/On-excluded generation/replay. All eight profiles pass both real solo
generators/replays. Six complete combined profiles pass with strict native FC
wallet accounting: OWN, OWN-excluded, Stone Tower normal/inverted, rusty keys
and all-Off, including actual circular-key rejection.

The two full-feature combined profiles exposed native random confined-item
preplacements behind their own locks. Source repair596d8b60/5b0f921c exports native
allowed-check sets and places confined items with the combined assumed fill,
preserving fixed-source sharing policy. Actual native component generation and
independent original-driver replay pass for both full-feature profiles. Fully linked
R20 public1d5188e2/tree918b3b99 now passes the complete eight-case native matrix
in CI and an independent five-stream local repeat:16 actual standalone native
fills/replays and8 combined fills/original-driver replays. All3 native first-drop
Vanilla cases pass. Linux all targets/resources/smokes/AppImage and Windows bothgames/UI/launcher/ZIP
pass. Cumulative fixture closures now have passing evidence for all324 registered
rows. Fresh cumulative CI, including mandatory actual Windows CLI link, remains
required; original optional CLI step was skipped, not passed. See the exact
hashes and evidence in docs/SHUFFLE_R20_NATIVE_GENERATION_20261109.md.
Independent review of the complete production delta through2eab526b found no
material issue; the separate fixed-source sharing review also approved the repair.

The user's contents-category follow-up identified a presentation gap: the new MM
environmental sources lacked OoT-style category markers. Environment owns its
native draw/category integration and focused tests; the controller owns the
existing CSMC menu's tooltip/cyan accent. This addition is integrated and passes
actual native graphics/category/lifecycle sanitizer controls in both guards, including all66 Rusty native/export categories.
The existing CSMC CVar/default and global accent toggle remain unchanged.

- [x] Complete environmental contents-category markers, native/foreign item
  categories, option/lifecycle controls and native rendering compilation.
- [x] Finish fresh R20 Linux all-target link/resources/smokes/native seeds/AppImage.
- [ ] Finish fresh R20 Windows games/driver/launcher/ZIP and cumulative fixture-fix
  full CI, correcting actual failures without diagnostic or assertion weakening.
- [x] Complete all eight freshly linked native parity profiles, including both
  games Own Dungeon together and Stone Tower normal/inverted physical ownership.
- [ ] Publish the verified follow-up source/checkpoint and final evidence record.

## Review Focus

- Legacy settings/save arrays with new appended identifiers must retain old meaning and safe defaults.
- Foreign items and combined fills must preserve native restrictions and physical dungeon ownership.
- Scene unload/death cleanup and MM time reset must not fabricate new progression rewards.
- NPC roles, schedules, shops and decorative viewer actors need deliberate gating rather than global actor-ID suppression.
- UI availability, saved selections, pool insertion, actual runtime and seed logic must agree under both original/combined guards.

## Tasks

### Task 1: Own-dungeon boss-key regression
**Files:** `soh/soh/FleetShipCombo/FleetComboRando.cpp`, native MM placement/generation adapters as required; `tests/shuffle_parity/boss_keys/`.
**Interfaces:** Existing native option serialization and combined fill functions. No new public ABI.
- [x] Capture actual native settings -> restricted placement -> combined fill behavior with a deterministic failing scenario if the reported leak exists; add a negative control if it does not.
- [x] Correct the smallest actual boundary, preserving Anywhere/Start With/Vanilla semantics.
- [x] Run seeded Own Dungeon combinations for both games and assert every boss key's physical owning dungeon; include Stone Tower/inverted routing and exclusions.
- [x] Commit owned files and report exact commands, seeds, coverage and unresolved paths.

### Task 2: MM Saria/Desired Items parity
**Files:** MM `MagicHints.h`, `MiscBehavior/SariasSongHint.cpp`, BenPort Desired Items adapters and native candidate menu as necessary; `tests/shuffle_parity/hints/`.
**Interfaces:** Existing `GetSariaPriorityItemCandidates`, `FindMultiItemPlacement`, saved/spoiler/config lists, and native Desired Items candidates.
- [x] Reproduce missing Progressive Magic and RPG upgrade candidates in the actual menu-export paths, not only the detached Rando helper.
- [x] Implement shared candidate/identity resolution across menus, Saria, desired hints, persistence and current ownership.
- [x] Compile actual relevant functions under original/Combo guards; cover native/fractional magic, RPG tiers, traps, duplicates, unreachable/obtained/excluded checks and save/config round trips.
- [x] Commit and publish a bounded verified checkpoint.

### Task 3: Souls and shared souls
**Files:** New focused `combo/rando/SoulShuffle*` catalogs; native OoT/MM options, items, save/grant/actor/logic/hint adapters; `tests/shuffle_parity/souls/`.
**Interfaces:** Catalog actor-role mapping; existing native RandoInf ownership and item receipt adapters; `CanKillEnemy` and corresponding OoT logic.
- [x] Establish failing native pool, receipt and actor/logic tests for ordinary OoT enemy souls and NPC/animal/misc families in both games.
- [x] Append stable IDs, default-off options, real item presentation/grants/saves, parameter-specific native gating, all relevant solver dependencies and exclusions.
- [x] Implement shared-soul deduplication/receipt against actor-role counterparts rather than numeric donor IDs; retain all existing MM souls.
- [x] Verify original/Combo native builds, combinations of enabled families, self-dependency prevention, room clear, custom viewer exclusions, schedules, shops and cycle persistence.
- [x] Commit independent family milestones; review before integrating the next dependency.

### Task 4: Native OoT enemy first-drop checks
**Files:** Native OoT first-drop runtime/check/pool/logic modules; catalog and exclusion adapters; `tests/shuffle_parity/enemy_drops/`.
**Interfaces:** Soul-aware OoT enemy-kill reachability from Task 3 and native check ownership.
- [x] Write failing native normal-death/special-death and one-check-per-enemy-type tests.
- [x] Add real checks, correct scene/age routes and exclusion groups; distinguish unloads from death, make duplicate callbacks idempotent, preserve existing MM behavior.
- [x] Run actual native callbacks and Vanilla seeded fills, including soul self-lock and disabled-option controls; commit.
- [x] Recover/validate genuine original and MQ archives privately and run complete native MQ/mixed first-drop generation: all12 Vanilla/MQ/mixed cases pass exact R20 modules, including660 own-soul negatives; layout fixtures are separate.

### Task 5: MM capacities, infinite upgrades and UI
**Files:** MM option/item/pool/grant/remove/save/logic adapters, `Rando/Menu.cpp`, shared wallet policy; `tests/shuffle_parity/capacities/`.
**Interfaces:** Native upgrade bitfields/capacity tables and actual usability predicates.
- [x] Write failing tests for each placeholder's pool, receipt, capacities, starting inventory, save round trip and Glitchless item-use gates.
- [x] Implement Deku Stick/Nut Bags, Child Wallet and Infinite Upgrades with native capacities and matching solver thresholds; preserve default capacities when disabled.
- [x] Replace placeholders with real options; organize related settings into capacities, songs, souls, time and progression groups without changing old cvar keys.
- [x] Verify zero-capacity/ammo denial, wallet affordability, capped/infinite tiers, shared-wallet combinations and all defaults; commit.

### Task 6: Native MM dungeon key rings
**Files:** MM catalogs, pool/grant/logic/receipt/save adapters and cross-game mappings; `tests/shuffle_parity/key_rings/`.
**Interfaces:** Native per-dungeon small-key inventory and restricted fill.
- [x] Failing tests for selected-dungeon grouping and receipt, separate from foreign OoT key rings.
- [x] Add one selected ring per MM dungeon, remove replaced small keys, implement full key ownership and receipt/spoiler/hint/cross-game mapping.
- [x] Verify native key totals, Own Dungeon/Anywhere/Start With, exclusions and save/reset policy; commit.

### Task 7: Rusty door keys
**Files:** Focused door catalog/native door adapters in both games; item/pool/save/solver mappings; `tests/shuffle_parity/door_keys/`.
**Interfaces:** Physical entrance identity retained separately from shuffled destination; native door usability.
- [x] Failing cases for each door lock with none/own/other key and shuffled entrances.
- [x] Add defined door-key items, native door interaction and matching logical exits; preserve disabled behavior and decorative door variants.
- [x] Verify each door route, no self-locks, save ownership and both games/ages; commit.

### Task 8: Song-event shuffle
**Files:** Shared seeded event-song catalog and native song-event runtime/logic adapters; settings/spoilers/plando; `tests/shuffle_parity/song_events/`.
**Interfaces:** A saved per-seed assignment read by both event activation and reachability; preserve native song-item shuffle.
- [x] Failing tests prove runtime and solver consult identical assignments and disabled defaults.
- [x] Port supported upstream event mappings with explicit form/age/song availability and actual native event hooks.
- [x] Verify deterministic seeds, settings/import, valid playable songs, clocks and form instruments, no disabled tricks; commit.

### Task 9: MM environmental checks and exclusions
**Files:** MM object/check catalogs, identification and native drop hooks, pool/region logic/menu/exclusions; `tests/shuffle_parity/environment/`.
**Interfaces:** Stable actor-placement identity; native check ownership; one source of category/location labels.
- [x] Failing real actor tests for rock, boulder/red-boulder, icicle and soil interactions.
- [x] Add precise identities, item spawns, correct item/form/scene access and persisted exclusions; avoid duplicate rewards across respawns/resets.
- [x] Run native actor tests for each family and seeded fills with all families, exclusions and default-off; commit.

### Task 10: Lottery nights and OoT clock
**Files:** MM `EnKujiya.cpp` and region/check/option/spoiler/exclusion paths; focused OoT clock runtime/item/solver adapters; `tests/shuffle_parity/time/`.
**Interfaces:** Existing lottery check retained for night one; appended night-two/three IDs; seed-owned clock mode and ownership.
- [x] Failing tests for independent lottery checks/rewards/messages and old single-check behavior when disabled.
- [x] Implement separate nights with actual time reachability/exclusions and one reward each.
- [x] Implement default-off OoT day/night clock item with deterministic configured start lock, grant/save and corresponding time logic.
- [x] Verify seed determinism, both clock start modes, scene reloads, songs, owl/warp travel and MM cycles; commit.

### Task 11: Integrate native form-instrument source
**Files:** Exact external source candidate's own manifest, with bounded native audio/player/resource adapters and its regression registration.
**Interfaces:** Verify actual commit/tree and dependencies before applying; MM banks 92/93/94 for Goron/Zora/Deku; Fierce Deity remains ocarina.
- [x] Retrieve exact recovery patch/commit and independently reproduce its tree/delta.
- [x] Integrate while preserving cumulative voice/face/eye changes and original MM instrument ownership.
- [x] Run provided real controller/native object regressions and interaction with song-event shuffle; commit provenance.

### Task 12: Integrated verification and publication
**Files:** Existing CI manifest/routing, source recovery docs/packages, feature-specific native fixtures.
- [x] Register all meaningful regressions and run affected complete command groups plus CI policy tests.
- [x] Run seeded combined and solo Glitchless fills with restrictions, exclusions and all newly enabled gates; check circular dependencies and native hint controls. Configured rendered acceptance remains separate.
- [ ] Compile affected production units/full targets where possible; publish WIP versus verified status honestly.
- [x] Obtain an independent review, correct material findings with failing regressions, and publish the dedicated draft PR/checkpoints with exact source trees.
- [ ] Deliver durable recovery artifacts and a concise remaining runtime test record; never merge/promote master.
