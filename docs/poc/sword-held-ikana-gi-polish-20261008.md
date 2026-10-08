# Sword held meshes and GI presentation candidate

Baseline: `a82852a3a02c8cabc541cce7c124da71c5755efb` on `develop` (daily polish PR #38).
Candidate branch: `poc/sword-held-upright-polish-20261008`. Keep this candidate separate from the accepted baseline until runtime acceptance.

## Behavior

- Four Sword and the progressive sword families reuse the exact redesigned GI mesh/material graph as held weapons in OoT and MM. Resource-only grip matrices attach them to the native adult, child, or MM human fist. Held weapons do not add GI particles or shimmer. All nine source meshes and their materials remain byte-for-byte unchanged.
- Vanilla ComboShip held swords use the existing shipped-only `@oot-gi-base` owner, including every mesh, matrix, vertex, and texture dependency. Alt selection stays live. Selected body/hand/equipment models, progressive piece overrides, legacy Four Sword pairs, and the final Din draw stage retain priority. Missing authored graphs retain the existing fallback. Biggoron remains eligible at zero knife health; broken Giant Knife keeps its native model.
- Selected standalone custom sword GIs rotate +X to exactly +Y with a 90-degree Z rotation. Model coordinate conversion remains separate from award effects.
- Custom sword pickups and world displays grow 15% about their existing fitted frame center. The blade, Din core/flame layers, particles, and shimmer receive the same outer scale. Authored vanilla GI sizing remains at its accepted fit. Shop shelf scale, placement, pose, and effect footprint stay unchanged; the accepted 1.8-radian shelf recipe is preserved separately from the straight-up pickup pose. MM Goron receipts keep their current fitted size because enlarging them worsens an existing Kokiri shimmer edge cut-off.
- Ikana's native OoT no-mod GI fallback now queues the same live MM-owned shield graph that its adaptive orientation query measures. This removes the old permanently cached host-pointer mismatch. The existing flat-mesh correction, local mod priority, MM native/foreign recipes, authored shield fallback, and MM-owned icon remain intact. Missing MM data skips the GI rather than borrowing a host graph.

No animation, melee reach, collision, save, inventory, equipment ownership, or transformation code is changed. Per-draw compound copies prevent later players/clones from changing an earlier deferred sword or body-color submission.

## Verification

Focused gates cover production selectors and real OoT/MM graphics/resource types, shipped-owner isolation and repeated Alt toggles, all held attachment matrices, native fist/hand classification, deferred clone lifetime, Din priority, original fallback callbacks, and matrix/owner restoration. Native C/C++ syntax checks include the changed player seams, equipment/item unity, GI renderers, MM draw helpers, and the existing 19 receipt translation units.

The supplied-pack gate uses these unchanged authorized inputs; their geometry is not redistributed with the source candidate:

| Archive | SHA-256 |
| --- | --- |
| `Din_Swords_Fire_POC3.o2r` | `5f8ee79131ce25ed1bb87cff524cbf26187758d503a1710bc6c9cbfe05a60bbd` |
| `zz_Din_Sword_GI_Icon_ComboShip_MM_POC1.o2r` | `c18b9676cce48c0de9b3ea5be49ab08ea9bb8df4b631c943074ae34c5e10397c` |
| `zz_Din_Sword_GI_Icon_ComboShip_OOT_POC1.o2r` | `5c07946a5cbc786333dfd93307568209e32b5cf9eb3335de171493be294d5fae` |

Camera checks use the actual serialized vertices and resource matrices for all ten pack roots, complete Din layers, both host array factories, and all five MM Item0 receipt cameras. Separate checks project packed procedural vertices through the same cameras, rejecting new or worsened clipping. Existing Goron Kokiri/MM Kokiri shimmer clipping is retained, not marked fixed. Shelf geometry stays within the original `[-22,52]` Y envelope and 76-unit spinning width.

Final focused checks pass, including exact shelf baseline equality, all nine held graphs and three grip frames, the two native player unity units, both MM held seams, and OoT's native GI callback. The supplied meshes' worst projected coordinate is 0.840958 against a 0.99 limit. Independent review found no remaining important source defect within these scopes.

Across the recorded combined-regression run and continuations, 77 of 79 commands pass. Two unchanged historical comparisons remain blocked: `nei_used_fx` requires unavailable revision `c77c18587a976f6d6cb5c8f91f27593286469218`, and the icon-route comparison requires `122dd5f68cb37fcf515c3726f8a77043db5dcdf4`. Checkpoint 12 records the same blockers. The full combined suite is not claimed green. Fixture boundaries were updated for the new presentation helper/callback; missing local CMake dependencies were supplied, and those checks then passed. ASan/UBSan remain enabled where requested; leak detection is disabled because this container cannot inspect the required process data.

The recovery checkpoint records exact command results, source hashes, the source commit/tree, forward and reverse patches, and the incremental git bundle. Controlled compiler/sanitizer/graphics-boundary checks do not establish an executable build or running-game/GPU behavior. No runtime visual acceptance is claimed.

## Runtime acceptance

- Receive every sword tier and Four Sword with independent OoT/MM Alt settings; toggle after caches warm. Confirm upright blades and proportional pickup effects, including Din and the long Great Fairy/Biggoron variants.
- Inspect native child/adult, MM human, and MM adult held grips, both LODs, Four Sword clones, and sword swings/grass interactions. Confirm authored vanilla held models and existing mod/Din priority.
- Check shelf previews and purchases in both games for the unchanged scale and shelf clearance.
- Receive Ikana through native OoT/MM and foreign rewards, with the installed shield packs and Alt on/off. Confirm it is upright, the authored fallback stays upright, and dialogue/equipment use the MM Mirror Shield icon. The supplied screenshot confirms a horizontal shield but does not identify its selected resource graph; unsupported pack pose graphs still need installed-stack runtime proof.

Do not promote this branch to `develop` or master based solely on these source checks.
