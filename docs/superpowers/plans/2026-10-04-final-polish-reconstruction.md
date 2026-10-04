# Final polish reconstruction

Recover the authorized final ComboShip polish after its transient source worktrees disappeared. This plan resumes the accepted checklist; it does not reopen product design. Source restored from the partial ZIP is exact; rebuilt missing source must earn new verification. Published baseline is `1c29c83f5d676d02ececa7302c138f7aa46b952a`; recovered-header checkpoint is `d1dfbc7b`.

## Binding constraints

- Publish the completed candidate to the existing PR #34 branch `poc/mm-gi-routing-followup-20261003` in `marsh6487/ComboShip`. Existing user authorization covers this. Keep the PR draft; no merge or master promotion.
- Preserve save layouts and valid balances, seeds, Rod of Seasons controls/weather/save/shop behavior, the Maps and Compasses Give Information setting, existing held geometry, Pegasus attachment, native forms and player-model/mod priority.
- Cojiro GI and 23 HD icons remain optional asset-only packs. Do not reinsert their replacements in built-in game trees.
- Hylia's Grace is MM item 194/0xC2; Fire Rod is 192/0xC0. Inventory slot 41 is Phantom Hourglass. Do not revive the retired Grace ability. Desire Sensor is a Slate power.
- Distinguish source/compiler/platform-build verification from in-game acceptance. The old 43-command gate is historical evidence only.

## Task 1: Wolf runtime

Own the MM Wolf port and its shared MM skin renderer, isolated from other tasks. Reuse the existing OoT `wolf_link_form.cpp` and `wolf_link.bin` format. Restore movement, dash, attacks, animation and rendering; Shadow Crystal toggles through full-width C/D-pad IDs. Missing/invalid assets must retain the normal player. Native mask transitions, other custom forms, scenes/death/teardown release input, colliders and rendering. Preserve native MM damage/freeze/thaw action ownership; Wolf must not reset it or double-apply damage. Binary validation must remain effective under fast-math, including NaN/Inf and bounds. Preserve Pikachu's renderer/root-motion/scale/animation settings. Use the recovered editor/Wolf spec as the detailed contract. Add production-code behavioral and malformed-asset fixtures plus real-MM-header syntax checks. Do not edit editor, item receipt/draw files or root gate script.

## Task 2: MM save editor and wallet digits

Restore Slate (slot 39), Hourglass (41), Shadow Crystal (44), Seasons (47) grants through production randomizer grant handlers. Grant All enumerates current NEI identities and all five Slate powers, all four seasons and cane/wand variants; do not only grant icons. Preserve ordinary inventory and repeat safety. Investigate new NEI-enabled Hourglass pools without rewriting existing seeds. Verify Grace/Fire Rod IDs and GI bindings. Fix native MM HUD and Tycoon wallet paths for restored balance 1667/wallet level 2 and all representable signed16 balances: every texture index stays 0..9, larger legitimate balances display safely, no currency/save clamping. Add production grant fixtures and sanitized digit boundary tests. Own `SaveEditor.cpp`, related editor catalog helpers/tests, `z_parameter.c`, `TycoonWallet.cpp`, a small shared digit helper and tests. Do not edit Wolf, item draw/receipt or root gate script.

## Task 3: GI model presentation and shimmer

Recover the host implementations corresponding to the exact restored draw headers/ABI. Preserve item-specific shimmer independently of custom/legacy model selection, with a separate effect pass and no geometry tint. Do not apply authored model-local effects to arbitrary external meshes. Verify fairy bottle TP blue-fire shell, bouncing pink fairy, one pink shimmer and partial-mod priority. Fix standalone sword +X blade to +Y orientation, removing the extra X rotation that laid selected Kokiri/Master and legacy Four Sword flat. Audit all 61 authored equipment GI model bounds/scales/pivots/rotations against pickup/shop/freestanding fit; adjust host fitting without reauthoring held geometry. Four Sword must fit vertically. Preserve Morpha eyeball/flame and retired tentacle paths, native boss-soul flame RGB, Double Defense heart, Lantern/Poke Ball/Mario routes. Restore themed song effects using recovered profiles and actual feather geometry for Soaring. Own GI host renderer/presentation files, fitting/shimmer helpers and GI tests. Receipt task owns receipt/icon/textbox code; coordinate if signatures cross. Do not edit Wolf/editor/wallet or root gate script.

## Task 4: Receipts and icon ownership

Preserve traditional OoT song/map/compass/small-key/boss-key receipt bodies when delivered in MM, including progressive and concrete routes. Enabled map/compass information appends to the traditional body. All MM song variants use descriptive localized receipts and colored clef textbox icons matching recovered song GI palette. Ensure early NEI song dispatcher reaches song presentation. Yellow magic receipt includes the RPG magic/stat remaining-upgrade count. Skulltula receipts retain OoT/Swamp/Ocean running totals. Exact boss soul name RGB matches recovered `ComboBossSoulColor.h`: Goht #0A8A2E, Gyorg #1363A5, Majora #E88015, Odolwa #911485, Twinmold #A8B414. Goht's Remains remains red. Native flame design remains unchanged. Ikana Mirror Shield receipts/pause/editor select MM-owned resources/overrides, including cold aliases, never OoT icon ownership. Own receipt exporters/builders, icon ownership, textbox glyph/tint implementation and tests. GI task owns effects and model rendering. Do not edit Wolf/editor/wallet or root gate script.

## Integration and publication

Independent tasks work in isolated branches and return commits, covering test commands/results, clear limitations and their report paths. The root reviews each change, integrates sequentially, updates the combined NEI gate with meaningful tests, checks real translation-unit syntax, formatting, whitespace and conflicts, and requests independent review. Fix review findings before publication. Check remote head before a non-force update; preserve concurrent corrections and actual ancestry. Verify uploaded tree equals the locally tested tree, then check complete Windows/Linux build/package gates on that exact head. Keep a durable Git checkpoint during upload so another interrupted session can resume.

## Recovery record

The supplied ZIP verifies 14 exact blobs from staging tree `dc4171c3587e52e62359ab6ee1f06ecaf03a870a`. Complete historical candidate `cb1d103b797f9f8bb8b9d449a67d6da2f4afd5bc` / tree `b3779f1f013d56c1048f181712b5e54a1aaad1f3` is unavailable. Do not label reconstructed source as byte-for-byte recovery.
