# Native Din's Fire implementation

The user approved the luminous, longer-hold, burnout/local-light preview and
explicitly requested native implementation, held locally for combination with
their separate vanilla NEI bugfix source before publication.

## Required result

- Normal gameplay camera throughout the cast. The two review angles are never
  spell camera behavior.
- Green precast trails, brief hand flare, red ground-strike pulse, luminous
  horizontally scrolling irregular flame ribbons, soft burnout and local light.
- Provide six seconds of unpaused interaction after normal control returns,
  followed by the accepted 0.6-second burnout. The fire phase lasts 138 native
  20 Hz updates: 8 expansion, 118 hold, 12 fade. Preserve the accepted atlas and
  geometric/UV motion coefficients, using continuous elapsed age through the
  longer hold and rebaking only the derived burnout samples for their new age.
- Return control after strike recovery while the actual actor remains anchored
  to the cast point. The actor retains magic ownership until cleanup.
- The lingering visible fire can light a held Deku Stick and ignite a normal
  arrow crossing it. Its interaction volume expires with visible fire.
- Implement both OoT and MM Din paths. Preserve setting-off/native fallback,
  Fire Medallion behavior, other spells, unrelated NEI work and live resources.
- Keep a default-off **Din's Fire Remake** checkbox in both hosts under
  **Enhancements → Graphics**, immediately after the Fire / Ice / Light Damage
  group. Use the existing What's New accent and the existing setting identity.
- Use the existing native Din's Fire Primary and Secondary cosmetic entries.
  Preserve the accepted full-color artwork at the default palette; custom
  palettes use neutral RGB with identical independent alpha so both entries
  control the fire without an orange color bias. Contact and lingering local
  light follow those same entries; the accepted green startup remains green.

## Baseline and recovery

Source: cd12567fe1c0d64a205e1d360165cb000166f159, tree
97da3eb62dbcab604927d2af3f902a46a6f681b8. Existing durable recovery:
ComboShip_Dins_Fire_Remake_POC2_Horizontal_20261110.zip.
Visual source: Dins_Fire_Burnout_and_Light_Study_Recipe.zip; atlas SHA256
9705407ffa7226f6d4f704a7b20575d66e4aa704759ef3a0670309de78fc186e.
Native integration baseline: 28cd69306bfb4a8d13cac8f5d8810ef85758d306, tree
2dc62dab292882801112e71b996123b2aaef12a1, preserved in
ComboShip_Dins_Fire_Native_Pre_Linger_Checkpoint_20261111.zip.
New branch: candidate/dins-fire-usable-linger-20261110.

## Implementation decisions

Use native actor updates for timing, collisions, ignition and light ownership.
Draw code remains observational and uses private vertices and named resources.
The cast mode is latched for a cast, with explicit release/handoff signaling;
turning a setting off or losing an asset must not strand the player or magic.
Retain actual existing player animation resources and connect release to the
cast action, not to a draw callback. Record remaining pose/runtime uncertainty.
Full application build and configured gameplay proof are distinct from native
translation-unit and production-function harness verification.

The native action reaches the existing frame-30 marker at update 20, holds it
through release at update 21, and returns normal control at update 27. This
keeps the accepted contact flash and local light aligned at 1.00 seconds and
fire release at 1.05 seconds. Native physical hand contact still requires a
configured animation/pose check. Burnout begins at update 147 (7.35 seconds)
and the fire expires at update 159 (7.95 seconds): six seconds after control
returns before burnout, plus 0.6 seconds of fading fire. Native inventory pause
skips actor updates, so time choosing equipment in that pause does not consume
the interaction window. Timing is shared by actors, rendering and asset baking
through combo/render/DinsFireTiming.h.

Preserve the original 8-update growth and 32-update strong-damage falloff. The
extra utility hold retains the existing one-damage tail; it does not stretch
high damage across the longer duration. Collision opportunities still last
longer and target invulnerability controls actual repeated hits. The actor
continues to own magic until cleanup, so additional magic actions wait while
ordinary sticks and normal arrows remain available.

Publication is held. No merge with the separate vanilla pickup work is made in
this pass. Deliver exact commits/trees, binary patch/bundle, changed sources,
asset identities, evidence and combination instructions, with no preview media.
