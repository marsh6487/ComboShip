# MM BlackMarket RenderFlight implementation plan

**Goal:** Obtain bounded MM native-shop tick, render-attempt and resource-cost evidence in the same candidate as the shop fixes.

**Baseline:** `2054eef2`, `candidate/night-music-dialogue-20261107`. Source implementation is distinct from native build verification, configured game evidence and user acceptance. No master promotion is authorized here.

**Architecture and approaches:** Reuse the existing recorder by compiling an MM-local wrapper, with configurable host and scope labels defaulting to OoT. This preserves existing output and bounds while avoiding a second diagnostic format. A full actor instrumentation port is outside this narrow shop probe; a summary-only MM probe would omit the resource costs needed to locate the slowdown. The selected wrapper is the smallest useful path.

**Preservation:** Do not change display lists, interpolation cadence, renderer optimization flags, assets, audio work, gameplay or draw ordering. Existing market guard and orientation changes belong to other workers. Diagnostics remain off by default using `gDeveloperTools.FrameTimingProbe`. The MM module's existing hidden ELF visibility keeps recorder symbols module-local.

**Exact choices:** Start at `Graph_StartFrame` only with the toggle on, active MM host, a live current normal-gameplay state and `ComboBlackMarket_IsInterior()`. End after `Graph_ProcessGfxCommands`, including its final audio wait. Close ticks on exceptions and flush buffered data on loss of eligibility, context change, `MM_PrepareForTransition` and shutdown. The handoff flush preserves the cumulative capture budget. Do not call the recorder or mutate collection flags on an initially disabled path. Enable `Fast3dWindow::SetCollectFrameTimings` and `Interpreter::SetCollectRenderCosts` only inside a tested RAII guard; reset on early returns and exceptions. Preserve the existing 256 MiB / 15 minute session cap and resource-key limits; MM's shop-only recorder cannot consume OoT's separate recorder budget.

**Output:** `logs/MM-RenderFlight-*.log` with MM host and shop-only scope labels. Capture build/commit, archive-manager order, backend, Alt Assets, interpolation FPS, resolution, MSAA, vsync and diagnostic detail settings. Keep every renderer attempt (including not-presented attempts), supported backend pacing/GPU status, sampled renderer summaries and full collected resource/texture/framebuffer tables. Unsupported GPU metrics remain explicit engine output. Native actor attribution is absent; named shop CPU spans cover room/shelves/effects/item GI/model-bounds traversal/seals/keeper work only. Durations are inclusive elapsed CPU-side time and may overlap.

## Implementation and validation

- [x] Add failing real-recorder MM label/output checks, retaining the default OoT case.
- [x] Add failing native-helper tests for eligibility, disabled-path immutability, collection cleanup on ordinary return, early return, constructor failure and render exception.
- [x] Implement shared host label parameters, MM wrapper, helper, and native frame/render hooks.
- [x] Execute the actual native eligibility body for live/active/interior gating; inspect frame boundaries, real renderer APIs, configuration and shutdown. Session/RAII and real output checks cover lifecycle behavior. Source-string assertions were removed from the final test suite.
- [x] Run both OoT and MM production-recorder ROM-free tests with the existing dependency bundle. Attempt native MM compilation, recording the precise environment blocker if SDL2 headers or other build inputs prevent it.
- [x] Handoff owned diff paths, verification evidence and remaining configured runtime test. Do not commit or publish from this worker; the parent owns checkpointing.

**Decisive runtime check:** In the existing developer console, run `set gDeveloperTools.FrameTimingProbe 1` (there is no dedicated checkbox), enter the same MM BlackMarket scene with the reported packs/order/backend, reproduce slowdown, leave/flip host/disable, and inspect the dedicated MM log. Compare enabled versus disabled performance because detailed diagnostic collection has overhead. No runtime evidence or acceptance is claimed by the source checks.


## Verification evidence and limits

- The exact production `MMFrameTiming_Eligible` body compiles and executes in combined and standalone fixtures. It rejects an absent/off toggle, null/mismatched game state, stopped state, nonnormal mode, inactive host and noninterior, and accepts only live active combined shop gameplay.
- Session/RAII execution covers initially disabled immutability, missing previous tick end, normal/early/exception cleanup, constructor failure, tick-end callbacks, explicit handoff flush, resumed capture and preserving the cumulative cap on handoff.
- Real MM and default OoT recorder suites execute production recorder, `PerformanceTrace`, JSON, async writer and file sinks; validate host/scope labels, attempts, full resource/texture tables, named aggregate semantics, Warn/Off normal logging, teardown and restart. The ROM-free fixture substitutes only Context subsystem initialization and the unrelated `SDL_AudioDeviceID` declaration; it does not initialize an audio device.
- `run_sword_receipt_tests.py` passes actual receipt/fallback/bounds bodies, selected-resource geometry, typed Ship/Fast API/C linkage and all five MM receipt cameras after adding the optional bounds span.
- Native `BenPort.cpp` syntax compilation was attempted without the test-only SDL declaration and failed at `libultraship/include/ship/audio/SDLAudioPlayer.h:3`: `SDL2/SDL.h` missing. This is not native game build proof. Windows DLL linking, actual configured game slowdown and runtime acceptance remain untested.

To stop and flush from the console, run `set gDeveloperTools.FrameTimingProbe 0`. Keep `gDeveloperTools.RenderFlightDetail` at its default `2` for full per-draw resource collection; `0` retains attempts without renderer costs, and `1` uses the existing sampled renderer cadence. `gDeveloperTools.RendererCostProbe 0` independently disables renderer cost tables. On MM leave/host handoff, capture flushes before MM yields the shared collector. Inspect `logs/MM-RenderFlight-*.log` for MM-only data; the OoT file/budget is separate. No source-level check establishes the slowdown's runtime cause.
