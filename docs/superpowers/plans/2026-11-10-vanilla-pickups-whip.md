# Vanilla Pickups and Whip Implementation Plan

> **For agentic workers:** Use test-driven implementation, independent file ownership, and a fresh final review. Keep progress in the adjacent checkpoint record.

**Goal:** Correct the first OoT optional-reward dialogue and grant flow, restore Whip targeting feedback, and extend actual lash reach.

**Architecture:** Preserve the two native reward adapters and their durable host receipts. Correct dialogue lifecycle ownership at its existing boundary; verify using production native functions. Keep Whip state machines and grip rules, with consistent native aiming feedback and extension limits.

**Tech Stack:** C/C++, Python native fixture runners, existing ComboShip native structs and engine code.

**Spec:** `docs/superpowers/specs/2026-11-10-vanilla-pickups-whip.md`

## Global Constraints

- Base: `a5b1abd697d284ecbca70c13091562821dfc3feb`; preserve its cumulative tree.
- Later user steering authorizes moving only the Rod to Lost Woods room 0's center. Do not change other locations, prices, prerequisites, stable check IDs, trade quests, or save schema.
- Whip reach: 1040 units; fixed swing length: 136 units; preserve existing grip eligibility and combat responses.
- Do not include gameplay logs, saves, external archives, GIFs, or animated previews in source deliverables.
- Name simulation and compilation results accurately; gameplay confirmation remains separate.

## Review Focus

- First reward dialogue with null, stale, or unrelated previous message owner must show the correct item.
- Native NPC put-away and textbox-start states must not discard the accepted interaction or Yes selection.
- Reward presentation must not grant a trade item or modify either native trade slot/flags; failures must not consume the reward.
- Reticle and lash must agree at the extended range, and unrelated first-person users must retain their existing readiness behavior.
- Longer lashes must respect renderer storage/segment bounds and preserve swing/release behavior.

## Task 1: Repair and simulate OoT reward interactions

**Own:** `soh/soh/Enhancements/Gameplay/ComboVanillaRewards.cpp`, new `tests/combo_vanilla/oot_pickup_flow_test.cpp`, new `tests/combo_vanilla/run_oot_pickup_flow_tests.py`, and necessary narrow updates to existing reward tests.

- [x] Trace `Message_StartTextbox`, `Player_StartTalking`, `Actor_ProcessTalkRequest`, `RewardText`, `RewardUpdate`, `MonitorReceipt`, and the native give-item path.
- [x] Add failing native simulations for first open and delayed NPC dialogue, then the Rod Yes-to-receipt-to-grant path. Verify failures reproduce the observed bug before editing production code.
- [x] Correct only the identified owner/lifecycle errors and add safe handling for stale reward text.
- [x] Cover the reward manifest, No, retry, exact ownership, trade invariants, stale tickets, cancellation, and durable save failure using real affected production functions.
- [x] Run the new runner and existing OoT reward/native mode/host suites; record results and checkpoint owned files.

## Task 2: Repair Whip aiming feedback and reach

**Own:** Both games' `mods/items/logic/item_whip.{c,h}`, narrowly required reticle helpers and dispatch lines, and new `tests/whip_traversal/` fixtures.

- [x] Reproduce missing MM reticle when native aiming is active but the legacy first-person timer is zero.
- [x] Verify baseline extension stops at 520; add a failing traversal case beyond 520 and within 1040, plus a target beyond 1040 that must not attach.
- [x] Repair native aim readiness and extend actual lifetime/range and safe visual segment budget consistently in both games.
- [x] Test target validity, range boundaries, occlusion where applicable, native versus legacy aiming dispatch, and unchanged fixed swing length/release/combat behavior.
- [x] Run affected native compilation under Combo/standalone guards and checkpoint owned changes. Record the unchanged baseline strict-C camera-helper errors; fixture linking is not an application build.

## Task 3: Integrate, review, and preserve

**Own:** Workflow test registration if needed, checkpoint documentation, recovery packaging, and integration verification.

- [x] Run the relevant existing Combo Vanilla suites including MM Hourglass/Whip controls and affected Whip/aim tests.
- [x] Obtain a fresh review of production changes and whether tests actually cover native lifecycle ordering and grants. Fix any material findings with reproducing cases.
- [x] Run changed native compilation checks and assess configured-build availability. Record missing application-build dependencies/assets and the limits of fixture proof.
- [x] Checkpoint reviewed source/tests and prepare the self-verifying source recovery packager. The generated manifest and packager output record the final integration commit/tree and independent restoration proof.

## Task 4: Later user steering — visibility, placement and startup delay

- [x] Obtain geometry from the exact Lost Woods archive named in the supplied log. Native and alternate collision both have the central square at x/z ±200, y 0, confirming center (0, 0, 0).
- [x] Reproduce the entry-relative Rod position and avoidable first-spawn modulo delay in native-layout tests before changing production.
- [x] Prefer the verified exact center for the Rod, constrain it to room 0 independent of entry route, retain loaded collision safety and nearby fallbacks.
- [x] Attempt OoT placement immediately on first safe readiness and room arrival, retain bounded later scans, receipt monitoring and all existing safety gates.
- [x] Simulate actual MM randomizer queue/custom-item timing. Keep native grant timing unchanged unless a separate concrete defect reproduces; distinguish queue blockers, parent handoff and presentation delay.
- [x] Include new source/tests and evidence in the independent review, routed regression run and recovery checkpoint.

## Task 5: Later user steering — MM forest NPC appearance

- [x] Trace Saria/Epona creation separately from object/model readiness and first draw.
- [x] Reproduce the post-draw hook plus incoming-transition/player-state gating with actual native scheduling functions and native types.
- [x] Permit exploration-only visual NPC creation at the earliest safe room-ready before-main phase, retaining prologue, outgoing-transition, resource, collision and duplicate controls. Preserve Termina Field song-giver timing and native interactions.
- [x] Run the new timing fixture and relevant existing resource/placement controls, obtain an independent review and retain prior source checkpoints unchanged.

## Final verification record

The initial seven routed groups passed. The later forest delta passed its two focused routed groups and independent review. The authentic MM starting-check diagnostic extension has its own narrow review and fresh standalone/Combo sanitizer evidence; its CI group also passed after that extension. No full application or configured gameplay acceptance is claimed. The final source package records exact identities, all scoped logs and the remaining runtime checks.
