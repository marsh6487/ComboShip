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

## OoT enemy runtime/combat milestone (WIP)

Native identity/grant/pool checkpoint: `78aa221e9d3b06d73206a7ce447bb79450423c60`.

All 43 OoT enemy soul identities now have native actor-role mappings. Inert Armos,
composer ghosts, child Skull Kid, Gohma-arena larvae, ordinary/Gold Skulltula,
bosses and unknown/custom shop actor IDs have deliberate exceptions. The real
`CollisionCheck_AC` skips damage to a missing-soul actor; actors are not removed,
so this path does not trigger category removal/clear bookkeeping. Flying pots and
Anubis additionally pause updates, preserving initialized actors and pointers,
because their wall/AT and flame-circle death paths bypass AC damage. Existing MM
souls/immunity remain untouched.

The entire native `Logic::CanKillEnemy` body now ANDs mapped soul ownership with
all existing weapon/distance/trick conditions. `CanGetDekuBabaSticks/Nuts` also
require the soul. These changes feed the native OoT oracle through its existing
logic object. This is not complete world-dependency parity: checks/events/edges
that never invoke these predicates still require an explicit audited manifest.

Evidence:

- `python3 tests/shuffle_parity/souls/run_enemy_tests.py`: entire production
  CanKillEnemy and Deku Baba drop bodies, actual native enums, original/Combo.
  Family off/absent/present and weapon absent controls; tricks are false.
  All native combat enum variants are exercised, with 43 distinct mapped enemy
  actor souls and role/parameter negative controls. Weapon/ability queries are
  fixture boundaries; this does not boot the full native logic object.
- `python3 tests/shuffle_parity/souls/run_runtime_tests.py`: whole production
  SoulShuffleRuntime.cpp with real native PlayState/Actor/SaveContext plus the
  unchanged complete production CollisionCheck_AC body. RED dispatched collision
  without the soul; GREEN blocks it, permits it after grant and leaves actor
  bytes unchanged. Special-death update bridge, vanilla/credits/unknown actors
  and independent miscellaneous GS option pass under both guards. The giant
  Actor_UpdateAll loop is compiled and wired, but not executed by this fixture.
- Exact production syntax checks passed z_actor.c, z_collision_check.c,
  SoulShuffleRuntime.cpp and logic.cpp. Initial fixture compile found C++ headers
  inside C linkage and a missing age macro; fixed by normal z64 inclusion and
  native linkAge comparison. No linked full executable or configured scene proof.

## Explicit direct-check dependencies (WIP)

Enemy runtime/combat checkpoint: `f54792d3f36f0b261a28100b6825948803a4276b`.

Added 244 explicit OoT and 108 explicit MM direct-check dependencies for native
cows, butterfly sources, Gold Skulltulas, business scrub purchases and specified
animal-backed rewards. OoT's real LocationAccess::GetConditionsMet and MM's real
stored CHECK lambda add the ownership requirement without replacing the original
condition. These are the boundaries used by original and combined oracle callers.
The native MM exported reachability adapter and OoT logic translation units pass
production syntax checks with these interfaces. Gold Skulltula check gates also
close alternate native weapon/strength branches that bypass CanKillEnemy.

`python3 tests/shuffle_parity/souls/run_dependency_tests.py` observes RED with
missing native boundaries, then GREEN for every manifest row under both guards,
with family off / absent / owned, unrelated-false predicate, and intentional OoT
No Logic bypass. Actual stored native MM CHECK macro and actual OoT LocationAccess
method are compiled, with ownership and seed context at fixture boundaries. A
source-category audit rejects newly introduced native cow/butterfly/GS/scrub checks
missing an explicit disposition. Production lookup uses enum IDs only.

The complete native MM pool/item/option TU suite and catalog/body suite were rerun
and passed after this change. This is a direct-check milestone only: no claim that
all actor/event/physical-edge causal chains are complete. In particular, native
OoT Talon dereferences spawned super-cucco pointers without null checks, so blanket
animal preallocation rejection is unsafe. Cucco traversal alternatives and MQ
Jabu cow-driven events/doors require separate native predicates. NPC/animal spawn
filtering and all shared-soul category code remain unimplemented.

## Native animal/miscellaneous spawn boundaries (WIP)

Direct-check checkpoint: `2548dd9294a8d62276586c7bb2ee56c6a54718d9`.

Both native engines reject missing optional animals/business scrubs/Gold
Skulltulas before allocation, overlay loading and initialization. Ordinary wall
Skulltulas retain enemy semantics. The gates positively identify actor roles;
custom viewers and generic shop/cutscene actors pass through. OoT ranch Talon
(the role that dereferences three super-cucco children), MM Grog at Cucco Shack
and MM dog-race host have matching animal requirements. Other roles pass through.
Existing MM enemy collision immunity remains untouched.

The actual OoT Actor_Spawn and MM Actor_SpawnAsChildAndCutscene prefixes are
executed to their allocation boundary by native runtime fixtures. RED reached
allocation with a missing soul; GREEN stops before allocation, proceeds after
ownership and preserves family-off/vanilla/credits controls. All animal/misc
roles, positive host scenes, unrelated actors and existing OoT enemy collision
checks pass in original and Combo guards:

