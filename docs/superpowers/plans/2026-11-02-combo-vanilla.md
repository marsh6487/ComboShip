# Combo Vanilla Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the approved paired Vanilla + NEI mode, fixed rewards in both worlds and native story progression.

**Architecture:** The combined container owns immutable mode/version and permanent reward checks. Native vanilla save initialization stays separate from randomizer creation. Each game queries mode before loading native state, filters sharing to approved NEI fields, and binds a common reward manifest to native scene/event/grant APIs.

**Tech Stack:** C/C++, nlohmann JSON, existing GameInteractor/ActorDB/get-item and MM CustomMessage services, Python native fixtures, clang-format 14.0.6.

**Spec:** `docs/superpowers/specs/2026-11-02-combo-vanilla-nei-design.md`, approved by cor November 2; dialogue icon repairs are independently checkpointed at remote `e8692bf274882ce8a731538367178a4038bdec83`.

## Global Constraints

- Fixed placements; preserve native major rewards, first-cycle story, game/solo guards and accepted archives.
- New files start in OoT. Store `combo.mode = "vanilla-nei"` and `combo.vanilla.version = 1`; existing seeded containers retain their behavior.
- Do not infer vanilla from a missing seed. Do not import stale randomizer settings or fabricate a seed.
- Shared NEI only, with the spec's explicit foreign aliases and base-sword prerequisites. Quarantine incoming MM NEI until `threeDayResetCount > 0`.
- Full-width item identities and explicit cane/rune grants; no generic progressive copies to represent a fixed skill.
- Work on the isolated feature candidate; publish only real deltas on a complete upstream tree, not recovery-only source imports.

## Review Focus

- Slot metadata reads during file-select previews must not change the active file's mode.
- Failed paired creation must not leave a playable half-pair or overwrite the previous disk container.
- First MM entry must preserve native opening/first-cycle state rather than force Human Link in Clock Town.
- Save/reload/copy/reset must retain collected checks and avoid progressive duplicate grants or double shop charges.
- Authored scenes can differ in geometry; validate each supported placement variant and leave untested variants explicit.

### Task 1: Per-file mode and atomic paired creation

**Files:** Create `combo/vanilla/ComboVanillaSave.h`, `combo/vanilla/ComboVanillaMode.h`, `tests/combo_vanilla/run_save_tests.py`; modify `combo/ComboShip.cpp`, `soh/src/code/z_sram.c`, `soh/soh/SaveManager.cpp`, `soh/soh/OTRGlobals.cpp`, `mm/2s2h/SaveManager/SaveManager.cpp`, `mm/2s2h/BenPort.cpp`.

**Interfaces:** `ComboVanilla::FileMode(const json&)` returns explicit vanilla, legacy randomizer, or invalid. `Combo_QueryVanillaMode(int slot)` is a per-game C shim backed by a launcher-registered query; `gComboVanillaMode` is hidden per DLL. `MM_InitVanillaSaveFile(int slot, const unsigned char* name)` initializes a genuine native save without rando startup grants. A host creation callback begins/commits/aborts a buffered container transaction; only a complete validated pair is flushed.

- [x] Write/run failing native tests for explicit mode/version, legacy seeded saves, malformed/mixed metadata, paired creation success/abort and interrupted writes. Use actual JSON with a filesystem boundary.
- [x] Implement the neutral mode/transaction helpers, wire native mode queries before save-load hooks, and add the native MM creator/export. Preserve randomizer initializer unchanged.
- [x] Wire native OoT creation to choose normal quest for the new-file Vanilla choice and buffer writes until MM creation succeeds. Surface failure to both native name-entry paths without a successful-file UI state.
- [x] Test the extracted native initialization/callback paths, valid slot/name transfer, failure rollback and seeded/solo negative controls. Commit the verified milestone.

### Task 2: Creation UI and MM opening

**Files:** Modify `combo/gui/ComboMenu.cpp`, `soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c`, `soh/src/overlays/gamestates/ovl_file_choose/z_file_nameset_PAL.c`, `soh/src/overlays/gamestates/ovl_file_choose/z_file_nameset_NES.c`, `mm/src/code/title_setup.c`, `mm/2s2h/FleetShipCombo/FleetWarpArrival.cpp`; extend `tests/combo_vanilla/run_save_tests.py`.

**Interfaces:** The creation-only CVar `gCombo.NewFileVanilla` selects Vanilla + NEI versus Randomizer; subsequent play consults the persisted slot identity. Native MM entry uses the saved opening/first-cycle state while the first reset count is zero.

