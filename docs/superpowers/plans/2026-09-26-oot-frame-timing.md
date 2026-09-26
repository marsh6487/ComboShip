# OoT Frame Timing Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan inline.

**Goal:** Identify which part of OoT's frame consumes time after archive reduction and lower resolution produced no reported improvement.

**Architecture:** An allocation-free, main-thread timing recorder collects scoped durations and emits one aggregate log per second. C-compatible entry points cover game code; C++ scopes cover resource and rendering boundaries. This isolated diagnostic candidate enables collection by default.

**Tech Stack:** C/C++, steady_clock, existing spdlog and JSON dependencies.

**Spec:** The bounded design below.

## Design and preservation

- Parent: `020a50354ae531a2d8fcee51aeedd0ec22b2941d`, matching the source behind the reported recovery build. This is a reproduction baseline, not an accepted performance baseline.
- Candidate: `poc/oot-frame-timing-20260926`; original integration and all archives remain unchanged.
- Preserve gameplay, assets, resource ownership/cache policy, weather, audio production, randomizer and rendering order.
- Record tick construction, Play update/draw, frame hooks, cosmetics, MM asset lookup, audio mutex acquisition, window events, interpolation, and the combined graphics/presentation call.
- Expose optional elapsed stages from Fast3dWindow (ready/setup/commands/GUI/present) to distinguish command processing from presentation waits. Preserve its existing call order and count a presented frame only when its existing return value is true.
- Report actual scope semantics: nested measurements overlap; graphics/presentation includes pacing and GPU waits and is not a GPU timer. Logic ticks are distinct from rendered frames.
- Reset aggregation across scene/room/age/Alt/pause/target changes. Disabled collection must discard partial data, avoid clock reads and ignore stale scopes.
- Log at most one aggregate per second. Log formatting is outside the measured tick. Scene transitions discard incomplete windows instead of mixing configurations.
- `gDeveloperTools.FrameTimingProbe` defaults to 1 only in this diagnostic candidate; setting it to 0 disables collection.
- Runtime request: reproduce the slowdown for about 20 seconds in Kakariko or Graveyard, then provide the log. No performance improvement is claimed.

## Global constraints

- No edits to saves, user settings, archives, scene geometry, or the integration branch.
- No claim of Windows compilation or in-game performance without corresponding evidence.

## Review focus

- Disabled and menu-only frames must not leak old timing data.
- Context changes and stale scopes must not mix measurements.
- Interpolated render calls must not be mistaken for game logic ticks.
- Nested spans must not be added together as exclusive CPU time.
- No locks or allocations on the collector's per-span path.

## Task 1: Timing recorder and call-site integration

**Files:** `soh/soh/Enhancements/debugger/FrameTimingProbe.{h,cpp}`, `soh/tests/frame_timing_probe_test.cpp`, `scripts/diagnostics/run_frame_timing_probe_tests.py`, and narrow scopes in existing OoT frame/cosmetics/MM loader call sites.

**Interfaces:** C `FrameTiming_BeginFrame`, `FrameTiming_BeginSpan`, `FrameTiming_EndSpan`, `FrameTiming_EndFrame`; C++ `FrameTiming::Recorder` accepts explicit nanosecond timestamps for deterministic tests, and `FrameTiming::Scope` supplies runtime timestamps.

- [x] Write tests for aggregate timing/counts, disabled reset, context reset, stale scopes, and nested timing semantics.
- [x] Run tests before implementation; expect missing recorder interface.
- [x] Implement recorder and logging adapter; rerun focused tests, expect all pass.
- [x] Add scopes at the existing boundaries without reordering work; compile C header and run touched-component tests.
- [ ] Review the complete diff, commit the isolated candidate, and obtain build evidence before describing a build as ready.

## Evidence ledger

- User reports no improvement from removing the large extra archive stack or reducing internal resolution. Exact FPS remains unmeasured.
- Existing PreludeLoadProbe is Lost Woods-specific and suppresses most ordinary slow frames; it does not resolve this question.
- Local baseline is clean; remote recovery branch still points to the parent above.
- Ruling: use a sibling worktree and implement inline to preserve the existing checkout and avoid unnecessary permission rounds for authorized local diagnosis.
- Recorder tests were observed failing before implementation, then passing. Renderer-duration accounting was also added through a failing test.
- ASan/UBSan run requires `ASAN_OPTIONS=detect_leaks=0` here because LeakSanitizer cannot inspect `/proc` in this runtime; the address/undefined-behavior checks themselves run normally.
- Independent review found one important issue: a context or enable change within the tick that closes a report window could emit mislabeled data. Fixed by checking all tags and enablement again at tick end. The regression test first failed on the mixed-window assertion, then passed with the fix. Six behavioral tests and C bridge compilation pass.
- Generated Python bytecode was removed from the index after review; it is not part of the candidate.
- The production logging adapter passes a C++20 syntax compile against real nlohmann JSON and spdlog headers. The existing custom-cosmetics regression suite passes. Full Windows build and in-game behavior remain unverified until CI and the user capture.
- Review limitations: rendering call order was inspected, not run against a GPU; no runtime overhead or root-cause claim is made. This candidate must not be promoted as a performance fix.
