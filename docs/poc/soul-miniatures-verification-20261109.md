# Soul miniature source verification — 2026-11-09

## Result and acceptance boundary

The cumulative source candidate implements detailed enemy and NPC soul GIs in
both receiving games. It uses actual owning-game model resources at GI scale,
with native animation or a representative pose and the soul flame. Grouped soul
identities use the documented representative in their owner catalog. Canonical
selected TP replacements are used where the pack supplies a compatible model.

This report records source, native translation-unit compilation, real archive
validation and headless renderer evidence. A newly linked executable and an
in-game visual acceptance run have not been produced for this candidate. The
existing R21 runtime remains the prior accepted runtime. The runtime matrix in
`tests/soul_mini_models/README.md` must be observed before visual acceptance or
promotion to an accepted runtime baseline.

## Catalog and controls

| Owning game | Enemy souls | NPC souls | Total |
|---|---:|---:|---:|
| OoT | 43 | 46 | 89 |
| MM | 47 | 53 | 100 |
| Both | 90 | 99 | 189 |

The formerly incomplete foreign MM cases are Bad Bat, Boe, Chuchu, Freezard,
Like Like, Dexihand, Gomess and Iron Knuckle. Static/composite geometry, native
palettes, materials and post-limb passes are represented where those models need
them. Kafei and Dark Link use native player-pose resources; no gameplay actor is
spawned for a GI.

Both Combo tabs expose the same two independent, live preferences:

- `gRandoEnhancements.SimplerEnemySoulModels`
- `gRandoEnhancements.SimplerNpcSoulModels`

Both default to zero (detailed). Checked selects token/flame for that category.
Both checkboxes are registered as new controls and use the gold What's New
accent in both game tabs when the global What's New setting is enabled.
The boss preference remains independent. An unavailable or unsupported selected
model also takes the safe fallback, and a later draw can retry. All four category
combinations and same-frame positive/negative foreign cache invalidation are
covered. Original standalone builds preserve their previous behavior and hide
the Combo-only controls.

## Actual selected resources

| Resource configuration | Recipes passing | Unique OoT resources checked | Unique MM resources checked |
|---|---:|---:|---:|
| Original game archives | 189 / 189 | 4,114 | 5,497 |
| Supplied TP/mixed packs selected | 189 / 189 | 5,241 | 5,731 |

The supplied OoT configuration selects 26 enemy and 36 NPC TP skeletons. All six
forced semantic OoT head substitutions have matching replacement paths. The MM
mixed pack selects 13 enemy TP skeletons and a Djipi Composer Bros skeleton.
Other identities retain their native models unless another selected pack supplies
compatible replacements. These exact packs contain no NativeNpcV1/PoseAnimations/
private-skin adapters; a different pack using such hooks needs separate support
and acceptance evidence.

The read-only archive gate checks owner/category coverage, resource kinds,
native and XML skeletons/limbs, hierarchy topology, animation and player-pose
bounds, visible body dependency closure, dynamic segments, effective flex matrix
consumers, authored palettes and fallback resources. Maximum actual hierarchy
depth is 11 in OoT and 13 in MM for both tested configurations.

Full input hashes and per-identity results are preserved in the source recovery's
`evidence/resource-report-native-final.json` and
`evidence/resource-report-tp-final.json`. Reports include paths and metadata only;
no private archive contents or extracted model data are distributed.

## Reproducible verification

The public gate is registered in the render suite:

```bash
export IMGUI_DIR=/path/to/the/project-pinned/imgui
python3 -B tests/soul_mini_models/run_tests.py --sanitize
```

It runs production catalog and setting checks, actual receiving-host cache
functions, two independent owner DLLs, native recorder transaction checks, the
actual MM animated-material factory, negative controls for the archive gate,
and the complete production recipe sweep through both native limb traversals.

The sweep covers 189 recipes in native/replacement/native phases at four frames:
2,268 draws per receiving host, 4,536 total. It requires actual body commands and
checks owner routing, Alt/cache changes and state restoration. Fixture resource
I/O is explicit; this is not a GPU appearance claim.

