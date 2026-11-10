# Authored door-key GIs

cor requested an authored Rusty Key GI in the first source revision, then explicitly expanded the request to an authored OoT overworld-door key and allowed realistic styling.

## Intent and design

Build two original, distinguishable 3D get-item models: an aged iron skeleton key with a broad worn bow, a substantial shaft and warded teeth for MM's 39 Rusty Keys; and a more refined aged-brass house key with a shaped bow and cut teeth for OoT's 27 overworld keys. Rust, edge wear and metal shading should read at pickup size. These material and silhouette choices are implementation judgments, open to visual revision.

Use native bundled display lists and vertex/texture resources. A single resource owner supplies each model in either host: MM owns RustyKeyDL; OoT owns OverworldKeyDL. The source revision must contain the models and their deterministic authoring recipe, not require a separately installed mod. Preview the serialized production geometry and materials with an explicit offline-render label.

## Approach

The chosen approach is new authored models bundled through the existing native asset pipeline. Recoloring the old small key would not supply the distinct authored geometry requested. A standalone mod would not satisfy inclusion in the source revision. Avoid introducing a new renderer or changing the native GI ABI.

## Preservation

Preserve all door identities, grants, save data, pools, logic, settings, CSMC categories, dungeon keys, boss keys and key rings. Both native and cross-game draws must select the model by key origin, including the three newly added OoT keys. Keep graceful existing-model fallback when a matching bundled resource is missing. Respect native/Alt resource selection. No merge or master promotion is part of this task.

## Shared asset contract

- MM resource root: `objects/combo_door_keys/RustyKeyDL`.
- OoT resource root: `objects/combo_door_keys/OverworldKeyDL`.
- Vertices use quarter-world-unit coordinates; each new model is drawn at scale 0.25.
- All nested resources remain under the owning game's `objects/combo_door_keys/` namespace and use distinct names between the two owners.
- Overall target height after scaling is about 65-75 world units, centered near the normal GI origin; enough thickness and bevel detail to read throughout a rotation.
- Material setup is self-contained; model roots restore relevant draw state. Resource lookups and callbacks must not mistake the gray CSMC category for an authored material.

## Verification

Validate complete resource dependencies, native vertex/index limits, non-degenerate triangles, winding and bounds, deterministic regeneration, both asset packers, compiled native/foreign selection, origin identity and fallback behavior. Inspect front, three-quarter and back views of the actual serialized models. Keep full-build and configured in-game appearance evidence separate from offline previews and targeted native checks.

## Accepted visual baseline and requested refinement

On 2026-11-09 cor enthusiastically approved the first models' appearance, then requested metallic shine beneath the aged surface and the same diagonal orientation as existing game keys. Local checkpoint `b0854044` preserves the initial authored models, native routing and previews. The approved silhouettes and weathering RGB remain the baseline; configured gameplay acceptance is still outstanding.

Use a restrained native texture-generated reflection base beneath a static-UV weathering coat. Opaque rust/patina hides the reflection; exposed metal allows it through. Reflection follows view/rotation, while rust/patina stays attached to the model. No emissive aura, moving rust pattern or preview-only metallic shader is introduced. Each texture load fits native TMEM independently; reload coat vertices after clearing texture generation and restore opaque state at root exit.

Existing vanilla and bundled small-key geometry has its shaft along the local +X,+Y diagonal, with the bow upper right and the bit lower left. The native receipt/shop paths supply Y rotation without adding a Z roll. Preserve the authored vertex bytes and add exactly one shared local Z rotation of -45 degrees in native callbacks and the existing supported foreign recipe paths; preserve inherited actor/camera transforms and unmodified fallback model orientation. Updated previews must apply that same roll before the turning motion.

cor requested preview before packaging. Show the updated still and motion preview for review; hold delivery packaging and source publication until that visual review is complete. Native parser/compile checks and local source recovery checkpoints may continue.
