# Evening cumulative publication implementation plan

> **For agentic workers:** Use superpowers:executing-plans for the integration, with independent parallel donor audits and a final whole-branch review.

**Goal:** Integrate the authorized evening source candidates into the current PR90 tree, verify their interactions, and publish a recoverable draft candidate with complete hosted builds.

**Architecture:** Apply bounded donor deltas to the exact published baseline. Preserve all existing production changes and combine shared menu and regression-manifest additions deliberately. Publish the final reviewed tree using verified Git blobs and an expected-parent ref update.

**Tech Stack:** C/C++20, native OoT/MM engines and assets, Python regression runners, GitHub Actions.

**Spec:** The user's supplied November 10 publication authorization (retained with recovery), and the existing donor specs for Din's Fire, vanilla pickups, GI light shaft, and Impa startup receipt.

## Global constraints

- Public destination: `marsh6487/ComboShip`, `candidate/seed-fill-regressions-20261110`, draft PR90.
- Starting published commit `a5b1abd697d284ecbca70c13091562821dfc3feb`, tree `da99bc7dd7191661e4dae8f829b2a98d2f22602b`.
- Preserve cumulative seed, hints, adult GI, trade/mask/equipment, and CI repairs.
- Keep both optional graphics settings off by default and retain What's New accents.
- Retain actual runtime assets; exclude GIF/video/animated preview media and private scene archives from new public payloads.
- Keep companion addons separate; preserve original private packs.
- Preserve source checkpoints before long verification/publication; distinguish source/native/build checks from configured runtime acceptance.
- No merge or master promotion. Continue authorized integration, fixes, builds, and publication without repeated approval.

## Review focus

- Shared menu/What's New/CI edits retain both effects and every old/new registered gate.
- Impa's presentation-only receipt does not regrant items or interfere with vanilla NEI item identity, dialogue, or native player boundaries.
- Native player changes retain Din's persistent-fire and Impa receive hooks in original/Combo guards.
- GI shaft eligibility covers the integrated frozen startup receipt, ordinary native receipts, and MM custom items without lighting shelves or previews.
- Required native audio/texture bytes match donor identities and build into both engine archives; no optional preview enters the publication.

## Task 1: recover and audit donors

- [x] Confirm live baseline and create an isolated integration worktree.
- [ ] Record exact donor tips/trees, original archive hashes, independent restoration proof, and source/build/runtime limits.
- [ ] Independently audit companion addons and retain their exact artifact identities and load order.

## Task 2: integrate source

Files: donor-scoped production files and tests; shared `scripts/ci/regressions.json`, `combo/gui/WhatsNew.h`, host graphics menus, and OoT player implementation.

- [ ] Apply NEI `91c3e56ccf36e119da2cb6a04108215c6ad82567`, Din `540be14d90634caa88c37491f400f7d94391d76c`, GI `28848dbae7647dc3a80cdf85b7cbdaafc604e2a0`, and Impa `08891206ab57f3c8fa0250d4433c46726f8a8bf5` as bounded deltas.
- [ ] Resolve shared additions without replacing a donor or the cumulative baseline wholesale.
- [ ] Run merged feature gates and focused affected receipt/equipment/renderer/actor/audio suites; repair concrete integration failures with native behavioral assertions.
- [ ] Compare untouched donor paths byte-for-byte and inspect every combined path; confirm all baseline CI entries remain.

## Task 3: review, recovery, and publication

- [ ] Obtain independent integrated source review and resolve material findings.
- [ ] Record exact candidate tree/commit, source and asset inventory; create and independently restore a durable recovery archive.
- [ ] Upload new required binary blobs with explicit base64 encoding, validate Git blob identities, reuse equal blobs, and create the exact reviewed tree.
- [ ] Recheck live parent, create publication commit, update candidate ref with expected SHA, and read back branch/commit/tree/PR state.
- [ ] Update draft PR90's scope, source validation, and explicit runtime limits.

## Task 4: hosted builds and handoff

- [ ] Observe Windows compile preflight, required regressions, full Windows/Linux builds and resource generation; address bounded failures and repeat affected gates.
- [ ] Verify final branch/CI trees and successful Windows, Linux, Linux AppImage artifacts.
- [ ] Save updated recovery/evidence and report published commit/tree, PR/build links, separate addons, and outstanding runtime acceptance.

## Baseline evidence

The live PR90 head was reread at task start and matches the exact baseline above. Its recorded final hosted pipeline passed 333/333 registered entries plus Windows/Linux full builds. This evidence is reused for unchanged baseline behavior; new merged candidate gates will be run against the new source.
