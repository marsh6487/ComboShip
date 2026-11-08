# OoT Winter terrain candidate — 2026-11-08

| Field | Record |
|---|---|
| Baseline | Cumulative POC4 `d294398c661d7495593cedd4a355f116c3afef0f`; prior seasonal source captured unchanged in `c3fb259b574e7da8b62fe484a38b01dbf905d811`. |
| Rendering boundary | Original Texture/interpreter source captured unchanged in `27b8357d0daaaa3d792c15c300abfef8ac8ed885`. |
| Candidate | Winter-only Death Mountain Trail (`0x60`), Gerudo Valley (`0x5A`), Gerudo Fortress (`0x5D`) ground, cliff, rock, masonry and stairs. |
| Hypothesis | Target material identity rather than green pixels; lift original luminance toward neutral white, retaining texture contrast and scene shading. |
| Preservation | Existing foliage palettes/cohorts; Spring/Autumn/Summer/Off; MM; actors, UI, water/waterfalls, doors, tents, fences, geometry, collision, UVs, progression, audio and packs remain outside scope. |
| Status | Implemented; native CPU/resource/cache and extracted renderer checks pass. Full game/shared-library compilation, GPU rendering and configured runtime are untested. No master promotion or user acceptance claimed. |

103 reviewed resource identities are listed in `SeasonWinterTerrainMaterials.inc`.
Classification used actual native `oot.o2r`, Djipi Map20 and Textures21 artwork,
not color thresholds or inferred filenames alone. Dedicated native CI terrain
palettes are included; wooden CI textures use different untouched palettes.
147 actual serialized texture payloads cover all 103 identities, including the
lower resolution Map20 textures and their Textures21 replacements. Source
identities, hashes and all fixture hashes are recorded in
`winter-terrain-material-evidence-20261108.json`.

The formula is `snow = (168 * maximum + 87 * originalMean) / 255`.
8-bit black maps to 168; white stays 255. RGB output is neutral, and luminance
still varies smoothly instead of painting an opaque solid white image. Native
RGBA16, RGBA32, decoded raw/image replacements, I4/I8, IA4/IA8/IA16 are processed
at their own dimensions. RGBA/IA alpha is unchanged. CI4/CI8 indices remain
unchanged while their dedicated RGBA16 palette is whitened. `LOAD_AS_RAW` CI
resources also keep their packed indices, matching the renderer's CI upload
path. `LOAD_AS_IMG` takes precedence and uses decoded RGBA even if a source
retains an indexed type tag. Original texture
bytes, resource metadata, format flags, dimensions and scaling are retained.
I4 and other low precision formats necessarily quantize the retained contrast.

Native DMT uses intensity textures and colored vertex/material shading. A
new default-off `Fast::Texture::NeutralizeShading` runtime field marks only
whole-terrain Winter copies. The shared triangle interpreter selects its
existing grayscale shader when the marked primary render texture is actually
used. It supplies neutral white grayscale attributes, retaining final shaded
luminance. This resolves inherited Djipi material/triangle child lists without
patching or cloning display lists. UI rectangles and unused/stale/secondary
texture slots cannot opt in. Persistent RDP grayscale state is never changed;
a subsequent draw uses its own texture and original state. This runtime field
is not serialized. Rebuild the executable and all engine-sharing game modules
together because the in-memory Texture layout changes.

Mixed atlases retain explicit normalized bounds: the stone portion of Djipi
Fortress `s12_kabe_09_3A600`, the left stone strip of `s12_kabe_03_2D400`, and
native room 1 ground above its wooden trim. Their protected pixels remain
byte-identical, and whole-draw shading neutralization is deliberately disabled
for these mixed resources so adjacent wood does not turn gray. Their final
stone shade still depends on native lighting; verify these three boundaries
in game. Water, doors, plants, posters, tents and distant town/mountain artwork
are excluded rather than whitening every texture in a scene namespace.

Replacements at any listed material identity automatically use the same
formula regardless of hue, dimensions or the supported texture encoding.
Entirely new authored material paths, different CI palette bindings, and mixed
atlases with rearranged artwork need explicit classification/updated bounds.
This does not require a particular texture pack or a new load order. Resolved
resource/cache handling retains the existing native and Alt path behavior.

Verification completed:

- `python -B tests/winter_terrain/run_tests.py --sanitize`: existing pixel,
  binding, 1505 material-cohort and ground palette suites passed with ASan/UBSan;
  new 309 native/Alt terrain-cohort cases passed, including non-Winter and
  wrong-scene restoration, changed source/reload, metadata and protected assets.
- New format tests cover all nine native texture encodings, raw/image RGBA
  variants, all 65,536 packed RGBA16 colors, alpha/indices, smooth detail,
  protected mixed-atlas pixels and malformed payload rejection.
- `--only-terrain --fixtures <JSON path list>` additionally processed all 147
  real native/Djipi OTEX payloads successfully. Paths can be rebuilt from the
  checked-in fixture evidence and the unchanged input packs.
- The runner compiles the production triangle texture selection, shader
  selection and grayscale attribute fragments against observing boundaries:
  256 primary/secondary/unused/rectangle/existing-grayscale combinations pass,
  with no persistent RDP state writes. This is not full interpreter compilation
  or proof of a GPU/backend executing the path.
- `git diff --check` passed. One pre-existing enum/non-enum warning appears in
  `season_foliage_cohort_test.cpp`; its 1505 assertions still pass.

Decisive runtime check: with matching rebuilt binaries, visit all three scenes
in Winter, native assets first and then Djipi Map20+Textures21 with Alt enabled;
check ground, cliff walls, fortress exterior/interior masonry and stairs at
child/adult setups. Compare texture detail/shadows and the three mixed-atlas
boundaries, confirm valley river/waterfall and doors/actors/UI retain their
colors, then switch Winter -> each other season and leave/re-enter the scene.
Repeat once with a differently colored/resized stone replacement at a listed
identity. Native and Alt runtime parity remain untested until demonstrated.

Recovery: retain the POC4 source and exact boundary checkpoints. The isolated
feature commit owns only the terrain source, Texture/interpreter additions,
focused tests and this evidence. Root voice, Pegasus and form-blink edits were
not staged or changed by this task. The parent recovery package must retain
these tracked changes and restoration refs; no remote publication occurred.

Follow-up format review: packed RAW CI4/CI8 regression fixtures reproduced the
initial rejection before the correction and pass after it. The tests also
check dedicated TLUT whitening/alpha, source and index preservation, IMG+RAW
precedence and rejection of malformed RGBA-sized RAW CI payloads. This addresses
future replacement formats without changing any input asset or load order.
