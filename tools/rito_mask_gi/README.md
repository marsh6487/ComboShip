# Authored Rito Mask get-item

This is original procedural geometry following the existing Rito inventory icon's ivory/cream bird face, gold beak and gold feather crest. It is not an icon on a quad, an existing mask substitution or a private donor model. The original OoT icon and name art are preserved byte-for-byte and copied to MM. Original resources remain unchanged.

The sculpture has two bowed ivory shell lobes with real open almond eyes, continuous aperture walls, narrow raised gold surrounds, a thick concave lined back, a projecting hooked and faceted gold beak, five independently sculpted feather vanes, an ivory brow and rear mounting pads. Its 13 closed intersecting components total 768 unique opaque triangles / 2,304 serialized vertex records. The closed-component construction is intended for get-item rendering and does not provide collision geometry or a boolean-unioned manufacturing solid.

## Reproduce and validate

From repository root, using Python 3, NumPy and Pillow:

```sh
python tools/rito_mask_gi/build.py
python -B tests/rito_mask/gi_assets_test.py
python tools/rito_mask_gi/build.py --check
python tools/rito_mask_gi/preview.py
```

Normal generation writes only this recipe's manifest, the new `rito_mask` asset directory in each host and the MM copies of the existing Rito icon/name. `--check` compares generated resource bytes and metadata without modifying files. Preview generation reads serialized production XML vertices/display lists, binary scale matrix and native textures; it does not render an authoring-only mesh. Only static PNG views are supported. No animation/GIF is generated.

## Renderer contract

Entry: `__OTR__objects/nei_gi_redesign/rito_mask/gi_dl`.

Both hosts ship byte-identical standalone resources. The root pushes its native matrix and pops it before returning. Use external draw scale **1.0**, upright with **no local roll**. Front is +Z, crest is +Y. Internal scale is the native 16.16 nearest representable 0.2: **0.1999969482421875**.

| Measurement | Minimum XYZ | Maximum XYZ |
|---|---|---|
| Serialized coordinates | −114, −124, −52 | 114, 125, 51 |
| After internal scale | −22.799652100, −24.799621582, −10.399841309 | 22.799652100, 24.999618530, 10.199844360 |

Swept XZ diameter is **48.735279985**. A frame-bounds catalog can use those scaled bounds and diameter directly. The existing generator in `tools/nei_gi/generate_frame_bounds.py` needs the owning integration's directory-count/catalog update when including this item.

## Native materials and limits

The root uses native v0 Fast3D XML display-list/vertex resources, a v0 binary N64 matrix and five v0 native RGBA16 32×32 textures. Each tile occupies 2 KiB; only one texture is loaded at a time. Triangle loads use at most 30 vertex slots. Signed 8-bit normal components and all integer coordinate/UV fields are validated. Back-face culling and native lighting stay enabled. An original reflection texture and primitive/environment interpolation give gold facets a broad, view-relative native texgen sheen. Ivory and back lining use static UVs and restrained procedural color variation.

The root establishes opaque rendering, its own material colors and texture state. At exit it disables texturing and texgen, restores white primitive/environment colors and balances the matrix stack. The renderer's established external NEI shimmer remains the integration's effect; this recipe does not add timing or particle behavior.

Tests validate every resource dependency, host parity, deterministic regeneration, exact icon/name parity, matrix balance, native texture format/TMEM size, vertex-cache bounds, nondegenerate geometry and signed normals, closed oppositely wound shared edges, positive component volumes, genuine through-open eye apertures, fitted bounds and material cleanup. Static front, three-quarter and back previews were inspected. Offline light/look-at/filtering are approximations. No configured in-game native/foreign receipt, shimmer, Alt Assets or spin/framing acceptance is claimed by these checks.
