# Enemy and NPC soul miniatures

## Request and baseline

cor requests recognizable miniature models for OoT enemy souls and NPC souls in
both games, with independent simpler enemy/NPC model checkboxes like the existing
boss control. The follow-up explicitly includes MM NPC souls, all eight MM enemy
foreign-rendering exceptions, and checks of both receiving hosts.

Baseline: published cumulative PR 89, `0063920e18145299afc1682aab98407ebd366fc5`,
tree `55582071d0afddc81529bcc1d495d3c083d32c2e`. Its independently restored durable
checkpoint is `ComboShip_R21_Complete_Source_Native_Proof_20261109.zip` (SHA256
`db2ea8dfe92f2d21275aa04e9a70926634baa5d3972e937d4293db1b3a1dc700`). Work is isolated
on `candidate/soul-mini-models-20261109`.

## Behavior

- Detailed models are the default. Cover all 43 added OoT enemy identities, all
  46 OoT NPC identities, all 53 MM NPC identities, and preserve the existing 47
  MM enemy miniatures. Grouped souls use a documented representative native role.
- Complete foreign recipes for Bad Bat, Boe, Chuchu, Freezard, Like Like,
  Dexihand, Gomess and Iron Knuckle. A normal supported-resource draw must not
  silently degrade to the old flame-only stand-in.
- `Simpler Enemy Soul Models` and `Simpler NPC Soul Models` are independent live
  visual preferences, default false. Both hosts expose the same two preferences;
  Combo tabs therefore cannot disagree. The existing boss setting stays separate.
- Checked means the existing token-and-flame fallback for that category. Missing
  or invalid model resources also take a safe fallback. No actor is spawned for a
  pickup; render native meshes/poses through the GI renderer.
- Native and foreign appearances share owner-authored recipes. Use canonical
  owner resource paths, proper face/material segments, model-specific scale and
  centering, animation where appropriate, and the existing soul flame materials.
- Preserve original-build guards and all Combo solo modes. Preserve resource
  ownership, Alt/vanilla changes, canonical owner replacements, item/matrix/segment isolation,
  and safe retry when an owning game is not ready.

## Approach

Reuse the existing owner-description/host-rendering architecture
(`CwItemAnimDrawInfo` and `ComboForeignAnim_Draw`) for native Combo pickups as well
as foreign ones. Extend its append-only recipe only where non-skeletal enemies
need static/composite parts, matrix palettes, material binds or a second texture
animation. Model catalogs derive their assets, limb counts and segment choices
from actual native actor/resource definitions. This avoids creating hundreds of
new mesh assets or invoking gameplay actor lifecycle code during item rendering.

The existing generic token remains the explicit simpler mode and failure fallback.
Keeping token GIs as the default fails the request; copying whole actor update
functions would introduce unnecessary gameplay state and lifetime coupling.

The owning module validates its concrete resource classes and returns plain C
metadata. OoT and MM's `SOH::Skeleton` classes have different vector layouts;
the receiving module must never cast a foreign resource to its own concrete
skeleton class. Both modules and the shared ABI must ship from the same revision.

The latest user clarification includes Twilight Princess replacements, as with
boss souls. Actual supplied packs select 62 OoT replacement rigs (26 enemies,
36 NPCs), 13 MM enemy replacement rigs, and a Djipi Composer Bros rig in the MM
mixed pack. An identity without a selected replacement uses its native model.
The packs checked here use canonical rig resources and do not require optional
NativeNpcV1/PoseAnimations/private-skin adapters. A future pack that uses those
adapters needs separate compatibility work; arbitrary custom actor behavior is
not invoked during GI rendering.

## Preservation

No changes to item/option IDs, soul ownership, pool counts, seed generation,
sharing, saves, NPC gating, schedules, door keys, collision or other cumulative
features. No native game assets enter source/checkpoint archives. Source-backed
recipes only. No master promotion or merge.

## Evidence

Require complete catalog coverage, native asset existence/type/limb consistency,
both host renderer dispatch, detailed/simple selection in both directions,
same-frame matrix and segment cleanup, missing-resource fallback and retry,
Alt-cache behavior, and regression checks for existing boss/foreign rendering.
Exercise actual production render helpers with real native GBI/header interfaces
where feasible and compile changed native translation units. Keep headless command
and resource evidence separate from GPU/in-game visual acceptance.
