# Seasonal bugfix candidate: resident Epona and Zora water camera

Baseline: `d3cf01741084c188eb0a01a263018fa3de23fbe1`, the immutable
`candidate/seed-fill-regressions-20261110` source underlying the seasonal work.
This change does not claim recovery of the missing seasonal source commit.

## Epona softlock

Reported route: ride Epona from Great Bay through the regional passage to Lake
Hylia, explore OoT, then enter a shuffled route to MM Pirates' Fortress. Epona
remained mounted over water, preventing dismount, ocarina or cycle escape.

MM's module remains loaded during OoT play. `gHorseIsMounted` is a process-global
marker, and `MM_ResumeGame` previously did not clear it before reconstructing
Player. The native overworld horse spawner checked only the scene layer. Its
mounted branch did not check the supported-scene table, so any layer-zero scene
could inherit the stale horse, including Pirates' Fortress.

The candidate clears transient horse markers on every Combo MM engine resume.
It preserves the Epona song, quest ownership and saved parked-horse position.
The native overworld spawner now rejects unsupported scene/layer pairs, clears
an invalid mounted marker for nonhuman players or missing song ownership, and
checks actor allocation before calling the native mount function. Native
minigame spawns remain on their original path.

Verification:

```sh
python3 tests/transition_horse_safety/run_tests.py --sanitize
```

Pass: actual production spawn/reset bodies; Pirates' Fortress exterior and
interior and an unsupported courtyard; every scene in the native table;
post-Gyorg Great Bay and unsupported layer; all nonhuman native forms; absent
song; failed horse allocation; transient reset; exact parked-save preservation.
The harness also verifies the real resume function calls reset before entering
the game loop. Engine services use counters; no running game was used.

The same fixture against baseline `--ref d3cf01741084c188eb0a01a263018fa3de23fbe1`
fails because the unsupported destination inherits the mount.

## Zora's Domain crash trace

The user separately reported a crash taking Lake Hylia's Zora's Domain shortcut
in Summer with a drained lake. A definite Zora water-camera bug was found, but
there is no crash stack or exact configured runtime replay connecting that
report to this specific function. Keep that distinction explicit.

The Unbound `WaterBox` layout stores camera, lighting, room and swim flags in
separate fields. The synthetic King-Zora waterbox still used the legacy packed
initializer `0x2104` as its sixth field. That set camera index 8452 rather than
camera 4, light 1, room 1 and swimmable. `WaterBox_GetCameraSType` then indexed the
loaded camera array without checking its length. The candidate restores the
decoded field values and rejects negative/out-of-range indices and null/empty
camera tables.

Verification:

```sh
python3 tests/transition_horse_safety/water_camera_test.py --sanitize
python3 tests/season_water/lifetime_test.py soh/mods/season_scene.cpp --sanitize
python3 scripts/diagnostics/run_zoras_domain_thaw_tests.py --sanitize /tmp/seasonal-zora-thaw-transition
```

All pass. The camera fixture compiles the actual engine `WaterBox` layout,
initializer, legacy unpacker, water lookup and camera-read functions. It checks
the legacy-properties equivalence, synthetic Zora lookup, ordinary Summer-dry
lookup, valid camera selection, and invalid/missing camera metadata. Baseline
`--case initializer --ref d3cf01741084c188eb0a01a263018fa3de23fbe1` fails the packed
field comparison. Baseline `--case bounds` at that ref produces an ASan SEGV
reading camera 8452. Current source passes ASan+UBSan.

The existing water-lifetime test confirms registered Play destruction restores
Summer's original surfaces before memory can retire. The native Domain thaw
suite passes both standalone SoH and Combo configurations across age, season,
legacy-ice settings, collision, caustics, and save immutability.

## Remaining runtime acceptance

Replay the reported mounted cross-game route, then return to supported MM
outdoors and verify parked Epona can still be summoned/mounted. Also verify
ordinary same-game mounted travel and Gorman/Romani minigames. Replay the drained
Lake shortcut with the user's exact save, entrance layout, packs, Alt setting
and age; a further crash requires its log/stack because the confirmed camera
fault alone does not establish the cause of that precise route.

No model, animation, water geometry, scene/setup, entrance permutation, quest
flag, save schema or bomb-wall data is changed. No complete executable build,
gameplay acceptance or publication is claimed. Revert these owned source/test
paths to the baseline for rollback; other workers' fixes are independent.
