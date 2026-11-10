# Clock glass sheen design

## Intent and baseline

Cor approved the authored Clock's appearance and requested a scrolling sheen
that makes its glass face readable. Baseline is
`5cd05ecf71a05e80030a199a58974a1184641ed3`; the prior source checkpoint is saved.
Candidate: `candidate/clock-glass-sheen-20261109` in the existing isolated
clock-spring-gi worktree. Prior GPU appearance remains unproven.

## Bounded design

Add a shallow clear lens just inside the bezel: radius 335, center Z170,
edge Z106, with actual triangle clearance above the raised hands and markers.
A pale cool-white diagonal reflection passes across the lens; native texture
alpha peaks at 70/255 and is zero away from the band. There is no uniform opaque
film. Glass is front-facing/back-culled and depth-tested without depth writes.
The body resources, blue enamel, brass aging and finished reverse are preserved
byte-for-byte. The new glass belongs to the Clock material regardless of the
optional gold NEI shimmer setting. This is an implementation choice for review,
not an assertion that the new glass effect has already been accepted.

## Approaches and chosen implementation

A moving highlight on the existing ivory dial would not sit over the raised
hands; a separate glass surface provides that depth. A segment-08 scrolling
display list was considered, but GPU segment state is not reliably recoverable
from CPU gSegments. The selected approach has no borrowed RSP segment:

1. `gNeiClockGlassTexDL` loads an owned 32x32 RGBA32 reflection texture.
2. The native Clock wrapper emits direct `gDPSetTileSizeLerp` for tile 0.
3. `gNeiClockGlassDL` establishes XLU material and submits the lens geometry
   without reloading the texture or resetting its tile offset.

The offset is `(uint32_t(frame) & 127)` quarter-texel units, next endpoint
`offset + 1`; mask 5/repeat gives a continuous 128-tick loop with native frame
interpolation. The preview evaluates the same texels, native mesh, UV mapping
and external phase input. Existing CUSTOM_GI body/glass split and narrow
Clock-only preparation hooks cover both foreign directions without new ABI.

## Preservation and evidence

- Keep all 14 prior Clock resources byte-identical, existing scale/spin, native
  item identity and cross-game fallback replacement.
- Preserve optional gold NEI shimmer control, all six MM half-day clocks,
  spring changes, other assets, UI, seed generation and save data.
- Reserve graphics commands/matrices before drawing; retain balanced resource
  owners, no segment writes, known render-state cleanup and original/combined
  host compatibility.
- Verify actual callback/custom-drawer commands, scrolling wrap and effects
  on/off; inspect real resource previews and lens/body clearance.
- Show static phase/comparison previews outside source; no GIF, video, APNG or
  animated preview in the source checkpoint.
- Preserve independent source recovery. No master promotion or publication in
  this material pass. Configured in-game appearance remains a separate gate.
