# Authored door-key asset report

## Result and provenance

Two original authored get-item models, including production native geometry, original procedural metal textures, deterministic recipe, and actual-resource previews. No existing key geometry, game textures, ROM data, mod data or private donor content was read by the authoring recipe. No image-generation mockup was used. The root coordinator inspected the contact sheet and accepted the designs for this source revision on 2026-11-09; that is a source/preview review, not the user's configured gameplay acceptance.

| Resource owner | Native root | Triangles | Serialized vertices | Draw scale | Drawn height |
|---|---|---:|---:|---:|---:|
| MM | `objects/combo_door_keys/RustyKeyDL` | 972 | 2,916 | 0.25 | 70.75 |
| OoT | `objects/combo_door_keys/OverworldKeyDL` | 744 | 2,232 | 0.25 | 70.00 |

17 bundled resources total; 502,978 source bytes. Triangle corners are deliberately serialized independently for straightforward bounded batches and correct bevel/face normals. Each vertex load contains at most 30 vertices. Textures are original 32×32 RGBA16, 2,048 texel bytes each, with opaque alpha and periodic wrap.

## Geometry and materials

Rusty Key: worn broad oval bow with a genuine opening and mildly irregular forged outline; substantial round shaft; stepped ferrule; asymmetric warded bit with beveled edges. Native textures combine dark iron, oxide patches and pit marks, with less rust and brighter exposed iron on bevels.

Overworld Key: refined octagonal bow with a genuine hole, raised hexagonal maker's escutcheon, narrower flat blade, asymmetrical diagonal cut teeth and a dark milled-channel detail on both blade faces. Original brass textures provide golden worn bevels, darker face metal, sparse pits, scuff lines and restrained green patina.

The meshes use closed intersecting parts. All individual connected components are manifold and outward-wound after quantization; this is an economical visual-only GI construction, not a boolean-unioned collision mesh. Surface pitting and scuffing are native texture detail. Each surface uses the same production detail in the preview.

| Model | Native minimum (X,Y,Z) | Native maximum (X,Y,Z) | Drawn minimum | Drawn maximum |
|---|---|---|---|---|
| Rusty Key | (-52,-140,-14) | (52,143,14) | (-13,-35,-3.5) | (13,35.75,3.5) |
| Overworld Key | (-48,-140,-9) | (48,140,9) | (-12,-35,-2.25) | (12,35,2.25) |

Materials explicitly set opaque lighting, texture filtering, two-cycle rendering and the `TEXEL0 × SHADE` combiner. They ignore CSMC/inherited environment tint. Model exit disables texturing and generated UVs and restores white environment color. The opaque geometry/render setup remains active as in other native GI lists; display lists cannot generally push/pop the caller's arbitrary prior state.

## Commands and verification

Executed from the repository root:

```sh
python tools/authored_door_keys/build.py
python tools/authored_door_keys/preview.py
python tests/authored_door_keys/asset_test.py
python tools/authored_door_keys/build.py --check
python3 -B tests/authored_door_keys/asset_test.py
```

The four focused tests pass. They cover:

- All native dependencies exist, are reachable, and remain under the correct owner and unique key prefix; no unused bundled file or shared cross-owner filename.
- Native signed-coordinate/UV and byte-normal ranges, RGBA16 texture dimensions and TMEM budget, every triangle index, every load's vertex range, batches no larger than 30, and terminating display-list commands.
- No degenerate triangle after integer quantization; exactly two oppositely wound incidences per edge; positive signed volume of each connected component; vertex normals aligned with geometry; centered native bounds and 65–75 drawn height.
- Authored material combine independence from environment tint and required state setup/teardown.
- Byte-for-byte deterministic regeneration of all resources and the manifest, with manifest counts/bounds and SHA-256 hashes.

Rusty Key: 1,458 undirected edges, all with exactly two incidences and zero winding imbalance; minimum geometric-normal/vertex-normal dot product approximately 0.7853. Overworld Key: 1,116 such edges; minimum dot approximately 0.99998. Both totals include all closed components.

The recipe uses NumPy only. Validation/parser imports use NumPy and Pillow, already present in the repository's regression environment. Preview rendering also uses Pillow and the installed DejaVu Sans font. Neither validation nor generation requires Blender or SciPy. The test imports the serialized parser but performs no rendering.

Native archive packaging, C++ routing/selection, binary compilation and in-game evidence are coordinated separately by the root worker. This report does not claim those checks or gameplay acceptance.

## Previews and reproducibility

`previews/door-keys-contact-sheet.png` is the reviewed 1500×1140 sheet, showing front, three-quarter and back views of both actual serialized production models. Each includes a 96×96 pickup-size inset. Individual transparent renders:

- `previews/RustyKey-front.png`
- `previews/RustyKey-three-quarter.png`
- `previews/RustyKey-back.png`
- `previews/OverworldKey-front.png`
- `previews/OverworldKey-three-quarter.png`
- `previews/OverworldKey-back.png`

Texture inspection PNGs are decoded from native RGBA16 resource bytes. `previews/preview-hashes.json` records individual-view hashes. `manifest.json` records every production resource hash and each model's exact bounds/counts.

The preview parses actual XML display-list calls, vertex loads, signed normals, UVs and original native texture bytes. It uses the repository CPU renderer's fixed offline diffuse lighting and generic small highlight. It adds no geometry, materials, PBR enhancement, extra textures, displacement or normal maps. Native lit diffuse rendering has no physically based metallic/roughness inputs; perceived metal comes from bevel geometry, native light normals and exposed-edge texels. The exact port lighting, screen filtering, GI framing and rotation require configured in-game review. The contact sheet explicitly says **OFFLINE LIGHTING APPROXIMATION**.

Final production root SHA-256:

```text
RustyKeyDL       21c7ca27453238124e221db48f84a11f93e7d4ba2d8efc0a78bf5c3cff5d9abe
OverworldKeyDL   3e707e96bc5a3cce8e8029df7a75868302753959734c6fe64ef52e5d9d6614fe
manifest.json   5238ab60c3ceb68bf8a4c97c9dc520e67b023cd74c2a08aaefed4688d11df53f
contact sheet   9cd5645f9522dcc4e570eb3e015df60f61257b2190cac9baf57e097b933448a3
```

No staging, committing or publication was performed by the asset worker. All modifications are within the assigned asset/recipe/test ownership paths.
