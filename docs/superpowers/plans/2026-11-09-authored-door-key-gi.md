# Authored Door-Key GI Implementation Plan

> **For agentic workers:** Use subagent-driven development with explicitly disjoint asset and routing ownership; the user's existing parallel-workflow authorization applies.

**Goal:** Include original Rusty Key and OoT overworld-key models in the first cumulative source revision and provide actual-model previews.

**Architecture:** Bundle native XML display-list, vertex and texture resources in the owning port archive. Native draws and existing cross-game exporters select the same origin-owned resources, with established fallback models when resources are unavailable.

**Tech Stack:** Python authoring and asset validation, native GBI resources, C++ item renderers, existing cross-game GI ABI and native fixture tests.

**Spec:** `docs/superpowers/specs/2026-11-09-authored-door-key-gi-design.md`

## Global Constraints

- Work only in the isolated authored-door-key worktree based on c1b9248ac456de86d664ace7c369cae162293974.
- MM owns `objects/combo_door_keys/RustyKeyDL`; OoT owns `objects/combo_door_keys/OverworldKeyDL`; both use draw scale 0.25.
- Preserve all gameplay, save and generation behavior and all unrelated key models.
- Use original authored geometry and textures, no private or externally sourced game data in this change.
- Root alone stages, commits and publishes; workers edit only their assigned paths.
- Preview serialized assets and disclose offline lighting; do not claim gameplay acceptance from a render.

## Review Focus

All 66 catalog identities including appended OoT keys; correct foreign resource owner and scale; missing-resource fallback; Alt-off/Alt-on selection; complete native asset packaging and valid display-list batches.

### Task 1: Authored native assets and preview recipe

Own `tools/authored_door_keys/`, `tests/authored_door_keys/asset_test.py`, `mm/assets/custom/objects/combo_door_keys/`, and `soh/assets/custom/objects/combo_door_keys/`.

- [x] Author deterministic original iron/rust and aged-brass models to the shared asset contract.
- [x] Validate resources, geometry, batching, winding, bounds and regeneration.
- [x] Render front, back and three-quarter previews from serialized native resources; record hashes and material approximations.

### Task 2: Native and foreign draw selection

Own relevant C++ render/export paths and focused routing tests under `tests/authored_door_keys/` except asset_test.py. Read the spec and existing renderer before changing it.

- [x] Add a focused failing behavior test for all 39 MM and 27 OoT identities, origin routing and fallback; use real production helpers/exporters where practical.
- [x] Wire the new bundled roots into native and foreign GI paths, including appended OoT identities and alias paths.
- [x] Run focused native tests, syntax checks and relevant existing key/export regressions; report any pre-existing failures precisely.

### Task 3: Integration, review, recovery and delivery

Root owns docs, CI registration, packaging and publication.

- [ ] Review actual previews and adjust visual defects before delivery.
- [ ] Independently review the combined asset/routing diff; correct blocking findings.
- [ ] Verify native resource packers and build the affected native targets if the existing build environment permits.
- [ ] Commit a bounded source checkpoint and preserve durable recovery evidence.
- [ ] Publish to the authorized candidate/draft PR with current remote-parent checks, preserving cumulative work; no master merge.
- [ ] Deliver previews and exact verification/source status, keeping in-game limitations explicit.

## Refinement requested after first preview

User accepted the first appearance and requested metallic sheen beneath the aged surface, plus matching the existing key diagonal. Preserve checkpoint `b0854044` and approved geometry/texture RGB.

- [x] Add native reflection base and static weathering coat; validate per-pass TMEM, alpha, texture generation and state teardown.
- [x] Apply one shared -45-degree local Z roll to native and foreign authored draws, preserving inherited transforms and existing fallback orientation.
- [x] Verify real consumer matrix replay in both hosts and parse the revised material through the native resource factories.
- [x] Render updated still and short turning preview from serialized assets, with no extra preview-only specular effects.
- [ ] Show the updated preview before packaging/publication, per the user's explicit request.
