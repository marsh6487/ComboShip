# Authored Clock glass sheen followup

Baseline: `5cd05ecf71a05e80030a199a58974a1184641ed3`, the reviewed Clock and
spring source candidate. Cor accepted the original Clock art, then requested
a scrolling sheen to make the watch glass readable.

Candidate branch: `candidate/clock-glass-sheen-20261109`.

## Appearance and resources

A shallow front lens carries a pale cool-white, soft-edged diagonal reflection.
The texture is transparent away from the band and peaks at alpha 70/255; there
is no uniform glass haze. The existing brass, ivory dial, raised hands, blue
enamel, crown and decorated reverse remain in the original fourteen resources.

The lens adds 120 triangles and five native resources per host:

- `gNeiClockGlassTex`: 32x32 RGBA 32, 4096 texel bytes.
- `gNeiClockGlassTexDL`: texture/tile setup.
- `gNeiClockGlassVtx` and `gNeiClockGlassGeoDL`: the curved front lens.
- `gNeiClockGlassDL`: unlit translucent material and lens submission.

The paired contract is texture setup, direct native tile interpolation, then
material/geometry. GlassDL deliberately does not reload/reset the texture tile.
The lens sits inside the bezel (radius 335), with edge Z106 and center Z170,
uses back culling and depth comparison, and does not write depth.

## Motion and routing

The tile origin advances one quarter texel per gameplay tick and interpolates
to the next tick. A 32-texel repeating tile makes the 128-tick wrap continuous.
The texture coordinates on the lens give the reflection its diagonal angle.
No RSP graphics segment is borrowed or rebound for this effect.

Native draws keep the existing Clock scale/spin, model ownership and optional
gold NEI shimmer. The glass belongs to the watch material and remains when
ambient NEI item effects are disabled. Both cross-game directions add the same
paired preparation immediately before their Clock glass draw. The earlier
Stone of Agony replacement remains in place, with no item identity changes.

## Verification record

- Deterministic resource regeneration and nineteen-resource paired closure:
  **PASS**. Both hosts match, and all fourteen body resources match the actual
  git baseline byte for byte.
- Native cache, coordinate/UV/TMEM limits, outward normals and material state:
  **PASS**. Actual serialized triangle overlap gives minimum clearance above
  the raised hands of 6.628342 native units.
- Reflection movement, 128-quarter-texel wrap, transparent clear areas, front
  visibility and pixel-identical rear preview: **PASS**.

- Actual native callbacks and complete foreign compositor command paths,
  including optional shimmer dispatch, owned texture -> scroll -> lens order,
  ten frame phases, effects/Alt flag combinations and cleanup: **PASS**.
- Tight graphics-arena fallback/exhaustion, no orphan texture preparation, no
  RSP segment writes and unrelated custom-item preservation: **PASS**.
- Four original/combined header configurations and five complete native
  translation-unit syntax checks: **PASS**. The native tile-scroll API avoids
  introducing strict-aliasing warnings in the new header.

Asset implementation: `4ec5431c4aa76f369661fc082a1f0a26b6c370ec`.
Runtime implementation: `d1e805257840ed37566b70f05a9bbf6b5f11ee19`.
Independent source/material inspection found no blocking issues; the final
review report accompanies the checkpoint with the reviewed refs.

These are source checks, not a linked game build or configured GPU run. The
command fixtures substitute availability/allocation and unrelated item handlers;
foreign lookup/cache/award queues and actual Alt replacement selection remain
outside their evidence. Static previews evaluate the actual native mesh, texture, alpha and tile-origin inputs.
They show the approved body beside left/center/right scroll phases, matched
angled views, the finished reverse and small-size views. They approximate
lighting, filtering and rasterization; they do not capture live GPU output.

## Source handoff

The new source checkpoint is a narrow additive patch from the named baseline.
The original Clock/spring checkpoint remains the recovery base, with SHA256
`f1928f99ce3af2ea4d84ebf82b505b643e048db78aa9f0e6897a98a39226c8e9`.
Apply only one of the equivalent source/integration patches. Their independently
restored trees must match the final candidate tree in the checkpoint manifest.

Source contains no GIFs, video, APNG or animated preview, and no history bundle.
Static comparison images are separate. This is an implemented source candidate
for preview review. Configured native/foreign GPU appearance and alternate
assets remain runtime checks; no master promotion or publication is implied.
