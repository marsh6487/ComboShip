# True Hidden UI control input correction

**Goal:** Receive L for True Hidden UI only while the player controls Link in OoT and MM. Keep startup and file selection usable because L recovery is no longer permitted there.

**Specification:** cor reports L inventory-page inputs unexpectedly change visibility and explicitly requires control of Link. The existing broad GameState handler violates that requirement. Preserve the chosen visibility through inventory/dialogue, existing minimap/Minimal UI behavior, scene/cross-game continuity, native input delivery and application settings access.

**Baseline:** Isolated copy of the Nov 7 cumulative source. All 201 checkpoint-manifest files match public tree `355ef7377653c3a6de7d8ebea4483fb7716939b7` (published commit `008534e3b1abcdfa8805a555de8d4639991b740f`). Original checkout has a stale Git worktree link, so this candidate uses a separately initialized, bounded source snapshot. No shared tree is edited.

**Architecture:** Keep native input handlers and shared visibility state; gate the handlers with the native gameplay/player control conditions. Do not consume, rewrite or defer L. Treat startup/logo/file-select/debug selection as application navigation and keep their rendering available while preserving hidden in-game HUD, inventory and dialogue behavior.

## Task 1: Input ownership
- [x] Update real production-body tests to reject L in pause/dialogue/nonplay/blocked-control states and preserve the selected hidden state.
- [x] Observe failures against the original input handlers.
- [x] Add minimal native gameplay/control gates for both games, retaining the OoT cheat/debug conflict policy.
- [x] Verify held-input and pause-close cases, ordinary/no-map gameplay, both host guards, and native headers.

## Task 2: Startup recovery
- [x] Add native draw regressions that prove boot and selection interfaces remain visible with the hidden mode armed.
- [x] Remove suppression only at startup/title/file-select/debug selection rendering boundaries; preserve gameplay suppression and fades/state updates.
- [x] Verify the focused drawing fixtures and attempt the complete UI suite. OoT/MM focused suites pass; the umbrella stops at the missing ImGui dependency after the 736 native input checks pass.

## Task 3: Review and handoff
- [x] Independent review of state-pointer safety, native control predicates, menu input and preservation boundaries.
- [x] Commit only owned source/test paths on the isolated candidate: UI `ed716879a4f2d2243cab78d7e8589b65221a322c`; audio `601ad917b66a9c25a26fecac73442f2396291031`. The recovery archive manifest records the final documentation commit, before/after hashes, and independent restore result.
- [x] Report source/test verification separately from full platform build and configured gameplay acceptance.

## Added task: Debug Warp night music

cor subsequently reported that Kokiri Forest night music was absent during Debug Warp. The adapter is not randomizer-gated; its real-save-slot guard rejects a fresh temporary debug save (`0xFF`).

- [x] Trace the real Debug Warp/save initialization route and capture a failing production-adapter regression.
- [x] Permit the temporary debug slot only in normal gameplay, retaining the existing music/ambience and dedicated-scene boundaries.
- [x] Verify the lifecycle and OoT normal/debug/MM adapters under ASan/UBSan. The later unchanged Hyrule Field test stops on the missing `nlohmann/json.hpp` SDK header.
- [x] Include the separate bounded audio correction and its evidence in the same recovery checkpoint.

Final independent review found no blocking or important defects. It re-ran all 736 native control checks, MM input/menu checks, and the diff whitespace check. The post-commit archive recipe validates affected baseline hashes, replays the patch in isolation, and checks the packaged source hashes; its outcome is recorded in the archive manifest.
