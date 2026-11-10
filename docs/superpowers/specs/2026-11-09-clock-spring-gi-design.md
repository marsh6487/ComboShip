# Authored OoT Clock and spring blossoms — candidate design

Baseline: `e22bcb222dcd06d18567cbbb47087d3d5979be96`, cumulative shuffle candidate.
Work only on `candidate/clock-spring-gi-20261109`. The cumulative checkout is
being published by another work chat and must remain untouched.

## Goal and art direction

Cor requests an authored GI for OoT's Clock, optionally with NEI shimmer, and
an iteration on the spring cherry blossoms, which read like papier-mache
cutouts in game. Cor has not selected a Clock art direction. The following
direction is the assistant's first reviewable proposal, not prior user approval.

The Clock will be a Hylian timepiece: aged brass bevels and a substantial rim,
ivory dial with restrained blue enamel accents, raised hands and markers,
small sun/moon detail, a shaped crown/loop, and an intentional finished back.
Use real authored geometry and native resources. Considered a miniature tower
clock and a magical hourglass; a portable timepiece has the clearest new Clock
silhouette and fits the accepted aged key art. Shimmer should use established
NEI behavior and its existing effects control, without hiding the dial.

The spring baseline is shared untextured vertex-colored geometry, not a bad
external texture. Preserve the accepted five rounded petals, localized
crimson core, small gold stamens and pale blush palette. Replace the flat
surface impression with soft petal detail, varied translucency and restrained
curvature. Preserve the 24 blossoms/72 loose petals, all particle motion,
visibility, fades, season controls, RNG, pause/interpolation and scene behavior.
Texture and geometry may change only where needed to improve this appearance.

First actual-resource preview review: retire the old polygonal red-core and
triangle-stamen overlays, because the new texture already supplies the accepted
crimson/gold detail and the duplicated overlays retain the cutout appearance.
Keep the outer five-petal XY silhouette and fit the natural texture edges just
inside it; use a transparent packing gutter instead of repainting generated art.

## Interfaces and preservation

Clock assets live under `objects/object_nei_clock/`, with opaque entry
`gNeiClockDL` and optional translucent entry `gNeiClockGlassDL`. Both game
archives carry identical authored resources so original and combined builds
do not require the other game's ROM assets for this GI. The model is centered
near the dial, with roughly a 400-unit radius and a 1000-unit total height.
The renderer owner confirms final sizing against existing get-item conventions.

Route native OoT `RG_CLOCK`, native MM `RI_OOT_CLOCK` and the foreign Clock
routes to the authored model. MM's six half-day Clock items retain their
existing tower-clock appearance. No item IDs, seed logic, progression or
save-state changes. Reuse existing NEI shimmer APIs, restore draw state and
retain effects-off model rendering.

Blossom assets must resolve identically in both games with and without Alt
Assets; keep resource paths unique. Preserve native texture/TMEM requirements
and avoid per-particle allocations or new emitters. Keep the particle triangle
budget at or below the existing 2064-triangle maximum layer where practical.

## Evidence and review

Show previews rendered from the authored resources, including normal-size
readability and depth views. Generated texture art is an input, not proof of
the final model or in-game appearance. Validate actual host draw routes,
texture/material restoration, cross-game and effects-off behavior, and the
existing season lifecycle gates. Static/offline proof does not establish GPU
runtime acceptance. Deliver previews before a user-facing install package or
publication; maintain a recoverable source checkpoint as required by AGENTS.md.

Cor explicitly requires no GIFs, videos or animated previews in the source ZIP.
No animation outputs are committed for this pass. Source recovery exports omit
binary Git bundles that could reintroduce old preview media through history;
use a verified binary-safe source patch and changed files instead.
