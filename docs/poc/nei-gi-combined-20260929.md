# Accepted NEI presentation and MM pickup rendering

## Baselines and authorization

- ComboShip parent: `ae2da508b60bedbde117432a6ce13ff0ab1a22dc` (PR 23, cumulative performance and PAK/menu fixes).
- SoH donor: `2ce2e5a28098261943c25f8a1ecebe730c1d06ea` (PR 17), relative to `628deea3cf851af19a18f915e0665d772294c383`. This includes the final local-player first-person correction on top of the accepted `5fadb2d` candidate.
- The user accepted the current third-person rod fit and reported that the latest Deku Leaf runtime no longer crashes. Din model wrist orientation is a separate task.
- The user authorized the first-person visibility fix and combined ComboShip push, including the previously queued MM pickup GI/icon correction. Master promotion is outside this task.

## Work plan

1. Hide all three local held rods while aiming in first person, retaining projectile/trail submission; exercise both ages and fallback paths.
2. Import the accepted SoH code/assets without replacing ComboShip's vendored dependencies or cumulative game integrations.
3. Reconstruct the waiting MM pickup fix: correct owning-game icon texture and dimensions; route custom GI presentation instead of native bottle row zero; preserve progressive award tier and fallback effects.
4. Run focused producer/consumer tests, source compilation, asset collision and preservation gates. Request an independent review of the changes before publication.
5. Publish the SoH correction and a stacked ComboShip candidate, then start and inspect runtime builds.

## Evidence ledger

- SoH hand fixture: baseline passes; new aiming assertions fail against the original drawer; corrected Fire/Ice/Light drawers pass, including single-shot and remote/third-person checks.
- Accepted donor patch applied cleanly to 657 files. Combo-specific vendored dependencies and existing workflow remain intact.
- The earlier MM item-presentation checkout/commit is absent from the restored workspace and was not published. Its diagnosis was recovered and the implementation reconstructed and verified anew.
- All 21 concrete NEI GI presentations route through native MM display-list submission, retaining opaque/energy/translucent layers, archive path lifetime, missing-resource fallback and legacy Cane effects. The SoH mesh submission body is shared without modifying its batcher behavior.
- The MM pickup message uses the owning game's resolved icon before granting the item, with its real dimensions and RGBA32/IA8 format. Custom GI callbacks cannot fall through to the native empty-bottle row.
- Progressive Cane presentation follows the native skill order for all 64 ownership masks. Raw previews resolve the current tier; a queued award keeps its pre-grant concrete tier. Concrete Roc Feather and unrelated items retain their identity.
- Passed: actual owner-descriptor/MM draw fixtures, icon producer-to-consumer fixtures, ASan/UBSan progression and icon tests, full affected C++ translation-unit syntax checks, and native MM message/rod C syntax checks.
- Passed: Fire/Ice interpolation including disappearing volleys, reused slots and impact heading; accepted Light charge/focus/release comparisons across 180 phases; Fire's Light-style charge using its private fire texture; Leaf, Whip, Lantern, Switch Hook, time-gate and item-stow regressions; exact resource/GLB parity for 21 GI models and 22 held components.
- Passed: PAK visibility, nested voice discovery, shared-item and grant-boundary preservation tests; asset collision check (1,733 shared paths, 181 known differing-content collisions, no new collisions); clang-format 14 and whitespace checks. Vendored dependencies and renderer performance implementations are unchanged.
- Independent reviews found and closed clone visibility and progressive Cane defects. Final review reports no remaining blocker within this scope.
- SoH PR 17's cumulative regression gate and formatting passed; its final first-person runtime build was started. ComboShip's existing platform workflow remains the build gate, with the new focused regression suite added.
- In-game proof for the new first-person hiding and ComboShip integration remains pending. Source tests are not runtime acceptance.
