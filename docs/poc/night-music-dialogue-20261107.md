# Night music dialogue continuity, 2026-11-07

Baseline: cumulative PR #86, `2054eef2f12aeedfb2d2d0f5519cc23272227e4e`.
Candidate: `candidate/night-music-dialogue-20261107`. No asset, setting, save,
music-selection or accepted-master changes.

OoT's actual `Player_SetupTalk` sets TALKING and IN_CUTSCENE, even for a gossip
stone. `Player_InCsMode` sees the latter bit. The scene-night updater treated
that control lock as an audio override, causing `Lifecycle::Update` to return
Stop, relinquish the SUB track and restart it with a 30-frame fade when dialogue
closed. The stone actor itself does not request a music stop.

The control lock now gates new starts/refreshes through `canStart`. Actual
scripted cutscenes, forced sequences, previews, fanfares, MAIN ownership,
pending SUB commands and combat retain the suspension gate. Disabling night
music and dawn cleanup still stop the owned track during dialogue. MM's
existing adapter does not suspend playback for dialogue and is unchanged.

The regression compiles the complete production host updater plus the actual
OoT `Player_SetupTalk`, `Player_InBlockingCsMode` and `Player_InCsMode` bodies.
It mechanically renames C's `this` parameter for C++ compilation and stubs
unrelated animation/textbox services. On the unmodified updater it aborts at
`ordinary dialogue must not stop/restart night music`. The final updater
passes three pages of 60 frames each, close/repeat, deferred refresh, scripted
cutscene/fanfare inside dialogue, off/dawn cleanup, preview, native ownership,
pending commands, scene boundaries and full-width custom replacement controls
under ASan/UBSan. A preliminary separation that deferred off/dawn cleanup was
rejected by an additional failing test before the final `canStart` fix.

Verified commands:

- `python3 -B tests/night_music/run_tests.py` (shared lifecycle, both host adapters, actual queue/cancellation and BGM row order).
- `python3 -B scripts/diagnostics/run_night_combat_test.py <temporary-build-directory>` (dedicated Hyrule Field/native sequence-mode/ambience, combat and preview handoffs).
- `python3 -B tests/black_market/compile_scene_music_units.py --unit soh/soh/Enhancements/audio/SceneNightMusic.cpp`, with and without `--standalone` (real-header production object compilation).

Local dependency headers are supplied externally through CPATH; LeakSanitizer
is disabled under ptrace while address/undefined checks remain enabled.
These are native CPU/build checks, not a full executable link or audible
gameplay comparison. The next configured test is repeated gossip-stone dialogue
at night with the reported packs and music selection, plus fanfare/cutscene,
dawn and checkbox controls. Existing cumulative fixes remain inherited.
