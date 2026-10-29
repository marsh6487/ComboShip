# OoT/MM mask and stray-fairy chest styling

Baseline: repaired ComboShip `3cbb22d1f09e85deb206d2b085fc62105bc09fad`. The mask-only checkpoint is `e2a999b59f911e489818a30b38f1120fe668e775`; this follow-up adds the user's accepted fairy styling and requested OoT parity.

| Reward | Existing 3DS model | Size when contents-based sizing is enabled |
| --- | --- | --- |
| Masks | Major | Large |
| Stray fairies | Minor | Small |

Disabling size matching retains the authored size. Style remains controlled independently by each engine's existing contents-based style options. No archive update is required.

MM aliases `RITYPE_MASK` to the existing Major body/lid and `RITYPE_STRAY_FAIRY` to Minor, with fairies included in the visual small-size rule. Native item IDs, randomized categories, rewards, actor types, flags, and draw-pass/resource fallback guards remain intact.

OoT already uses Major art for its mask rewards. A chest-only C-linkage helper recognizes all five concrete fairy IDs and valid foreign fairy checks. It selects Minor art and small size only for chest cosmetics; actual `GetItemEntry` categories, progression flags, item tables, other container cosmetics, and cross-game delivery are unchanged. Foreign traps and unknown/missing category metadata retain their existing appearance. Invalid foreign check IDs never reach the lookup.

Verification commands:

```sh
python3 scripts/diagnostics/run_mm_chest_contents_tests.py
python3 scripts/diagnostics/run_chest_size_tests.py
```

The MM regression failed first on the absent fairy model mapping, then on the old large-size rule. The OoT concrete-fairy regression failed on the old large-size rule. Removing only the new foreign-fairy override also made the foreign-size regression fail; the real classifier was then restored. The complete suites cover authored chest types, native/randomized/foreign rewards, independent style/size controls, partial model fallback, preserved reward identity, masks, minigame concealment, and existing positioning/opening checks. The OoT harness executes the real C/C++ classifier and compiles the complete C actor for both ordinary OoT and `COMBO_BUILD`; MM includes UBSan and complete C actor syntax compilation.

Final checks: both engine suites passed. OoT's two build modes emitted no compiler warnings; MM retained 57 existing controller-macro/const warnings. Independent review found no issues. `git diff --check` passed.

Runtime status: implemented and harness-verified. Full executable linking and configured in-game validation remain pending, including opening alignment for fairy rewards in authored large chests. Preserve the publication hold on the combined actor/music candidate and the accepted runtime baseline.

Recovery: retain the isolated source commits and apply their bounded source/test changes onto the repaired parent or held combined candidate. The source diff adds no authored assets or save-format changes. Revert only these changes to discard the probe; keep the prior actor/audio repairs and original chest archives.
