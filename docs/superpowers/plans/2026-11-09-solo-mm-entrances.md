# Solo MM Entrance Shuffle Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Make saved, logic-aware entrance shuffle available in Solo MM.

**Architecture:** Reuse the MM portal catalogue, add 40 native overworld pairs,
and build dedicated Solo coupled/decoupled and selectively mixed routes.
Separate solo state from the combined coordinator and install it at native
generation and selected-save load boundaries.

**Tech Stack:** C/C++20, native 2Ship randomizer, nlohmann JSON, Python regression drivers.

**Spec:** `docs/superpowers/specs/2026-11-09-solo-mm-entrances-design.md`

## Global Constraints

- Baseline R22 `191794e8c7534c30a55ee4ab4554e49356c6ca75` remains recoverable.
- Existing combined and Solo OoT paths remain intact; all solo endpoints are MM.
- Options default off and append existing IDs; use saved settings during play.
- Preserve the native ten-attempt fill budget and real final reachability replay.
- Preserve concurrent candidates and exclude animated previews/private inputs.

## Review Focus

- File preview/copy/erase must not replace the active selected save's mapping.
- Solver inventory reset and headless fills must retain the correct layout.
- Grotto aliases and paired returns must take exactly one composed route.
- Boss death/blue warp/cycle state must follow the saved return policy and stay local.
- Boss completion follows physical identity for every boss when actively mixed
  with another category, and the source boss door in ordinary boss shuffle.
- Seasonal aliases must preserve normal routing without capturing boss warps,
  owl landings, save starts or authored private-scene transitions.
- Malformed or foreign imported endpoints must fail without stale active state.

## Task 1: Native generation and logical routes

**Owner:** generator agent. **Files:** new `mm/2s2h/Rando/SoloEntrances.*`,
`Logic/SeedGeneration.cpp`, `Logic/Logic.cpp`, `Spoiler/Apply.cpp`,
`Spoiler/Generate.cpp`, `combo/rando/CrossDungeonMm.inc.h`,
`combo/rando/CrossEntranceMmBoss.inc.h`, relevant regression fixtures.

**Interfaces:** Implements the spec's `Seed`, `ApplySeed`, `Clear` codec and
explicit solo graph installation/resolution. Consumes appended settings and
the native `soloEntrances` POD. Produces validated local maps for runtime/hints.

- [x] Add and run failing production-behavior tests for MM-only generation,
  deterministic layout, scoped pool bijection, malformed import and legacy clear.
- [x] Implement canonical validation, native fill/retry installation, graph
  route dispatch and source boss completion without foreign callbacks.
- [x] Verify off/vanilla/combined paths, inventory resets and final native replay.
- [x] Commit only owned paths with named evidence and remaining limitations.

## Task 2: Gameplay routing and selected-save persistence

**Owner:** runtime agent. **Files:** native save definitions/conversions,
`combo/rando/CrossDungeonMmRuntime.inc.h`, targeted `BenPort.cpp`,
SaveManager/file-select integration, runtime regression fixtures.

**Interfaces:** Owns POD storage and native serialization; implements
`MM_SetSoloEntranceRuntimeSeed(const char*)` and
`Combo_MM_SoloEntranceOnSaveLoad`. Consumes the shared validated solo codec.

- [x] Reproduce baseline solo routing rejection with an actual adapter fixture.
- [x] Add local-only runtime state and native selected-save restore, preserving
  combined-only foreign queues and avoiding preview-load global side effects.
- [x] Exercise entry/return, grotto materialization, death/void/reload, boss
  completion and no-layout/invalid-layout resets under production boundaries.
- [x] Commit only owned paths with evidence.

## Task 3: Settings and saved-map hints

**Owner:** UI/hints agent. **Files:** native MM options/types/menu, What's New,
map/compass receipt information and its production-boundary tests.

**Interfaces:** Produces appended pool, decoupling and selective mixing settings;
consumes `Rando::SoloEntrances::Seed` for saved entrance and boss mapping.

- [x] Verify native controls persist to generated seeds; all default off.
- [x] Add descriptions/accents, category dependencies and Vanilla disabling.
- [x] Test inverse entrance hints and boss/reward lookup against a known solo
  mapping, including normal/inverted Stone Tower and an unchanged combo map.
- [x] Commit only owned paths with evidence.

## Task 4: Integration, build and recoverable handoff

**Owner:** controller. **Files:** verification tools/CI registry as necessary,
`combo/rando/SoloEntranceMmOverworld.h`, POC report and source recovery artifact.

- [x] Derive and independently verify all 40 paired outdoor connections against
  native graph directions/regions, shared-pool ownership and seasonal aliases.

- [x] Run existing focused solo/cross-entrance baseline tests and retain failures.
- [x] Build changed native MM code and link the actual game module using the
  configured local SDK; run native deterministic fills/replays for pool profiles.
- [x] Run targeted regression gates for entrances, solo modes, hints and saves.
- [x] Request independent whole-change review and resolve material findings.
- [x] Verify the exact diff, checkpoint it, and save a source recovery package
  that independently restores from its named baseline.
- [x] Integrate on the current candidate without overwriting concurrent work;
  report publication and in-game evidence boundaries accurately.
- [x] Close the pinned CI formatting gate and independently verify formatter-only
  token/preprocessor equivalence to the source used for the native matrix.

## Final evidence and remaining external step

The complete nine-suite entrance gate, existing cross/solo/receipt gates, Linux native targets, and all 14 repeated native fill/replay profiles passed. See `docs/poc/solo-mm-entrances-verification-20261110.md`. The native retry RNG carryover found by the matrix is fixed and independently reviewed.

- [ ] Publish the reviewed cumulative source to the existing draft PR #89. Cor explicitly authorized publication, but automatic approval review rejected the resumed upload because it requires authorization naming the source payload and public `marsh6487/ComboShip` destination. No branch ref or PR body was updated.
- Configured in-game traversal/user acceptance and a current Windows build remain distinct from the completed source/Linux native evidence.
