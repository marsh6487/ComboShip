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
- Only one delegated implementation worker at a time; explicit file ownership. Independent research may run concurrently. Controller edits separate owned files.

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
- [ ] Capture actual native settings -> restricted placement -> combined fill behavior with a deterministic failing scenario if the reported leak exists; add a negative control if it does not.
- [ ] Correct the smallest actual boundary, preserving Anywhere/Start With/Vanilla semantics.
- [ ] Run seeded Own Dungeon combinations for both games and assert every boss key's physical owning dungeon; include Stone Tower/inverted routing and exclusions.
- [ ] Commit owned files and report exact commands, seeds, coverage and unresolved paths.

### Task 2: MM Saria/Desired Items parity
**Files:** MM `MagicHints.h`, `MiscBehavior/SariasSongHint.cpp`, BenPort Desired Items adapters and native candidate menu as necessary; `tests/shuffle_parity/hints/`.
**Interfaces:** Existing `GetSariaPriorityItemCandidates`, `FindMultiItemPlacement`, saved/spoiler/config lists, and native Desired Items candidates.
- [ ] Reproduce missing Progressive Magic and RPG upgrade candidates in the actual menu-export paths, not only the detached Rando helper.
- [ ] Implement shared candidate/identity resolution across menus, Saria, desired hints, persistence and current ownership.
- [ ] Compile actual relevant functions under original/Combo guards; cover native/fractional magic, RPG tiers, traps, duplicates, unreachable/obtained/excluded checks and save/config round trips.
- [ ] Commit and publish a bounded verified checkpoint.

### Task 3: Souls and shared souls
**Files:** New focused `combo/rando/SoulShuffle*` catalogs; native OoT/MM options, items, save/grant/actor/logic/hint adapters; `tests/shuffle_parity/souls/`.
**Interfaces:** Catalog actor-role mapping; existing native RandoInf ownership and item receipt adapters; `CanKillEnemy` and corresponding OoT logic.
- [ ] Establish failing native pool, receipt and actor/logic tests for ordinary OoT enemy souls and NPC/animal/misc families in both games.
- [ ] Append stable IDs, default-off options, real item presentation/grants/saves, parameter-specific native gating, all relevant solver dependencies and exclusions.
- [ ] Implement shared-soul deduplication/receipt against actor-role counterparts rather than numeric donor IDs; retain all existing MM souls.
- [ ] Verify original/Combo native builds, combinations of enabled families, self-dependency prevention, room clear, custom viewer exclusions, schedules, shops and cycle persistence.
- [ ] Commit independent family milestones; review before integrating the next dependency.

### Task 4: Native OoT enemy first-drop checks
**Files:** Native OoT first-drop runtime/check/pool/logic modules; catalog and exclusion adapters; `tests/shuffle_parity/enemy_drops/`.
**Interfaces:** Soul-aware OoT enemy-kill reachability from Task 3 and native check ownership.
- [ ] Write failing native normal-death/special-death and one-check-per-enemy-type tests.
- [ ] Add real checks, correct scene/age routes and exclusion groups; distinguish unloads from death, make duplicate callbacks idempotent, preserve existing MM behavior.
- [ ] Run actual native callbacks and seeded fills, including soul self-lock and disabled-option controls; commit.

### Task 5: MM capacities, infinite upgrades and UI
**Files:** MM option/item/pool/grant/remove/save/logic adapters, `Rando/Menu.cpp`, shared wallet policy; `tests/shuffle_parity/capacities/`.
**Interfaces:** Native upgrade bitfields/capacity tables and actual usability predicates.
- [ ] Write failing tests for each placeholder's pool, receipt, capacities, starting inventory, save round trip and Glitchless item-use gates.
- [ ] Implement Deku Stick/Nut Bags, Child Wallet and Infinite Upgrades with native capacities and matching solver thresholds; preserve default capacities when disabled.
- [ ] Replace placeholders with real options; organize related settings into capacities, songs, souls, time and progression groups without changing old cvar keys.
- [ ] Verify zero-capacity/ammo denial, wallet affordability, capped/infinite tiers, shared-wallet combinations and all defaults; commit.

