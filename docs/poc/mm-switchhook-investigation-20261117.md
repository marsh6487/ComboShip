# MM Switch Hook investigation

User report: eligible targets pulse blue while holding the Switch Hook, but
attempting to use it does not swap positions. The follow-up identifies a broken
first-person camera: Link freezes and the camera tries to return to third person.
A native weapon-aim retention defect is reproduced and corrected below. Full
configured-runtime swap acceptance remains pending.

## Source and runtime identity

The uploaded `Fleet of Harkinian.1(2).log`
(`libfile_3fdabf7d7a948191872a9557e90ac668`) identifies the actual MM build at
line 704 as `ComboShip v0.4.0 | Combo Build e1 soh-ecd889c mm-e8757c1`, commit
`45dbd6901c6d59342ff28384cf4e8123112d8d2e`. This is distinct from the separate
standalone SoH model-test log. The exact commit is retrievable from the public
repository and was fetched read-only for comparison.

The uploaded runtime's following source files are byte-identical to the
prepublication source baseline `d3cf01741084c188eb0a01a263018fa3de23fbe1`:

- `mm/src/overlays/actors/ovl_Arms_Hook/z_arms_hook.c`
- `mm/src/overlays/actors/ovl_player_actor/z_player.c`
- `mm/mods/extended_player.c`
- `mm/mods/items/logic/item_switchhook.c`
- `mm/mods/items/helpers/target_select_helper.c`

It is therefore incorrect to say that simply using this source baseline adds
an already implemented Switch Hook fix missing from the uploaded executable.

## Active behavior traced

The custom projectile state machine in `item_switchhook.c` is bypassed. The
Switch Hook aliases `PLAYER_IA_HOOKSHOT` and the native `Arms_Hook` actor owns
target selection, launch, swap, held-actor links and recovery. While parented to
Link, `ArmsHook_Wait` scans and tints its current live target blue. That feedback
does not prove that the player's native hookshot launch/release gate ran.

On launch, the player clears `heldActor`/`actor.child` and the hook's parent;
`ArmsHook_Wait` then resolves a fresh live target, checks actual reach, consumes
a charge and calls `ArmsHook_StartSwap`. The target and Link are teleported and
held for two frames, with collider reanchoring; recovery restores native held
actor links. Empty charges and out-of-range targets restore held state without
swapping. Manual C-Up aim uses a separate ray selection; ordinary selection
intentionally ignores height, while the firing range check does not.

## Confirmed manual-aim defect and correction

The previous C-Up handler set `unk_AA5 = 3` through `func_80831010`, but never
entered the native hookshot upper-body aiming action. An idle held hook has
`PLAYER_STATE1_8` set and `unk_ACC = 0`. The native `Player_Action_43` immediately
rejects that state because `func_800B7128` requires both the held-weapon flag and
a nonzero `unk_ACC`. It exits and clears `unk_AA5`. The custom handler reapplied
`unk_AA5 = 3` on every subsequent update, repeatedly reopening the rejected
camera. The blue target selection independently continued in `ArmsHook_Wait`.

The new native-source fixture reproduces this on the baseline's first aiming
update: `native exits=1 AA5=0 ACC=0`, followed by the retention assertion failure.
The prior passing swap fixtures do not exercise this native aiming lifecycle.

The correction is confined to MM's active Switch Hook manual-aim handler. C-Up
first uses the native ground/camera admission and hookshot aiming gates, then
calls `func_80830E30` once. That installs native upper action 7 and seeds its
ready timer. Existing upper actions 7/8 retain the camera and process the
ordinary equipped-button shot. The handler consumes the C-Up edge so toggling
out cannot immediately reopen native C-Up peek. It no longer forces the camera
state every update; native cancellation can complete.

Manual ownership is cleared on item/action changes, native aim exit, blocked or
damaged states, cutscenes, transitions, A/B/R cancellation and player lifecycle
reset. Cleanup preserves a different item's newly installed aim. The common
cleanup/reset calls live in `custom_items_stow.c` alongside other transient tools.
No native player, ordinary Hookshot, OoT, charge, magic or target-selection
production behavior is changed. There is no magic cost in the active swap path;
the existing five-charge mechanism remains in place.

The Switch Hook retains its existing child-age inventory requirement in both
source hosts. The user did not identify the age used for the Switch Hook report
(both ages were specified for the separate Beetle camera report).

## Verification and limits

With `ASAN_OPTIONS=detect_leaks=0` and the project-pinned JSON include directory,
these actual-source fixtures pass ASan and UBSan:

```
python3 -B tests/mm_nei/run_switchhook_aim_tests.py --sanitize
python3 -B tests/mm_nei/run_use_tests.py instant_switchhook --sanitize
python3 -B tests/mm_nei/run_use_tests.py manual_hook_live_targets --sanitize
```

The new aim fixture executes production manual input, native admission,
upper actions 6/7/8, actual held-hook release, camera-mode selection, C-Up action
handler and `Player_Action_43` retention with native MM structures. It retains
manual aim for 30 updates, turns the aim, releases the hook through the native
button state machine, consumes C-Up without native peek, checks lock-on/focus/
camera/airborne admission rejection and checks cancellation plus ordinary
Hookshot admission. The same fixture fails against `d3cf0174` at its first native
aim update. Rendering/audio, high-level equipped-button dispatch, engine camera
mode changes and selected unrelated engine services remain fixture boundaries.
This is stronger than the previous component checks, but still is not a running
MM scene reproduction or configured renderer/camera acceptance.

The instant-swap fixture exercises the production hook launch branch and held
recovery with selection/swap services mocked. The manual-target fixture executes
actual MM manual-ray/live-list validation. Full `z_player.c`, including the new
handler and lifecycle calls, also passes the real-header native syntax check with
both original and `COMBO_BUILD` guards. These are not linked executable builds.
LeakSanitizer is disabled because it cannot run under this environment's tracing;
address and undefined-behavior checks remain enabled.

Runtime acceptance must still confirm that C-Up enters a controllable camera,
releasing the equipped button swaps the manual target, cancellation returns
normal movement/camera control, and ordinary blue-selection swaps work without
manual aim. The reported general swap failure remains open until that run. No
publication, executable packaging or deployment was performed.
