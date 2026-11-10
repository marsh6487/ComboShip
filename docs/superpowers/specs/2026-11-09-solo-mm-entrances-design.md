# Solo MM entrance shuffle

## Request and baseline

Implement entrance shuffle for Solo MM in cor's current ComboShip candidate.
The working baseline is recovered R22 commit
`191794e8c7534c30a55ee4ab4554e49356c6ca75`; the isolated branch is
`candidate/solo-mm-entrances-20261109`. PR #89 currently publishes R21
`0063920e18145299afc1682aab98407ebd366fc5`. These are distinct states.
Concurrent Rito mask and Clock glass changes remain in their own candidates.

## Design

Reuse the existing MM entrance catalogue and native region predicates. Give Solo MM its own
seed, solver and runtime state. The combined coordinator needs two games, so
calling it for a solo seed or removing all combined-mode guards would create
unwanted foreign travel and state ownership risks. A separate reimplementation
of all portal metadata would duplicate working knowledge unnecessarily.

The user's screenshots extend the requested scope to dungeons, bosses,
overworld, interiors, grottos, decoupling and mixed pools with selected categories.
Expose default-off native MM settings for these, retaining separate shop and
door-interior controls. Preserve all old option IDs and add What's New accents.
The catalogue covers 124 portal identities: five temple entrances (normal and
inverted Stone Tower separately), seven expanded dungeon entrances, four bosses,
42 classified interiors, 26 grottos and 40 paired overworld connections.
The overworld pairs are independently checked against both directions and exact
source/destination regions in the native EXIT declarations. Seasonal spring and
cleared-swamp variants identify the same ordinary portals; special boss warps,
owl landings and authored Black Market/forest portals retain their own identities.

The default policy keeps categories separate and returns coupled. Selected
mixed categories share a permutation pool; enabled categories not selected for
mixing remain in their own pools. Decoupling generates an independent return
permutation. A dedicated Solo layout builder validates both directions and
recovery routes without changing existing combined seed decoding or ordering.

Ordinary boss shuffle credits the source boss door. When bosses actively share
a mixed pool with another category, all four completion effects instead stay
with the physical boss. Applying source credit only to the remaining boss-to-boss
links would duplicate some region clears and make others impossible. Blue warps
and death recovery still follow the saved route policy. Logic, runtime and compass
hints must use this same completion rule.

The native generator installs a candidate layout before MM item placement,
including its glitchless predicates, time slices, physical rusty door gates,
form and song rules. A failed fill may retry a deterministic new layout inside
the existing ten-attempt budget. No-logic modes retain their stated semantics.
Vanilla logic and disabled options use the original graph. Combined and Solo
OoT generation retain their original paths.

## Persistence and interfaces

Native `RandoSaveInfo` gains a POD `soloEntrances` field containing version,
layout seed, pool and mixing flags, catalogue count, bounded 256-entry forward
and reverse permutations,
checkpoint and completed-source-boss mask. JSON serialization uses the actual
selected save's POD; previewing another slot cannot install its layout.
Legacy saves and spoilers without this field clear it to disabled. Imported
maps must match the canonical selected MM catalogue and be bijective within
their configured separate or mixed pools in both directions, with no OoT
endpoints or malformed recovery aliases accepted.

`Rando::SoloEntrances::Seed(const RandoSaveInfo&)` reconstructs the full seed
record, `ApplySeed(RandoSaveInfo&, const nlohmann::json&)` validates/imports it,
and `Clear(RandoSaveInfo&)` resets it. Runtime installation uses
`MM_SetSoloEntranceRuntimeSeed(const char*)`; actual selected native save load
uses `Combo_MM_SoloEntranceOnSaveLoad`. Logic and gameplay use separate state
so generator inventory simulation cannot corrupt live routing.

Map entrance hints and compass boss/reward hints read the saved Solo MM mapping.
Boss completion, blue warps, death, void, save/reload, owl saves and cycle resets
must retain correct local ownership. Foreign arrivals and foreign recovery
queues stay combined-only. Native song/owl warps remain outside the pools.

## Preservation and evidence

Protect R22 source, existing combined shuffle ordering, native solo saves,
physical door ownership, authored private-scene portals and all unrelated
assets/UI features. No merge or promotion to master is part of this task.
Use production-behavior regression fixtures, strict malformed-input cases,
native compilation/linking and actual seed generation where available.
Report configured in-game traversal separately; compilation cannot prove it.
Checkpoint bounded changes and produce a durable source recovery package with
restoration instructions; exclude animated previews, ROMs and private assets.
