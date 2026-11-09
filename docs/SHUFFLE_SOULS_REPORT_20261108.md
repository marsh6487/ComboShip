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

## Recovered native shop milestone — 2026-11-09

The transient executor lost the prior isolated worktree and commits `64eb6d72`
and `662fd1bf`. This is a behavior reconstruction of the nine-path shop milestone,
not a claim of byte-for-byte recovery. It starts from independently restored WIP4
commit `a5d58c11232a9c4650791ceb04b0b344c69560fb`, tree
`e45b9ef56a0577f711adf23b71a8754152aa3e47`.

Explicit positive scene/role manifests classify seven OoT EnOssan placements and
three MM EnSob1 placements. Unknown/custom scenes and unrelated shop roles remain
independent. The two OoT bazaar locations share their native scene/role. Added
requirements cover 64 OoT shelves and nine MM shelves, bringing explicit totals
to 290 OoT and 117 MM checks. Trading Post schedule alternatives and the mixed
Bomb Shop/Curiosity fourth slot remain pending; they are not guessed here.

Fresh recovery verification used GCC 14 and headers under
`/workspace/scratch/3fea646b81e1/deps`:

- `CPATH=/workspace/scratch/3fea646b81e1/deps CXX=g++-14 python3 tests/shuffle_parity/souls/run_runtime_tests.py`
- `CPATH=/workspace/scratch/3fea646b81e1/deps CXX=g++-14 python3 tests/shuffle_parity/souls/run_mm_runtime_tests.py`
- `CXX=g++-14 python3 tests/shuffle_parity/souls/run_dependency_tests.py`

Both native runtime fixtures first failed the actual missing-soul spawn assertion,
then passed original/Combo modes after implementation. They cover all positive
placements, owned/off states, unknown scenes and unknown roles, plus the existing
animal/enemy/vanilla controls. Direct-check fixtures execute actual native stored
predicate boundaries for every manifest row. Existing MM header warnings remain.
The prior full CMake environment was lost; no fresh compile-database check or
configured game runtime is claimed. Broad NPC and shared reconstruction remain
incomplete. Existing stable IDs and MM key-ring foundations are unchanged.

## Recovered shared-soul milestone — 2026-11-09

Recovered shops are commit `3f2f7f59154a061645786fc75b24e3ae3bace559`, tree
`bbab7195b5b26135132175fafa88100e0aab1ceb`; the controller independently restored
and durably preserved that tree. The lost shared commit
`662fd1bf2d73d7661e2e970083ae67160573fa14` is reconstructed here from the retained
implementation record and current native source. This is behavior reconstruction,
not byte-identical recovery of that unavailable commit.

Four independent category bits (17–20) and saved keys are restored without
changing the 17 scalar upgrade families or any existing native item/flag/option
IDs. Effective category sharing requires explicit boolean `soulOptions` from both
real native dumps; absent/old/malformed fields disable it. Both dump exporters
read prepared native options. Hidden shared CVars are read but menus remain hidden.

The explicit catalog has 26 enemy, 36 NPC, four animal and two miscellaneous
edges. These form 66 components because MM Wallmaster and Skulltula each merge two
OoT souls. The native graph joins legacy foreign aliases, prefers native OoT
representations and caps soul oracle credits at one. The actual combined fill
uses effective categories before deduplication. Unshared unique souls remain
native items. Flying Pot is not invented as an MM native soul.

Both complete native SoulShuffleShared translation units use actual SaveContext,
saved family settings, slot/mode guards and actual soul ownership flags. New
native flags use the catalog; only the established contiguous MM enemy block
uses its existing flag conversion. Runtime and fill use the same shared groups.
Receipts grant merged siblings and peers, avoid recursively granting a source
whose native flag has not yet been set, and suppress old FC fallthrough only for
a handled foreign alias. The explicit 24-row legacy-ID manifest avoids numeric
alias assumptions and preserves published FC IDs. Deferred native Fleet pulls
reconcile matching-slot snapshots under existing receive guards; old/malformed
state and incompatible effective masks do nothing. Peer exports retry resolution
when absent. Existing save-direct native grant functions retain their receive
and persistence behavior.

Fresh verification (GCC 14, `CPATH=/workspace/scratch/3fea646b81e1/deps`):

- `CXX=g++-14 python3 tests/shuffle_parity/souls/run_shared_tests.py` passed two
  production graph tests, 32 real-header combined-fill cases (four categories,
  both starting games, enabled/disabled peer controls, original/Combo builds),
  and four complete native adapter builds. The initial saved-key parser failed
  before restoration, then passed. A separate `--runtime-only` run also passed.
