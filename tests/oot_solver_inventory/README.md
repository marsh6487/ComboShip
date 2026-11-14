# OoT solver inventory isolation regression

Run the native fixture from any directory:

```sh
python tests/oot_solver_inventory/run_tests.py --json-include /path/to/native-deps/include
python tests/oot_solver_inventory/run_tests.py --sanitize --json-include /path/to/native-deps/include
python tests/oot_solver_inventory/run_tests.py --source-rev a85c4ad5d059f41d0c9101bea1e131c776741a70 --json-include /path/to/native-deps/include
```

`CPATH` may supply nlohmann and spdlog instead. `--output-dir` retains the exact generated production excerpts, compiler commands, compile logs, execution logs, and exit codes. Default execution covers original and `COMBO_BUILD`; `--mode original` or `--mode combo` narrows a diagnostic run. Native graphics/controller ABI flags are explicit. `--imgui-include` can supply actual ImGui headers; the default stub defines only GUI value types needed by native headers and executes no GUI operations.

The semantic negative control must compile and fail behavioral assertions. It selects complete production function bodies and metadata from the named git revision, with the current native declarations. The unsafe legacy invalid-index and copied-save-deletion lifecycle probes are omitted when the selected methods lack the solver helper block. The legacy query isolation, item ownership, 16-bit preservation, and combined fill/replay assertions still execute. Do not interpret an extraction/compile error or a crash as the expected negative-control result.

The public baseline above has the same complete Git tree as local verification baseline `95383b7ee50de52540281495b65084fa45f74b35`, used in the retained negative-control receipts.

The fixture uses the real `Logic`, `SaveContext`, `Item`, `OptionValue`, `Context`, and NEI declarations. It extracts complete production `HasItem`, `CanUse`, `ItemUseAllowed`, `HasStrength`, `ApplyItemEffect`, `Reset`, inventory accessors and their small delegates. New private helpers are included verbatim through the marked helper section. It also executes production `ApplyStartingInventory`, `Item::ApplyEffect`, native item-slot/upgrade arrays, `ExtInv_GetItemSlot`, NEI slot getters/setters, and runtime equipment/wand ownership readers. The global setter is wrapped only to count calls; its full production body performs the actual write. Presentation fields are omitted from the NEI registry, retaining item and slot fields parsed from its native table and running the native lookup function over those fields.

The actual `LOCATION(RC_GC_MAZE_LEFT_CHEST, ...)` predicate is extracted from Goron City. The modeled region supplies only the Goron City scene to the real item-use restrictions. No production location requirement is changed. Production `CrossWorldCombinedFill` and `RunPlaythrough` headers drive a small fixture world whose mask and strength capabilities come from those native logic methods. Fixed validated placements force negative fill cases through final validation. A full/mask query primes state before each negative. Self-lock and two-item-cycle placements fail; a free early mask permits the maze prize in a later sphere.

Boundaries: `Context` construction and its singleton/catalog services are local deterministic fixture services; inventory settings use the native `OptionValue` storage. Items have fixed supplied GI metadata so presentation/progressive receipt resolution is outside this test. Starting inventory is an explicit fixture input to the production application loop; this does not test the entire settings-to-starting-inventory generator. Timers/audit notifications are no-ops. Silver-rupee, small-key, unmodeled Pictograph/Power Keg ownership, and dungeon-object branches abort if reached. The full-reset Fire Temple loop boundary returns locked; the tested partial reset still executes its complete production method. The synthetic MM oracle exposes only its victory marker and the portal is explicitly open. This is native behavioral evidence, not a full-game build, complete world search, historical-seed replay, or runtime acceptance proof.
