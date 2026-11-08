# UI control input and Debug Warp night music — POC 1

## Status and scope

Source corrections and focused native regression checks are implemented. This checkpoint does not contain a rebuilt game executable. Configured in-game behavior, audible output, and user acceptance remain pending. It has not been merged or promoted to an accepted master.

The reported issues are: True Hidden UI L input changes visibility when paging inventory; startup can appear black while the application menu remains reachable; and the scene-night-music option does not start in a fresh Kokiri Forest Debug Warp session.

## Baseline and recovery

Work is isolated in `candidate/ui-control-input-20261108`. The prior full source directory has a stale Git worktree link and was left intact. The isolated copy uses a bounded Git snapshot, not reconstructed public repository ancestry. Local baseline `c821464` includes the unchanged affected source boundaries. The bounded UI commit is `ed716879a4f2d2243cab78d7e8589b65221a322c`; the bounded audio commit is `601ad917b66a9c25a26fecac73442f2396291031`. These local snapshot commits are not descendants of the public repository commit.

The original 201-file cumulative checkpoint was independently matched against the original source directory. Its publication receipt identifies public commit `008534e3b1abcdfa8805a555de8d4639991b740f`, tree `355ef7377653c3a6de7d8ebea4483fb7716939b7`, and local equivalent `70e30dcc86af91fbd7a8ce7b5e6eaa30960c2f61`. The checkpoint archive SHA-256 is `9785095c8025787f0d4487d0f9972ccaac5aaa8457c4a32774c98ce40371ff2d`. These are historical receipt identities, not a claim about the current remote branch or CI state.

The recovery package contains the changed files, a patch, before/after SHA-256 values, validation logs, and an application script that validates every affected baseline before writing. It adds no external asset dependencies and does not contain saves, user game logs, or asset packs. Apply it to the matching Nov 7 cumulative source or a checkout whose affected files match the manifest. Rebuild using that source tree's normal platform instructions.

## L input ownership

Both native `GameState_UpdateUiVisibility` handlers now require a fresh controller-one L press in the current, running `Play_Main` state, normal game mode, and a live player actor. They reject pause/debug inventory, dialogue, game over, transitions, frame advance, actor/player freezes, zero health, native disabled-input flags, and native cutscene/control-blocking predicates. Simultaneous Start+L is reserved for entering the pause menu. The OoT developer/cheat L conflict policy is retained.

Rejected presses are neither consumed nor queued. Inventory paging and dialogue receive the original input bytes. Holding L while closing inventory does not toggle on return; a fresh gameplay press is required. The chosen hidden/shown latch persists across inventory/dialogue. Existing Minimal UI and minimap behavior remains covered by its regression fixtures. No map availability or real-save-slot requirement was added to the UI toggle.

The tests execute the real input handler plus the real `Play_InCsMode`, `Player_InCsMode`, and `Player_InBlockingCsMode` bodies against native engine headers. The OoT spell classifier is also real. Both initial visibility states are exercised; swimming, shielding, sword use, and no-minimap gameplay remain eligible.

## Startup/navigation rendering

The previous global draw suppression also hid the startup logo, title prompt, file selector, and debug scene selector. The visibility latch starts hidden, so enabling True Hidden UI could make those navigation screens appear black. A gameplay-only L gate must not leave those screens dependent on an unavailable recovery input.

The correction removes suppression only from those navigation draw boundaries in both games. It retains their native updates, transitions, fades, display-list marker balance, and allocations. It does not reset the selected gameplay visibility state. Gameplay HUD, pause inventory, dialogue, scene cards, and MM dawn-card suppression are preserved. Both menu tooltips explain the control requirement and visible startup navigation.

The source explains one route to the reported black startup; no configured reproduction of the user's executable was performed. Other causes of black output are not diagnosed by this checkpoint.

## Kokiri Forest night music

`soh/soh/Enhancements/audio/SceneNightMusic.cpp` previously required `fileNum < 3`. A fresh Debug Warp session uses the temporary file value `0xFF`, so that session was ineligible even when the option was enabled. There is no randomizer-save requirement in this adapter.

Eligibility now accepts real slots or `0xFF` while still requiring `GAMEMODE_NORMAL`, a valid scene music source and nature ambience, and the existing dedicated-scene exclusions. Title, file-select, and credits modes stay excluded. No song-selection, queue, ownership, or MM audio policy was changed.

The relevant setting is **Enhancements → Audio Editor → Audio Options → Enable Night Music** (`gAudioEditor.EnableNightMusic`, default off). It lets eligible outdoor areas use their configured daytime music at night. The separate Hyrule Field Night Music option does not enable Kokiri Forest. The debug regression uses Kokiri scene `0x55`, music `0x3C`, ambience `4`, night time `0`, and temporary slot `0xFF`.

## Verification

The changed inventory-input and navigation expectations failed against the original production bodies before their fixes. The fresh-debug night adapter also failed against its original save-slot filter. Removing the cutscene gate from temporary test copies causes the native blocked-player checks to fail in both games.

Fresh final checks:

| Check | Result |
| --- | --- |
| Native input dispatch, OoT original / Combo | 196 + 196 checks pass with ASan/UBSan |
| Native input dispatch, MM original / Combo | 172 + 172 checks pass with ASan/UBSan |
| Focused OoT UI suite | Pass, both build guards; input, gameplay suppression, startup/title/file/debug selection, fades, allocation and marker behavior |
| Focused MM UI suite | Pass, including 46 full native syntax checks under original and Combo guards |
| Full changed `game.c` object compilation | All four OoT/MM original/Combo objects compile with the existing project's compatibility flags; MM emits existing button-macro redefinition warnings |
| Night lifecycle and production adapters | Pass with ASan/UBSan for normal OoT, temporary Debug Warp OoT, and MM, including dialogue/cutscene, combat/fanfare/preview ownership, disable/dawn, scene change, and pending-command behavior |
| Full UI umbrella | Blocked after the passing native input tests: ImGui source directory is absent |
| Full night-music runner | Blocked after the passing adapters: unchanged dedicated Hyrule Field unit cannot find `nlohmann/json.hpp`; its later row-order stage is not reached |
| Independent production review | No blocking or important defects; reviewer independently passed all 736 input checks and MM input/menu checks |
| Diff whitespace check | Pass |
| Full executable build and configured game runtime | Not performed |

Commands from the source root:

```sh
python3 -B tests/ui_visibility/run_control_input_tests.py
python3 -B tests/ui_visibility/oot_run_tests.py
python3 -B tests/ui_visibility/mm_run_tests.py
python3 -B tests/ui_visibility/run_tests.py
python3 -B tests/night_music/run_tests.py
git diff --check
```

The last two Python runners retain their real dependency requirements and fail rather than silently skipping unavailable checks. The full GUI dependency suite and platform build must be run in the project's equipped build environment. `clang-format-14` is not available locally; no automated formatting pass is claimed.

## Configured acceptance still needed

1. With True Hidden UI enabled before boot, confirm the logo/title, file selector, and debug selector remain visible in both engines.
2. While controlling Link, press and release L to hide/reveal UI. Open inventory while shown, page with L, then close while holding L; visibility should stay shown until a new gameplay press. Repeat from the hidden state and during dialogue/cutscenes; menu presses must not alter the latch.
3. Start a fresh OoT Debug Warp session, choose child/night Kokiri Forest, enable Enable Night Music, and confirm the selected track is audible. Check disabling it and moving to a new scene. Repeat with a normal save as a control.

A source-test pass proves these exercised boundaries; it is not audible or configured runtime proof. No unrelated equipment, asset, or broader performance investigation is included.
