# Final cumulative gate follow-up — October 19, 2026

## Woods release scope

cor clarified that only these Woods of Mystery integrations belong in the bundled build:

1. Green hex shimmer at the OoT/2Ship bridge.
2. The exported wood tunnel replacing the original doorway, with its minimal native wall/canopy fit support.
3. Seasonal tint routing, including private grass, wall-trim, foliage and grass-actor resources.

The complete Woods makeover assets are not bundled. The normal stock archive input contains exactly eight files under `mm/assets/custom/custom/cor_woods_bridge/`; it contains no Woods scene archive or full makeover textures. Full-scene authoring tools and recovery recipes remain source records outside the stock asset tree.

## Transition fixture link correction

The exact published candidate `b3049a6e285b6e3a0536cc8ac3e33f71b653e07f` passed hosted formatting and the repaired native weather/winter preflight. Its [Build Artifacts gate](https://github.com/marsh6487/ComboShip/actions/runs/39338675119) then reproduced a local transition-fixture link failure: the copied MM host block includes the unrelated `MM_ApplyComboPortalArrival` entrypoint, which references `MagicMirror_QueueArrival`.

The cross-dungeon test exercises native fades and handoff hooks, not that arrival entrypoint. Its MM handoff compiler command now uses function/data sections and linker section collection, matching the existing standalone forest fixture. Executed handoff code still links and runs under address/undefined sanitizers; no game implementation, callback or arrival state is changed. Actual Mirror arrival behavior retains its separate production runtime/forest fixture coverage.

The original command failed with the missing Mirror callback before this correction. The focused MM handoff sanitizer command and the complete four-variant transition command pass afterward. The remaining cumulative gate and exact-head full application builds are separate pending checks, not inferred from this fixture result.

Recovery inputs and the complete 45-workstream inclusion remain recorded in `final-cumulative-merge-20261019.md` and `final-cumulative-checkpoint-20261019.json`. This source follow-up does not claim configured gameplay acceptance or promotion of any external scene master.

## Remaining fixture dependencies

The broader gate exposed additional fixture maintenance after combining the accepted workstreams:

- The shared OoT key/silver-rupee cosmetic fixture now includes the actual pure-data `BlackMarketShelves.h` dependency used by the production editor's default shelf tint. Full key/heart and SoH compatibility groups pass, including sanitizer cases.
- The foreign-soul runner now extracts the actual native `SkelAnime_ApplyNpcSkin` helper along with its draw functions. The fixture explicitly models boss souls with no NPC skin plan, executes the real helper's null-plan path, and rejects unexpected skin blending. The complete foreign-soul/imported-boss group passes in normal and sanitizer configurations; NPC replacement skinning has its own separate coverage.
- The young-Epona actor fixture supplies the combined-game mode and fails if forest-only presentation services are unexpectedly called in its native young-horse cases. Its player compiler also includes the existing `combo/menu` header path, required by elemental-sword receipts. The complete Epona actor/player/save/cosmetic group passes, including full player/horse translation-unit checks.

Each missing dependency was reproduced before correction. These changes affect test fixtures and their compiler commands only. No game implementation, item behavior, scene resources or asset packaging changes are made in this follow-up. Exact-head hosted full application builds remain a separate gate.
