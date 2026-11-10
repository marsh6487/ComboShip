# 2Ship adult GI framing implementation plan

> **For agentic workers:** Use superpowers:subagent-driven-development. One implementation task owns the coupled presentation code; parent handles independent baseline/adjacent verification, review and packaging.

**Goal:** Apply native SoH adult item framing to ordinary adult Link/Din in MM.

**Architecture:** Add a narrow render-aware ordinary-adult receipt query, feed the existing MM item cameras the donor height/shot, and position custom receipts from actual adult hands. Restore native GI drawing after adult skeletal rendering.

**Tech Stack:** Existing C/C++ game engines and Python-driven native regression fixtures.

**Spec:** `docs/superpowers/specs/2026-11-10-mm-adult-gi-framing-design.md`

## Global constraints

Preserve baseline fdbfb868/tree639ce09 and all cumulative assets/source. Leave Zelda's Letter and gate code entirely unchanged. Restrict to local loaded ordinary adult Link and alternate adult assets such as Din. Preserve child/native/custom forms, fallback/peer paths, unrelated cameras, grants, save/seed behavior and item draw resources. No public push, merge or master promotion. Native compilation/fixtures do not prove in-game visuals or a full application build.

## Review focus

- Adult mode alone cannot change child-rig, native transformed, custom-form, missing-rig or peer presentation.
- The custom actor's shape offset must be applied exactly once.
- Native GI_SHIP placeholder must not draw a duplicate item.
- ITEM0 and large-chest behavior must preserve other settings, collision and camera return.
- The first20 hidden frames, one reward callback and eventual receipt cleanup must be preserved.

### Task 1: Adult presentation correction and native regressions

Owned source: `mm/mods/items/logic/adult_link_render.h`, `mm/mods/items/logic/adult_link_render.cpp`, `mm/2s2h/CustomItem/CustomItem.cpp`, `mm/src/code/z_camera.c`, `mm/src/overlays/actors/ovl_player_actor/z_player.c`. Owned new tests: `tests/adult_mm_item_presentation/`. Do not edit unrelated paths; parent owns docs/CI wiring.

Interfaces: Existing `AdultLink_UsesAdultPresentation(const Player*)` remains unchanged. Add C-compatible read-only helpers under adult_link_render.h/.cpp as needed for ordinary-adult item eligibility and visual height/translation. Consumers are the two item camera functions and CustomItem update. Do not globally modify Player_GetHeight or native transformation tables.

- [x] Write/execute regressions against baseline production functions before changes: adult held receipt final transform, adult ITEM0 camera output, long-chest final shot and positive native GI draw. Preserve failing evidence outside source tree.
- [x] Add the ordinary-adult guard, preserving the existing guard's broader semantics. Account for model scale/YOffset in the appropriate coordinate spaces.
- [x] In ITEM0, port donor height68, target10%, radius50%, pitch-20, FOV60, yaw-preserving MM flags, timer6 and interpolation0.25. In long-chest, retain native sequence and use adult height only for the intended setting/actor.
- [x] Anchor visible custom held receipts to actual hand midpoint +14Y/+3.3forward, compensating the actor shape offset. Use SoH's adult lift20 for overhead receipts while preserving bob/timers.
- [x] After adult skeleton drawing, submit positive native GI IDs with native guard; GI_SHIP zero remains suppressed.
- [x] Run tests with normal and ASan/UBSan configurations and compile changed source as far as the local dependency environment supports. Cover native/custom forms, fallback/readiness/peer guards, scale/offset, timing and camera scoping.
- [x] Self-review and report exact changed files, commands/results, baseline failures and limitations. Do not publish or spawn agents. Parent commits after independent review.

### Parent integration

- [x] Verify baseline and adjacent adult-rig, camera and Black Market receipt tests with concrete scope.
- [x] Inspect final source diff and run independent focused checks.
- [x] Fresh independent review; address blocking findings with corresponding regression.
- [x] Verify gate/letter/assets unchanged and prepare the source checkpoint with a restore-verifiable delta and evidence.

Post-checkpoint delivery: save the recovery artifact and report implementation, build/fixture and gameplay status accurately. Commit identity, final restoration proof and delivery are recorded with the artifact so completing delivery does not change the source tree being verified.

## Verification outcome

The exact CI entry passes normal and ASan/UBSan runs for all nine native production-function case groups. Complete changed camera/player C translation units pass native syntax checks. Adjacent adult flex (432 frames), camera donor parity (5,616 frames), both-host Magic Mirror camera adapters and the native MM Black Market receipt path pass. Independent source review has no Critical or Important finding.

The chest fixture covers the final shot and transition to state 30; the changed preceding interpolation was reviewed algebraically rather than executed through a complete chest sequence. Full C++ translation units are blocked by absent real spdlog headers, and no complete game build or adult Link/Din visual acceptance is claimed. A stale legacy Black Market receipt fixture already fails on the unchanged baseline; the current native receipt integration check passes.
