# Deku bubble regression probe

Baseline: ComboShip public PR 90 `f3931e973c5d4dd55890822295dc77cf15dbfbdc`;
isolated source snapshot `e46924e`. No game archives are stored in this test.
The existing single tracked projectile is retained; surface splats and fixed
impact emitters each have eight bounded slots.

Run `python3 tests/deku_bubble/run_tests.py`. Add `--native-headers` in a complete
checkout to compile the entire effect and projectile against SoH and Combo
headers. The behavior cases compile actual production function bodies with
ASan/UBSan; collision queries, queue reset/submission, resources, audio output
and graphics are engine service boundaries. This does not run either game.

The resolver and native player handoff cases execute `ResolveItemOnButton`,
`Player_GetItemOnButton`, `Player_ProcessItemButtons`, `Player_StartDekuBubble`,
`Player_DekuBubbleCleanup` and upper actions `func_8083442C`, `func_808351D4`,
`func_808353D8`. They preserve the real release/re-nock transitions and MP cost.
The frame-owner case executes the production prefix before action ownership
can return. The collision adapter clears hit flags on submission, matching
SoH's `CollisionCheck_SetAT`; moving the hit check below submission breaks it.

The initial source failed alias, held/released shot, rapid fire, charge-loop
flags, frame ownership, actor hit/reflection, torch/world impact, splat lifetime,
projected audio and cleanup cases. Native acceleration, expiry, insufficient
magic and cancel controls passed before the repair.

Donor references at that exact public commit:

- `mm/src/code/z_bgcheck.c`: `func_800C0094` surface frame. The real output is
  checked for equal dimensions and downhill orientation at +/-X, +Z and floor
  normals; OoT's fallback stretches +/-X tangents by sqrt(2).
- `mm/src/overlays/actors/ovl_En_Arrow/z_en_arrow.c`: `func_8088A514` speed law,
  `func_8088A594` release sound and charge clamp, `func_8088ACE0` deflation,
  qualifying actor reflection, projectile line test, Stone1/SBN impacts,
  VANISH fixed-world emitter and flight refresh loop.
- `mm/src/overlays/effects/ovl_Effect_Ss_Sbn/z_eff_ss_sbn.c`: 93-frame lifetime,
  scale 600..1500, downhill surface frame, five flat/twelve sliding textures,
  slide scrolling, decal render state and final ten-frame alpha fade.
- `soh/mods/sound_translator/mm_audio_sfx.cpp` and `mm_asset_loader.cpp`: the
  current backend uses the native MM SFX bank/Sequence_0; clearing `SFX_FLAG`
  selects refresh-driven BREATH/SHOT_LEVEL. FIRE and VANISH are one-shots.

Runtime acceptance still needs a game build with the same packs/load order:

1. As Deku in the Temple of Time, tap B and release, then hold/release B and
   rapidly re-press it. Each successful shot costs 2 MP once. A exits aim.
   Confirm charging visibility/size near the camera: the inherited first-person
   charge sphere placement and near clipping were preserved, not runtime-proven.
2. Hear inflation, one release cue, flight and impact. Aim at a nearby wall:
   release and pop tails both finish. Repeat with insufficient magic.
3. A fully charged shot starts slowly, accelerates/deflates over frames and
   expires. Turn the camera, leave aim, take damage or use an instrument while
   a shot is already in flight; its independent world lifetime continues.
4. Check enemy hit and hard/shield reflection, torch hit, floor/wall/ceiling
   impact, and dynapoly. Static surfaces receive a native splat; actors and
   dynapoly receive the impact burst without a static decal.
5. Check the flat splat sequence, wall slide and fade with Alt Assets both
   off/on. Pause and redraw: simulation stays frozen; unpause resumes. Change
   scene/save, detransform or die: no stale projectile, decal or audio emitter.
