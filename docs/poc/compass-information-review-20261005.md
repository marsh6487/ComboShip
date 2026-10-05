# Forest Temple Compass information review

| Field | Record |
| --- | --- |
| Baseline | `9eff802cbf678047f72bfaa28ea00f999780857b`, including the Grace/hourglass and Slate title section; published PR #34 baseline is `1147a1a8`. |
| Candidate | `poc/compass-information-20261005-correction`; isolated `work-compass` worktree. |
| Scope | Diagnose the Forest Temple Compass receipt in MM and verify the seed setting through the real OoT export and MM native/foreign receipt consumers. Tests and this evidence note only. |
| Preservation | Native tutorial bodies, appended ordinary/masterful information, actual placed rewards, grants, receipt source append, save state, Start With ownership, Well/Ice exclusions and pedestal progression requirements. |
| Configuration | Clip is an English receipt in MM with Save Editor visible. Its seed settings, build identity and asset configuration were not supplied. Test fixture uses an enabled/generated OoT seed with dormant OoT, no OoT PlayState, and an active MM randomizer save. |
| Evidence | Direct inspection of the three supplied clip frames; exact Forest Temple Off/On donor and receiver tests, plus the full receipt/dungeon-information/altar/tracker runner. |
| Verdict | Existing enabled receipt code passes the focused compiled checks; no production change is justified by the available evidence. This is not an in-game acceptance or master promotion. |
| Recovery | Tests are reconstructed from current source; production source and baseline remain unchanged. Root publishes the coherent commit within the authorized checkpoint workflow. |

The clip shows the native Compass tutorial followed by the Forest Temple
Compass title, without a boss/reward paragraph. This matches the existing
**Off** route, but the clip does not establish its saved option value. The
attached log begins mid-session and contains no compass option evidence; its
visible seed ID must not be treated as proof of the clip's configuration.

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
seed setting was silently enabled. If the saved seed option is confirmed On
and the receipt still omits reward information in game, that is a different
reproduction requiring the actual save/seed and build identity.
