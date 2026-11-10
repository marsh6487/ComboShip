# Shared Magic Mirror controls: integration handoff

## Source and preservation

Baseline: `4608e863`, following published R21. The reviewed source sequence is
`c77cb40e`, `874ed473`, `ecb96bb4`, `ded3cff5`, followed by this documentation and
formatting closure. Controller integrates into the current cumulative candidate;
this handoff does not replace that candidate or promote master.

Both existing Combo game-tab controls now edit one effective Mirror setting,
including OFF. Native MM cards, flat OoT widgets, presets and bulk controls use
that setting. Solo preferences stay separate, and the solo launcher branches to
the native menus before the unified Combo menu is installed. No additional
global control, item/option ordinal, save-layout change, asset or travel behavior
is introduced.

Preview follows the live shared preference; native generation and replay follow
the frozen snapshot. Both native seed policies apply OFF explicitly. Import and
replay normalize both native snapshots before restoration. Only legacy inputs
without a canonical preference use the previous OR migration. Temporary save
creation restores the user's newer UI preference after using the seed settings.

## Decisive recovery verification

The interrupted handoff lacked the final real-ImGui GREEN result. Recovery ran
these focused checks with the retained GCC13 sysroot/native ABI:

| Check | Result |
| --- | --- |
| `tests/shuffle_parity/ui_refresh/run_tests.py --fixture tests/shuffle_parity/ui_refresh/native_test.cpp --sanitize` | PASS original and Combo. Executes actual ImGui mouse input, native MM card/CVar widget, ShipInit dispatch, native pools, exclusions, rings and accents. For all four native solo-preference combinations, Combo OFF→ON→OFF gives exactly +1/+1 and restores the original item/check counts and solo CVars. |
| `tests/magic_mirror_shuffle/run_shared_settings_tests.py --sanitize` | PASS original and Combo. Executes the production widget adapter and shared policy, both tab callbacks, OFF, presets, migration and both snapshot restore orders. |
| `tests/magic_mirror_shuffle/run_option_tests.py` | PASS actual normalization, both native seed policies, snapshot/restore bodies, legacy migration and catalogue identities. |
| Pinned clang-format 14.0.6 | PASS all 16 changed production files; recovery corrections are whitespace only. |
| `git diff --check` | PASS. |

The retained pre-fix real-MM-card control fails its shared-setting assertion,
while the recovered current source passes. Earlier completed evidence also
covers full native MM pools, solo boundaries, config import, Grace and Seasons
pool compatibility, and affected OoT/MM translation units under original and
Combo guards. Launcher, unified-widget and headless translation units passed
with their production compile definitions. Compiler warnings already present
in native headers were retained rather than presented as new failures.

The new shared-policy sanitizer gate is registered in the existing `entrances`
row named `Magic Mirror combined route logic policy snapshots and queue
invalidation`. The native real-ImGui fixture remains in its existing required
CI row. Preserve other cumulative CI additions when integrating this branch.

## Evidence and remaining boundary

The controller receives the original RED log and recovery GREEN logs:
`shared-mirror-real-ui-red-final.log`,
`shared-mirror-real-ui-green-recovery.log`,
`shared-mirror-policy-green-recovery.log`, and
`shared-mirror-native-green-recovery.log`.

Prior durable source checkpoints already preserve each implementation milestone.
The controller must include this final source and evidence in the cumulative
independently restorable checkpoint before publication. This is implemented and
focused-test verified; it is not a claim that the newer complete game modules
have linked, that every cumulative regression has rerun, or that configured
gameplay has been accepted. Rebuild both games, launcher and UI together with
the rest of the current integration.