- Native tests cover every catalog edge, saved flags, category-off, masks,
  vanilla/single-game guards, source-before-flag behavior, idempotent grants,
  stale slots, absent/malformed JSON and unknown names. The full actual OoT
  FleetShared_OnNativeObtained body is extracted and executed for the legacy
  Wallmaster path, proving two local merged souls, one peer identity, no duplicate
  old-FC fallthrough, receive-depth protection and the unchanged disabled-category
  FC path. Native name/ID/count source audits pass.

These are production header/TU fixtures. Full native grant persistence and
cross-module lifecycle are test boundaries; peer symbol lookup uses real dlsym
with a fixture export. Combined-fill oracle callbacks expose reachable fixture
sources, not the full native region graphs. No fresh full native CMake build,
dual-module save/reload, gameplay scene/schedule proof or actual headless CLI seed
is claimed after executor loss. The compile-db runner now includes both adapters,
both Fleet TUs and OoT dump TU for a recovered build environment.

Three hidden-option smoke configurations are restored under
`tests/shuffle_parity/souls/configs/`. Once modules are available, start with
`comborando --settings tests/shuffle_parity/souls/configs/enemy_shared.json --master-seed 20261108 --count 1 --starting-game oot`,
then MM start and spoiler replay with configured tricks only. All-shared remains
a diagnostic configuration until NPC dependencies are complete; enabling all
tricks is not acceptance. The complete NPC implementation remains pending. No
broad NPC behavior was added during this recovery task.

The existing scalar/FC/display/requiredness, launcher slot/cross-delivery and
plando regression suite also passed freshly with
`CPATH=/workspace/scratch/3fea646b81e1/deps CXX=g++-14 python3 scripts/diagnostics/run_combo_shared_test.py --json-include /workspace/scratch/3fea646b81e1/deps`.
Its negative-control invalid-layout diagnostic is expected. Recovered
clang-format 14 formatted new C++ files and modified ranges of existing C++ files;
`git diff --check` passes. Native source audits were repeated after formatting.

## Direct receipt slot/category handshake repair — 2026-11-09

Shared recovery checkpoint: `1edd71162f29e9366d78cc2eed4c27a1880d0bc3`.
Review found that direct peer receipts carried only a name, so a failed dormant
save load could leave another valid slot resident and receive/save the new slot's
soul. Snapshot reconciliation's slot guard did not protect that separate path.

The new `SOH/MM_GrantSharedSoulForSlot(name, sourceSlot, sourceMask)` export requires
the resident slot to match and the soul's category to be enabled on both sides.
The export name changes deliberately: an older module lacks the new symbol and
safely receives nothing rather than misinterpreting a changed ABI. Internal local
merged grants stay local. Rejected/unavailable peer delivery leaves the normal
source soul flag as the durable reconciliation source; a later matching-slot
pull applies it idempotently. No volatile item queue crosses save slots.

Fresh `run_shared_tests.py --runtime-only` first reproduced RED on the actual
native GrantPeer/dlsym route with a different resident peer slot. After repair,
all four original/Combo native builds pass. Every mapped soul exercises mismatched
peer slot, wrong peer category, matching-slot grant, the actual native receiver
export with wrong source slot/category, duplicate delivery and deferred snapshot
recovery after the peer loads the matching slot. Tests use real native local
SaveContext and the production shared runtime for the peer boundary; they do not
claim a dual-DLL gameplay/save reload. Existing receive-depth and legacy FC
fallthrough checks remain green. Source/header formatting and diff checks pass.

## MM successful dump schema repair — 2026-11-09

Slot/category receipt repair: `ef60d7ed5d89d3415a5058e85a0271570416271b`.
Review caught a reconstruction error: soulOptions had been inserted into MM's
confinement-error serialization rather than its normal successful dump. Manual
combined-fill fixture dumps therefore passed while a real successful MM export
would disable every shared category. The option object is now serialized only
on the actual normal-success path.

`CPATH=/workspace/scratch/3fea646b81e1/deps CXX=g++-14 python3 tests/shuffle_parity/souls/run_dump_tests.py`
first failed on `result.contains("soulOptions")` with the real successful native
serialization body, then passed original/Combo builds across all 16 saved option
combinations. It executes the actual option expressions and successful return
block with native RandoSaveInfo; unrelated generated pool/check arrays are fixture
boundaries. This verifies the returned schema/value boundary, not full pool
preparation or a linked native module. Error dumps remain errors and carry no
misleading effective-sharing metadata.