- `python3 tests/shuffle_parity/souls/run_runtime_tests.py`
- `python3 tests/shuffle_parity/souls/run_mm_runtime_tests.py`
- `python3 tests/shuffle_parity/souls/run_native_compile.py --database /workspace/scratch/198fa7c65e91/build-cmake/compile_commands.json` with `--only` for both engines' z_actor.c and SoulShuffleRuntime.cpp passes exact production syntax.

The runtime fixtures compile whole production soul bridge translation units and
native engine prefixes, not full game initialization/destruction. MM's native
headers emit existing controller macro redefinition and comma-subscript warnings;
fixtures required the proper native Flags_GetRandoInf signature and save enum.
No configured room, minigame, schedule or full executable runtime is proven.
Animal physical traversal/event gates and all NPC/shared behavior remain pending;
these options remain hidden from menus while integration is incomplete.

## Direct equipment damage immunity repair

Optional actor checkpoint: `8f978d20`.

Review found real enemy-soul bypasses in Ikana shield drain, Trident direct
Actor_ApplyDamage calls and Shovel direct HP subtraction. Actor_ApplyDamage now
preserves health when the soul is missing; Ikana and Shovel test the same native
bridge before their direct HP edits. Missing-soul Ikana drain cannot kill, drop,
heal or broadcast a successful drain. Existing timers/equipment guards, allowed
damage and unrelated shovel lifetime behavior are preserved.

`python3 tests/shuffle_parity/souls/run_direct_damage_tests.py` compiled and
executed the actual full native Actor_ApplyDamage, Ikana_OnShieldBlock and
ShovelHole_Update bodies with native Actor/Player/SaveContext. Each reproduced a
RED independently before its fix, then both original and Combo guards passed
missing/owned/off/vanilla and guard-window controls, lethal/nonlethal drain and
Leever/Deku/Hintnut shovel cases. Only C's local `this` identifier is renamed for
the C++ fixture; audio, effects, network emission and equipment state are fixture
boundaries. This exercises the central function used by both Trident damage
sites, not Trident animation/state-machine execution. Native compile-db syntax
checks of complete z_actor.c and z_player.c (the unity TU containing both custom
items and equipment) pass. No full scene/room-clear runtime proof.

## Animal/miscellaneous physical routes and events (WIP)

Direct-damage checkpoint: `98aa9241`.

Fourteen explicit Cucco-assisted native check/physical-edge branches in Gerudo
Valley, Kakariko and Zora River now use CanUseCucco (carrying ability AND soul),
while hookshot, hovering, damage, age and other original alternative routes remain.
MQ Jabu's entry->lift edge conservatively requires cow ownership to aim at entry
switches; no donor-only blind-shot trick is enabled. The 15 source/target IDs are
recorded in tests/shuffle_parity/souls/physical_dependencies.json and exercised
from the actual native predicate expressions, including missing/owned/off,
missing carrying/weapon ability, and independent alternate routes under both
guards by `run_physical_tests.py`. The Cucco test failed on the real prechange
predicate before implementation. It executes extracted native predicate lambdas,
with other abilities/seed options as fixture boundaries, not complete Regions.

Native MM EVENT now applies explicit SoulShuffleEvents.inc requirements before
its existing condition. The telescope business scrub entering its grotto and the
well's cow milk refill are gated. `run_event_tests.py` reproduced RED on the real
stored event macro, then passed both guards with missing/owned/off and missing-song
controls. The actual region EVENT declarations are used, not duplicate expressions.

A separate source audit corrected 18 earlier overly conservative MQ cow-labelled
wonder check rows: EnCow_Update registers only OC and contains no switch mutation;
EnWonderItem_InteractSwitch owns its independent AC collider and switch setting.
Later elevator/holes/basement/before-boss/falling-like-like/jiggly wonders therefore
remain available without cow. Native milk and entry chest/wonders retain their
requirements, matching donor's conservative entry aiming assumption. Updated real
check-boundary tests include negative controls for these independent wonders.
The direct-check catalog now has 226 OoT and 108 MM rows. Original counts above
are historical evidence for their checkpoints, not current coverage claims.

Complete production syntax checks cover all four edited OoT region TUs, OoT
logic.cpp, MM Central.cpp and BeneathTheWell.cpp. No full combined graph/fill or
configured MQ/entrance-shuffle gameplay proof is claimed. NPC dependencies and
shared categories remain incomplete and soul options remain hidden from menus.

## Remaining work

Native runtime/solver gates for OoT enemy (43), NPC (46 OoT / 53 MM), animals (4
each), miscellaneous (2 each), and effective shared categories remain in progress. Existing MM enemy souls
and collision immunity are unchanged. Catalog/grant/pool foundations are present as described above. Solver dependency
coverage, safe role-aware actor lifecycle, shared receipts/reconciliation, native
full builds, deterministic combined fills, room clears, schedules and configured
runtime proof are not yet complete.
