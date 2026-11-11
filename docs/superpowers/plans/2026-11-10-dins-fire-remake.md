# Din's Fire Remake Effect Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans. Steps use checkbox syntax for tracking.

**Goal:** Implement an optional remake-inspired Din's Fire visual POC in both game hosts.

**Architecture:** Retain the existing gameplay actors and cast actions. Add a shared, bounded visual renderer with native adapters, privately named authored textures and Din-only draw hooks. Existing rendering is the disabled/missing-resource fallback.

**Tech Stack:** C native game/render APIs, C++ menu settings, Python native-resource encoder and behavioral fixtures.

**Spec:** `docs/superpowers/specs/2026-11-10-dins-fire-remake-design.md`

## Global Constraints

- Parent is `a5b1abd697d284ecbca70c13091562821dfc3feb`; preserve its cumulative changes.
- `gEnhancements.DinsFireRemake` defaults off. New resources use only `custom/dins_fire_remake/`.
- Preserve gameplay timing, damage, magic, cast animations, sound, MM protection release, Fire Medallion, trident, sword and shield.
- No gameplay RNG use, extra damaging actors, stale texture pointers or shared vertex mutations.
- Source/command-stream verification and actual configured runtime acceptance are separate.
- No publication, merging or master promotion; no GIF/video files in source package.

## Review Focus

- Early player charge must be visible before actor spawn, without moving spawn or consume boundaries.
- Actor pre-cast scale is zero; geometry must use a fresh world transform.
- Missing textures or a live option/Alt change must preserve a safe fallback and fresh resource identity.
- MM's shared actor must not apply the Din visual path to Fire Medallion.
- Per-frame allocations and interpolation identity must remain bounded across repeated casts and transitions.

## Task 1: Production draw path and behavioral coverage

**Ownership:** New `combo/render/DinsFireRemake*` shared helpers, native game helper files if required, the OoT/MM Din spell draw functions and player charge draw hooks, associated new tests/runner, and the existing MM magic-fire binding fixture dependency adaptation.

**Interfaces:** Consume `gEnhancements.DinsFireRemake` and the two named resources in the spec. Expose small draw-only entry points for initial player charge and actor lead-in/active effect; report exact native adapter signatures. Root supplies resources; another worker supplies menus/settings. Do not edit their paths.

- [x] Capture the baseline MM sphere-binding check; solve missing local fixture headers using exact tracked files/includes, not invented stubs.
- [x] Add failing meaningful production tests for enabled pre-cast drawing, green/orange transition and option/resource fallback.
- [x] Implement shared bounded geometry/material renderer and host adapters; preserve native gameplay function bodies and guard unrelated spells.
- [x] Test both host paths, finite allocations/vertices, transition/fade, cosmetics, resource freshness and MM medallion isolation. Adapt existing extracted fixture only for real added dependencies.
- [x] Run focused native syntax/compile and relevant existing bindings/weapon checks; record exact limitations.
- [x] Report owned files and verification. Root commits after review.

## Task 2: Accessible POC control

**Ownership:** Existing OoT/MM enhancement menu files, shared Combo settings sync/config routing and What's New registration where necessary. A narrow menu-binding check may use existing infrastructure; avoid a large new test framework.

**Interfaces:** Checkbox label `Remake Din's Fire`; CVar `gEnhancements.DinsFireRemake`, default 0. Tooltip explains green charging particles and layered fire visuals. Place adjacent to related magic/effect settings. Register What's New using existing conventions. Combo setting must stay aligned across hosts.

- [x] Inspect current host menu and shared-setting/What's New conventions.
- [x] Add checkbox, routing/sync and accent using existing mechanisms.
- [x] Verify exact CVar/default, UI discoverability, and both-host synchronization with scoped existing checks or direct source/native evidence.
- [x] Report owned files. Root commits after review.

## Task 3: Authored texture and resource package

**Ownership (root):** `tools/dins_fire_remake/`, `scripts/assets/build_dins_fire_remake.py`, matching new private resource files under both hosts and documentation.

- [x] Generate and inspect a grayscale turbulent flame texture; retain its authored source.
- [x] Encode the flame and exact spark primitive to the resource contract, with hashes and reproducible metadata.
- [x] Verify dimensions, RGBA intensity/alpha, payload sizes, two-host parity, unique names and deterministic regeneration.
- [x] Inspect actual mapped fire geometry and texture samples where possible; label any preview as offline, not game capture.
- [x] Review the complete diff and resolve findings; run focused final gates and record the bounded source candidate.
- [x] Create an independently restorable recovery checkpoint; retain exact final archive/tree verification in the delivery manifest.

## Execution record

User request authorizes the reversible isolated iteration. Routine implementation choices above are recorded as POC decisions. Independent source/visual investigation completed before implementation. Work proceeds in this session; any in-game acceptance or publication remains a separate state.

The implementation includes both player pre-cast and actor draw paths, original
textures, both menus and a registered native render gate. Independent review
found and resolved a nested native display-marker reservation bug; the fixture
now uses the real graphics setup functions and exact arena boundaries. Asset
review also fixed cross-platform deterministic archive metadata.

The source audit preserves 25 existing gameplay functions and 149 protected
paths. New effect, native texture, MM binding and relevant weapon/render checks
pass. The existing MM Din damage fixture still fails locally with SIGSEGV and
also reproduces from exact published-baseline inputs, including the CI ABI
adapter. Its cause remains unknown and no unrelated repair was attempted.

Full native translation-unit syntax passed under original and Combo guards.
A full application build was not performed; CMake is absent. Offline visual
studies do not establish in-game appearance, hand attachment, floor contact,
performance or user acceptance. These runtime checks and any publication remain
pending. See `tools/dins_fire_remake/README.md` and the recovery archive's exact
refs, evidence and verified-restoration manifest for continuation.
