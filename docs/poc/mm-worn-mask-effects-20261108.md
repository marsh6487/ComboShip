# MM worn masks and Couple's Mask recovery — source POC3

This candidate extends the cumulative UI/shovel/winter/Lost Woods pedestal POC2. It is a source checkpoint, not an executable or an accepted in-game result. No external assets are changed.

## Findings and changes

OoT already exposes **Invisible Non-Transformation Masks** in both its NEI and enhancement menus, backed by `gEnhancements.HideNonTransformationMasks`. Its imported MM mask renderer hides ordinary worn masks while leaving their update/effect state alone. MM had no corresponding checkbox or native draw guard.

MM now exposes the same disabled-by-default setting at:

- **Skijer's NEI → Masks → Invisible Non-Transformation Masks**
- **Enhancements → Items/Songs → Masks → Invisible Non-Transformation Masks**

The native head renderer suppresses ordinary worn mask display lists, including their attached visual decorations. Persistent Bunny Hood has a separate head-render hook, which now honors the option without clearing its saved speed-boost state. The four transformation masks, models held during equip/unequip, native animation timing, quest checks and gameplay effects are preserved. Boss remains, scripted NPC masks and Hyrule Warriors Styled Link's decorative Keaton accessory retain their existing rendering; they are separate systems from this ordinary equipped MM-mask option. Play as Kafei's Link replacement and the Time Gate adult renderer use the covered native player callback.

Couple's Mask's extra passive recovery was implemented only in OoT's `MmMaskWear_Update`. MM's native Couple's Mask still had its story interactions, but no passive recovery update. This is a missing MM implementation; the source does not establish a fractional-magic failure as the cause of the reported symptom.

MM now runs the passive effect after its native player update:

| Period | Recovery | Limit |
| --- | --- | --- |
| Day | 1 health unit every 4 gameplay updates | Current health capacity |
| Night | 1 magic unit every 7 gameplay updates | Current initialized magic capacity |

This matches the existing OoT NEI cadence. Magic capacity is read from the native solver's result, so fractional RPG capacities and native magic upgrades use the same path. Recovery never grants magic ownership, changes a magic tier or increases capacity. Capacity-building/fill states are left to the native magic solver. Removing the mask, changing away from human form or dying resets its transient timer. Pauses, game-over states, scene transitions and cutscenes suspend recovery. Hidden worn masks keep the same mask state and passive recovery.

## Source audit: native MM behaviors and OoT additions

The old SoH mask-wear reimplementation is intentionally a stub in `mm/mods/transformation_masks/transformation_masks.c`: native MM owns mask use and its four original transformations. The native behaviors and the additional OoT behaviors therefore cannot be treated as one shared implementation. This is a source-route audit, not a runtime certification of every mask or quest.

| Masks/system | MM implementation | Difference from OoT NEI |
| --- | --- | --- |
| Deku, Goron, Zora, Fierce Deity | Native player forms and native fast-transform/equip hooks | Does not run the SoH ported form implementation |
| Stone, Bunny, Blast | Native enemy recognition, movement and blast action; persistent Bunny Hood is a separate enhancement | Worn visibility is now covered in both native and persistent draw paths |
| Kamaro, Scents, Truth, Postman, Gibdo, Circus Leader | Native player/actor/quest checks remain in their existing MM routes | No blanket replacement with OoT quest actors |
| Couple | Native story recognition plus the new passive update | Day/night passive recovery was missing in MM; added here |
| All-Night | Native grandmother story checks (`ovl_En_Nb`) | OoT's forced daytime spawning of night-only Gold Skulltulas is OoT-specific and has no equivalent MM hook |
| Great Fairy | Native stray-fairy attraction (`ovl_En_Elforg`) and native mask hair/particles | OoT's extra fountain reward/warp controller is absent in MM |
| Bremen | Native march/chick interactions | OoT's custom cucco spawning/controller is absent in MM |
| Don Gero | Native frog choir (`ovl_En_Minifrog`, `ovl_En_Geg`) | OoT's Zora's River frog-reward shortcut is tied to OoT checks |
| Romani | Native membership/NPC checks (`ovl_En_Tab`) | OoT's Link's House cow/Chateau Romani rule is absent in MM's cow implementation |
| Captain | Native graveyard/stalchild checks (`ovl_En_Tk`) | OoT's giant nighttime Hyrule Field spawner is OoT-specific |
| Giant | Native giant-mask action/scale/magic path | Does not run the separate SoH giant-mask controller |
| Kafei, Keaton, Garo, Gerudo custom forms | Existing MM-native registry in `mm/mods/forms/custom_forms.cpp`, controlled by `gForms.*` | Native custom forms are distinct from the legacy SoH mask-wear stub |

Primary comparison: `soh/mods/transformation_masks/mm_mask_wear.cpp`, especially its `MmMaskWear_Update` cases. MM counterparts remain in `mm/src/overlays/actors`, `mm/src/code/z_player_lib.c`, `mm/mods/forms` and `mm/2s2h/Enhancements/Masks`.

### Existing menu discrepancies found by the audit

The MM NEI Masks sidebar still contains legacy controls which are not aliases for current native implementations. This candidate does not claim those controls have been repaired:

| NEI control | Legacy key | Active MM route |
| --- | --- | --- |
| Kafei/Gerudo/Garo Mask Transform | `gMods.KafeiMaskTransform`, `gMods.GerudoMaskTransform`, `gMods.GaroMaskTransform` | `gForms.Kafei`, `gForms.Gerudo`, `gForms.Garo` in Enhancements → Items/Songs. The Kafei legacy key also influences a PAK model guard, so it must not be blindly substituted. |
| Enable Transformation Masks | `gMods.TransformMasks.Enabled` | Original MM transformations are native; the removed SoH controller's global gate is not their owner. |
| Instant Transform | `gMods.TransformMasks.InstantTransform` | Native `gEnhancements.Masks.FastTransformation` supports multiple modes. |
| Instant Blast Mask | `gMods.BlastMask.Instant` | Native `gEnhancements.Masks.BlastMaskCooldown` controls the cooldown. |

Inventory-page and native-form settings have different responsibilities. The new visibility checkbox uses its actual draw-path key, and the repaired Couple's Mask passive is tied to the worn native mask state; neither depends on those legacy controls.

## Validation and limits

- `python3 -B tests/mm_mask_effects/run_tests.py`: actual native head-draw block, shared held-mask segment loader, persistent-render guard and recovery function, compiled with real MM types under ASan/UBSan in original and Combo configurations. Checks all 24 native masks, transformation/held/equip timing, persistent Bunny Hood state, day/night cadence, native/fractional caps, removal/death and blocked gameplay. C++ fixture executes both actual new menu registrations.
- `python3 -B scripts/diagnostics/run_mm_fast_transformation_tests.py`: existing production fast-transform and Easy Mask Equip caller regressions. Requires the project's real nlohmann JSON dependency; v3.11.3 was supplied for this check.
- Full changed native player and renderer C17 translation units compile in original and Combo configurations (four object compilations). Menu registration and persistent-render-prefix fixtures validate the new C++ expressions; a complete GUI/application build was not performed.
- Independent review and checksum-gated original-baseline/prior-checkpoint restoration are recorded in the checkpoint evidence.

In-game acceptance remains pending: toggle visibility while wearing ordinary and persistent masks, confirm speed/blast/fairy/quest behavior, test the Couple's Mask across day/night with a spent fractional meter, and compare both ages at the Lost Woods stump under both Story Skip settings. The prior pedestal/UI/shovel/winter evidence remains in the cumulative package.
