# Unbound authored equipment regression investigation

cor reports that the authored 2Ship Kokiri Sword representation no longer appears with Use Vanilla Held Swords unchecked. The saved configuration confirms the setting is zero. A configured game reproduction is unavailable here, so this investigation covers both held-model and GI selection without claiming that every installed-pack condition is resolved.

## Baseline and bounded candidate

Published combined recovery baseline: `marsh6487/ComboShip`, checkpoint `fe279a77c03f1d2df17f30fd076ab1bf70867004` on `checkpoint/combined-chest-major-17748a4-20261029`. The affected header has Git blob `a5496f5a38259c7a15d0f37c63fe8d27a5fbfe0d` there and in the local source before this patch.

Candidate branch: `candidate/unbound-equipment-provenance-20261029`. Local patch parent: `8cbd5cd7b8a4c63de9a5d9673b500a002cb31e13`. It starts from complete local source `22b6569a8f9f13880311609734e6c549f22d7ede` and preserves all 40 exact tracked files of the latest combined snapshot `361eba78b3f59147d112e4ba4059b1a74c6266c3`, published as the checkpoint above. This retains the corrected Major mask/fairy chests, actor/NPC, music, and fixture repairs. Accepted builds, original archives, and earlier source checkouts remain unchanged.

## Confirmed source defect

OoT mounts its generated base as `oot-unbound.o2r`, resolved in the `soh` application namespace. `NeiAssetPriority::IsModAsset` still recognizes only the older OoT base filenames. It therefore reports native resources in the generated stock archive as selected mod replacements.

`NeiHeldSword_EquipmentSelected` checks OoT combined-hand resources even for MM Kokiri Sword, and `WeaponUpgrade_ApplyHeldSwordDL` declines the authored sword when that ownership gate is true. GI `HasLegacyGiMod` also queries native OoT resource paths. The defect can consequently suppress held swords, GIs, and adjacent equipment through this shared lookup.

The sole production change adds the exact resolved stock Unbound archive path to the existing OoT builtin list. Files in mod directories with the same basename still count as mods. Checkbox behavior, actual replacement priority, per-owner Alt state, missing-owner handling, authored geometry/materials, and all item/progression logic are preserved.

## Verification

The new regression compiles the actual provenance functions and held-sword ownership gate with controlled archive/path boundaries. Before the production edit it failed with: `Unbound stock hands incorrectly suppress the authored MM Kokiri Sword`. Afterward it passes under both native header environments, normally and with ASan/UBSan. It checks exact stock paths, the OoT application namespace, a similarly named mod, metadata-only Alt overrides, live selection changes, GI/boot ownership, and a missing donor.

Passed commands:

```sh
python3 -B tests/nei_asset_priority/run_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 -B tests/nei_asset_priority/run_unbound_tests.py --sanitize
python3 -B tests/nei_held/run_equipment_tests.py
python3 -B scripts/diagnostics/run_sword_asset_toggle_tests.py --sanitize
python3 -B scripts/diagnostics/run_mm_chest_contents_tests.py
python3 -B scripts/diagnostics/run_chest_size_tests.py
```

Existing controller macro and native chest type warnings remain visible in the saved logs. Leak detection is disabled only for this host because LeakSanitizer cannot inspect its task list; ASan and UBSan remain enabled. Independent read-only review found no blocking issues. Its minor suggestion to validate the application namespace was added to the fixture and retested.

Status: source/native verified candidate; full application build, Windows DLL execution, configured rendering, and user acceptance remain pending. No gameplay build was published or accepted master promoted.

## Recovery and decisive runtime check

Apply the bounded `source.patch` from the recovery ZIP to the published combined checkpoint above with `git apply --check source.patch`, then `git apply source.patch`. The ZIP also contains the complete changed files, verification logs, and exact source identities. Its patch restoration was checked independently against the patch-parent files.

Build a matching complete candidate. In MM, check Kokiri Sword in Link's hand and as a pickup with the checkbox off, then the live vanilla toggle. Check one native upgrade and one imported equipment item. Preserve the existing packs/order for comparison; an intentional selected equipment or custom player replacement can still own its model under the existing rules. A stock-human control distinguishes that expected override from this repaired stock-archive error. No other sword or equipment has been visually proven here.
