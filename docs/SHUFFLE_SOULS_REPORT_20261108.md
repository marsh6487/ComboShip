# Native/shared soul parity implementation record

Task 3 candidate begins at `fa0478de` on isolated `work/shuffle-souls-20261108`.
No master promotion, full-game execution, or runtime acceptance is claimed.

## Compatibility milestone

The real `BenJsonConversions.hpp` serializer rejected a shorter historical
`randoInf` array (observed RED: `old shorter native randoInf array cannot load`).
The decoder now zeroes the destination and copies only known words. This preserves
old indices, clears missing ownership even when reusing a populated save context,
and ignores unknown future words.

Command:
`CPATH=/workspace/scratch/198fa7c65e91/build-deps/include:/workspace/scratch/198fa7c65e91/build-deps/sysroot/usr/include python3 tests/shuffle_parity/souls/run_save_tests.py`

GREEN: actual native MM save codec roundtrip, shorter/reused/empty tails and longer
future tail. Combined graphics ABI flags are explicit. This milestone introduces
no soul items, options, actor gates, shared edges, or behavioral completion claims.
Initial compile lacked spdlog headers; resolved by the prepared dependency sysroot.

## Native identity/grant/pool milestone (WIP)

Compatibility commit: `bd18d935416b0cdb67879125081db419725d473e`.

Added 154 semantic native identities: 43 OoT enemy, 46 OoT/53 MM NPC, four
animals per game and two miscellaneous souls per game. The semantic catalog is
consumed by both native adapter tables. All serialized item/option/flag IDs append
after the previously published IDs. Existing 47 MM enemy souls and FC legacy
indices are unchanged; no duplicated FC aliases were added.

Native paths now include OoT `HasItem`, `ApplyItemEffect`, runtime
`RandoGetToRandInf`, and pool insertion; MM `GiveItem`, `RemoveItem`, flag-only
`GiveItemForOracle`, and the complete native pool generator. Registered item
names, nonempty native skull-token draw IDs, major classification, and default-off
settings exist. Menus are deliberately not exposed while actor/solver/shared
semantics remain unfinished. Programmatically enabling these options is NOT yet
supported gameplay parity. No shared setting is implemented in this milestone.

Evidence:

- `python3 tests/shuffle_parity/souls/run_catalog_tests.py`: original/Combo
  native-ID compilation plus extracted unchanged production ownership, grant,
  removal, oracle and OoT pool blocks. Repeated grants are idempotent; removal
  clears only the relevant packed flag; family-off ownership bypass works; legacy
  MM numeric-ID snapshot passes. RED initially found absent catalog, grant path,
  OoT pool block and item presentation.
- `python3 tests/shuffle_parity/souls/run_pool_tests.py`: actual complete MM
  `GeneratePools.cpp`, `StaticData/Items.cpp`, `StaticData/Options.cpp`, original
  and Combo guards. All eight family combinations produce exactly one copy of
  every enabled new soul and no disabled copies; actual names, native draw IDs,
  major type, enum-name lookup and zero option defaults pass. RED first found
  absent souls in real generated pools. Only unrelated game/UI/RNG boundaries
  are fixtures. This does not run the combined filler.
- Actual production syntax checks, using the prepared CMake compile database,
  passed MM Items/Options/GiveItem/RemoveItem/GeneratePools/BenPort and OoT
  item_list/logic/item_pool/settings. An initial OoT pool compile exposed missing
  `Rando::` namespace qualification; corrected and rerun. Reproduction:
  `python3 tests/shuffle_parity/souls/run_native_compile.py --database
  /workspace/scratch/198fa7c65e91/build-cmake/compile_commands.json` after sourcing
  `/workspace/scratch/198fa7c65e91/build-deps/env.sh`.
- Native MM JSON tests rerun against expanded ownership/options arrays in both
  original and Combo guards. The current fixture verifies flag roundtrip and
  short/reused/empty/future tails; it does not emulate Song of Time.

Fixture commands need the prepared JSON and dependency include paths from the
compatibility command above. Compiler tests are not full links or gameplay proof.

## Remaining work

Native runtime/solver gates for OoT enemy (43), NPC (46 OoT / 53 MM), animals (4
each), miscellaneous (2 each), and effective shared categories remain in progress. Existing MM enemy souls
and collision immunity are unchanged. Catalog/grant/pool foundations are present as described above. Solver dependency
coverage, safe role-aware actor lifecycle, shared receipts/reconciliation, native
full builds, deterministic combined fills, room clears, schedules and configured
runtime proof are not yet complete.
