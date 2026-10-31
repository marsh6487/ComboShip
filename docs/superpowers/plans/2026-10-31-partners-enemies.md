# Native Partners and Enemies Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** Add native Malon/Cojiro companions, direct control, four custom enemies,
enemy replacement and opt-in natural encounters throughout OoT and MM, with
What's New accents for every integration.

**Architecture:** Adapt donor behavior to native host hooks and actors; no full
external mod loader. Share engine-independent policy and placement identities,
but use explicit per-game actor, input, camera, save and collision adapters.
Land recoverable, independently checked milestones.

**Tech Stack:** C/C++, native ActorDB/actor tables, GameInteractor, ImGui menus,
existing NEI save state and deterministic o2r packaging.

**Spec:** ../specs/2026-10-31-partners-enemies-design.md

## Global Constraints

- Parent ba37349b82ee855c2f7179cd00b9b5871891f418, tree f7e611d2b96b2126842583e2185484c36f6bcac6.
- candidate/partners-enemies-20261031; preserve accepted masters and cumulative fixes.
- Both games, Solo and Combo; no dormant-world actor/input updates.
- Cucui Ganon excluded. Companion and natural encounter toggles default off.
- Preserve native quests, randomized rewards, scene placements and load order.
- Preserve baked animations and provenance; no runtime retargeting.
- Every integration gets existing What's New control accents in both menus.
- Save durable source/asset recovery before broad changes and long builds.
- Runtime, build verification and user acceptance are separate statuses.

## Review Focus

- Cross-game switches during direct control: release camera/input and all transient actor references.
- Missing assets or incompatible replacement rigs: graceful unavailability without shared-resource mutation.
- Existing Grog check already complete: grant Cojiro eligibility without duplicating ordinary rewards.
- Natural encounters and enemy replacement both enabled: preserve additive actors and prevent duplicate spawns.
- MM resets, room changes and OoT age/setup changes: clear actor ownership and apply defined respawn rules.

## File structure

- `combo/partners/PartnerPolicy.h`: engine-independent ownership/control-release policy.
- `soh/mods/partners/`, `mm/mods/partners/`: separate native Malon/Cojiro actors,
  Command Melody and engine hooks; adapter headers keep game structs separate.
- `combo/encounters/EncounterPolicy.h`, `NaturalEncounterPlacements.h`: stable
  encounter identities, game/scene/room/setup filters, counts and visit state.
- `soh/mods/encounters/`, `mm/mods/encounters/`: ground/clearance validation,
  owned spawning/removal, actor liveness and active-world checks.
- Existing `soh/mods/actors/trutefel/` and `mm/mods/trutefel/`: reuse/audit three enemies.
- `soh/mods/actors/lynel/`, `mm/mods/lynel/`: native Lynel implementations/assets.
- Existing `soh/soh/Enhancements/ExtraModes/EnemyRandomizer.cpp`: OoT integration.
- `mm/2s2h/Enhancements/Gameplay/CustomEnemyRandomizer.cpp`: MM adapter.
- Existing native menu sources, `combo/gui/WhatsNew.h`, NEI save sources and
  game CMake inputs: connect controls, persistence, accents and build inclusion.
- `tests/partners/`, `tests/custom_encounters/`: policy and real native adapter fixtures.
- `scripts/mods/package_partners_enemies.py`: deterministic required asset packaging.

## Task 1: Restore baseline and verify existing enemies

- [x] Read both spec and parent AGENTS.md; preserve a restorable source checkpoint
  and record exact donor files, licenses and archive hashes before changing code.
- [x] Audit both games' Miniblin, Molmauk/Hammergeist and Scissors Beetle build
  inclusion, ID registration and missing-resource behavior; reuse valid host code.
- [x] Add native tests in `tests/custom_encounters/test_registration.py` for
  each actor's actual registered callbacks/ID and absent resource behavior.
  Run before changes to establish any missing behavior; repair only failures.
- [x] Compile affected native actor/registration translation units with the
  project's available toolchain. Record unavailable full-build prerequisites.
- [x] Commit bounded fixes and a recovery record with exact asset requirements.

## Task 2: Partner ownership and direct control

Produces `PartnerPolicy::CanBegin`, `PartnerPolicy::MustRelease` and per-game
native adapters consuming current game, owned song, enabled partner and transient
actor/camera/input state. Policy functions accept plain values, never engine pointers.

- [ ] Write failing policy/native tests for default-off behavior, old saves,
  completed Grog eligibility, ordinary reward preservation, scene reset and
  cross-game release. Run `python3 -m unittest discover -s tests/partners -v`.
- [ ] Add versioned NEI ownership/toggle state with old-save defaults. Reuse
  current Command Melody ownership rather than creating a duplicate song.
  Preserve current acquisition rules; do not silently grant the song at start.
- [ ] Adapt command_melody donor control to native ocarina/input/camera hooks.
  Targeted partner wins, otherwise nearest eligible partner; implement release
  on transition, loss, Link damage, ground loss, range, disable and game switch.
- [ ] Implement per-game Cojiro actors, friendly native cucco animations, blue
  local textures, follow/hops/recall, carry/throw/slow fall and three-hit recovery.
- [ ] Implement Malon native actors and required model/animation resources,
  follow/recall, bow targeting, carrying/throwing and Epona seat ownership.
  OoT child/adult selection and MM model binding are separate validated paths.
