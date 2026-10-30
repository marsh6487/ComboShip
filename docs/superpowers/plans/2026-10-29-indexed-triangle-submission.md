# Indexed triangle submission implementation plan

> **For agentic workers:** use superpowers:executing-plans; root owns implementation. Read-only agents verify integration target and final preservation.

**Goal:** reduce duplicate packed vertex submission while preserving the exact rendered triangle stream, then integrate it into cumulative source.

**Architecture:** per-batch uint16 indices with command-scoped vertex reuse, existing state/flush boundaries, real DX11/OpenGL indexed buffers and legacy fallbacks.

**Tech Stack:** C++20, existing Fast3D backends, native Python/C++ fixtures.

**Spec:** `docs/superpowers/specs/2026-10-29-indexed-triangle-submission-design.md`.

## Global constraints

- Exact parent b43e86ea; one source implementer; no new diagnostic build or assets reduction.
- Preserve native/wide vertices, ownership/Alt resolution, materials, shadows, effects and both game paths.
- Unsupported backends and triangle-dependent output retain the legacy path.
- No protected merge/release; existing related-candidate source checkpoint authorization only.

## Review focus

- A reused RSP index after a vertex/material/nontriangle command must emit new attributes.
- Indexed/direct mode changes and full-buffer rollover must not lose, reorder or duplicate triangles.
- LOD, rectangles, shared samplers, alpha, fog, grayscale and toon/shadow paths must match the legacy stream.
- Backend index format, buffer capacity, VAO/device state and lifecycle must remain correct.
- Cumulative integration must preserve Major fairies, actor/ABI/audio/equipment fixes and current child Zelda placement.

## Task1: shared interpreter and API

- [x] Extend the existing production-function fixture with shared-edge, representation, invalidation, fallback and full output-equivalence tests; observe RED against the old implementation.
- [x] Add backend capability/API and interpreter index stream, per-command reuse mask, invalidation and mode-safe Flush.
- [x] Verify meaningful native tests plus sanitizer pass; commit bounded source milestone.

## Task2: backend implementation

- [x] Add real DX11 dynamic index buffer/DrawIndexed and OpenGL element buffer/glDrawElements; share original state preparation.
- [x] Test indexed buffer/draw boundary with actual production functions; run native CPU comparison and report limits.
- [x] Check platform compilation availability, independently review and commit backend milestone. Full platform compilation is unavailable here; do not treat the native GPU API boundary fixture as that build.

## Task3: cumulative integration and durable handoff

- [x] Apply only the bounded Market fix and Chamber guard/emitter additions to the preserved complete parent.
- [x] Keep all existing regression groups; run checks relevant to renderer and cumulative changes.
- [x] Review the whole candidate independently; resolve substantive findings with reproducing tests. The inactive toon test was reproduced and corrected; both reviewers report no unresolved Important findings.
- [x] Commit and durably checkpoint exact source/tree using the known complete remote22 base; verify independent restoration.
- [x] Record source checkpoint, tested behavior, actual build status, runtime limits and rollback for the source handoff. No FPS or visual-acceptance claim without runtime proof.

Verification note: real DX11/OpenGL platform compilation remains unavailable in this host; actual new methods compile against recording GPU API boundaries. CPU repeated/disjoint native measurements are recorded without a game FPS claim.

Publication status: explicit public-destination authorization resolved the earlier blocker. The checkpoint branch is published at 32680828cdd96fec1173d9cd95069857db12f149 with the exact f2ca8f24 source tree e0fe8178202834acdcca45db02acd48d38e39b09. The related candidate adds the recovery mapping, exact original-history objects and focused publication verification; see docs/recovery/oct29-cumulative-publication-20261029.md. Full platform/GPU/runtime proof remains pending. The follow-up requests normal candidate CI; no protected merge or release publication is authorized.
