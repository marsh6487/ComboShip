# Floating pickup counters — source candidate

Baseline: `candidate/prepublication-outfits-skinning-20261118` at
`8f81ec85d8e36b5e9009c3e7c0491193ba800a8f` (PR #91).
Candidate branch: `candidate/pickup-counters-20261119`.

The accepted design displays an item icon, brief glint, animated current/capacity
and small admitted gain beside the player. Same-type pickups refresh one row;
at most three types stack and fade after 2.8 seconds. The Bombchu mouse glyph and
Deku Stick leaf at the upper tip are included. The complete visual design was
accepted on November 19; configured gameplay remains unverified.

Both games have a default-off **Enhancements > Graphics > Pickup Counters**
checkbox with the gold What's New accent. Size and **HUD Rupee Counter** options
appear beneath the enabled checkbox. The latter defaults to Show; Hide suppresses
the native wallet icon/digits while keeping floating feedback. Disabling the main
checkbox restores the native display. MM's separate Tycoon renderer follows the
same option. Native inventory changes, capacity policy, rupee accumulation and
hook return behavior are preserved.

## Asset and publication boundary

This change contains only UTF-8 production source, native functional tests and
documentation. **No preview images, videos, review generators, texture payloads,
model packs, animation payloads, private logs or saves are included.**

The authored icons and glints use ImGui drawing primitives. Existing inventory
textures and fonts may be reused from the mounted game resources. There are no
new image binaries, base64 image strings, texture banks or compression recipes to
upload; existing resource/archive encoding is unchanged.

## Verification

Run the registered rendering gate commands:

```sh
python3 -B tests/pickup_counter/run_tests.py --sanitize
python3 -B tests/pickup_counter/run_native_tests.py --sanitize
python3 -B tests/pickup_counter/run_hud_tests.py
```

The optional `run_native_tests.py --syntax --deps /path/to/dependency/sources`
also compiles both complete GUI/hook translation units in standalone and Combo
guards, using the project's patched ImGui and native dependencies. Native adapter
tests execute the production functions with real host headers/inventory tables.
The HUD fixture exercises eight actual visibility predicates and the actual MM
and Tycoon digit-rendering bodies under ASan/UBSan, with default/master-off/hide
and invalid-option controls. Visibility settings are absent from inventory/ammo/
rupee update functions. Existing CI checks remain enabled.

The accepted visual review used the actual shared ImGui draw code, with finite
vertex/index checks. Those review-only generators and their output stay outside
the publication. Local checks used GCC/G++ 14 and the combined float-matrix/
32-bit-vertex ABI; only local LeakSanitizer was disabled. Full platform executable
builds, menu interaction and configured in-game presentation remain separate gates.

## Gameplay review

Collect rapid rupee chains and mixed ammo drops; verify live capacities, admitted
gain, spending during a popup, full bags and independent Bombchus without an
ordinary Bomb Bag. Review child/adult OoT and MM forms. Pause/resume, toggle True
Hidden UI, change scenes and switch games; check for stale or duplicate rows.
Review widescreen/mirrored/aspect-corrected edge placement. Switch HUD Rupee
Counter between Show and Hide with ordinary and Tycoon wallets, then disable
Pickup Counters and confirm the native HUD returns.
