# NEI editor coverage and MM Wolf Link

Baseline: ComboShip PR #34 at `91aa4f9aa672873fc8e5f04bd09a4dc296c114ca`.
The user requested restoration of the four empty inventory entries through the save editor,
grant-all coverage of the current NEI item table, investigation of Phantom Hourglass seed
availability, and the existing OoT Wolf Link functionality in 2Ship.

## Behavior

- Keep the canonical inventory slots: Slate 39, Hourglass 41, Shadow Crystal 44, Seasons 47.
- Drive individual grants and grant-all from current randomizer item identities and their
  grant handlers. Include all five Slate powers, all four seasons, and existing cane/wand
  unlocks; granting an icon alone is insufficient. Preserve existing NEI grants.
- Determine whether Hourglass is present in newly generated NEI-enabled pools. Do not
  rewrite existing seeds or claim why an unidentified older seed lacked the item.
- Port the existing OoT Wolf model loader, animation, attacks, dash and locomotion to MM.
  Shadow Crystal toggles the form using full-width item IDs on C and D-pad buttons.
- Missing or malformed model data must leave the normal player intact. Native mask
  transitions, other custom forms, scene teardown and death must release Wolf ownership,
  colliders, input restrictions and rendering safely.
- Reuse the existing Wolf binary when compatible. Preserve MM Pikachu rendering and the
  existing native forms, custom player models, equipment and optional GI/icon pack behavior.

## Validation

Use production-code fixtures with real MM headers for grants, input/form transitions,
skin root motion/interpolation and attack behavior. Check changed translation units and the
existing NEI regression suite. Runtime acceptance still requires the actual game with the
Wolf model asset; no master promotion is implied by source/build checks.
