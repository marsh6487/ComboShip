# Ammo, pool, and Bowling repairs implementation plan

> **For agentic workers:** Use the systematic debugging, test-driven-development, and verification-before-completion workflows for each owned task. Independent worktrees isolate parallel changes; root integrates and publishes the reviewed candidate.

**Goal:** Repair the confirmed ammo, native item-pool, magic progression, Bowling dependency, and pause-count inconsistencies, then publish the verified candidate.

**Architecture:** Keep policy at native ownership/generation boundaries. Share persistent progression deliberately and transfer consumable counts without resurrecting spent ammo. Maintain separate ordinary, solo, combined, and legacy-save controls.

**Tech Stack:** C11/C++20, native SoH/MM types, Python regression runners, GitHub Actions.

**Spec:** `docs/poc/combo-ammo-pool-bowling-20261113.md`.

## Global constraints

- Baseline is `b8068f8c24246c99e6d490f08b33b7e7196ec5d9`; original checkout remains preserved.
- Only root publishes to `candidate/seed-fill-regressions-20261110`; PR #90 remains open and draft.
- cor authorized implementation and publication and opted out of further approval requests.
- No private logs, dumps, saves, configuration, mod packs, or trace packets enter the public repository.
- No unsolicited asset edits, ordinary item restriction bypass, consumable max-merge, or legacy capacity reduction.
- Commit bounded milestones and preserve source/recovery metadata before publication.

## Review focus

- Independent shuffled bags must remain required when not shared; actual consumption must survive switching.
- Legacy native-magic capacity and RPG rules must survive retries, reload, and both-host synchronization.
- Native pool/oracle rules must agree across combined UI, headless generation, and replay; solo modes remain valid.
- Missing Bowling Lady must not cause duplicate target spawns, broken active-round behavior, or soul bypass.
- Pause effective usability changes must not force restricted/zero counters white or remove HUD alpha dimming.

## Task 1: Bowling dependency safety and native Feather supply

Owner: isolated `ComboShip-pool-bowling`; root reviews/integrates.
Files: `soh/src/overlays/actors/ovl_En_Wall_Tubo/z_en_wall_tubo.c`,
`soh/src/overlays/actors/ovl_Bg_Bowl_Wall/z_bg_bowl_wall.c`, Feather blocks in
`mm/2s2h/Rando/Logic/GeneratePools.cpp`, relevant action-pool/native tests.

- [x] Add behavior assertions: repeated missing-host updates remain safe; later
  host discovery resumes; owned/minigame path remains functional; no duplicate
  bullseyes. Observe baseline failures.
- [x] Guard dependency state transitions at native lookup, then pass controls.
- [x] Add native generator controls for combined/solo, gear/NEI, plentiful,
  native Feather supply and separate progressive Roc counts. Observe failure.
- [x] Exclude both combined MM native fallback supplies; retain solo/legacy.
- [x] Commit bounded actor and pool changes with red/green evidence.

## Task 2: Ammo bag/capacity/sync consistency

Owner: root in `ComboShip-fixes`.
Files: `mm/include/rando_capacity.h`, relevant ammo paths in
`mm/src/code/z_parameter.c`, `mm/2s2h/Rando/GiveItem.cpp`,
`mm/2s2h/BenPort.cpp`, both `FleetSync.cpp` files, and native regression tests.

- [x] Pin intended independent Bombchu capacity and bag-sharing semantics from
  actual seed settings/shared item families, using existing probes as red cases.
- [x] Test use, refill, native grant, import, return, missing bag, and ordinary
  solo controls; preserve actual consumption.
- [x] Implement effective local ownership/capacity gates before import and use
  independent Bombchu capacity wherever the combined bag requires it.
- [x] Pass focused tests and compile affected native paths; commit milestone.

## Task 3: Magic RPG pool, oracle, and legacy progression

Owner: isolated `ComboShip-magic`; root reviews/integrates.
Files: magic blocks in MM pool generation/BenPort, combo generation/replay
interfaces and RPG helpers as required, OoT RPG receipt/shared floor paths,
and existing/focused native RPG tests. No unrelated ammo edits.

- [x] Demonstrate native leakage with RPG on, preserve RPG off/solo controls.
- [x] Propagate frozen rule to native pool and all affected generation/replay
  oracle entry points; test actual MM magic requirements with RPG receipts.
- [x] Demonstrate legacy floor masking on a real subsequent pickup; test
  minimum equivalent progression credit, no capacity loss, exactly-once
  increment, default/adjustable caps, sync/reload idempotency.
- [x] Implement agreed compatible progression; preserve ordinary magic and
  saved rules. Avoid replacing tests with a mocked formula.
- [x] Run both existing RPG suites and new controls, then commit milestone.

## Task 4: Pause counter effective restriction

Owner: isolated `ComboShip-hud`; root reviews/integrates.
Files: MM `z_kaleido_item.c`, focused pause draw-command regression.

- [x] Assert native Human white/full green/zero gray and restricted transformed
  gray; assert Unrestricted Items produces allowed nonzero pause digits.
- [x] Observe baseline failure for override; route through the icon's effective
  restriction decision and pass all controls.
- [x] Retain C-button enabled/disabled alpha evidence; no unsupported HUD fix.
- [x] Commit verified pause correction.

## Task 5: Integration, recovery, review, and publication

Owner: root, with independent review of final diff.
Files: CI regression manifest, scoped POC record, and merged owned changes.

- [x] Review/cherry-pick bounded commits; resolve shared-file hunks explicitly.
- [x] Register new regressions without dropping prior entries; run affected
  gates, source formatting, CI handling tests, and relevant build preflight.
- [ ] Save/restore-check source checkpoint and publication manifest; verify
  changed path/mode/blob set excludes all private diagnostic material.
- [x] Complete independent review and address concrete findings.
- [ ] Publish with expected-head protection; verify exact remote tree and draft PR.
- [ ] Observe hosted checks and fix scoped failures; report actual build/test
  status and runtime limits without requesting another authorization.
