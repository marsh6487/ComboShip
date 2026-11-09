# ComboShip shuffle parity integration

## Authorized result

Implement on `candidate/ootmm-shuffle-parity-20261108`, starting at PR #88 commit
`ced18542387d5ac8879e0f1cb303e790e4e3a1a8` (tree
`ab1a683f2c91fd14a039cc58800a112ae92966b9`). The user explicitly authorized
implementation, incremental public WIP checkpoints, a dedicated draft PR,
durable recovery packages, and scoped regression/CI repairs on November 8, 2026.
No merge, master update, or implication of configured runtime acceptance.

### Required features

- NPC souls in both games, native OoT enemy souls, and OoT first enemy-type drops;
  retain the existing MM enemy-soul and first-drop implementations.
- Shared souls and animal/miscellaneous soul families.
- Native MM dungeon key rings; rusty overworld-door keys in both games.
- Song-event shuffle in both games, distinct from song-item shuffle.
- MM rock/boulder/red-boulder/icicle/soil environmental check families.
- Separate MM lottery rewards for each night; OoT clock item.
- Functional MM Deku Stick Bag, Deku Nut Bag, Child Wallet, Infinite Upgrades.
- Clear item-pool UI grouping, preserving settings, with implemented placeholders removed.
- Every new check in exclusion controls with correct location/category labels;
  saved/imported exclusions honored by generation and Glitchless validation.
- MM Saria's Song/Desired Items parity for Progressive Magic and RPG upgrades,
  including candidates, selection, persistence, actual location and ownership resolution.
- Integrate the exact independently authored native form-instrument source candidate:
  Deku/Goron/Zora instruments and MM banks in OoT; Fierce Deity uses the ocarina;
  preserve native MM behavior. Locate and verify its actual commit/dependencies.
- User follow-up: reproduce/check Stone Tower Boss Key placement with BOTH games'
  boss keys set to Own Dungeon, including combined generation/cross-game placement.

## Preservation and semantics

New shuffles default off. Preserve baseline IDs, old saves, setting identities,
solo/combined guards, cumulative integrations and accepted resource paths.
Keep MM's existing enemy immunity behavior. NPC soul filters must use actor roles
and parameters, not blindly hide generic decorative cutscene actors.
Every capacity, soul, key, song and time gate must match the actual solver path.
Glitchless never assumes a disabled trick. Own-dungeon restrictions must survive
combined filling and foreign conversion. Excluded checks never receive required progression.
Prevent self-locking placement and false room completion; preserve MM reset ownership.

## Evidence and recovery

CPU policy, actual native production fixtures, seeded generation, full object/build
checks and configured game testing are separate evidence levels. Record failures
honestly. Each independently verified milestone gets a bounded source commit and
recovery record before long builds/publication. Publish only source/tests/docs;
external packs, ROMs, private recordings, saves and credentials remain excluded.

## Reference

OoTMM `0c0f8a8e1412f3bc979e77a2563293770d58a439` is checked out at
`../OoTMM`. The source audit is `../deliverables/ComboShip_OoTMM_Shuffle_Gap_Audit_20261108.txt`.
The current source baseline ZIP is `../deliverables/ComboShip_Shuffle_Baseline_20261108.zip`.


## Visual follow-up confirmed during implementation

Soul and first-enemy-drop hints must color the named enemy/source and reward
item using each host's established hint colors, with correct resets and actual
encoded controls. Tests must cover the native encoder and composed clue, not
just visible marker characters. Existing hint color settings and translated
names remain intact.

All new soul get-item rendering, including native and cross-game receipts,
must use the existing owning host's boss-soul flame material and resource
handles so the previously authored upscaled flame packs apply. Preserve
existing boss/enemy models and palettes, new item identities and native
scrolling. Do not substitute arbitrary boss indices for new souls.

The retrieved POC5 alpha-safe package confirms exact override resources:

| Host | Native texture paths (Alt overrides prepend `alt/`) |
| --- | --- |
| OoT | `objects/object_gi_fire/object_gi_fireTex_000000`, `objects/object_gi_fire/object_gi_fireTex_000200` |
| MM | `objects/gameplay_keep/gameplay_keep_Tex_01A8B0`, `objects/gameplay_keep/gameplay_keep_Tex_01AAB0` |

Source provenance: `BossSoul-Flame-Alpha-Repair-POC5-20261020.zip`, SHA256
`8444ff614a508679bf0f3703cd52481a41de590df6321ca20a735ffab1876ded`.
Archive resources are local audit inputs only and must not enter the public
source checkpoint. Callback/resource-path tests are distinct from configured
GPU/mod-stack acceptance.
