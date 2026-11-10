# Clock Glass Sheen Implementation Plan

> **For agentic workers:** Use the established parallel agent workflow with
> disjoint file ownership, then one independent final source/material review.

**Goal:** Make the authored Clock's glass readable with a soft scrolling sheen.

**Status:** Asset/runtime implementation and focused verification complete;
static comparison reviewed internally. Configured GPU and user glass acceptance
remain pending. The handoff manifest records the final source commit and tree.

**Architecture:** Add a separate transparent lens and paired texture/material
entrypoints. Reuse native presentation and foreign CUSTOM_GI routes with a
Clock-only interpolated tile offset; no borrowed graphics segment.

**Tech Stack:** Native C/C++ GBI, XML/OTEX assets, deterministic Python tools.

**Spec:** `docs/superpowers/specs/2026-11-09-clock-glass-sheen-design.md`

## Global constraints

Baseline 5cd05ecf remains recoverable. Preserve its 14 Clock resources byte-for-
byte, scale/spin/shimmer controls, all MM half-day clocks, and spring work.
No extra UI, ABI kinds, seed/save changes or animation preview media. Lens is
radius 335 / center Z170 / edge Z106, peak texture alpha 70, 32x32 RGBA32, front-facing XLU.
Scroll phase=(uint32(frame)& 127), next=phase + 1, repeat mask 5.

## Review focus

- Texture prep survives native setup 25 without a texture/tile overwrite.
- Glass stays above hands inside bezel; no rear reflection or opaque haze.
- Original and both combined hosts use the correct texture/model owner.
- Low arena space and optional shimmer cannot overflow commands or leak state.
- Frame wrap is continuous and no GPU segment binding is changed.

## Task 1: native assets and actual-resource preview

Owner: clock_glass_asset. Files: tools/authored_clock/** and both hosts'
assets/custom/objects/object_nei_clock/**.
Produces: gNeiClockGlassTexDL, gNeiClockGlassDL and their static dependencies.
Consumes: direct tile 0 offset installed between those entrypoints.

- [x] Add lens/texture/material resources with unchanged baseline body bytes.
- [x] Extend resource/geometry/material validation for paired dependencies,
  alpha bounds, winding, actual lens clearance and final state.
- [x] Render static baseline/phase/angle/small-size previews from real resources.
- [x] Run deterministic build/check and validator; commit owned files.

## Task 2: native and foreign draw integration

Owner: clock_glass_runtime. Files: combo/NeiClockGI.h, Clock-only additions
in combo/menu/ComboForeignDraw{OOT,MM}.h and tests/authored_clock/**.
Consumes: Task 1 paired entrypoints; retains gNeiClockDL body.

- [x] Extend focused actual-command tests for new glass layer, intrinsic
  effects-off behavior, texture/scroll ordering, state/owner balance and wrap.
- [x] Add bounded native prep and Clock-only foreign prep using direct native
  tile interpolation; no RSP segment writes or ABI changes.
- [x] Run callbacks/foreign tests and affected native syntax checks for
  original/combined hosts; commit only owned paths.

## Root closure

- [x] Inspect final static phases and exact baseline/body preservation.
- [x] Independently review whole diff and resolve concrete findings.
Final delivery saves the independently restorable media-free source checkpoint
and static preview separately, recording the exact source tree and archive scan
in the checkpoint and presenting implementation/runtime evidence accurately.
