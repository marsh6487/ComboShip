# Starting ammo and Impa receipt attribution

Baseline: `d3cf01741084c188eb0a01a263018fa3de23fbe1`, with the existing
November 13 exact-ammo transfer and capacity-receipt replay corrections.

## Starting sticks and nuts

The supplied `Combo-02-36-07-93-90.json` (Library identity
`libfile_34cbd2f2108c8191af6f7943388d72e3`) explicitly records:

| Seed setting | Value |
| --- | --- |
| `gRandoSettings.StartingSticks` | `0` |
| `gRandoSettings.StartingNuts` | `0` |
| `gRando.Options.RO_STARTING_CONSUMABLES` | `1` |

MM's `Rando::GrantStartingItems` gives both consumables and sets their ammo to
the current capacity when that MM option is enabled. The ordinary shared
inventory then carries those amounts to OoT. Both native new-save inventory
initializers have zero ammo, and this MM option and both OoT starting-ammo
options default off. No unconditional fresh-save refill was established.

The source change clarifies the existing MM **Consumables Full** tooltip:
in Combo these amounts also enter OoT's shared inventory. Turning the option
off when generating future seeds keeps starting ammo earned through gameplay.
Explicit starting-item lists, capacity pickups and Infinite Upgrades retain
their existing behavior. No existing seed, inventory, or save is rewritten.

## Impa receipt question mark

Both startup receipt adapters previously emitted the toast suffix
`<item> — Song from Impa (OoT)`. `Notification::Window::Draw` uses
`ImGui::TextColored` without selecting a different font. The configured GUI
initialization loads `AddFontDefault` plus the FontAwesome icon range.

The repository pins ImGui `v1.91.9b-docking`. Its `imgui_draw.cpp`
`GetGlyphRangesDefault` includes U+0020 through U+00FF; U+2014 is absent.
Its fallback selection tries the replacement glyph, then `?`, then space.
The relevant pinned source was inspected directly at
<https://raw.githubusercontent.com/ocornut/imgui/v1.91.9b-docking/imgui_draw.cpp>
(SHA256 `8823f49f5c7a5181c0435a5fa01a03c3ff5bece283482b81001e71a66993f997`).
This explains a question mark **after the item name in the notification**.
Both adapters now use the ASCII separator ` - `, preserving item identity,
source attribution and acknowledgement behavior. The native item textbox and
all 3D get-item model routes are unchanged. If the report concerns a floating
3D question mark or the native textbox, that separate symptom still requires
configuration-specific visual evidence.

## Verification

- `tests/ammo_sync/run_tests.py --sanitize`: passed, zero native-host failures.
- `tests/startup_receipt/run_tests.py --sanitize`: passed, including native and
  foreign receipts, all MM skip policies, exact-once grant/acknowledgement,
  copy/reload/context boundaries, and save-creation ordering.
- Both actual startup adapter fixtures now require the exact ASCII toast
  suffix, covering both source-owned games and MM's native/foreign cases.
- `git diff --check` passed for this bounded change.

The tests used the repository-pinned nlohmann JSON `v3.12.0` single header
(SHA256 `aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63`).
No full executable build, installed-game reproduction, user acceptance or
publication is claimed.