## Native starting-soul ownership and oracle bounds — 2026-11-09

Successful MM dump repair: `961fedfe26a3f8ae0b30cb70d0038ba2fdb1681c`.
Native MM configured starting souls are removed from its generated pool and
preloaded only into its own oracle. The shared fill previously had no starting
ownership metadata, so it could keep a redundant OoT copy and omit immediate
shared-peer access. Dumps now export exact native `startingSouls` names. MM reads
the saved/truncated starting array; computed enemy soul starts occur only when
the enemy family is disabled, so cannot enable shared categories. OoT refreshes
its native StartingInventory before exporting recognized names, including known
legacy aliases. No new starting-inventory setting or native ID is introduced.

Shared connected components retain this starting ownership. Fill deduplication
removes remaining free copies of already-owned components; oracle views credit
all member identities from the first query. The same metadata reaches normal
playthrough, requiredness replay and the dedicated target search. Unknown/malformed
names and disabled categories do not invent ownership; merged Wallmaster/Floormaster
components credit both identities exactly once.

A related review verified native runtime GrantStartingItems reads the bounded
saved array, while the headless MM reset used the original unlimited config
vector after saving it. The oracle now reads back the saved list before adding
computed starts, matching runtime truncation and cleared reused tails.

Fresh verification:

- `run_shared_tests.py` passed both graph modes, all native adapter modes and
  48 combined-fill cases including each category starting in either game.
  Starting-copy/sphere-zero tests first failed before the baseline integration.
  Focused original/Combo fill reruns additionally prove requiredness explicitly
  classifies an unrelated advancement as unnecessary using the starting baseline.
- `run_dump_tests.py` passes actual MM successful serialization, exact native
  saved-list getter/setter and oracle starting-grant block, plus OoT's actual
  starting export block in both modes. The overflow test failed before repair:
  a soul beyond native capacity was granted to the oracle. Green covers the last
  admitted item, rejected overflow, and a reused long-to-short saved list.
- `run_pool_tests.py --json-include /workspace/scratch/3fea646b81e1/deps` passes
  complete native MM pool generation in both modes, including configured starts
  from all four categories being removed and an unrelated soul remaining once.
- Existing `scripts/diagnostics/run_combo_shared_test.py --json-include /workspace/scratch/3fea646b81e1/deps`
  passes the scalar/FC, playthrough/requiredness, launcher and plando suites.

Commands used `CPATH=/workspace/scratch/3fea646b81e1/deps CXX=g++-14`.
Fixture setup initially needed the normal OoT include paths and the exact
ConvertItem two-argument signature; those compilation issues were corrected
before behavioral RED/GREEN. Pool generation and native saved-array bodies are
real; other oracle abilities, configured-item input, item conversion and effects
are fixture boundaries. This is not a full native-region seed or configured
save/gameplay proof. Formatting and diff checks pass. The separately identified
MM direct-receipt legacy fallthrough repair remains the next bounded change.

## MM callback legacy fallthrough repair — 2026-11-09

Starting ownership/oracle bounds checkpoint:
`6edfa013025c4610efd981f3afac64d96d069b8b`.
A full MM callback test identified a second stale-slot path: after guarded native
soul fanout rejected the peer, FleetShared_OnNativeObtained continued to its old
unqualified FC peer grant. Earlier native-adapter tests and the full OoT callback
fixture did not execute that MM continuation.

The runtime now reports any active shared component as handled, preventing the
second peer grant. This does not skip native FC accounting: MM GiveItem already
records obtained/applied FC counters before invoking the callback, and that
production body is unchanged. Category-off and unmapped items preserve their
existing legacy behavior. Matching-slot native soul delivery and later native
registry reconciliation remain available.

`CPATH=/workspace/scratch/3fea646b81e1/deps CXX=g++-14 python3 tests/shuffle_parity/souls/run_shared_tests.py --runtime-only`
first reproduced RED in the full actual MM Fleet callback after a stale-slot
rejection; all four original/Combo fixtures then passed. Both full Fleet callback
bodies are now extracted from production. MM controls cover stale peer slot,
wrong peer category, matching Wallmaster delivering both OoT identities, receive
depth, and the unchanged category-off FC route. The source audit checks FC counter
updates precede the callback. The fixture initially redeclared an existing RPG
enum, which was corrected before the behavioral test. Native save-direct effects
and peer save storage remain test boundaries; no dual-module gameplay proof is
claimed. The known review issues for direct slot/category receipts, successful MM
dump metadata, configured starting ownership and oracle overflow are addressed.
Broad NPC/world parity and full configured runtime validation remain unfinished.

