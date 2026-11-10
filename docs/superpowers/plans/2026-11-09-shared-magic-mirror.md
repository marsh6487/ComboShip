# Shared Magic Mirror Implementation Plan

> **For agentic workers:** Execute directly in the isolated owned checkout using superpowers:executing-plans. Controller integrates and publishes.

**Goal:** One bidirectional Combo Mirror toggle in both native game tabs, with independent solo preferences.

**Architecture:** Add a small shared CVar/native-edit policy and a JSON snapshot normalizer. Connect existing native widgets/presets and seed-setting entrypoints; retain native IDs and all Mirror runtime behavior.

**Tech Stack:** C++20, native CVar bridge, existing nlohmann JSON and native regression fixtures.

**Spec:** `docs/superpowers/specs/2026-11-09-shared-magic-mirror-design.md`

## Global Constraints

- No donor/controller edits or publication from this worker.
- Both tabs and OFF are mandatory; solo CVars remain independently owned.
- No enum/save-layout/asset/travel changes or weakened assertions.
- Save bounded independently restorable source milestones before long checks.

## Review Focus

- Explicit shared OFF with either/both solo values ON.
- Preset clear/load when the solo value already equals the requested value.
- Missing/conflicting old snapshot fields and both restore orders.
- Preview versus frozen generation/replay state.
- Original builds and each solo session must ignore Combo overrides.

### Task 1: Reproduce and implement the shared policy

- [x] Add actual-normalizer OFF regression and verify RED.
- [x] Add shared CVar/native-edit scope and snapshot normalization with focused tests for all native/Combo combinations, presets and old imports.
- [x] Commit and independently save the bounded policy milestone.

### Task 2: Connect production entrypoints

- [x] Wrap actual Combo widget and native preset application boundaries.
- [x] Apply effective shared values to actual MM preview and both native seed policies, including OFF.
- [x] Normalize launcher/headless snapshots before restore; preserve native solo values in native snapshot callbacks.
- [x] Update existing exact native fixtures for the approved shared policy without removing retained key/save/legacy controls.
- [x] Commit and independently save the production milestone.

### Task 3: Verify and hand off

- [x] Run focused actual native policy, UI/preset, pool and JSON fixtures with sanitizers using the real GCC13 SDK/native ABI.
- [x] Run affected production TU syntax checks with existing production definitions/options.
- [x] Record verification and send immutable source/evidence to the controller for the required cumulative checkpoint before publication.

Completed source handoff: `docs/poc/shared-magic-mirror-verification-20261109.md`.
The integrated candidate still requires its own fresh platform build and configured
gameplay acceptance; the focused tests do not claim those results.
