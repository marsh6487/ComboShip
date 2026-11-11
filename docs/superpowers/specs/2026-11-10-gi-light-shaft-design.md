# Optional item-get light shaft

## Intent and baseline

Cor requested an opt-in shaft shining down on Link during item presentation,
using the supplied 3.329-second remake clip as a reference. The inspected clip
shows broad warm light, feathered vertical streaks and subtle moving haze. It
clears when the presentation ends. Exact scroll direction is not established by
the short recording; slow downward texture travel is the implementation choice.

Baseline: published PR #90 head `a5b1abd697d284ecbca70c13091562821dfc3feb`,
tree `da99bc7dd7191661e4dae8f829b2a98d2f22602b`. Isolated branch:
`candidate/gi-light-shaft-20261110`. Preserve all adult GI framing, trade fixes,
item models/shimmer, receipt timing, cameras, audio and seasonal atmosphere.

## Approach

Use a small feathered world-space mesh with an immutable procedural I8 texture.
This follows the existing summer beam rendering approach while allowing the
reference's subtle movement. Reusing the seasonal state would introduce weather
and daylight gating; a screen overlay would not respect scene depth or Link's
world position. A full volumetric renderer is outside this bounded source pass.

Mark actual native held-item draws and MM's drawn custom receipt actors. Consume
those value-only markers once at the end of each world's draw. Do not hook the
generic item renderer, because it also draws shops, pause previews and world
items. No extra actor, external asset archive, light slot or shared RNG use.

One shared CVar, `gEnhancements.Graphics.ItemGetLightShaft`, defaults to zero in
both hosts. Label: `Item Get Light Shaft`, under Enhancements / Graphics beside
Elemental Spin Effects, with a New entry in the existing What's New registry.

## Visual and lifecycle contract

- Warm ivory/gold, soft sides and ends, slow downward texture travel.
- Anchor at Link's world position; size from native/form/adult presentation
  height and the actual item height. Remain vertical as camera and item rotate.
- Fade in briefly; clear when the held receipt ends. No lingering actor.
- Time advances on gameplay ticks, not repeated render calls; hide during pause
  and transitions, and reset on scene/play lifetime changes or clock rewind.
- Default-off submits no new render commands or arena allocations.
- Keep native/foreign/custom receipt coverage, and exclude exchange displays,
  chest opening before the item appears, minor overhead pickups and Kafei.
- Check arena space before mutation and restore the matrix/material boundary.

## Evidence and acceptance

Behavioral native tests, production translation-unit compilation and source
review establish implementation evidence. They do not establish the in-game
appearance. Runtime checks remain: native and foreign receipts in each host,
child/adult and short/tall forms, a transparent GI, toggling off, pause/transition,
and an Alt Assets comparison. Publish/merge/master promotion is not requested.
