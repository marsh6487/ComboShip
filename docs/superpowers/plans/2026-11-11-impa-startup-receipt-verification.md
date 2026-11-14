# Impa startup receipt: verification and handoff

## Source identity and outcome

Implemented on `candidate/impa-startup-receipt-20261111` from local baseline
`beba199a5258c1ae36550ab8879f36d079d641fc`, tree
`da99bc7dd7191661e4dae8f829b2a98d2f22602b`. This is the same baseline tree as
published PR90 commit `a5b1abd697d284ecbca70c13091562821dfc3feb`.
The recovery archive records the exact final commit/tree; no remote publication
or master promotion is part of this handoff.

The eligible combined randomized new file starts with Zelda's Letter and leaves
Letter shuffle disabled. Its actual **Song from Impa (OoT)** reward is captured
once, then presented in whichever game reaches safe foreground gameplay. Both
hosts identify the item before its source. The foreground game's existing GI
category/skip policy controls the animation. Skipped GI gives a corner
notification, including junk, without a replacement blocking textbox.

OoT-owned rewards retain the existing starting grant. MM-owned rewards wait for
their matching initialized save and starting inventory. Presentation never grants
inventory or repeats sharing, traps, equipment changes, or item-specific story
effects. The saved frozen identity preserves the awarded progressive tier and
static/animated disguise rather than resolving against the later inventory.

Both native saves and the READY receipt commit together after an MM award. A
fresh post-award OoT snapshot is required after queued saves drain. Native context
replacement is guarded until both awarded snapshots exist. Failed write retries
do not award again. Failed presentation acknowledgements are remembered across
game/scene changes in the running process; a replaced save generation releases
stale retry state. If the process exits before a failed acknowledgement is ever
persisted, the visual receipt can repeat after restart, but an already committed
reward does not. Existing saves without the new record are not backfilled.

## Final integrated verification

The final integrated command exited **0**:

```sh
python3 -B tests/startup_receipt/run_tests.py --sanitize --native \
  --json-include /workspace/scratch/80615aa2b0c7/deps
```

The dedicated gate is registered in `scripts/ci/regressions.json` under the
native item tests. It compiles actual extracted production boundaries with native
types and boundary doubles; it is not a linked application or visual test.

| Boundary | Verified behavior |
| --- | --- |
| Receipt state and serialization | Absent/malformed records, bounded names/numbers, generation changes, MM-only GID_NONE (-1), unchanged unrelated save fields |
| Launcher and real atomic file writes | Matched MM initialization, both native snapshots, fresh peer capture, failed writes and post-award snapshot exceptions, snapshot-only retry, copy/reload/restart, exactly one progressive/shared award |
| Acknowledgement | No replay after success; cross-game suppression after a failed write; idempotent success for the same shown generation; permanent rejection for absent, malformed or replaced generations |
| OoT adapter and actual player receive boundary | Original/COMBO guards, Letter eligibility and story flags, all seven categories and three native skip modes, no duplicate grant/trap/equip/story effect, cancellation, one notification, stale token suppression |
| MM adapter and completion boundary | Both item owners, three importance classes and four native skip modes; native full GI versus skipped notification; actor ownership, cancellation, slot/seed/generation changes, initialized progressive resolution, one grant/share and save-only retry |
| Frozen rendering | First Cane/Progressive Roc/Stone after inventory advances; native concrete MM item; custom static draw callback; animated trap disguise; ordinary rendering dispatch preserved |
| Native context guards | Seven actual OoT pre-overwrite guards reject unsafe replacement and allow a later retry; original build behavior preserved |

ASan and UBSan pass for state, host and MM fixtures. Leak scanning is disabled
because LeakSanitizer cannot operate under this environment's ptrace setup;
address and undefined-behavior checking remain enabled. The OoT boundary fixtures
and native object checks run separately without sanitizer instrumentation. The
host intentionally logs two injected snapshot failures and a blocked context
replacement; these are asserted recovery cases, not test failures.

## Native object compilation

- **12 complete C translation units passed:** player, file choose, PAL nameset,
  NES nameset, z_sram and opening, each in original and COMBO configurations.
  Existing player-source warnings remain; the other five files have no warnings.
- **8 complete OoT C++ translation units passed:** startup adapter, savefile,
  randomizer draw and item_list, each in original and COMBO configurations,
  with zero warnings. This compiles both frozen draw exports in their actual
  producer translation unit.
- **MM:** the complete new adapter compiled in COMBO and standalone
  configurations, and the complete `DrawItem.cpp` object compiled after the
  static and animated frozen dispatch changes. The combined adapter was compiled
  again after adding the native skipped-GI notification.
- Both engine CMake source globs use `CONFIGURE_DEPENDS` and include the new
  gameplay adapter files; no separate source list edit is required.

The OoT full C++ command used staged actual dependencies, including the project's
pinned ImGui `v1.91.9b-docking` headers with its existing imconfig definitions:

```sh
CPLUS_INCLUDE_PATH=/workspace/scratch/80615aa2b0c7/deps:/workspace/scratch/11816b473f06/dins_audio_native_dependencies:/workspace/scratch/2a4e504bb390/native-deps/imgui \
  python3 -B tests/startup_receipt/run_oot_tests.py --native-cpp
```

No substitute production headers or dependency source edits were introduced.

## Affected existing regressions

Passed: vanilla host creation and reward receipts; vanilla MM receipt lifecycle;
cross-dungeon boss launcher behavior; shared-item runtime; native/foreign reward
delivery; MM starting-setting normalization/repair; dungeon pickup importance;
sword-element receipt/decoder; reward GI rendering/resource traversal; MM foreign
model scale/texture ownership; MM mask/import and static GI bridges; and the ten
workflow unit tests. The relevant existing sanitized suites also passed.

Three fixture maintenance changes were necessary to run those existing checks
against this source: declare the new startup boundary in the extracted sword
receipt fixture; update the dungeon-importance fixture to its already-current
production signature/queue API; and add the missing always-enabled
`tests/test_require.h` that the MM settings fixture already included. Launcher
fixtures received only the added globals or preservation/completion stubs needed
by their extracted functions.

## Independent review

A separate read-only review found no remaining Critical or Important source
blockers after the final corrections. Review specifically covered both-save
atomicity, synchronized receipt publication, fresh snapshot provenance, guarded
context replacement, GID_NONE validation, failed/stale acknowledgement lifecycle,
category parity, frozen progressive/static/animated drawing and skipped-GI
notification behavior. The reviewer independently ran `git diff --check`; final
functional/native execution is recorded separately above.

## Remaining verification limits

The full configure attempt stopped because `cmake` is absent. Complete
`BenPort.cpp` and `debugconsole.cpp` compilation reaches the missing upstream
libzip `zip.h`; their changed completion/reset boundaries are covered by the
compiled production-boundary fixtures. No complete launcher object, linked game
binary, or configured in-game run is claimed.

Visual/audio acceptance still requires the candidate in a playable environment:
start once in each game with eligible Letter settings, exercise the native GI
skip categories, and verify the item model, camera, fanfare and item/source text.
The source checks do not replace that runtime evidence. No original user save
was available to reproduce the previously reported missed notification.

## Recovery

`ComboShip_Impa_Startup_Receipt_20261111.zip` contains full changed source files,
their baseline versions, a binary patch, exact tree/commit metadata, verification
evidence and a deterministic recovery recipe. Its verifier restores the included
delta in an independent temporary repository; with the ComboShip repository it
also reconstructs the exact full Git tree using a temporary index/object store.
The archive requires the unchanged baseline repository for a complete checkout.
It introduces no external game assets and includes no user saves or gameplay
recordings.