### Task 6: Native MM dungeon key rings
**Files:** MM catalogs, pool/grant/logic/receipt/save adapters and cross-game mappings; `tests/shuffle_parity/key_rings/`.
**Interfaces:** Native per-dungeon small-key inventory and restricted fill.
- [ ] Failing tests for selected-dungeon grouping and receipt, separate from foreign OoT key rings.
- [ ] Add one selected ring per MM dungeon, remove replaced small keys, implement full key ownership and receipt/spoiler/hint/cross-game mapping.
- [ ] Verify native key totals, Own Dungeon/Anywhere/Start With, exclusions and save/reset policy; commit.

### Task 7: Rusty door keys
**Files:** Focused door catalog/native door adapters in both games; item/pool/save/solver mappings; `tests/shuffle_parity/door_keys/`.
**Interfaces:** Physical entrance identity retained separately from shuffled destination; native door usability.
- [ ] Failing cases for each door lock with none/own/other key and shuffled entrances.
- [ ] Add defined door-key items, native door interaction and matching logical exits; preserve disabled behavior and decorative door variants.
- [ ] Verify each door route, no self-locks, save ownership and both games/ages; commit.

### Task 8: Song-event shuffle
**Files:** Shared seeded event-song catalog and native song-event runtime/logic adapters; settings/spoilers/plando; `tests/shuffle_parity/song_events/`.
**Interfaces:** A saved per-seed assignment read by both event activation and reachability; preserve native song-item shuffle.
- [ ] Failing tests prove runtime and solver consult identical assignments and disabled defaults.
- [ ] Port supported upstream event mappings with explicit form/age/song availability and actual native event hooks.
- [ ] Verify deterministic seeds, settings/import, valid playable songs, clocks and form instruments, no disabled tricks; commit.

### Task 9: MM environmental checks and exclusions
**Files:** MM object/check catalogs, identification and native drop hooks, pool/region logic/menu/exclusions; `tests/shuffle_parity/environment/`.
**Interfaces:** Stable actor-placement identity; native check ownership; one source of category/location labels.
- [ ] Failing real actor tests for rock, boulder/red-boulder, icicle and soil interactions.
- [ ] Add precise identities, item spawns, correct item/form/scene access and persisted exclusions; avoid duplicate rewards across respawns/resets.
- [ ] Run native actor tests and seeded fills with each family, all families, exclusions and default-off; commit.

### Task 10: Lottery nights and OoT clock
**Files:** MM `EnKujiya.cpp` and region/check/option/spoiler/exclusion paths; focused OoT clock runtime/item/solver adapters; `tests/shuffle_parity/time/`.
**Interfaces:** Existing lottery check retained for night one; appended night-two/three IDs; seed-owned clock mode and ownership.
- [ ] Failing tests for independent lottery checks/rewards/messages and old single-check behavior when disabled.
- [ ] Implement separate nights with actual time reachability/exclusions and one reward each.
- [ ] Implement default-off OoT day/night clock item with deterministic configured start lock, grant/save and corresponding time logic.
- [ ] Verify seed determinism, both clock start modes, scene reloads, songs, owl/warp travel and MM cycles; commit.

### Task 11: Integrate native form-instrument source
**Files:** Exact external source candidate's own manifest, with bounded native audio/player/resource adapters and its regression registration.
**Interfaces:** Verify actual commit/tree and dependencies before applying; MM banks 92/93/94 for Goron/Zora/Deku; Fierce Deity remains ocarina.
- [ ] Retrieve exact recovery patch/commit and independently reproduce its tree/delta.
- [ ] Integrate while preserving cumulative voice/face/eye changes and original MM instrument ownership.
- [ ] Run provided real controller/native object regressions and interaction with song-event shuffle; commit provenance.

### Task 12: Integrated verification and publication
**Files:** Existing CI manifest/routing, source recovery docs/packages, feature-specific native fixtures.
- [ ] Register all meaningful regressions and run affected complete command groups plus CI policy tests.
- [ ] Run seeded combined and solo Glitchless fills with restrictions, exclusions and all newly enabled gates; check circular dependencies and accurate hints.
- [ ] Compile affected production units/full targets where possible; publish WIP versus verified status honestly.
- [ ] Obtain an independent review, correct material findings with failing regressions, and publish the dedicated draft PR/checkpoints with exact source trees.
- [ ] Deliver durable recovery artifacts and a concise remaining runtime test record; never merge/promote master.
