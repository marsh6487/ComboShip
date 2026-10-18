# Optional native NPC models and fitted poses

This POC adds five separate O2R candidates using the optional paired-rig pattern
from Young Fado. The source checkpoint targets ComboShip develop
`a82852a3a02c8cabc541cce7c124da71c5755efb`. It is not a game executable or a
runtime-accepted release. Keep the existing accepted build and packs intact.

| Pack | Native and placed actor coverage | Private namespace |
| --- | --- | --- |
| Saria Anime | En_Sa, Demo_Sa OPA/XLU, static Saria | objects/object_sa/NativeNpcV1 |
| Saria Render | Same Saria actors; alternative to Anime | objects/object_sa/NativeNpcV1 |
| Kokiri Twins Render | En_Ko girl types 1, 5, 6, 9, 10; static Kokiri girl | objects/object_kw1/NativeNpcV1 |
| Young Malon | En_Ma1; static child Malon | objects/object_ma1/NativeNpcV1 |
| Adult Malon Render | En_Ma2, En_Ma3; static adult Malon | objects/object_ma2/NativeNpcV1 |

The girl types cover the twins' house, forest, shop, and story-gated shop awning.
En_Ko type 12 and static Fado remain separate. Install only ONE Saria variant.
The girl, young Malon, adult Malon and selected Saria packs can coexist.

## Build and install

Apply the source checkpoint to its recorded base, or reconcile it into the
cumulative source branch before the next normal build. Place the desired O2Rs
in that build's usual mods directory and enable Alternate Assets. These packs
need the matching adapters; an older executable does not acquire new actor
support by loading the files. No full game executable was built for this POC.

Each pack has a native base skeleton and an Alt custom skeleton under a private
path. Both currently preserve native joint order, count, hierarchy and
translations. SkeletonPatcher can switch the loaded actor between them. The
native animations, dialogue, placements, progression and actor identity remain
owned by the game. Missing optional packs use the original skeleton path.

The current ComboShip base does not include the full Young Fado NPC adapter from
Shipwright's PR20 branch. This patch does not transplant old actor files over
new ComboShip changes. Reconcile that earlier Fado adapter separately in the
cumulative merge. The new pose resolver already understands its private rig
`objects/object_fa/YoungFadoNpcPOC1/Skel` and legacy `Animations/Seated5` alias.

## Archive-only correction contract

One matching build installs the resolver. Later fitting revisions use O2R
resources under the selected model's namespace:

| Revision | Resource contract |
| --- | --- |
| Hand basis, fingers, mesh seams, rigid vertex ownership | Existing Body display lists and vertex arrays |
| Limb proportions and bind offsets | Alt Rig/LimbNN; keep native hierarchy, joint count and matrix slots |
| Pose-specific rotations | alt/NS/PoseAnimations/FULL_NATIVE_ANIMATION_PATH |
| Pose-specific hand, arm, cloth or prop mesh | alt/NS/PoseAnimations/FULL_NATIVE_ANIMATION_PATH/Limbs/LimbNN |
| Adult Malon basket, including static actor | alt/NS/PoseAnimations/FULL_NATIVE_ANIMATION_PATH/Props/Basket |
| Eyes and mouth | Existing Faces/LxRyMz wrappers and FaceParts; neutral head remains available |
| Texture/material fitting | Private texture and display-list resources |

`NN` is the zero-based native limb index. Callback limb indices are NN+1.
`FULL_NATIVE_ANIMATION_PATH` includes its object directory; it is not only the
clip basename. Example:

```
alt/objects/object_kw1/NativeNpcV1/PoseAnimations/objects/object_os_anime/gKokiriSittingCrossedArmsLegsAnim
```

Private pose clips are ordinary normal-animation resources. The runtime rejects
wrong types, joint counts, frame counts, channel bounds, and root translations.
It preserves the native animation identity used by dialogue/cue/prop code and
uses the engine's normal frame interpolation and positive/negative morphs.
Alt OFF or a missing/invalid fitted clip selects the native animation. A fitted
static crossed-legs clip owns its legs/head too; the older fairy-leg overlay is
then skipped. Without a fitted clip that overlay behaves as before.