- [ ] Write/run failing tests showing Vanilla Start requires no seed, Randomizer Start still does, previewing another slot does not switch active mode, and pending creation failure stays in name entry.
- [ ] Add explicit creation mode choice and correct native start label/disabled state; randomizer generation/settings stay available for Randomizer mode.
- [ ] Preserve native MM opening entrance/cutscene/form on first visit; prevent portal-destination overrides and incoming NEI from bypassing first-cycle state. On later entries retain ordinary Combo travel/resume.
- [ ] Run source/native tests for first entry, Deku first cycle, first reset, later portal/resume and old seeded files. Commit.

### Task 3: NEI sharing policy and reward persistence

**Files:** Create `combo/vanilla/ComboVanillaSharing.h`, `combo/vanilla/ComboVanillaRewards.h`, `tests/combo_vanilla/run_reward_tests.py`; modify both `FleetSync.cpp` files and the launcher container callbacks.

**Interfaces:** The sharing filter consumes native-independent receiver capabilities and returns an allowlisted JSON payload. Stable check IDs and explicit reward payloads distinguish normal item, cane skill, rune, season and upgrade. Reward transactions retain pending receipts and commit native save plus permanent check together.

- [ ] Write/run failing native tests for native inventory/story/capacity isolation, all NEI flag families, MM quarantine, imported masks/Ikana/Pendant aliases, and sword prerequisites.
- [ ] Filter both normal and transient sharing paths; prevent FC generic deficits from reintroducing native progression or randomly selecting specific skill rewards.
- [ ] Implement per-slot versioned checks and pending/acknowledged reward transactions; test interruption, re-entry, copy, day reset, idempotent recovery and shop debit exactly once.
- [ ] Audit existing Mirror, Feather, songs, Grace and seasons before adding duplicates; record exact identities. Commit.

### Task 4: Fixed native rewards across both games

**Files:** Create `combo/vanilla/ComboVanillaPlacements.inc`, `soh/soh/Enhancements/Gameplay/ComboVanillaRewards.cpp`, `mm/2s2h/Enhancements/Gameplay/ComboVanillaRewards.cpp`; extend reward tests; modify existing shop/bonus hooks only where required by the approved manifest.

**Interfaces:** Each manifest row names stable check ID, world/scene/room/setup, native prerequisite, explicit grant payload, reward kind and safe placement variant. Per-game adapters offer the reward through existing GI/receipt machinery and report ownership acknowledgment to Task 3.

- [ ] Write/run failing tests for every manifest row's explicit payload, prerequisite and source uniqueness; name every protected reward and verify no substitution of it.
- [ ] Bind early tools and retained Mirror/Feather first, then implement each region's approved rods, skills, runes, seasons, equipment and optional upgrades. Use native capabilities and safe return paths; avoid heavy new actors or archive edits.
- [ ] Use actual minor-check IDs only after verifying their native reward; otherwise add a nearby optional pickup. Shop prices are the approved 40/60/80/90 values, bounded by native wallets.
- [ ] Verify explicit grants survive reload and cross-game transfer, and that the item never gates its own source. Record supported stock/authored positions and untested geometry variants.
- [ ] Run all affected native gates, register new gates without deleting existing ones, format with clang-format 14.0.6 and obtain independent whole-branch review. Commit and upload the complete source candidate/PR.

### Task 5: Handoff and configured runtime

**Files:** Update `combo/gui/WhatsNew.h` and checkpoint documentation.

- [ ] Add the mode and NEI discoveries to What's New with accurate scope.
- [ ] Verify actual GitHub game-build results separately from native fixtures; correct concrete failures.
- [ ] List the decisive runtime checks: paired file creation, first MM visit/reset, travel both ways, representative early/late rewards, NPC bonus/shop debit, save/reload and stock/authored visibility/collision. Do not label runtime untested behavior accepted or promote an asset master.

## Execution ledger

Authorization: cor approved the design and asked to implement; proceed inline without another permission round. Icons are already checkpointed. Begin Task 1 after unchanged source inputs are recovered; retain bounded checkpoints throughout.

Task 1 source/native milestone: explicit mode and transaction helpers, actual launcher/MM creator, actual OoT SRAM/name-entry success/failure branches pass local ASan/UBSan fixtures. Native ROM initialization, game serialization and game services are fixture boundaries. Full engine linking and configured runtime are pending. Creation UI, opening/first cycle, NEI sharing/rewards and final review remain incomplete.

Task 1 review corrections: explicitly invalidate native MM (including fileNum=0xFF, save type, owl residency and per-file mode) on rejected identity and aborted/throwing creation; read-only mode peeks support the worker serializer. Remove stale seed/speedrun sections from combined Vanilla writes while preserving unknown mod sections. New native extraction tests reproduced the defects and pass after repair.

Task 1 correction review: independent reviewer verified all three findings resolved, reran host creation and serializer ASan/UBSan suites with leak detection disabled for this host, and found no consequential new issue. This closes the source/native foundation milestone; feature remains WIP through Tasks 2–5.
