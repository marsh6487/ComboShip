# Seasonal player outfit packs

The renderer supports outfit sets for vanilla Link or any compatible custom
player model. `din` is the first supplied set; the engine does not require Din
or a source-code change for another set.

Enable **Seasonal Player Outfits** and choose **Outfit Set** in the custom-model
settings (OoT) or Graphics settings (MM). The checkbox defaults off. The saved
Rod/gameplay season selects the outfit; weather-editor previews do not. Off,
no active season (`SEASON_OFF`), or a missing age/season keeps the player's
normal model. Merely installing a private pack does not replace that normal
model.

## Pack contract

Name each age/season archive `something.private.o2r` and install it directly in
the active game's mods directory before startup. In ComboShip, install the
corresponding packs in both `mods/soh/` and `mods/2ship/`. Restart after changing
packs. Ordinary `.o2r` equipment dependencies remain ordinary globally loaded
mods; only the `.private.o2r` suffix opts a body into this catalog.

Use one age and season per private archive. The canonical skeleton is:

```
objects/seasonal_outfits/<set>/<age>/<season>/gLinkAdultSkel
objects/seasonal_outfits/<set>/<age>/<season>/gLinkChildSkel
```

`<set>` is 1–63 ASCII letters, digits, underscores or hyphens. `<age>` is `adult`
or `child`; use the matching skeleton name. `<season>` is `spring`, `summer`,
`autumn`, or `winter`. For example:

```
objects/seasonal_outfits/my_link/adult/winter/gLinkAdultSkel
```

The source passes these as `__OTR__...` resource strings. This is a private
resource address, **not a different rig**. Keep the accepted compatible
21-limb Link Flex hierarchy, joint translations, display-list count and native
animation contract. A different silhouette does not require a new animation
set. The router changes no joint or animation data.

Every body, outfit, face, eye/mouth, skin texture, limb and outfit-specific hand
resource must use that private set/age/season namespace. Rewrite every binary
path and hash reference consistently. Place native canonical body/pose/face
aliases beside the skeleton with their usual basenames, such as
`gLinkAdultWaistNearDL`, `gLinkAdultLeftHandClosedNearDL`,
`gLinkAdultRightHandHoldingBowFirstPersonDL`, and `gLinkAdultEyesOpenTex`.
Provide equivalent `alt/` aliases so Alternate Assets selects the same outfit.
The face aliases may retain the model's existing texture or blink-display-list
representation; the router does not reinterpret them as raw pixels.

Outfit-specific held-item wrappers remain private so the correct hands follow
the body. Their intentional references to shared weapons, shields and other
gear keep the existing shared paths. Do not copy shared public body aliases
into every private pack. Extract byte-identical common gear once if several
outfits require it; keep that dependency installed alongside the private packs.

Only recognized private skeletons opt into O2R body swapping. Existing generic
community O2R equipment handling, PAK/ZOBJ choices and forced/custom forms keep
their previous behavior. Seasonal choices do not overwrite normal saved model
preferences. Private archives stay mounted once used; unique paths allow a full
season cycle without unloading deferred display-list resources or relying on
archive priority.

## Optional weighted companion

Rigid Flex models remain valid. A model that deliberately opts into weighted
player rendering adds the documented NS/v1 companion resources below its exact
private skeleton address:

```
<exact-skeleton-resource-path>/Skin/Palette
<exact-skeleton-resource-path>/Skin/Limbs/LimbNN
```

Follow [player-flex-skinning-20261118.md](../poc/player-flex-skinning-20261118.md)
for the binary palette/limb contract. Keep the original rigid limb DLs as the
fallback. Installing engine support does not generate weights for a rigid
model, and seasonal routing does not convert or re-rig models.

## Verification before release

Test a full season cycle, including returning to the first outfit, then Off and
a missing season. Check both ages and games, scene re-entry, Alternate Assets,
blink/head movement, held sword/shield/ocarina, first-person aiming and a forced
form. Confirm the normal model returns when the checkbox or saved season is
Off. Native component tests and archive reference checks do not establish these
visual/runtime results.
