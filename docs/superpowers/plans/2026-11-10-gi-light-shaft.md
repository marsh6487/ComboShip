# Item-get light shaft implementation plan

> **For agentic workers:** Use superpowers:executing-plans for implementation,
> with parallel read-only traces and a fresh final reviewer.

**Goal:** Optional, gently scrolling light above Link during real held receipts.

**Architecture:** Shared host-independent state/mesh and native renderer include;
two small host adapters mark actual receipts and draw once at world end.

**Tech Stack:** C/C++20, native GBI display lists, GameInteractor, existing menus.

**Spec:** `docs/superpowers/specs/2026-11-10-gi-light-shaft-design.md`.

## Global constraints

Use `gEnhancements.Graphics.ItemGetLightShaft`, default zero in both hosts.
Preserve the exact baseline's item behavior, camera and adult/form framing.
Use no external assets, extra actors, global RNG or publication operations.

## Review focus

Native and overridden custom receipt draw paths must both mark the effect.
No beam on shelves, exchanges, Kafei or the early chest-opening frames.
One beam per world draw, even if multiple markers arrive.
Pause, reused play memory, scene transitions and clock rewind clear stale state.
Translucent state, wide-vertex ABI and arena exhaustion must remain safe.

## Task 1: shared state, mesh and rendering

Files: `combo/render/ItemGetLightShaft.h`,
`combo/render/ItemGetLightShaftRenderer.inc`,
`tests/gi_light_shaft/state_test.cpp`, `tests/gi_light_shaft/run_tests.py`.

- [x] Write and run missing-feature tests for opt-in gating, marker consumption,
  duplicate ticks, reset, finite mesh sizing and vertical camera-independent shape.
- [x] Implement `ItemGetLightShaft::State::Step(const Input&) -> Frame`,
  `Marker::Begin/Mark/Consume`, and `BuildMesh(const Frame&, Vec3 eye) -> Mesh`.
- [x] Render the fixed mesh in XLU with an immutable 32x64 I8 tile and native
  tile scrolling, with a preflight arena budget and restored matrix/material.
- [x] Run normal and ASan/UBSan behavioral tests.

## Task 2: native host boundaries and shared checkbox

Files: native `z_player_lib.c` in both hosts, new Graphics adapters,
`combo/render/ItemGetLightShaftBridge.h`, both Graphics menus,
`combo/gui/WhatsNew.h`, focused adapter/renderer tests.

- [x] Mark native draws after their actual GI draw; add MM OnActorDraw filtering
  for visible GIVE_ITEM_CUTSCENE custom receipts with overridden draw callbacks.
- [x] Clear the marker at world start, consume once at world end, reset at scene
  and play destruction, and check pause/transition/current-player guards.
- [x] Add the default-off shared checkbox and What's New registration.
- [x] Exercise real adapter guards and renderer boundaries, compile changed
  native units in original/combined guards, rerun adult GI preservation coverage.

## Task 3: review and recoverable handoff

- [x] Run focused registered checks and inspect the full bounded diff.
- [x] Obtain a fresh independent review and resolve concrete correctness issues.
- [ ] Commit the candidate and create a source recovery containing exact refs,
  changed source, original files, patch, test evidence and restoration instructions.
- [ ] Verify restoration independently and save the deliverable. Report runtime
  appearance separately from native compilation and behavioral test evidence.

## Source verification status

Implementation, native checks, preservation checks and independent review are
complete. The remaining checkpoint/restore steps are executed after committing
this plan; their exact results belong to the recovery archive manifest. Full
executable build and in-game visual acceptance remain unverified.
