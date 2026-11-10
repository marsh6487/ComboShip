# Authored door-key material and preview report

## Result

The user-approved Rusty Key and overworld key geometry/weathering are retained, with a restrained native metal reflection beneath a static weathering coat. Local key presentation is **−45° Z roll**, applied by shared draw code before inherited turn/Y spin; no position, normal or UV in the approved vertex resources was rotated or edited.

| Owner / root | Unique triangles | Submitted triangles | Serialized vertices | Scale | Upright authored height |
|---|---:|---:|---:|---:|---:|
| MM / `objects/combo_door_keys/RustyKeyDL` | 972 | 1,944 | 2,916 | 0.25 | 70.75 |
| OoT / `objects/combo_door_keys/OverworldKeyDL` | 744 | 1,488 | 2,232 | 0.25 | 70.00 |

21 bundled resources; 610,569 source bytes. There are two submissions of each original triangle, sharing the original vertex resources. Every load uses at most 30 vertices. No new geometry or extra surface offsets are introduced.

## Preserved approved baseline

Root checkpoint: `b0854044`. Local evidence is retained under `tools/authored_door_keys/baseline/`:

- `approved-native-resources.zip`: all 17 original resources, with deterministic archive entries.
- `manifest.json`: exact original SHA-256 hashes, counts and bounds.
- `door-keys-contact-sheet.png`: approved upright matte contact sheet.
- `REPORT.md`: original asset report.

The asset tests verify every archived resource against its original manifest, every current vertex resource byte against the approved version, and every current coat RGB texel against the original native-decoded RGB. RGBA16-to-RGBA32 expansion uses the native interpreter's `SCALE_5_8(VAL) = VAL * 255 / 31` integer rule. Only alpha was added to weathering colors.

The existing original previews remain under `previews/`; the revised still previews are under `previews/sheen/`. The optional rotating GIF can be generated locally with `preview.py --animation`; it is excluded from publication at the user's request and is not a game resource or build/test dependency.

## Native material construction and evidence

The first pass is opaque and uses native lit texture generation. `G_TEXTURE_GEN` and `G_TEXTURE_GEN_LINEAR` produce view-dependent coordinates from signed vertex normals. A new original 16×16 RGBA16 soft reflection map, with `Texture S/T=960`, modulates explicitly set metal colors. The combine equations are:

```text
metalRGB = ((primitiveRGB - environmentRGB) * reflectionTexelRGB + environmentRGB) * shadeRGB
metalAlpha = 1
```

The second pass clears both texgen flags, restores static texture scale `65535`, reloads the identical vertices, and draws the weathering coat using `G_RM_AA_ZB_XLU_DECAL2`:

```text
coatRGB = originalWeatheringRGB * shadeRGB
coatAlpha = weatheringTextureAlpha
finalRGB = coatRGB * coatAlpha + metalRGB * (1 - coatAlpha)
```

Orange oxide, green patina and dark pit texels are fully opaque. Exposed metal coat opacity remains substantial: iron body 185/255, iron edges 145/255, brass body 195/255 and brass edges 155/255. The dedicated patina material is entirely opaque. This keeps the shine beneath fixed surface weathering and concentrates it on exposed metal. No frame counter or scrolling UV command moves rust or patina.

Texture-alpha distribution (out of 1,024 texels each):

| Coat | Fully matte texels | Exposed-metal texels |
|---|---:|---:|
| Rusty body | 891 | 133 |
| Rusty edges | 316 | 708 |
| Brass body | 100 | 924 |
| Brass edges | 127 | 897 |
| Brass patina | 1,024 | 0 |

Both passes keep lighting enabled. The coat is native RGBA32 resource type 1, loaded with `Size=3`; its 32×32 tile uses exactly 4,096 bytes. The separately loaded reflection tile uses 512 bytes. There is one active texture per pass, so no two-texture TMEM overrun or independent UV assumption occurs.

Source support was inspected directly:

- `libultraship/src/fast/resource/factory/DisplayListFactory.cpp` recognizes `G_RM_AA_ZB_XLU_DECAL2`, texture generation, and 16-/32-bit `LoadTextureBlock` macros.
- `libultraship/src/fast/interpreter.cpp` computes generated texture coordinates per vertex while lighting is active, honors `ZMODE_DEC`, converts RGBA16 with `SCALE_5_8`, and imports RGBA32 alpha.
- `libultraship/include/fast/lus_gbi.h` defines the translucent decal mode with depth comparison, framebuffer blending and no depth update.
- `libultraship/src/fast/backends/gfx_opengl.cpp` implements decal rendering with `GL_LEQUAL` and polygon offset. The existing backend's user-configured bias still matters at runtime.
- Existing native `object_nei_mogma_mitts/gMogmaMittsPlateDL`, Nut Bag, Crawl GI and redesigned sword assets already use the same decal render mode.

The model root establishes opaque state before the base pass, restores opaque render mode after the coat, disables texturing and generated UVs, and resets environment color to white. Its explicitly set metal colors prevent inherited CSMC tint from becoming a material input.

## Verification

Executed from the repository root:

```sh
python tools/authored_door_keys/build.py
python3 -B tests/authored_door_keys/asset_test.py
python tools/authored_door_keys/build.py --check
python tools/authored_door_keys/preview.py --animation
```

The **five focused tests passed**. They verify complete owner-local dependencies, unique filenames, native coordinate/UV/normal ranges, texture type/dimensions/TMEM budget, vertex loads and indices, terminating display lists, nondegenerate quantized triangles, closed components, outward winding, normals, dimensions, deterministic resource bytes/manifest, state restoration, identical two-pass geometry, original vertex/RGB preservation, and matte/exposed alpha masks.

Unique geometry remains unchanged: Rusty Key has 1,458 undirected edges, each with two opposite incidences; Overworld Key has 1,116. Every connected component has positive signed volume. The models deliberately contain closed intersecting visual components, with no boolean-union/collision requirement.

| Model | Native minimum | Native maximum | Drawn upright minimum | Drawn upright maximum |
|---|---|---|---|---|
| Rusty Key | (-52,-140,-14) | (52,143,14) | (-13,-35,-3.5) | (13,35.75,3.5) |
| Overworld Key | (-48,-140,-9) | (48,140,9) | (-12,-35,-2.25) | (12,35,2.25) |

The recipe requires NumPy. Tests/parser use NumPy and Pillow, already present in the regression environment. No Blender, SciPy or external art dependency is required. Tests import the parser but do not render previews.

Native parser validation and native orientation/routing verification are coordinated by the root and other worker; no packaging or publication is performed by this asset worker. The user requested preview approval before packaging, and this report does not claim configured in-game acceptance.

## Current actual-resource previews

- `previews/sheen/door-keys-contact-sheet.png`: front, three-quarter, back; small back-view insets moved into empty bottom-left space.
- Optional local animation (`preview.py --animation`): 900×530, 36 frames, 90 ms/frame, 3.24-second loop; not included in the repository.
- `previews/sheen/RustyKey-{front,three-quarter,back}.png`
- `previews/sheen/OverworldKey-{front,three-quarter,back}.png`
- Texture inspection PNGs decoded from current production resources.
- `previews/sheen/preview-hashes.json` records individual still hashes.

`preview.py` reads actual serialized XML calls, vertices, UVs, normals, texture bytes and material state. `offline_render.py` evaluates both passes in native order, generates reflection coordinates **per vertex** before interpolation, applies static-UV alpha weathering, and respects the coplanar coat depth relation. It applies local `Rz(-45°)` before the Y spin and view elevation, matching the draw-code ordering.

**Offline lighting/look-at approximation:** the diffuse light and look-at basis are fixed approximations, not a game capture. Bilinear filtering, orthographic framing and an ideal coplanar depth comparison approximate the native backend. No generic/PBR/specular shortcut is added: the moving sheen comes solely from the serialized reflection texture and native combine equations. Runtime camera/look-at configuration, lighting and decal depth bias still need configured in-game inspection.

Current production root SHA-256:

```text
RustyKeyDL       a0e02ba05d451090d7ea45c59b17b52b00b3022aaca311c4f01c8690fc0cdc71
OverworldKeyDL   c852b35c1581d6c233ab34e01781590e96ad18868f9e70d97697834716dc1e3a
manifest.json   443d0dd35fe63924666a4308ec27738ae2d1b736d7a73e03dae63ff45889994b
contact sheet   52afbf81fd0a534c1b3380e1790cc64c06a95240bcbee8444df22a1f0c4a0569
```

`manifest.json` records all production hashes. All edits remain within the asset worker's assigned asset/recipe/test paths. No staging, commit, package or publication was performed by this worker.
