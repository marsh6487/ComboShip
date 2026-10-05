# Forest Temple Compass information review

| Field | Record |
| --- | --- |
| Baseline | `9eff802cbf678047f72bfaa28ea00f999780857b`, including the Grace/hourglass and Slate title section; published PR #34 baseline is `1147a1a8`. |
| Candidate | `poc/compass-information-20261005-correction`; isolated `work-compass` worktree. |
| Scope | Diagnose the Forest Temple Compass receipt in MM, verify native seed-setting boundaries, and block generation/plando reload from the dormant OoT file-select while MM is foreground. |
| Preservation | Native tutorial bodies, appended ordinary/masterful information, actual placed rewards, grants, receipt source append, save state, Start With ownership, Well/Ice exclusions and pedestal progression requirements. |
| Configuration | Clip is an English receipt in MM with Save Editor visible. Its seed settings, build identity and asset configuration were not supplied. Test fixture uses an enabled/generated OoT seed with dormant OoT, no OoT PlayState, and an active MM randomizer save. |
| Evidence | Direct inspection of the three supplied clip frames; exact Forest Temple Off/On donor and receiver tests, plus the full receipt/dungeon-information/altar/tracker runner. The user subsequently reported generating a new seed with the information checkbox On and still receiving generic text. |
| Verdict | The reported enabled runtime route remains broken. Isolated donor/receiver checks pass and do not reproduce that complete route; no production root cause is established by them. This is not an in-game acceptance or master promotion. |
| Recovery | Baseline remains unchanged; the candidate changes only the shared foreground file-select predicate in ComboMenu and adds focused tests. Root publishes the coherent commit within the authorized checkpoint workflow. |

The clip shows the native Compass tutorial followed by the Forest Temple
Compass title, without a boss/reward paragraph. The clip does not establish
its saved option value. The
attached log begins mid-session and contains no compass option evidence; its
visible seed ID must not be treated as proof of the clip's configuration.
The user's later report explicitly describes a newly generated seed with the
checkbox On and the same missing information. An Off explanation is therefore
insufficient for the reported setup; the complete generation, saved-setting,
donor hydration and receipt sequence must be investigated.

The checkbox is **Randomizer → Hints/Traps → Static Hints → Maps and Compasses
Give Information**. Its default is Off. Receipt and pause code reads the
generated/loaded save's option in the randomizer context, rather than the live
menu CVar. Checking it changes generation settings; it does not retrofit an
already loaded file. Old save arrays and seed snapshots without this appended
option intentionally load it as Off.

The new donor fixture exercises Forest Temple Compass with saved Off and On
while OoT is dormant. Both retain the complete native message-table body and
the seed's masterful suffix. On appends Phantom Ganon and its placed Deku Leaf
reward. Off contains neither boss nor reward information.

The receiver check links the actual extracted OoT production exporter as a
separate shared library to the actual MM receipt implementation and real item
catalogs. It exercises `RI_OOT_COMPASS_FOREST_TEMPLE` and a foreign item named
Forest Temple Compass. Off → On → Off across seed generations produces equal
native/foreign bodies, keeps the full tutorial and MQ suffix, refreshes the
foreign-check cache, preserves the bank-source append and leaves save/grant
state unchanged.

Validation commands, from the candidate worktree with the dependency CPATH
specified by the task:

```sh
python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py --sanitizers
git diff --check
```

Both receipt commands and the diff check completed with exit 0. The existing
song-renderer fixture reports unused-variable warnings; the new donor library
is compiled with warnings treated as errors. An initial sanitizer attempt
without `ASAN_OPTIONS` stopped in the unchanged icon fixture when LeakSanitizer
could not inspect `/proc`; the listed command is the successful rerun.

The receipt runner also covers native/foreign descriptions and grants, frozen
receipt identities, all ten concrete OoT small keys, four MM boss rewards,
Start With pause ownership, foreign reward names, old/new seed/save settings,
Well/Ice exclusions and pedestal/tracker reward-hint replacement while keeping
progression requirements. LeakSanitizer cannot inspect `/proc` under this
environment's tracing; the sanitizer command disables only leak detection and
retains AddressSanitizer and UndefinedBehaviorSanitizer.

The clip SHA-256 is
`31f4359ecb09a7dbd4fc0cc154127a87bdaf2bed25be320ebff6642e9db0be25`.
No runtime test was run in the user's game, no active save was edited, and no
seed setting was silently enabled. The enabled runtime failure reported after
these initial tests remains unresolved. The active save/seed and build identity
have not been supplied; the tests above replace runtime context ownership with
fixtures and must not be described as proof of the generation/load pipeline.

The native setting probe now uses the actual CVar-backed Option constructor,
registration/default, complete SetAllToContext and FinalizeSettings bodies,
native Context/Logic/dungeon declarations and constructors, complete settings
dump/restore exports, and SaveManager's real array traversal/templates. Off →
On → Off → On survives MM-start preparation, seed snapshots and save-array
round trips with an opposing live menu. Missing old keys/array entries default
Off. This rules out ordinary omission at these tested boundaries; the fill
worker, actual disk sections, module binding and game loops remain outside
this probe. Removing the appended option from the actual copy loop is a
behavioral negative control that fails at its first enabled generation.

A separate concrete menu defect was reproduced. MM-first handoff runs the
native SOH_ParkForComboMMResume, leaving gPlayState null and the dormant
GameState main pointer equal to FileChoose_Main. Native SOH_IsOnFileSelect
therefore still returns true. Both complete production menu panels previously
accepted that query while MM was foreground and invoked Generate/Save mutation
callbacks. The candidate requires OoT foreground as well as the native file-
select query in both panels. The test executes both complete panels and native
park/query bodies, with rendering and worker/reload callbacks as observers.
Baseline fails four MM-foreground assertions; candidate passes true OoT file
select, foreground return, gameplay, null/other state, missing exports and busy
generation controls. This defect is **not** established as the cause of the
reported freshly generated enabled receipt.

Additional commands (normal and AddressSanitizer/UndefinedBehaviorSanitizer):

```sh
python3 -B tests/item_receipts/run_mm_first_seed_gate_test.py
ASAN_OPTIONS=detect_leaks=0 python3 -B tests/item_receipts/run_mm_first_seed_gate_test.py --sanitizers
python3 -B tests/item_receipts/run_mm_first_seed_gate_test.py --baseline 23a10094e51abbb39c03ec4d65dd32d4e55880fb
python3 -B tests/item_receipts/run_seed_settings_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 -B tests/item_receipts/run_seed_settings_tests.py --sanitizers
python3 -B tests/item_receipts/run_seed_settings_tests.py --drop-appended-option
```

The baseline and drop-option commands intentionally exit 1 at the named
behavioral assertions; current normal/sanitizer commands exit 0. These tests
do not run either game or demonstrate the user-facing layout. Subsequent user
steering explicitly replaces the enabled compass tutorial with title, shuffled
boss and actual reward name plus a sprite beside the reward line in one box;
the previously passing tutorial-preservation On controls are historical
evidence, not acceptance of that revised layout.
