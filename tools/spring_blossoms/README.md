# Spring native surface candidate

Baseline: `e22bcb222dcd06d18567cbbb47087d3d5979be96`.

Both hosts previously drew shared untextured vertex-colored geometry. The
candidate retains the five rounded petals and detached-petal outline, but
uses authored transparent petal surfaces with fine veins, localized crimson
center and tiny gold anthers. The old raised polygon core/anther overlays
were removed after visual review because their triangles dominated the softer
art. Whole flowers use 30 triangles; detached petals use 12. The existing
24/72 split totals 1584 triangles, down from 2064.

`SOURCE/*-source.png` are the root-provided ImageGen inputs. Source ownership,
prompts and generation references are retained alongside those inputs.
`pack_textures.py` only resamples the supplied artwork to 240x240 and places
it inside a transparent 256x256 canvas, providing an 8-pixel gutter so the
natural alpha edge fits within the preserved native mesh. It does not synthesize
petal color or detail. The packer writes identical resources to both hosts:

- `objects/object_spring_blossoms/blossom_tex`
- `objects/object_spring_blossoms/petal_tex`

Each resource uses the existing OTEX v1 RGBA32 raw convention: physical
256x256 pixels, logical 32x32 tile, HByteScale/VPixelScale 8, 4096 logical TMEM
bytes, clamped bilinear sampling. The resource manifest records source and
serialized hashes. Shared material code multiplies texture * vertex shade in
cycle 0 and combined * primitive in cycle 1, for both RGB and alpha. It owns
only TEXEL0. The original primitive alpha fade, particle tint, motion,
width flutter, lifecycle, RNG, pause and interpolation are retained. Cupping
only changes local Z and neutralizes the old flat blush vertex colors; artwork
supplies most of the new depth impression under the existing billboard.

## Reproduce

From the repository root, with Pillow/numpy and the project's native C++
header dependencies available:

```sh
python tools/spring_blossoms/pack_textures.py
python tools/spring_blossoms/pack_textures.py --check
python tests/seasons/run_spring_texture_tests.py
python tests/seasons/run_spring_combiner_tests.py
python scripts/diagnostics/run_soh_season_particle_tests.py
python tests/seasons/run_native_weather_tests.py
python tools/spring_blossoms/preview_resources.py --output /tmp/spring-previews
```

The static preview exporter reads the baseline header using `git show`, parses
actual packed VTX/triangle lists, decodes actual serialized OTEX pixels and
rasterizes the texture/shade/primitive combiner with fade alpha. The sheet shows
front and angled inspection views plus 16/32/64-pixel bounding boxes over light
and dark backgrounds. Preview outputs are kept outside the checkout and source
handoff. No GIFs or videos are generated. These are software resource previews,
not game screenshots or proof of host GPU, scene or Alt Assets behavior.

The decisive remaining runtime check is the same outdoor spring scene in both
hosts with Alt Assets off/on: verify soft edges, no solid rectangles, fades near
Link/camera, pause/re-entry, and correct following XLU effects. Other seasons
must retain their existing appearance. This candidate is not promoted or
accepted by virtue of offline checks.
