# Lost Woods ritual: Temple of Time setting parity — POC 2

Implemented source candidate above the UI/shovel/winter POC1. No in-game visual acceptance or master promotion is claimed.

## Requested comparison

Both child and adult Lost Woods stump interactions should reflect the same difference made by **Skip Story Cutscenes** at the Temple of Time. The reference is the repeatable Master Sword time-travel ritual, rather than the one-time Rauru/Sheik acquisition story.

| Setting | Native Temple behavior | Candidate Lost Woods behavior |
| --- | --- | --- |
| Off | Complete child/adult departure script; child terminator frame230, adult frame210; normal white reload transition | Same native script/cue/sound/camera-spline and terminal-frame timing; normal white transition |
| On | Branch after frame20; normal white transition without an extra screen fill | Branch after frame20 directly; no custom eight-update fill; normal white transition |
| Either, after age change | Hold the first arrival pose for 20 completed action updates, then run the native age-specific hop/flourish | Same hold and native player action, adapted to the local stump return |

## Confirmed difference and correction

The previous local automatic skip first ramped an additional environment screen fill over eight updates, using RGB255. It then forced fast white fades. Native script and transition white use RGB160, and the native repeat-travel terminator chooses the normal white transition. This explains a source-level route to the reported brighter, faster flash; no supplied gameplay clip was viewed in this task.

Automatic Story Skip now follows the Temple's frame-greater-than-20 branch directly. It cannot enter the separate local fill ramp. Both ordinary completion and automatic skip use TRANS_TYPE_FADE_WHITE for departure and arrival. The native temporary cutsceneTransitionControl latch is set at the terminal handoff.

The local arrival no longer bypasses the initial pose hold. Its existing native action now performs all 20 completed hold updates before releasing the child hop or adult sword flourish. This restores the native timing with either setting.

The native child/adult cutscene arrays and portable script builder remain unchanged: four departure camera splines, player cues, sword handoff, sounds and the original full-script 20-frame fill remain intact. The old independent fresh-B convenience skips retain their existing behavior; this change targets the Story Skip setting paths.

## Preserved integration

The pedestal remains a repeatable local, progression-free age swap. It stays in Lost Woods room10, preserves the floor-safe return position, selected sword/hand resources, owned age-compatible equipment, B-button repair exception, and normal scene-music restoration. The authored local fixed arrival camera and its identity/ownership/cleanup remain intact; absolute Temple coordinates are not transplanted into the stump.

The native first non-randomizer adult acquisition path can move Link to Sheik and bypass raising the sword. That path also owns story flags, medallion handling and Temple-specific location/camera policy. It is deliberately excluded from this decorative local prop.

All prior hidden UI, Debug Warp night music, shovel and winter changes are retained. The only changed prior POC1 production file is the OoT player source, whose additional diff is confined to the native arrival hold.

## Verification

- Existing broad pedestal fixture initially failed before running because its typedef extractor assumed the old anonymous collision context. The extractor now accepts the current named context. Current native button constants/API boundaries were supplied in the fixture without changing gameplay implementations.
- A genuine nlohmann/json.hpp v3.11.3 header supplied the missing test dependency; no JSON stand-in was used.
- The updated automatic-skip expectation failed on the original production code: no transition was started at frame21 because the extra fill ramp began instead.
- The complete production-function pedestal suite passes after the fix: both ages, normal/randomizer contexts, setting values -1/0/1, frame20/21, native full-script terminal frames, automatic versus independent B skips, exact 20-update arrival hold, progression/equipment preservation, room/return, camera allocation/reuse/teardown and hand/source selection.
- Separate production sword-source/PAK selection suite passes.
- Full changed actor/player C translation units compile to actual objects in both solo and Combo builds: four objects, no full executable link claimed.
- Independent source comparison and final review found no blocking defects. The stale immediate-arrival test comment was corrected.
- The combined source patch and checksum installer are independently replayed from the packaged baseline and above the prior UI/shovel/winter POC1; conflict refusal and idempotence are verified in the manifest.

Reproduce in a complete matching tree:

    python3 -B scripts/diagnostics/run_time_pedestal_tests.py
    python3 -B scripts/diagnostics/run_pedestal_sword_selection_tests.py

Use the normal build dependency include path for nlohmann if it is not globally installed. Native object verification commands and logs are packaged with the source.

## Runtime check

At the Lost Woods stump, test child and adult with Story Skip explicitly off, then on. Compare with repeated Temple time travel under the same settings and player model. Off should retain the complete native ritual; On should move from its brief opening directly into the normal native white fade without the brighter preliminary flash. In both modes, verify the arrival hold, hop/flourish, local framing, music and safe return. Also retain the existing manual fresh-B skip control as a regression check.

The source tests and compilation are verified; final visual matching in the configured game remains pending.
