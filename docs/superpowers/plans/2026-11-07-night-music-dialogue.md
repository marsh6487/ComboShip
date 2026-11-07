# Night music dialogue continuity implementation plan

**Goal:** Keep an already playing night track continuous during ordinary gossip-stone dialogue.

**Baseline:** `2054eef2f12aeedfb2d2d0f5519cc23272227e4e`, cumulative PR #86. Candidate: `candidate/night-music-dialogue-20261107`. Baseline is build-tested source, not configured runtime acceptance.

**Cause:** OoT's `Player_SetupTalk` sets both TALKING and IN_CUTSCENE. `Player_InCsMode` consequently blocks the scene-night updater, whose unconditional suspension stops the owned SUB track. Closing dialogue starts it anew with a 30-frame fade.

**Architecture:** Keep actual audio overrides in the suspension gate. A player-control lock instead gates `canStart`, deferring starts/editor refreshes while permitting owned playback and dawn/disable cleanup. Preserve scripted cutscene, forced music, preview, fanfare, combat and native-player ownership checks. MM already defers without suspension and needs no production change.

**Files:** OoT `SceneNightMusic.cpp`; `tests/night_music/host_test.cpp` and `run_tests.py` for the existing production-updater fixture. No assets, settings or save changes.

**Review focus:** Multi-page dialogue/closing must issue no stop or new start; refresh during dialogue must defer; real cutscenes and fanfares during dialogue must still suspend; native SUB owners and pending commands must remain protected; dawn/toggle/scene cleanup must retain existing behavior.

- [x] Extend the native fixture using the actual OoT talk setup and cutscene predicates; demonstrate dialogue continuity failing on baseline.
- [x] Split player-control deferral from audio-override suspension in the OoT adapter.
- [x] Run existing night lifecycle/host/queue checks and dedicated field/combat regression; inspect formatting and diff.
- [x] Commit a bounded candidate, preserve recovery, and record full-build and audible runtime limits.

**Decisive runtime check:** With the reported mod stack and Enable Night Music on, talk to a stone at night, advance pages, close and repeat. The track should maintain its position without silence/restart. Verify one item fanfare and real cutscene still take precedence. No audible runtime claim follows from the native fixtures.