- [ ] Add companion/direct-control menu controls and their matching What's New
  entries together. Preserve the global accent toggle and native widget identity.
- [ ] Run partner tests, affected native compilation and save/input regressions.
  Commit OoT and MM milestones separately, with untested runtime paths recorded.

## Task 3: Lynel and enemy randomizer parity

Produces per-game ID/callback access for all four enemies; separate selection
CVars in each engine. Additive encounter actor identities are excluded at the
replacement boundary defined by Task 4, not by disabling ordinary custom enemies.

- [ ] Add failing native tests for custom selection, missing assets, zero eligible
  choices and clear/timed-room exclusions. In particular Lynel/Molmauk must not
  enter timed rooms and unsupported spawn surfaces must fail safely.
- [ ] Adapt Lynel's actor and assets to OoT, then MM, preserving combat, projectiles,
  headshot stun, mount input, death and cleanup. Retain provenance and stable keys.
- [ ] Extend OoT's existing replacement list and native menu for four enemies.
  Implement MM's replacement adapter with deterministic selection and explicit
  supported actor/room filtering; do not rewrite unrelated item randomizer logic.
- [ ] Add per-game enemy selections and randomizer control What's New entries.
- [ ] Run `python3 -m unittest discover -s tests/custom_encounters -v`, native
  actor/randomizer fixture compilation and relevant existing enemy/collision checks.
- [ ] Commit each game's integration with recovery and known runtime gaps.

## Task 4: Natural world encounters in both games

Produces stable `EncounterId` placement entries and per-game
`IsNaturalEncounterActor(const Actor*)` ownership queries. The latter validates
actor membership/lifetime; never trust stale raw pointer equality alone.

- [ ] Write failing policy/native tests for option off, natural only, randomizer
  only and both; duplicate prevention, defeat in current visit, removal of owned
  actors only, missing floor/clearance and room/game/reset changes.
- [ ] Build a placement inventory from actual scene collision and authored
  additions, recording scene/room, coordinates, allowed age/setup and reason per
  entry in `NaturalEncounterPlacements.h`. Use sparse outdoor habitat placements
  across Hyrule and Termina; avoid hubs, entrances, bridges and progression rooms.
- [ ] Implement floor and clearance validation against the loaded scene before
  spawning. Skip incompatible positions and report bounded diagnostics.
  Spawn once per identity/visit; keep defeat state through room reloads, reset
  on scene re-entry, and clear state explicitly on MM cycle or OoT age changes.
- [ ] Add separate default-off Natural World Encounters controls to both menus.
  Make live disable remove only actors owned by this feature. Exclude those
  actors from both replacement adapters without altering ordinary placed enemies.
- [ ] Add natural encounter What's New entries and run policy/native tests,
  compile affected adapters and inspect the placement table for both games.
- [ ] Commit source and placement inventory; runtime status remains explicit.

## Task 5: Packaging, cumulative checks and handoff

- [ ] Package required models, animations and textures deterministically with
  donor/license records; verify required resource paths and no shared vanilla
  resource overwrites. Record archive/order/Alt requirements.
- [ ] Test What's New key coverage for each delivered control, gold/cyan kind,
  no duplicate keys and disabled-accent behavior. Inspect native menu rendering
  when runtime is available; do not count static coverage as visual proof.
- [ ] Run targeted existing save, input, native actor and Combo/Solo guard
  regressions plus the new suites. Compile supported game/module targets.
- [ ] Record per-game runtime matrix for companion follow/control/carry/bow/
  Epona, enemy combat/death/mount and every natural placement. Test room changes,
  save/reload, game swaps, Song of Time, age, Alt Assets and performance A/B.
  Mark unavailable checks untested; do not promote or claim runtime success.
- [ ] Save and independently restore the final source/asset recovery artifact.
  Deliver candidate commits, required packs, controls, placement inventory,
  actual test results and known gaps. No public release/protected merge.

## Candidate implementation disposition — 2026-10-31

Implementation for Tasks 2–5 is present in candidate source; detailed mapping, candidate region
inventory, resource identities, verification commands and untested runtime matrix are in
`docs/donors/partners/INTEGRATION.md`. The original boxes above remain the original
acceptance checklist; they must not be read as gameplay acceptance.

The actual native actor/control sources live under each game's
`src/overlays/actors/ovl_NeiPartners` and `ovl_NeiLynel`; host glue and encounter adapters
are under `mods/partners`. Engine-independent encounter policy and the native shared adapter
are under `combo/encounters`. Curated scene masks and original native scene-entry anchors
replace invented fixed coordinates (recorded ruling in the execution ledger). Actor destruction
clears owned identities before free. Original native source IDs exclude all custom actors,
so a separate IsNaturalEncounterActor exclusion is unnecessary at the replacement boundary.

Completed source evidence: ten new executable tests (six partner, four encounter), strict
native new-actor compilation, Solo/Combo host compilation, existing save compatibility,
ActorDB and 93 OoT equipment age-routing assertions. Existing baseline full z_actor.c
resource-pointer warnings require the fixture's explicit legacy warning mode. Existing
player full-TU prerequisites and time-pedestal fixture limitations are documented.
Independent review found three defects (MM age validation, live command disable/ownership,
MM aiming flag); all were fixed, covered or source-verified, and reviewed again.

Pending acceptance: full platform build/link; every in-game matrix row; final user playtest.
No public release or protected-branch promotion is authorized by this source checkpoint.