## NPC cluster 1: Anju and animal hosts — 2026-11-09

Shared review checkpoint: `d6ae61cb0e3ee0732c3157433ed3cdf96bf1f282`.
This bounded NPC cluster adds OoT EnNiwLady (Anju) in Kakariko and Impa's house,
MM native Grog in Cucco Shack, and MM Mamamu Yan in Doggy Racetrack. Positive
scene/role tables run at the existing preallocation boundary; unknown scenes,
generic viewers and Grog's unused alternate role are unaffected. Grog's native
0xFE01 parameter is normalized from the engine's signed s16 spawn argument.
EnNiwLady and EnAob01 do not assign separate roles through actor params.

Native check adapters now support multiple required souls per explicit check.
Anju's child reward requires both Anju and Cucco; her adult egg and Pocket Cucco
trade require Anju only (an inventory egg is not a spawned Cucco). Grog requires
Grog and Cucco; the race reward requires Dog Lady and Dog. Existing age, time,
item, speech, mask and event conditions remain ANDed. There are now 292 OoT and
117 MM explicit check rows; three rows contain two requirements. Adjacent crates,
trees, pots and the independent racetrack chest remain ungated by these NPCs.

Source evidence: EnNiwLady_Init selects Impa's-house night behavior; native
z_en_niw_lady.c grants child bottle, adult Pocket Egg and Cojiro through its three
reward hooks. Kakariko's native region contains the corresponding three checks.
MM z_en_hs.h documents the actual 0xFE01 role and unused params==1 branch;
MilkRoad.cpp owns Grog's Bremen-mask and dog-race Mask-of-Truth predicates.
EnAob01_Init owns spawning both ordinary dogs and racedogs. Native EnNwc_FindGrog,
EnNwc_IsFound and EnNwc_CheckFound already refuse the march when Grog is absent;
the pointer-using follow state is only entered after that check. No initialization
cancellation, destructor workaround or extra chick suppression is introduced.

Fresh tests first reproduced RED independently in both real check adapters and
both native spawn bridges. GREEN passed original/Combo guards:

- `CXX=g++-14 python3 tests/shuffle_parity/souls/run_dependency_tests.py` executes
  the native adapters/stored predicate boundaries for every manifest row, removes
  each required soul independently, covers options-off and unrelated false
  conditions, and tests the adult inventory/ambient-check negative controls.
- With `CPATH=/workspace/scratch/3fea646b81e1/deps CXX=g++-14`, both
  `run_runtime_tests.py` and `run_mm_runtime_tests.py` pass whole production bridge
  TUs and actual engine preallocation prefixes. They cover exact native scenes,
  Grog's signed params, missing/owned/off states, both animal/NPC combinations,
  unknown placements and existing enemy/vanilla controls. MM also executes the
  actual native chick finder/capture functions with native structs: no Grog, no
  Bremen march, out of range, and successful normal capture. Only the C `this`
  identifier is renamed; sound and state-transition effects are test boundaries.
- `run_native_compile.py --database /workspace/scratch/198fa7c65e91/build-cmake-recovered/compile_commands.json`
  with `--only` for both SoulShuffleRuntime.cpp files, OoT logic.cpp, Kakariko.cpp
  and MM MilkRoad.cpp passes complete production syntax using recovered native
  flags. The runner copies CMake's forced PCH header as text and remaps paths to
  this worktree, preventing another checkout's compiled native definitions from
  contaminating this test. MM initially lacked SDL's architecture include path;
  sourcing recovered-tooling/production-env.sh resolved that environment failure.

This proves these roles/check boundaries, not configured scene transitions or a
complete native-world seed. NPC families beyond this cluster remain incomplete,
including MM Anju's schedules and OoT Grog/Dog Lady. The controls stay hidden.
No native item/flag/setting IDs, pools, UI, capacity or key-ring behavior changed.

Controller CI review additionally requested full clang-format 14 normalization of
both SoulShuffleRuntime.cpp files and OoT logic.cpp. Their dry-run/Werror formatter
checks pass. The controller subsequently assigned MM Logic.h formatting to this
cluster as well; its dry-run/Werror check passes. The controller owns the separate
OoT enemy-header formatting repair. Native tests were rerun after the cluster's initial formatting and passed;
no unrelated suite was rerun solely for final whitespace normalization.
