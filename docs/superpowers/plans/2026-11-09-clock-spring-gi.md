# Authored Clock and Spring Blossoms Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development. Execute the owned independent workstreams in parallel per existing user and developer authorization.

**Goal:** Create reviewable authored Clock GI and improved spring blossom candidates.

**Status:** Implementation, focused verification and independent source review are complete. Static previews are ready; configured GPU appearance and user art acceptance remain pending. See `docs/poc/clock-spring-gi-verification-20261109.md` for evidence and limits.

**Architecture:** Separate native resource authoring from Clock routing, and isolate the shared spring renderer changes. Reuse existing native model and NEI effects infrastructure.

**Tech Stack:** C/C++, native GBI/XML resources, Python asset generation and preview tools.

**Spec:** `docs/superpowers/specs/2026-11-09-clock-spring-gi-design.md`

## Global Constraints

- Baseline is e22bcb222dcd06d18567cbbb47087d3d5979be96; only this isolated candidate is editable.
- Preserve gameplay, item identities, all MM half-day Clock models, other seasons and accepted cumulative work.
- Preserve spring particle counts, motion, fades, RNG, pause and interpolation.
- Native/foreign, original/combined and Alt/base resource paths must remain valid.
- No publication or install package before preview review; source recovery is permitted and required.

## Review Focus

- Clock display through the opposite game must select the same authored model.
- Disabling NEI effects must retain the Clock body and restore draw state.
- Translucent petals must preserve particle fade alpha and avoid solid rectangle edges.
- Spring resource changes must not leak texture or combiner state into other effects.
- Preview textures, curvature and transforms must match serialized production resources.

## Task 1: Clock resource authoring

**Owned files:** `tools/authored_clock/**`, `soh/assets/custom/objects/object_nei_clock/**`, `mm/assets/custom/objects/object_nei_clock/**`.
**Consumes:** Art direction and dimensions in the spec; existing authored-door-key and NEI mesh/preview conventions.
**Produces:** `objects/object_nei_clock/gNeiClockDL` (no glass pass is needed), matching resources in both hosts, deterministic rebuild, faithful preview and resource verification.

- [x] Author the timepiece mesh, UVs/materials and finished reverse; preserve readable physical depth.
- [x] Serialize native resources with valid vertex-cache batches and dependency closure; verify both archive copies match.
- [x] Render actual-resource front, angled, rear and small-size previews; document offline approximations.
- [x] Commit only owned files and report asset dimensions, triangle count, dependencies and exact checks.

## Task 2: Clock rendering and NEI shimmer

**Owned files:** Clock branches in `soh/soh/Enhancements/randomizer/{draw.cpp,draw.h,item_list.cpp}`, `mm/2s2h/Rando/DrawItem.cpp`, `combo/menu/{ComboItemDrawOOT.h,ComboItemDrawMM.h}`, new `combo/NeiClockGI.h` if helpful, `tests/authored_clock/**`. Request ownership before other edits.
**Consumes:** Task 1 native paths and dimensions. Coordinate exact entry points directly with asset author.
**Produces:** Native and foreign authored Clock paths with established NEI shimmer and effects-off rendering.

- [x] Trace baseline routes and analogous native/foreign NEI effects; reproduce the foreign Stone of Agony selection in a focused test.
- [x] Add authored Clock draw routing, guarded for both original/combined hosts, with preserved half-day paths.
- [x] Verify real route command behavior and effects-off restoration; run relevant existing GI/foreign tests and native header checks.
- [x] Commit only owned paths; report exact checks and remaining runtime limits.

## Task 3: Spring blossom surfaces

**Owned files:** `combo/NeiSpringBlossoms.h`, `soh/soh/Enhancements/Graphics/SeasonParticles.cpp`, `mm/mods/items/objects/object_spring_petals.h`, spring-only fixture changes in `soh/tests/season_particles_renderer_test.cpp` and `tests/seasons/**`, `tools/spring_blossoms/**`, both hosts' new `assets/custom/objects/object_spring_blossoms/**`. Request other ownership as needed.
**Consumes:** Current spring geometry/motion and root-provided texture art if required. Root owns image generation and user-facing preview delivery.
**Produces:** Softer, curved blossoms and petals, matched host assets and before/after resource previews.

- [x] Trace active shape/material path and establish the smallest appearance change addressing the untextured cutout impression.
- [x] Implement petal surface/curvature using native resource conventions, preserving the declared lifecycle invariants.
- [x] Verify fade/texture/XLU restoration and the existing OoT/MM season suites; compare actual before/after resources at game-size readability.
- [x] Commit only owned paths and report changes, counts, tests and runtime limitations.

## Root closure

- [x] Review independently each workstream and cross-resource interfaces, inspect final previews and resolve concrete defects.
- [x] Run the combined focused gates and verify that the cumulative recovery and narrow integration patches independently restore the candidate tree.

Final handoff refreshes the checkpoint to the delivery commit and presents both static previews with the documented runtime limits. Its exact commit, tree and media-scan result are recorded in the checkpoint manifest and restore verification.