A pose-specific mesh is optional independently of a fitted clip. This permits a
hands-only or cloth-only correction. A full body-part display list must use the
existing native matrix-slot convention and restore its host matrix. Faces or
props included in a pose-specific head/hand wrapper must preserve their required
expression/prop routes. Renderer recursion and vertex-cache limits still apply.

## Build and inspect assets

Python dependencies: numpy, Pillow, scipy. The native archive and original GLBs
are read-only inputs and are not included in the source checkpoint.

```sh
python3 scripts/npc_models/native_npc_models.py /path/to/oot.o2r /path/to/glbs /path/to/output
python3 scripts/npc_models/verify_native_npc_models.py /path/to/oot.o2r /path/to/glbs /path/to/output
python3 scripts/npc_models/preview_native_npc_models.py /path/to/oot.o2r /path/to/output
```

The writer preserves source triangle counts and the exact original 4096x4096
diffuse pixels. It uses the recovered MIT Fado resource writers under
`scripts/npc_models/native_assets/`. The preview decodes exported display lists,
vertex caches, matrices and texture references, then poses them with real native
animation data and any fitted clip in the candidate. It is an offline review,
not an in-game capture. The preview reads the Alt rig offsets too. For a later
proportion-fit revision, pass `--allow-fitted-rig` to the verifier; the native
fallback, hierarchy and matrix layout must still be preserved.

For a narrow pose correction, start with a JSON recipe for the exact parent pack:

```json
{
  "schema_version": 1,
  "parent_sha256": "SHA256_OF_THE_SELECTED_O2R",
  "clips": [{
    "native_animation": "objects/object_os_anime/gKokiriStandingArmsBehindBackAnim",
    "rotations": [{"joint": 9, "mode": "add", "binang": [0, 0, 128]}]
  }]
}
```

The values above demonstrate a small rotation; they are not an approved sway
fit. Angles are integer native binary angles; 32768 is a half turn. Joint 0 is
root translation and cannot be edited. Omit `frames` for every frame, or supply
an explicit list of frame indices. `mode` may be `set` or `add`.

```sh
python3 scripts/npc_models/edit_native_npc_pose.py parent.o2r oot.o2r recipe.json fitted.o2r
```

Enable the full `fitted.o2r` in place of its parent. `--overlay` instead creates a
small patch containing only edited private clips plus provenance; it needs the
exact parent enabled and patch precedence in the archive load order. The editor
also understands `YoungFadoNpcInfo.json` from the recovered Fado NPC packs.

Review sway from front/back through the whole loop, and seated crossed arms
from front/side before accepting a correction. Check wrist axes, palm direction,
elbow volume, arm contact, cloth coverage, feet and tracking. Preserve separate
pose-4 and pose-5 behaviors. A change to an actor's story behavior, rig topology
or an unsupported rendering feature can still require code.

## Verification and remaining work

The checkpoint records focused native actor compilation/routing, the production
C++ pose resolver, actual native animation sampling/morphing, Kokiri donor-pose
controls, culling controls, Saria diagnostics and archive readback. The normal
fast regression gate now calls the new adapter tests. Run them without game
assets with:

```sh
python3 scripts/diagnostics/run_native_npc_model_tests.py
python3 scripts/npc_models/test_native_npc_models.py
```

The recovered native archive is additionally needed for Kokiri pose and export
checks. No platform link, full game build, live archive toggle, story-gate test
or in-game visual acceptance is claimed.

These are fitting candidates. Seated tunic/leg intersections, shoulder/neck
joins, Malon's collar/scarf, Saria's ocarina grip and facial expression art still
need refinement and runtime review. The GLBs have a single painted face and no
expression morphs: the supplied eye/mouth states use their source surface and
atlas pixels, with exact neutral artwork retained. Do not treat the expression
routing check as visual acceptance. The new contract lets those fitting edits
ship as updated assets after the matching adapter build.
