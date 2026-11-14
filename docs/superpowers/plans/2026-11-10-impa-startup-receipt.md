# Impa Startup Receipt Implementation Plan

> **For agentic workers:** Use subagent-driven development with disjoint host ownership and a final independent review. Continue within the user's implementation authorization.

**Goal:** Show the real Impa startup item and source in either game with the existing GI cutscene policy, without granting twice.

**Architecture:** A versioned POD receipt crosses the DLL boundary and is stored in `combo.startupReceipt`. The launcher owns persistence and MM initialization ordering. Each host owns only its native presentation adapter.

**Tech Stack:** C/C++, existing GameInteractor/native player APIs, nlohmann JSON, production-boundary native regression fixtures.

**Spec:** `docs/superpowers/specs/2026-11-10-impa-startup-receipt.md`

## Global Constraints

- Preserve the exact published baseline and use only `candidate/impa-startup-receipt-20261111`.
- Eligibility remains starting Zelda's Letter enabled and Letter shuffle disabled in combined randomized mode.
- Display the actual item and `Song from Impa (OoT)` in the foreground game once.
- Honor native GI skip settings; always show item/source feedback for a skipped GI.
- Presentation never grants an item, repeats a trap, mutates equipment or dispatches unrelated story/receive effects.
- Freeze the selected progressive identity. MM-owned grants wait for the matching initialized MM save.
- Old records are not inferred from inventory; original/solo/vanilla behavior is preserved.
- No external publication or animated preview assets.

## Review Focus

- A new MM-start file following another slot must resolve against its own initialized inventory.
- Native receive hooks and delayed trap/story/equip code must not execute for a display-only receipt.
- Canceling a GI before acknowledgement must leave the durable receipt pending without touching unrelated events.
- Both hosts must retain the pregrant concrete progressive model and name after save/reload.
- Failed deferred-MM persistence must never save an awarded item separately from its ready receipt or award twice in memory.

### Task 1: Durable receipt ABI, state and launcher integration

**Files:** `combo/rando/StartupCheckReceipt.h`, `combo/rando/StartupCheckReceiptState.h`, `combo/ComboShip.cpp`, `tests/startup_receipt/state_test.cpp`, `tests/startup_receipt/run_tests.py`.

- [x] Add a failing native state test for absent/invalid records, initialization, pending foreign grant, ready receipt and exactly-once acknowledgement.
- [x] Define the shared POD ABI and bounded validation. Implement source capture, foreground peek/acknowledge, per-slot reset, and deferred MM completion after new-save creation and matching-save load.
- [x] Stage both MM and OoT writes during deferred completion so both native saves, shared effects and the ready receipt are written together. Preserve every unrelated container field.
- [x] Run state/production-host tests normally and host boundary under ASan/UBSan; retain the failing-first evidence.

### Task 2: OoT capture and presentation

**Files:** new `soh/soh/Enhancements/Gameplay/ComboStartupReceipt.cpp` and header; `savefile.cpp`; narrow `z_player.c` receipt guards; foreign named renderer if required; `tests/startup_receipt/oot_*`.

- [x] Write failing production-boundary tests for capture/eligibility and no-grant native presentation.
- [x] Capture the resolved starting Impa entry/name/category, or queue MM-owned resolution. Keep all other starting checks unchanged.
- [x] Present the frozen item using native actorless GI and the existing skip setting. Include item and source text/toast. Guard native grant, pending flags, equip, trap and story effects.
- [x] Test normal and original guards, cancellation, repeated callback, progressive model identity and native policy.

### Task 3: MM completion and presentation

**Files:** new `mm/2s2h/Enhancements/Gameplay/ComboStartupReceipt.cpp`; bounded named foreign draw wrapper; `Rando.h`/`DrawItem.cpp` as needed; `tests/startup_receipt/mm_*`.

- [x] Write failing tests for MM-owned completion after initialization and one-time presentation.
- [x] Resolve the concrete MM item against its initialized save, capture its name/category, grant through the existing dormant-resolved path, then capture saves after synchronized publication.
- [x] Present via native CustomItem actor ownership, with a no-grant callback, source attribution and existing category policy. Use `DrawResolvedItem` or frozen foreign draw without inventing a check.
- [x] Test all MM skip modes, both owners, canceled/reloaded actors and unrelated-event preservation.

### Task 4: Integration, review and recovery

- [x] Run the new combined runner and the specifically affected existing delivery, GI-policy, receipt and lifecycle regressions.
- [x] Compile changed native boundaries; attempt the available full build path and report any precise blocker without claiming runtime success.
- [x] Register focused regression coverage, review the whole diff independently, and resolve important findings.
- [x] Commit verified source, package baseline/changed sources and exact restoration metadata, verify independent restoration, and save the recovery artifact.

## Execution ledger

- Baseline identified and preserved. Dedicated checkout created; published PR90 remains unchanged.
- Design ruling: preserve local OoT startup grant; defer only MM-owned grant until its matching save exists. This avoids the proven initialization-order hazard while keeping ordinary save initialization behavior.
- Ownership: root owns shared ABI/state/launcher/CI/docs/recovery; host implementers own only their explicit host and test paths.
- Milestone 1: state/launcher tests pass, including real atomic-file failures, post-award native snapshot failure, save-only retry, copied pending state, metadata preservation, generation isolation, and old-save no-inference. Existing vanilla host receipt regression also passes. ASan/UBSan pass with leak scanning disabled because LeakSanitizer cannot run under this container's ptrace environment. OoT/MM adapters and configured runtime proof remain unfinished at this checkpoint.
- Final integration expands the transaction to both native saves, publishes the awarded receipt under the host mutex, and requires a fresh drained OoT save after shared effects. Exact native context replacement guards preserve unsnapshotted rewards. Durable ACK retries suppress repeated presentation across both games in the same process.
- Independent review added concrete category, progressive/static/animated draw, stale-token receive, failed-ACK, concurrent publication and MM GID_NONE regressions. Final verification and configured-runtime limits are recorded in the accompanying verification report.
- Implementation commit `9364a0d9b4c3fff740a171d4c7f86e02d6e14bcb` passed the final integrated gate and independent source review. Recovery restoration reproduced the included source and exact full Git tree/commit. The final recovery manifest also includes this documentation closeout. Full linked-build and configured in-game acceptance remain unverified for the dependency limits recorded in `2026-11-11-impa-startup-receipt-verification.md`.
