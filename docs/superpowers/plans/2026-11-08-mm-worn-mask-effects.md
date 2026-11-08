# MM worn masks and Couple's Mask recovery

Baseline: cumulative UI/shovel/winter/pedestal POC2 at 8150ed3489722f983b660e5e5d5b430c2467afc6. Unchanged MM renderer boundaries are captured through local commit 3a6537babe05f7a30d86fa00f17af4cbfb29aac1. Preserve the previous checkpoint.

- Add the existing `gEnhancements.HideNonTransformationMasks` checkbox to MM's Skijer's NEI → Masks and Enhancements → Items/Songs menus, disabled by default.
- Suppress only worn non-transformation mask drawing, including Persistent Bunny Hood's separate renderer. Preserve native mask state, effects, held equip/unequip models and transformation sequences.
- Restore the missing MM Couple's Mask passive effect: one health unit per four gameplay updates during the day, one magic unit per seven updates at night, matching the OoT implementation. Clamp to the active native meter capacity, including fractional RPG capacities. Do not create magic ownership, change capacity or interfere with meter initialization. Pause during blocked gameplay and reset on removal/death.
- Audit native MM masks against OoT's NEI additions. Document source-proven differences; do not copy location-specific OoT rewards, spawners or quest logic into MM.
- Execute native C fixtures with ASan/UBSan in original and Combo configurations, menu registration checks, existing fast-transform/easy-equip tests, and full changed player/renderer object compilations. Independently review the changes and produce a recoverable cumulative source checkpoint. Full build and in-game acceptance remain separate.
