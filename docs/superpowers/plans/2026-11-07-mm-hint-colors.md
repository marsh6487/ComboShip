# MM hint color preservation plan

Baseline: `e374318dc96024f03a981196fd97df0e0af79707`, containing the preceding
dialogue/entry/performance-capture source candidate. This is not accepted
runtime evidence. Work on `candidate/hint-colors-mask-menu-20261107`.

The screenshot matches the foreign gossip pool's generated plain sentence,
`<item> can be found at <area> (OOT)`. Native MM free/paid/Moon hints already
mark items green and locations yellow; authored OoT templates use several
different intentional palettes. Shared item/location getters and tracker
records must stay plain.

Apply red item/yellow location presentation only when displaying the known
foreign placement sentence. Recognize both overworld `at` and dungeon
`hoarded in` forms and the OOT suffix; leave unrelated rumors unchanged.
Strip recognized preexisting percent-color escapes within that recognized
sentence before applying the two requested colors, avoiding nested resets and
unsupported OoT cyan/black codes. Preserve the visible wording, trap disguise,
save payload, generated pool order/weights, RNG stream and tracker identity.
This display-only approach also fixes existing saved seeds without migration
or regeneration. No generator/loader/shared-name changes.

Audit other dynamic MM hint surfaces and repair missing spans in Sensor Rune
item/location hints and Moon-child required-item messages using the existing
native palette. Preserve authored dialogue palettes and plain tracker UI.
The audit also exposed Sensor runtime/menu comparisons that treated the
mid-enum RI_NONE sentinel as an ordering boundary and rejected ordinary wishes;
repair those predicates with explicit sentinel/range checks and independent
selector/picker coverage.
The worn-mask visibility option is confirmed OoT-only; answering that location
question does not add a new native-MM feature.

- [x] Reproduce the screenshot with the complete native foreign-gossip callback.
- [x] Add the bounded formatter and exercise the actual MM color encoder with literal expected control bytes, including old saves and already-marked text.
- [x] Verify native/foreign/paid routing, trap disguises, all clarity tiers, generated data/RNG/weights and tracker reveal identity remain intact.
- [x] Verify additional dynamic hint fixes and record intentional palettes/plain surfaces from the broad audit.
- [ ] Review, checkpoint and independently restore the cumulative source; full build and in-game pixels remain separate.

Decisive configured check: reload the reported seed, read the same stone,
confirm Bomb Bag red, Gerudo Fortress yellow and white connecting prose, then
check ordinary MM/paid/Moon/NPC hints and tracker readability. Publication and
runtime acceptance remain separate; the preceding public write was blocked.
