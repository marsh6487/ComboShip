# Jabu skipped feeding route

Baseline: local `19718cda`, public `4e77cd70`, identical tree
`e38fd76633d9fdedb414ca5c42284a055990bb55`. Prior Graveyard, soul,
equipment, camera and audio repairs remain intact. No savewarp behavior changes.

The newly supplied `Fleet of Harkinian (5).log` continues the previous rotated
file and records build `ec2335e`, which contains the normal-cutscene repair.
Its SHA-256 is
`0234c49ab6b487477ac52d7e3d9a7ef97deffe5520d5a7da26ae3e29b9f893bf`.
At 21:43:06.574 it sets fish-offered flag 0x3A; at 21:43:07.779 it loads
scene 2, entrance 0x28, without any entrance-resolver trace. The same bypass
occurs at 21:33:58/59 and 21:49:15/16. Regular exits do record resolver traces.
This supersedes the earlier explanation based only on the older build's attempt.

`Skip Misc Interactions` handles feeding through a separate
`SkipJabuFeedingCutscene` helper; its default is enabled in randomizer.
That helper retained the native-entrance-shuffle guard removed from normal
cutscene case 24. Cross-game routing can be active with native shuffle off,
so this shortcut still assigned Jabu's vanilla entrance directly.

Remove only that native-shuffle guard, matching the existing normal-cutscene
and regular-mouth paths. Preserve bottle consumption, fish flag, chime,
transition trigger/type, nonrandomizer behavior and disabled/ineligible hooks.

The expanded production-code regression failed before this correction in
combined randomizer/native-off/cross-only mode: actual 0x28, expected 0x100.
After correction, all 96 normal/skipped/regular-mouth routing cases pass across
standalone and combined builds, including local/foreign targets, native shuffle
on/off, single resolver application, pending foreign handoff and hook guards.
Both ordinary and ASan/UBSan runs pass. Configured gameplay remains untested.

The separate hosted rendering failure was an obsolete Twinrova fixture include:
the extracted owner recipe now uses `gBossSoulFlameDL`, declared in
`soh_assets.h`. Include that production declaration and assert the dedicated
flame identity. Retain all existing head/hair/replacement checks. Ordinary and
sanitized Twinrova and MM imported-boss bridge tests pass locally.
