# Authored Clock GI and spring blossom candidate

Baseline: `e22bcb222dcd06d18567cbbb47087d3d5979be96`, the cumulative shuffle
checkout already containing the reviewed soul and authored-key work.
Candidate: `candidate/clock-spring-gi-20261109`.
Implementation reviewed through `e52a3ce18f83df83f4503d0158ab6783641cf307`.
Independent review completed at `ddeae7ab91462f1ccfa3de349aed7ab111fd26fb`
with no blocking correctness or specification findings. Final status edits are
documentation only; executable code and resources remain unchanged.
Masters and the other chat's cumulative checkout remain untouched.

## Clock

The new native mesh is an aged Hylian timepiece: a bevelled brass case, recessed
ivory face, raised blue hands and hour markers, sun/moon inset, suspension loop,
blue enamel waist and decorated reverse. It uses native metallic reflection
and static weathering. All fourteen resources under
`objects/object_nei_clock/` are byte-identical in both game archives and have
no external ROM/model dependencies. Opaque entry: `gNeiClockDL`.

Native bounds are `[-403,-403,-112]` to `[403,601,147]`. Shared scale `0.04`
gives approximately 32.24 x 40.16 x 10.36 world units. The mesh has 2532 unique
triangles and 4068 submitted triangles including the brass material's second
pass. Native vertex batches, signed coordinates/UVs, outward normals, texture
footprints, resource closure and exit state are verified.

| Route | Candidate |
| --- | --- |
| OoT `RG_CLOCK` | Authored Clock from OoT's own resources |
| MM `RI_OOT_CLOCK` | Same authored Clock from MM's own resources |
| OoT Clock displayed through MM's foreign route | Authored Clock; Stone of Agony substitution removed |
| MM Clock alias displayed through OoT's foreign route | Same authored Clock with explicit owner routing |
| Six MM half-day clocks | Existing tower-clock models and dispatch retained |

The existing NEI ItemEffects control enables gold `Hourglass` shimmer around
the Clock. Turning it off retains the body. Existing appearance refresh keeps
foreign toggles live. No new preference, item identity, save or gameplay change
is introduced. Clock body/material previews are rendered from serialized
resources; they do not show a captured in-game shimmer effect.

## Spring

The prior spring particles were untextured vertex-colored geometry. The new
shared material uses authored transparent blossom and petal artwork, restrained
cupping and soft surface detail. Tiny crimson/gold centers are part of the
texture; redundant polygonal core/stamen overlays were removed after preview
review because they retained the cutout appearance.

Both hosts carry identical OTEX v1 RGBA32 resources at physical 256x256 with
raw8x scales, sampled through a logical 32x32 tile (4096-byte logical TMEM).
An eight-pixel transparent gutter lets most edges follow the natural artwork
instead of the mesh boundary. Some narrow whole-flower valleys still follow
the preserved polygon boundary at enlarged inspection size.

The production combiner samples owned TEXEL0 only, multiplying texture and
vertex shade in the first cycle and existing particle tint/fade in the second.
Layer cleanup restores the following consumer's XLU material. Counts remain
96 slots, 24 flowers and 72 loose petals; motion, flutter, visibility, RNG,
pause/interpolation and other seasons remain unchanged. Maximum spring-layer
geometry drops from 2064 to 1584 triangles (30 per flower, 12 per loose petal).

## Verification

- Clock deterministic asset rebuild, fourteen-resource closure, native cache,
  TMEM/range/winding checks and matching host resources: **PASS**.
- Actual Clock native callbacks, foreign export/owner-path/custom draw commands,
  gold shimmer and effects-off behavior, Alt flag combinations, frame wrap,
  matrix restoration, grayscale cleanup and owner-scope balance: **PASS**.
- Seven full native translation-unit syntax checks across original/combined
  engines, including MM's real exporter; four shared-header configurations:
  **PASS**. These are syntax checks, not a linked game build.
- Existing full NEI GI, imported fallback and foreign texture/scale/lifecycle
  regression suites: **PASS**.
- Both hosts' seasonal renderer/weather/lifecycle suites, exact autumn texture
  preservation, native spring TextureFactory loading and lifetime, and actual
  shader-generator TEXEL0/alpha dependencies: **PASS**.
- Spring pack `--check` is nonmutating and validates exact regenerated resources
  and manifest. Matching and intentionally corrupted isolated-copy cases:
  **PASS**.
- The two newly registered CI entries were replayed through the real regression
  runner on the final implementation: **PASS**, 19.96 s for Clock and 24.66 s
  for spring texture/combiner verification. Platform-hosted CI was not invoked.
- Static front/depth/rear Clock views and before/after spring views, including
  normal-size inspection on light/dark grounds, were generated from actual
  serialized resources and inspected. They are offline appearance evidence.

Focused foreign fixtures execute actual owner routing/custom body code and the
shared shimmer renderer separately. They do not replay the complete foreign
compositor. No newly linked executable, installed-pack GPU acceptance or user
art acceptance is claimed. Existing compiler warnings remain warnings; no
new compilation failure was observed.

## Recovery and preview separation

Cor explicitly requires no GIFs, videos or animated previews in the source ZIP.
The inherited authored-key GIF is removed from this candidate; export-ignore
rules omit legacy preview media from ordinary source archives. The recovery
ZIP contains source patches, changed source/required authored assets, tests,
documentation and small verification records. It contains no Git history bundle
that could bring back old animation bytes. The payload scanner also rejects
animated PNG/WebP and media inside nested ZIPs.

`integration.patch` applies this pass to the exact cumulative baseline above;
`source.patch` restores the full candidate from published R21
`0063920e18145299afc1682aab98407ebd366fc5`. Both patches are independently applied
to separate indexes and must produce the exact recorded candidate tree. The
key-GIF deletion carries full-index deletion metadata only, without embedding
its previous image bytes. Required source artwork and deterministic conversion
tools are included. User-facing static previews are delivered separately.

This is a reviewable candidate. The remaining decisive check is configured
gameplay: native/foreign Clock pickup with effects on/off and both Alt settings,
then an outdoor Spring scene in each game for natural edges, fades and following
XLU effects. Publication and promotion have not occurred in this pass.
