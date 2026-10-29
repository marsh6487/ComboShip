# OoT/MM Major chests for masks and stray fairies

Baseline: repaired ComboShip `3cbb22d1f09e85deb206d2b085fc62105bc09fad`.

The user's final instruction is to preserve OoT's existing Major classification for stray fairies and mirror it in 2Ship. Masks also use Major chests. The earlier Minor/small candidate is superseded; its local commits remain available for recovery.

| Reward | Existing 3DS model in both engines | Size with contents-based sizing enabled |
| --- | --- | --- |
| Masks | Major | Large |
| Stray fairies | Major | Large |

With size matching disabled, authored chest sizes remain. Existing style settings, minigame concealment, fallback, and transparency guards remain in effect. No archive update is needed.

The final production change is two entries in MM's visual model table: `RITYPE_MASK` and `RITYPE_STRAY_FAIRY` both use the existing Major body/lid. The original MM size predicate already makes these categories large and is retained. Item IDs, actual reward categories, actor types, collection flags, and delivery are unchanged.

OoT production chest/category code is byte-identical to the repaired parent. Its five concrete fairy rewards and its foreign `strayFairy`/`mask` category mapping already use Major. The temporary OoT fairy appearance override has been removed. Other container cosmetics, item tables, progression flags, and cross-game delivery remain intact.

Verification:

```sh
python3 scripts/diagnostics/run_mm_chest_contents_tests.py
python3 scripts/diagnostics/run_chest_size_tests.py
```

Revised Major/large assertions were observed failing against the superseded Minor/small implementation, then passed after correction. The MM suite covers native/randomized/foreign masks and fairies, authored types, independent style/size controls, pairwise resource failures, and preserved reward identity. The OoT suite exercises all five concrete fairy identities and the unchanged real foreign-category mapper, with complete C actor compilation in ordinary OoT and `COMBO_BUILD`. Both suites use UBSan.

Runtime status: harness-verified; full combined executable linking and configured in-game rendering remain untested. Preserve the accepted executable and original packs. The combined candidate remains held for gameplay publication; source-only recovery checkpoints are authorized separately.

Recovery: restore the named checkpoint branch for complete source, or apply its two-entry MM production patch and associated tests to the repaired parent. Retain existing actor/music fixes in the combined candidate. Prior local history, including the superseded Minor/small experiment, is preserved.