Additional evidence:

- The actual MM material factory accepts all 293 native OTAN resources. The
  unchanged native linear interpolation handler executes every frame of all
  seven native type-3 clips, including their final frames, under ASan/UBSan.
- Existing shared-soul fallback tests pass for 95 OoT and 59 MM identities;
  existing boss model/flame and Great Fairy texture-cycle ownership checks pass.
- Real ImGui UI refresh and What's New tests pass, including Combo and original
  menu paths. Exact two-menu binding checks cover all four live preferences.
  A focused native-widget probe checks both soul checkboxes' gold caption
  vertices, normal styling when highlights are disabled, stable IDs/geometry,
  real click/CVar/save behavior and unchanged surrounding styles in both games.
- Both native draw consumers, both owner export translation units, both frame
  recorders, both matrix engines, and both menu translation units compile in
  Combo and original modes using the existing native compile database and SDK.
  The changed MM material factory and destructor also compile. Existing compiler
  warnings are retained; these checks do not constitute a complete native link.
- An independent reviewer reran the revised both-host sanitizer suite, owner DLL
  boundary, native recorder transaction tests, and actual 293-resource material
  factory checks. The seven initial important/material findings were addressed;
  a final real-resource comparison also identified Gomess' required type-3
  handler, covered by the corrected material-shape fixture and native parity test.

## Review corrections included

OoT and MM have different concrete `SOH::Skeleton` layouts. Resource validation
therefore executes inside the owning module and returns POD metadata. Receiving
code uses the common resource interface and verifies raw-pointer identity; it
does not cast foreign resources to host-specific concrete skeleton classes.

The reviewed renderer rejects a limb override that would add a flex matrix where
the selected rig had no original mesh, validates mesh/limb resource types and
layouts, and checks the real remaining capacity of each game's 20-entry matrix
stack on every draw. This includes cache hits from an already nested caller.

Failed loading or a throwing native callback rolls back both graphics heads,
frame allocation, the native matrix stack and the isolated interpolation record.
Earlier successful item records survive. Incomplete cache entries are removed,
so failure does not prevent an immediate retry.

Material corrections include OoT Freezard's native translucent ice colors and
scroll, TP Stalfos' material segment, and MM Iron Knuckle's XLU post-limb setup.
Gomess' core uses type-3 linear color interpolation, now implemented in the shared
renderer with the native rounding order. Its actual [3,1,1] material sequence is
represented in the all-catalog fixture; the archive gate rejects unported handler
types rather than treating well-formed resource data alone as render support.
The MM factory validates actual array extents and list termination, cleans up
partial reads, and supplies valid terminal color storage for native linear clips.

## Source integration and recovery

Published cumulative baseline: `0063920e18145299afc1682aab98407ebd366fc5`, tree
`55582071d0afddc81529bcc1d495d3c083d32c2e`.

Latest inspected local cumulative revision:
`4608e863a0ad5551eed7f20c8cf41ea285ec1254`. Its sole final-tree difference from
the published baseline was a four-line `IMGUI_DIR` test fix. That change is
preserved as `ce019e99` in this candidate. The isolated candidate therefore
contains the latest inspected cumulative content plus this bounded soul work;
the original checkout and remote PR were not rewritten.

Candidate branch: `candidate/soul-mini-models-20261109`. Exact delivery commit,
tree, patch hash and independently restored tree are recorded in the source
checkpoint's manifest and restore verification. The checkpoint includes committed
source, a binary-safe patch, a Git bundle, this report, verification logs and
metadata-only resource evidence. The base can also be restored from
`ComboShip_R21_Complete_Source_Native_Proof_20261109.zip`, SHA256
`db2ea8dfe92f2d21275aa04e9a70926634baa5d3972e937d4293db1b3a1dc700`.

No accepted master/runtime promotion is asserted. Final framing, scale, eyes,
props, material appearance and flame composition still require observations of
the newly built candidate in all four source-game/receiving-game routes.
