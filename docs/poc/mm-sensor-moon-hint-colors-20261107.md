# MM Sensor and moon-child hint corrections, 2026-11-07

Parent source: `e374318dc96024f03a981196fd97df0e0af79707`, candidate
`candidate/hint-colors-mask-menu-20261107`.

## Bounded presentation correction

The Sensor Rune composed its desired item and location as plain text. The
moon-child trial requirement callback also emitted plain required-remains and
transformation-mask names. These messages now use MM's existing native hint
palette: green item spans (`%g`), yellow location spans (`%y`), and explicit white
resets (`%w`) before surrounding prose. This preserves the original wording,
lookups, requirement counts, gates, message IDs and continuation behavior.

All four boss-remains requirements and the three existing Deku/Goron/Zora mask
requirements receive item spans. There was no fourth transformation/Fierce Deity
requirement in the source; type-four behavior remains unchanged.

## Separate confirmed Sensor selection defect

Failing-first tests found that `SensorGetDesire` and the MM menu's
`SensorItemChoices` treated `RI_NONE` as the first enum value. In the actual
`RandoItemId` enum, `RI_UNKNOWN` is first and `RI_NONE` appears after ordinary
items such as Bomb Bag, Bow and the transformation masks. The existing
`item <= RI_NONE` comparison rejected those ordinary wishes and hid them from
the picker.

The runtime and picker now reject values at/below `RI_UNKNOWN`, the explicit
`RI_NONE` sentinel, and values at/above `RI_MAX`. The picker still uses the native
named item table and its existing alphabetical sort. The preview and `(empty)`
selection use the same explicit invalid/no-desire checks. Wish priority, item
identity, outstanding-check filtering and the success/refusal boundary before
the Sensor's existing charge remain intact.

## Verification

`tests/hint_dynamic_colors/run_tests.py` compiles actual MM `Player`,
`SaveContext`, `CustomMessage::Entry`, item enums and slot/quest macros. It
executes the production Sensor helpers, moon-child `OverrideSubJsText`, picker
helpers and complete picker callback. It also runs the real MM
`CustomMessage::Replace` and `ReplaceColorChars` bodies and asserts green/yellow
native control bytes, including embedded zero-byte white resets.

Controlled boundaries are desired-item CVars, names/locations, ImGui/window
services and final textbox/font submission. This is a native headless fixture,
not an in-game glyph/screenshot or complete executable test.

Before changes, the three presentation cases failed their expected color-byte
assertions. Separately, the selector reproduction reported rejected Bomb Bag,
Hero's Bow and Deku Mask wishes, and the production picker omitted those same
ordinary items. After changes, both combined and standalone builds pass under
address/undefined sanitizers:

```sh
python3 tests/hint_dynamic_colors/run_tests.py --sanitize
python3 tests/hint_dynamic_colors/run_tests.py --standalone --sanitize
```

The runner accepts `NLOHMANN_JSON_INCLUDE` (or the normal system include path).
Local verification used
`NLOHMANN_JSON_INCLUDE=/workspace/scratch/a725ee441d3d/publication-deps`.
Coverage includes first-answerable wish order and fallback to the next wish,
unshuffled/obtained/cycle-obtained checks, refusal clearing old text, a complete
enum-ID sweep, negative/oversized/sentinel and slot bounds, real sorted picker
choices/previews/empty states, all-four-remains and all-three-form gates, type
four, numeric-mask count, dismissal and null talk actor. Presentation callbacks
preserve native save bytes; viewing the picker does not write CVars or persist.

A complete `EnJs.cpp` native object compile was attempted with real dependency
headers and normal MM compiler definitions. It stopped at missing
`SDL2/SDL.h`, included by `ShipUtils.h`/`Fast3dGui.h`, before compiling the
translation unit. No whole-unit or executable build success is claimed.

## Audit boundaries

Native MM gossip/bank/spider-house hints already use green items and yellow
locations, with some explicitly authored special NPC palettes. OoT gossip uses
its own authored green-item/red-location convention. These palettes and shared
item/location accessors are preserved. The foreign gossip screenshot fix is
handled separately at its display boundary; this work changes no hint pool,
seed RNG, generation weighting, foreign payload, saved hint identity or tracker
read key.

Both hint trackers deliberately display plain prose with their own read/type
colors. OoT entrance signs deliberately display a bare destination label. Those
surfaces are not treated here as missing textbox formatting.
