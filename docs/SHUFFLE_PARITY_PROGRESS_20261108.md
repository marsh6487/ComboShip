# Shuffle-parity implementation ledger — November 9, 2026

Plan: `docs/superpowers/plans/2026-11-08-shuffle-parity.md`.

## Current integrated source and authorization

The full user authorization remains active for implementation, six concurrent
workers, integration, regression repairs, builds, independently recoverable
checkpoints and publication to candidate/ootmm-shuffle-parity-20261108 / draft
PR89. No merge, master update or release promotion. The obsolete single-worker
restriction is removed. All six workers are active; no implementation is queued.
Controller owns shared integration, identifiers, cross-feature review and pushes.

Current controller checkout: `/workspace/scratch/c690840d9328/ComboShip`.
Public full-source checkpoint: `af7891b6c7383ab1655262d8913933ab20b6800d`
(tree3979ee578e674442801a6f5ac78d568f267915e4, localb6ac6346).
The source below is integrated locally through reviewed production fixes `d3c5aa17`, with post-publication fixture repairs;
The full source is published on the existing draft. Current CI repair changes
close real test dependencies and Castle formatting; no feature is deferred. Source
implementation and configured gameplay acceptance remain separate.

A host rollback removed the original later worker trees. All larger features
have now been reconstructed, committed, independently saved and integrated;
none is represented only by the lost reports. The older checkout is preserved.
The R2 integrated recovery package at f7c2ce34 independently restores tree
4051739b476a9d06cacb2e4111b9a65ba0b47783; persistent ID
libfile_6b9e89a0be2c819190b008becdd537df. Subsequent worker checkpoints and
exact-tree evidence are listed in their feature reports. Save the next combined
checkpoint before the next publication or long build.

## Requirement checklist

| Requirement | Owner | Production source state | Evidence / next action |
| --- | --- | --- | --- |
| OoT enemy/NPC/animal/misc and MM NPC/animal/misc souls | souls | Integrated native roles, actor behavior, room completion, scripted rewards, schedules, shops, quests and physical blockers; decorative roles preserved | 130 OoT / 213 MM rules; 391 / 200 check dependencies; 23 / 37 quest events; 21 suites, 9 sanitizer suites, 76 complete TU checks; final source83f2547a |
| Shared soul mapping/pools/grants/saves/reset/controls | souls | Integrated68 semantic counterpart edges, pool deduplication and host controls | Native ownership/grant/category/alias checks; source44bc3904/83f2547a; full new native seeds pending |
| OoT first enemy-type drops, V/MQ, defeat paths, persistent rewards | firstdrops | Integrated43 families/204 witnesses, actual menu, runtime special deaths and layout-aware pools/exclusions; Skull Kid native trade-chain witness repaired | All4096 layout masks, runtime/pool/hash both guards;16a6aac0; actual new linked On seed required |
| Soul/firstdrop colored hints and translations | souls/firstdrops | Encoded source and reward colors with resets;147 soul names and43 enemy sources EN/FR/DE; host settings preserved | Native147 composed clue checks;44bc3904; no rendered gameplay claim |
| Native/foreign soul GIs | firstdrops | Owning host boss-flame paths/materials, palettes, scrolling and compatible override paths integrated | 95 OoT +59 MM actual native/foreign callbacks; recovered source verified |
| Rusty overworld-door keys | door_keys | All27 OoT /39 MM identities, pools, grants, saves, native interactions and matching exits integrated; Castle actual gate and guard payment now bound | M8b600e996 integrated319c60a1; missing/wrong/right/Skeleton/age/form/off/nativeER/legacy24 tests; scene-changing returns free to prevent ER traps |
| Old door saves/settings and physical entrance ownership | door_keys | Hidden catalog version distinguishes legacy24 and new27; actual generation stamps only new seeds; imports default legacy; metadata excluded from randomization | Legacyv0 identity, coupled/decoupled native ChangeConnections, returns, save/import assertions;dd5d479c/319c60a1 |
| Actual song-event shuffle in both games | song_clock |18 OoT /13 MM persisted native events, solver, forms/age/instruments, spoilers/import/plando integrated | Native adapters/world checks both guards; review repairs integrated342dfc94/89985404: native learned-song dispatch and actual alternate-mask Human capability |
| OoT Clock/start time/ownership/day-night/travel | song_clock | Default-off deterministic start lock, grants/save/time transitions/native Sun undead stun integrated | Native tests; review repair integrated342dfc94; actual native tracker/recognition/ownership tests pass both guards |
| Song mapping lifecycle | song_clock | Native event mapping shared by saved runtime and solver | Preview and native-save-load provenance fixes integratedcc7e3176/180735d3; actual RED/GREEN native lifecycle tests pass both guards |
| MM rocks/boulders/red boulders/icicles/soil | environment |211 checks/279 physical aliases across36 native regions integrated; real break/bug receipt/retry/cycle safeguards | Full native lifecycle/pools/exclusions and all actual region scene assignments pass; source0efb6489/69f32cc4; solo default181 excludes30 unreachable IronBoot rocks, combined211 |
| MM Stick/Nut Bags, Child Wallet, six Infinite endpoints | song_clock/ci_native | All native grant/receipt/save/HUD/ownership endpoints retained | Fresh retained integrated64start cases and complete TU checks pass at45f4970a plus fixture closure1a890529 |
| Four counted MM key rings1/3/1/4 | song_clock/ci_native | Native pools/grants/save/reset and native/foreign rendering retained | All16 choices and actual dungeon behavior pass retained integrated tests; full dual Own Dungeon seeds pending |
| Three MM lottery-night rewards | song_clock | Native separate-night checks, tickets, receipts and cycle behavior retained | Actual native lottery suite passes frozen45f4970a plus dependency closure |
| MM Saria/Desired Progressive Magic and seven RPG families | song_clock | Native source locations and current ownership retained | Actual registered Saria/Desired suite and complete TUs pass frozen45f4970a plus closure |
| Native form instruments/MM soundbanks in OoT; FD ocarina | song_clock | Recovered native controller/audio routing retained; native MM behavior preserved | Actual controller,20audio scenarios, bank/mixer/PCM tests pass frozen45f4970a; no live audio acceptance claim |
| Menus/pool organization/gold-cyan/global toggle | environment/controller | Actual controls exposed, exact existing CVar IDs, highlights and disabled behavior; pool/exclusion metrics refresh on all new settings including shared controls | Real ImGui input/native pool/cache tests both guards;556d7b6c; Vanilla policy and inactive lottery/tracker receipt repairs integrated621975da/d3c5aa17; independent original/Combo review active |
| Exclusions/categories/locations/imports/generation | feature owners | Native catalog categories and saved/imported exclusion application integrated | Native environmental/firstdrop/door pool tests; real all-feature native matrix required |
| Native solo/combined validation adapters | ci_native | Actual SOH and MM fill/replay entrypoints, native pool budget rejection, full seed dump metadata, original/Combo guards integrated | Review repairs integrated3f6e456a/89ff751a/cc4b07c7; actual RED/GREEN transaction/health/replay tests pass; latest linked proof required |
| Full Linux/Windows targets/link | ci_native/controller | Required workflow builds all native modules and stages runtime | Publice360 Linux all targets/link and ordinary+door smokesPASS; Windows all native targets andZIP PASS; exact latest tree builds still required |
| Native graph/fill/replay, Glitchless, solo/combined | ci_native/controller | Required8-case actual native matrix plus firstdrop Vanilla cases implemented | Actual e360 Off combined fill/replayPASS; e360 On exposes real Skull Kid witness failure now fixed16a6aac0; latest matrixUNRUN until rebuilt |
| Dual-game Own Dungeon, Stone Tower normal/inverted, foreign placement | ci_native/controller | Required actual native matrix implements both simultaneous policies, rings, exclusions, negative circular replay | Earlier controlled fixture alone is not completion; latest native generation proof pending |
| CI/regression failure closure | controller/ci_native | Meaningful assertions and normal diagnostics retained; fixture real dependencies closed | Native heldNEI/sword/env/firstdrop/retained suites now pass; formatted-song fixture closure repaired93ead007; integrated native regression sweep active |
| IDs/save/default compatibility | ci_native/controller | Existing IDs preserved, appended clocks then doors then environment; mirrored FCI tables retained | Compiled36568 prior IDs,360 OoT and93 MM CVar/defaults unchanged both guards; FCI_MAX465 <=512; native historical JSON extents/tails pass;37889186 |
| Durable checkpoints/PR89 publication | controller | R1/R2 source and individual reconstruction milestones saved with exact restoration; draft/unmerged maintained | Full R4source independently restored and published; libfile_9d1eaddca38c819194b2cc96dd1f27e4, SHAdc0a48a8770d1979cd09b1609b14cd176e10369a064e18bd966ca4b33fee3304; next fixture checkpoint/push in progress |

## Current integrated regression evidence

Frozen integrated4542544e (tree7b7b4e7cd43db2871fdea37111a1da7794dd8d62)
passes native27/39 door/Castle/legacy/ER/save/exclusions both guards and18 scoped
complete syntax checks. Firstdrops/hints/native+foreignGI/replay pass20 command
rows, including4096 native layout/runtime/pool/hash controls and sanitizers.
Environment actual211/279 lifecycle, access, eligibility, pools/exclusions and
all50 complete translation-unit checks pass after real owning-header closure.
All21 soul suites and9 sanitizer suites pass both guards after9 test-only
real-song dependency, namespace/schema and format-extraction repairs. Those
fixture repairs are integrated. No assertion or diagnostic policy was weakened.
The latest combined MM song runtime/solver/JSON/oracle/message suite passes both
guards with sanitizers, including5 real Keeta world predicates. Native UI/Vanilla
generator/save/import controls pass both guards; independent lottery/tracker
follow-up approval confirms preserved receipts and exact active totals.

Public af7891b6 run42638388589 stopped at a whitespace-only Castle gate diff in
clang-format before starting native builds. The exact repository formatter was
then executed over all2251 local source units: only that file changed. Its
idempotent formatting check and all33 CI policy tests pass. Next CI includes
required actual door, full environmental and menu/Vanilla compatibility suites.
The separate Windows preflight42638388511 remains active and is not markedPASS.
Latest-tree Linux/Windows links and actual complete seed-generation matrices
are still required; component checks do not replace them.

## Active work and genuine verification limits

All six owners completed production implementation and bounded independent
review repairs, and now run frozen integrated regressions and native source
checks concurrently. Controller integrates fixture-only closure and publishes
the full tree; ci_native will execute actual compiled modules once available.
Environmental runtime/placement audits, dualguard save/ownership/receipts,
learning/song/form/time restrictions and originalIDs/defaults have concrete
native evidence. No started CI run or passed fixture counts as full generation.
The independent Vanilla follow-up checks actual tracker totals, lottery slots
and preserved receipts; it is integratedd3c5aa17. No production feature is
intentionally deferred.

The actual e360 native failure evidence is retained separately. Its pool-budget
failure is repaired by propagating errors and giving parity configurations real
check capacity, not by dropping progression or weakening assertions. The actual
Skull Kid witness requires any native adult-trade item at or after Saw, matching
the native actor; Claim Check is valid. New-source full graph/fill/replay remains
required. The real `oot-mq.o2r` quest archive is unavailable for local configured
MQ full generation;4096 layout fixtures are distinct evidence. Gameplay/GPU/audio
acceptance and unavailable quest-data tests will be identified honestly rather
than claimed passed. Production implementation continues independently.

## Historical progress (superseded where stale)

CI fixture repair ready: copied RandoCheckId declarations in item_grant_audit
and cross_hints/gossip now resolve the actual repository icicle catalogue.
Both reproduced the exact missing include before correction; complete native
runners now pass, including cross-hint ASan/UBSan. Production/assertions remain
unchanged. No equivalent copied-enum closure found in the remaining fixtures.

Native first-drop integration harness preserved as WIP: 12 derived configs,
actual DLL graph probes and actual combined driver fill/replay commands.
Export signatures/schema review passes. Fresh-only evidence directories,
pinned child SHIP_HOME, real CLI symlink negatives and exact native junk IDs
are tested. Helpers do not synthesize native graph outputs. Actual DLL/seed
execution is UNRUN locally; standalone solo full fill is unsupported by the
current export surface and is not relabeled as combined seed acceptance.

Published checkpoint `3aee089e433596d3c7da603edee1596e5bc41bbd` exactly
matches local `fbdc6c3ff309196f3d3dfe4411d839c0440218be`, tree
`b8d62d6e0b02377c1a71ddc905ada9629dbbeb32`. MM recovery ZIP
`libfile_56fcfacadad48191852faf02e85b43b2`, SHA256
`22364ba4c48fb5b6234df2bae52a08526d12a9c078f380eb423bead46bd5f370`,
independently restores that exact tree. Draft PR89 remains unmerged.

Current CI42574618218 fast source checks pass; Linux/Windows builds remain
in progress. Actual items job129020226454 exposed a copied Types.h enum block
whose new MMEnvironmentIcicles.inc relative include is unavailable in its temp
fixture. This extracted-fixture closure is being repaired without changing
production or assertions. Preserve the current native build until it reaches
seed evidence; a new branch update would cancel it.

User clarified that soul/first-drop hints require colored enemy and item names,
and new soul GIs must use the existing boss-soul flame texture path. Exact
POC5 archives were retrieved/audited locally; their bytes are excluded from
source publication. Independent hint and rendering workstreams are verifying
actual encoded color spans and native/cross-game resource resolution. These
visual follow-ups are not claimed implemented in the published checkpoint.

Hint/startup recovery: local `580f232da9a73458ceb9e9d16ab74e78afaa5291`,
independently restored tree `dbede42f4b23b352b3094b93bb8de296f9f3a5c7`, ZIP
`libfile_f2f4d232ba3081918c6b315bcf0a55b9`, SHA256
`536bdd637b1350906cf86b848b2436811f0f9942235c7cdb76a37e4529affde6`.
Post-format native first-drop runtime/combat/pool/hash matrices and soul/hint
suites pass; clang-format14 and all 28 CI policy checks pass.

MM first-drop vague hints now identify the actual enemy species, replacing
the old generic misspelled string. Exact check naming remains unchanged.
Full native location/selection/Desired message and combined clue generation
fixtures pass both guards and independent spec/quality review. These verify
source identity and composition, not in-game UI or full graph traversal.

MM environment source checkpoint is a hidden seven-icicle Snowhead entrance
slice only. Exact authored placement IDs, break-only callbacks, native pool/
exclusion persistence and physical pickup eligibility are implemented. Failed
allocation retries bind scene and room; crossing rooms discards stale requests.
Independent review passes. Native runtime/save/pool sanitizers, 128 exclusion
subsets and 20 full original/Combo TU syntax checks pass. Actor-parameter,
proximity/geometry and conservative access-predicate gaps remain before exposure;
remaining icicles/rocks/boulders/red boulders/soil remain unimplemented. No
family completion, full Glitchless seed or gameplay acceptance is claimed.

Reviewed source milestone: 43 OoT first-drop families now have 204 audited
physical witness regions and per-layout 38–43 check pools (43 Vanilla, 40 MQ).
Runtime, pools, exclusions, settings identity and distinct action-oriented hints
use the same layout rule. Native runtime/combat/pool/hash/hint fixtures pass
original/Combo, including all 4096 Vanilla/MQ masks. Full native seeded graph
proof remains open; the setting stays hidden and off by default.

Soul hint follow-up adds 147 individual native item hint identities and exact
native translations. Existing name fallbacks were retained; direct Item::GetHint
now has proper text. MM Saria admits useful legacy enemy souls without changing
item classifications and resolves only genuine enabled shared counterparts.
Actual native accessor/encoding/export, combined seed clue generation and MM
Saria/Desired/provider ownership and source-location fixtures pass; independent
soul-specific spec/quality review and fresh sanitizer reruns also pass. Full-world
traversal and configured hint presentation remain acceptance gates.

Confirmed native Linux seed-smoke failure: CI 42566230059 compiled/linked all
native targets and generated resources, then crashed (SIGSEGV 139) inside
Rando::Item::operator= from StaticData::InitItemTable, called by
SOH_InitRandoHeadless. The real table extent RG_MAX was 682 while newly appended
MM key rings occupied 683–686. The correction preserves all 686 existing
non-sentinel IDs, reserves old slot 682 with a blank non-progression EVENT row,
and moves RG_MAX to 687. Name lookup skips the empty row. Before-fix native
bounds assertions failed; complete real InitItemTable runs twice under strict
ASan/UBSan in original and Combo modes after the fix. Independent spec/quality
review and independent sanitizer reruns pass. This is startup-fixture proof;
a fresh linked native seed run is still required. No passing build or gameplay
acceptance is inferred.

The e2b4 CI run has completed with the seed backtrace, so the earlier publication
hold has ended. The next checkpoint includes the reviewed source fixes and
required native item-table/soul hint regressions. Previous run failures remain
historical evidence, not evidence that the new tree passes CI.

UI fixture follow-up: e2b4 render CI exposed missing native gItemSlots linkage
retained by inline capacity policy in the extracted Interface_Draw fixture.
It now extracts that actual inventory table, leaving UI assertions unchanged.
The complete UI visibility suite passes native ABI sanitizers, including input,
HUD/title/file-select, lifetime/vote behavior and 46 full MM original/Combo
syntax checks. This repairs the fixture; no rendering-policy change.

MM Saria/Desired source-location follow-up: hints now carry actual source
game/check text, native current magic tiers, metadata-gated shared magic and all
seven RPG families. Excluded/skipped/cycle-obtained targets no longer win Saria.
New slot-checked native OoT provider reads real saved placements even without a
generation pool; Saria additionally uses an initialized native graph and live
inventory traversal. Inventory/Logic/region/exit/location/no-logic state is
restored, including exceptions; independent review traced the actual traversal
mutation footprint and found no blocking gameplay-state omission. Both MM guards
and native provider/filter/restoration fixtures pass, with full native MM
conversion/tier functions and exact source location assertions. Four complete
MM Rando/Saria TUs pass syntax; full OoT compilation still lacks local libzip.
No unsafe live headless initialization. Absent native graph yields no cross-world
Saria target; Desired can still locate actual saved copies. Actual cold launch,
full-world engine traversal, two-module linking and gameplay remain unclaimed.

Verified pinned STB acquisition repair: dependency CMake previously downloaded
directly over its cache without checking status or content. A historical
negative control accepted an empty response. The corrected block reuses only
the exact pinned SHA256, downloads with TLS/status checks to a temporary
candidate, verifies bytes before promotion and rejects failures during configure.
Actual pinned HTTPS, seven failure/cache cases and native stbi declarations
pass. Pinned revision is unchanged. Windows e2b4 build failed for missing stbi
declarations; its actual downloaded bytes were unavailable, so this consistent
acquisition failure cause is not claimed independently confirmed or MSVC-fixed.
No public update until the running Linux seed/backtrace attempt completes.

Separate MM lottery nights source-integrated: default-off real control, original
night-one identity, two appended checks and three positive ticket-access events.
Native day/fee/number matching is unchanged; enabled rewards require the exact
ticket day followed by 18:00–23:00 on its matching night. Callback/messages,
permanent/cycle guards, full pools (+2 filler), every exclusion subset, JSON,
native cycle hooks and clock/boundary/fee controls pass original/Combo sanitizers.
All six complete actor/Central/Menu TUs pass syntax; independent spec/quality
review passes for bounded candidate scope. Gold real-CVar registration and CI
native runner added. Three full pool extraction fixture suites pass with real
Lottery header dependency. Full app link, rendered UI, foreign receipt gameplay
and configured acceptance remain pending. Tracker retains its generic original
label; enabled exclusion lists additionally display Night 1 without renaming its
persistent key. See SHUFFLE_MM_LOTTERY_20261109.md.

Latest unpublished CI recovery: local
`f560c076445de139f2983be8592c31896e03f8ef`, exact independently restored tree
`3f0eb06dac02abc943abfcf7629a9f6326c6a57e`, ZIP
`libfile_f179daa5f458819192aa2421c812b6fd`, SHA-256
`572d064d252304c60a6f5c84b4d3048545e6ce3f2104f97130415f38796671a1`.
Public branch remains e2b4e6f while its native CI runs; no build restart.

CI e2b4e6f follow-up: actual item/system failures were two old assertions
equating the previous last option with RSK_MAX; retain their entire old prefix
chain and assert the actual appended first-drop neighbor instead. The sword
receipt fixture now uses real native dungeon-item visual IDs/count policy and
an assert-false unrelated ring-geometry boundary. Black Market's fixture now
extracts the real inventory upgrade masks/shifts needed by its retained wallet
debit guard. Sanitized sword receipts, full native seed settings, full MM item
receipts, and direct/entry Black Market runners all pass under native ABI
adapters. No production/assertion/warning-policy workaround.

Publication policy: subsequent ref updates cancel older native CI builds in the
current Actions concurrency configuration. Hold the next public update until
the current e2b4e6f build (42566230059) reaches seed/backtrace evidence. Local
source commits and independently restored durable packages continue meanwhile.

Infinity fixture follow-up: actual CI c169 equipment/systems errors identified
missing InfiniteUpgrades policy declarations in the Grace and seasons extracted
pool fixtures. Added the real header there and in the equivalent editor pool
fixture. All three full native runners pass under repository ABI adapters;
editor includes sanitizer/bounds and actual SaveEditor/GiveItem syntax checks.
No assertion or warning policy was weakened.

Infinity integration recovery: local `4284a2b3f1e62ccd08fb027717b4f58c21abd74c`,
independently restored tree `8d853cc41086951f3ef015b9066e4229be937251`, ZIP
`libfile_d354fe16c7388191bf7809ae604679f3`, SHA-256
`df067728cdf454e0c38b5936f77af67fdda29ec2e6ca13fb0297bed8c6fcd64c`.
Real ImGui What's New suite passes 5/5; CI policy 28/28. Full current native
builds, linked seed validation and configured gameplay remain pending.

Infinite Upgrades integration: real default-off control now replaces the final
capacity placeholder, with gold real-CVar registration and required CI fixtures.
Native/custom magic costs, synchronous and pending wallet debits, all 64 starting
selections, native JSON/spoiler/icons and bounded foreign receipts pass original/
Combo sanitizer fixtures. Existing bag/wallet controls pass. Review found and
fixed possible reduction of an imported tier-3 wallet, plus four-digit HUD
registration with Tycoon off. Exact raw tiers/base ownership/Chateau remain
unchanged. Independent spec/quality review passes for bounded candidate scope.
Additional failing-first native regression fixes headless infinite Tycoon
capacity: the private endpoint ceiling is at least 5000 independently of HUD
initialization or a shared-table 999 import. Raw tiers/table remain unchanged;
finite controls, missing bases, 5000/5001 affordability and custom 6000 capacity
pass original/Combo sanitizers.
Fresh full native player and parameter C syntax passes with real headers;
standalone equipment fragments need their actual unity includes and are not
claimed compiled separately. C++ production checks currently lack libzip headers;
the full current executable, live config UI, linked seeds and gameplay are not
claimed accepted. See the capacity report for precise fixture boundaries.

First enemy-drop source checkpoint: local
`c0a454748c3952efe6c030548ce3212ed792ead6`, exact independently restored tree
`c95883fe1d617fc1d25394ac97979b653c4e9dc3`, ZIP
`libfile_2904d296396c8191a62ad3e5282271cb`, SHA-256
`ca709c741281543e9f6ef30adf3a331f6610233661738d2d77cd360ea109e0de`.
43 native defeat checks, saved defeat/receipt flags, unique-reward redelivery,
native pools and exclusion round trips are verified in bounded fixtures. 39
source-backed witness families at this checkpoint; four missing and full V/MQ/
seed validation remain hard gates, so the option is hidden and not randomized.
Original MM drops are unchanged. New CI runners cover the native runtime,
combat predicates and actual pool/exclusion algorithms.

Public checkpoint `c169dc014934e0756cf94f17729ee3b3e50ad5b9` exactly matches
local `fd77df56a2c1434f9705e4eda903620eaa90f6a4`, tree
`a444294f0cb3e870d9d5f5fcd532e05818f34472`. It includes the earlier infinity
foundation, gallery/bowling souls and export/fixture/diagnostic fixes. CI
42565140750 fast source checks pass; native build/regression matrix is pending.
No merge, master promotion or gameplay acceptance.

Soul follow-up: native OoT gallery controller/host and bowling host, plus both
MM gallery hosts, now require their exact soul only in native scenes. Eight
reward checks and the positive OoT bowling/refill event agree with runtime;
unrelated actors/scenes, wallet, language and bow requirements remain tested.
Native runtime, dependency/CHECK/event, enemy, damage, physical, pool and full
shared graph/fill/bridge fixtures pass. Full NPC/shared menus remain hidden:
many major quests and scheduled families are still unclassified.

Recovered actual Windows annotations identify C2375 on shared-soul exports:
both prior native header declarations lacked COMBO_EXPORT present on their
definitions. Both headers now match; native complete bridge TUs and guards pass,
but the MSVC rerun is still required. This supersedes the earlier unavailable-
logs statement. The unrelated catalogue fixture's relative SharedSoulCatalog
include and save fixture's missing local spdlog dependency remain recorded.

CI follow-up: repaired the five extracted fixtures' missing native soul/key-ring/
wallet dependencies without weakening production or assertions. Sword fallback
(both hosts, ASan/UBSan), Moon's Tear (all five paired phases, 4197 scenarios per
phase) and wand suites pass with the actual native ABI compiler adapters.
Damage soul boundary matches its native header under GCC14 -Wall -Werror;
the full damage fixture remains locally blocked by existing GCC14 native pointer
conversions and an En_Rd resource declaration, not claimed passed. CI's original
failure was its missing soul symbol. Failure-only Linux seed-smoke gdb backtrace
collection now preserves original failure/signal exit codes; CI-policy tests
pass 28/28. The actual linked seed crash is not yet diagnosed or fixed.

Infinite foundation recovery: local `3814c766dd6010de22f5e50a32057ddd75752d10`,
exact independently restored tree `770a37999125b88eda58daea33afecab9c81b979`,
ZIP `libfile_d06f952f7b288191baa28d02339e56fa`, SHA-256
`bdcc3c7ed0b2ebf32a197fc4dbc18b47c573cee01897005215f39458e02f9f73`.
Fresh six-family pool and runtime original/Combo sanitizer fixtures pass.

Ruling: user explicitly requested parallel execution after poor elapsed-time
progress. Replace the earlier one-implementation-worker cap with disjoint owned
workstreams: CI fixtures, MM infinity completion, OoT first enemy-type drops,
existing soul actor/logic gaps, plus read-only linked-seed investigation. Root
owns staging, shared presentation/CI registries, ledger and publication. Workers
do not commit or stage; no shared file has two implementation owners.

Task 5 Infinite Upgrades foundation (WIP): append-only six-item catalogue,
default-off boolean option, unique pool supply and native saved ownership are
implemented. Native ammo/currency, immediate magic costs, all five continuous
magic states and NEI common magic helpers pass original/Combo ASan/UBSan tests.
Missing-base ownership and unchanged raw upgrade tiers/Chateau event are tested.
UI activation, starting category/icons, dedicated receipt/save round trips,
player/custom-equipment affordability gates and four-digit wallet dispatch are
unfinished. Do not enable the placeholder or claim gameplay acceptance yet.

Ruling: keep the requested Infinite Upgrades checkbox and six distinct native
endpoint items. Do not add speculative Progressive/Condensed UI modes. Separate
endpoint identities avoid shared scalar-family trimming discarding MM's extra
infinite copy, preserve all old finite progressive chains and shared ABI tiers,
and require the ordinary base item/bag/wallet. Infinity ownership is not a
Chateau Romani event, a raw capacity-tier promotion or an RPG-stat grant.

Latest published Child Wallet checkpoint: public
`fac20a0132d087d0ebaf5c8637ec4fb3fee47f04`, local
`9c1408fe4d16f4c2ec91fef447ed128769fd0139`, exact tree
`e10179139cbbda75f73f06139876eeb52d0c4ec4`. CI run 42556362719:
fast source/world/entrances passed; Linux compiled and linked all targets and
generated both resource archives, then seed smoke crashed (exit 139, undiagnosed).
Windows compilation and systems/items/equipment/render regressions failed.
Known fixture omissions: sword damage soul predicate; sword fallback key-ring
dispatch declaration; Moon's Tear key-ring/wallet dependencies; Storm actor
spawn soul predicate. These are not yet claimed repaired. Windows full logs
currently fail transport; no cause inferred. No merge/master/gameplay approval.

Task 5 Child Wallet follow-up: actual progressive/starting/JSON/old-save and
foreign-name receive paths pass bounded original/Combo ASan/UBSan fixtures.
Ghost Hut enemy-drop and Playground source-event regressions each RED→GREEN;
native day/clock and free-interaction controls retained. Real default-off Child
Wallet UI replaces its placeholder with the gold accent and shared-wallet
disabled policy; requested selection is preserved. Eight wallet/region TUs
pass real-header syntax, Menu passes native GNU policy, BenPort currently lacks
libzip headers after reset and is not claimed compiled. Full linking/seeds/
cycle/gameplay and Infinite Upgrades remain pending. CI debug renderer include
order, seasons-pool and scalar lifecycle dependencies repaired and tested.

Task 5 Child Wallet WIP: native catalogue/pool RED→GREEN and missing-wallet
currency/affordability RED→GREEN, original/Combo guards. Native paid CHECK
regression RED→GREEN. Saved base ownership, progressive grant/oracle/remove,
effective capacities, paid-check/refill gates and Shared Wallet snapshot
normalization implemented. Pool tests cover solo/combined/shared/Tycoon/starts;
runtime tests use native C currency/flags and the actual dormant flush. Menu
activation, full wallet save/start/world validation and Infinite Upgrades remain
pending. Second workspace reset recovered exact published bag tree b2d297fe;
fresh CI remains separate from previous lost local build outputs.

Task 5 resumed: native bag catalogue/pool regression RED→GREEN, then missing-bag
runtime/logic regression RED→GREEN under original/Combo guards with ASan/UBSan.
Shared receipt/oracle bag helpers, independent saved ownership, capacity-before-bag
handling and native refill/player gates implemented. Six whole production TUs
pass native syntax checks with fresh text headers. This is a WIP foundation;
bag UI/cross-world validation, Child Wallet, Infinite Upgrades and final world
seed verification remain pending. See SHUFFLE_MM_CAPACITIES_20261108.md.

Task 5 bag follow-up: real default-off controls now replace the two bag
placeholders in a Bags & Capacities card and use gold What's New accents. Actual
starting/spoiler/save adapters, JSON old-save defaults and native foreign-name
receive traversal pass bounded original/Combo sanitizer fixtures. Menu native
syntax and real ImGui accent regression pass; CI policy 25/25 passes. Full linked
seed/cycle/gameplay validation is still pending; Child Wallet and Infinite
Upgrades are still not implemented. Bag foundation durable checkpoint is
`4a7ad053e7c6479d0cafe55c410900fbcc0e4a2e`, tree
`8353d3d8b9f76b12d01de8adba8ff9a6e5ed1569`, source ZIP
`libfile_59985575248481918ecf205824554224` (SHA-256
`a9538dae2ebc5f2868c1d713e2f1cf6108b7014800bc24c435373592b17f5121`).

Baseline: `ced18542387d5ac8879e0f1cb303e790e4e3a1a8`, tree `ab1a683f2c91fd14a039cc58800a112ae92966b9`.
User authorized the full spec and incremental public WIP source checkpoints. Dedicated candidate branch exists. No source implementation was performed before the final authorization.

Baseline checks: `python3 -B -m unittest discover -s tests/ci -v`: 25/25 pass. Initial attempted path `tests/ci_policy` did not exist; corrected discovery path without changing tests. One chained source-read attempt stopped at a nonexistent DesiredItemHint.cpp; no test result is inferred from it.

Baseline recovery: `../deliverables/ComboShip_Shuffle_Baseline_20261108.zip` independently restored 5,349 source files with exact Git blob identities. SHA-256 `9b1ca6bec5362c61e0536a76219ec800e5bbcce7f6d22ed14de29908f5b3ce1b`; saved as `libfile_2362c5944e2c81919ca100f8a97e1743`.

Published scope checkpoint: local `b3280ce58e9993ff9180d8b83096984c7961aa8a` -> public `f6d92583ff8b136ecfec81b4b08d29e26e38db63`, identical tree `aad3d16c3719e0ef93410ad57081b8a9ed305b3e`. Dedicated draft PR #89 created against PR #88's candidate. This initial public checkpoint contains the scope/plan only.

## Preflight shared interfaces

| Tasks | Shared boundary | Finding and handling |
|---|---|---|
| 1, 3–10 | Native item/check/options and combined fill | Preserve all baseline IDs. Append new families; controller integrates sequential commits and resolves new-ID ordering before accepted seeds. |
| 2, 3–10 | Hint candidates and native item identity | Candidate selection must reflect the actual exported UI and dynamic conversion, not only Rando helper functions. |
| 3, 4 | Enemy soul ownership and kill reachability | Task 4 consumes actual soul-aware kill logic; no runtime-only gating. |
| 5, 6, 7 | Capacities, small keys, door interaction | Keep native dungeon key ownership distinct from overworld door keys and foreign OoT rings. |
| 8, 10, 11 | Playable songs, time and form instruments | Seed event assignment and solver must agree with each form's actual songs and bank ownership. |
| 1–11, 12 | CI fixtures and shared enums | Register checks after native production verification; never weaken existing assertions for convenience. |

Ruling: Retain MM enemy immunity for initial parity rather than silently changing to OoTMM spawn suppression — preserves native behavior; changing later requires an explicit runtime milestone.
Ruling: Treat upstream equivalent-check candidates (egg hatching/diving/fairy/bush subfilters) as research rather than quietly asserting missing functionality — approved implementation targets the named confirmed gaps; costs a follow-up if deeper parity proves useful.

## Active work

- Task 1: reproduced and fixed stale live-save confinement. Native MM catalogs/pools and real combined fill passed 64 Own Dungeon seeds, 64 excluded-check exports and 16 Anywhere controls; randomized confined self-locks reject. Review-found failed-dump fallback publication now rejects before GUI structural work. See `docs/SHUFFLE_BOSS_KEY_REPORT_20261108.md`; linked dual-native generation and configured gameplay remain pending.
- Task 2: actual MM catalogue reproduction failed for missing RPG entries. Six append-only RPG selectors and matching native Saria/Desired Items paths implemented; source evidence in `docs/SHUFFLE_HINT_PARITY_20261108.md`. Native original/Combo sanitizer tests pass; review corrections skip capped/disabled Saria RPG stats and filter menu availability by configured pool. Bounded fix review approved `d3df01069fceba8dfa83c4441c600e663f32e1cf`. Complete cross-world/game/runtime verification remains pending.
- Task 11: exact original form-instrument candidate `76a1fee1be88aa8c01cc06f44d20f485031987ad` recovered from its source-only ZIP; target tree `5d13866641bcc62066a2cf8195171c55e2381a95`, baseline tree matches PR88. Package SHA-256 `11a7b7db1977f191fecf0dc189ca406bc3fccf1c7f8a9fa8371b1d9f3999c3b7`, saved as `libfile_7426260765b08191830915b66e3ec70c`. Independent verification matched all 45 archive checksums, 20 changed-file blobs/modes/lengths, 21 evidence records and reconstructed the exact full tree from PR88. The verified patch is integrated without overlaying the shared CI manifest. Integrated controller tests pass 58 cases across original/Combo; CI policy 25/25. Native object builds, integrated audio and configured runtime remain separate gates. Required MM Soundfont 0/samples and existing model resources are not included or published.
- Task 3: append-only MM soul ownership save codec integrated; 154 new native catalog/grant/pool/oracle foundations integrated as `3e688310ce7a04550a9f36c3997ce5364c0bb311` from isolated worker commit `78aa221e9d3b06d73206a7ce447bb79450423c60`. Runtime/solver/shared behavior remains WIP and menus stay unexposed.
- Tasks 4–10,12: pending implementation, retained in the plan.
- Additional screenshot regression: Chateau Romani shelf renders black. Source correction now binds native actor lights for each shelf item; existing shelf fixture RED then GREEN in both hosts under combined ABI/ASan/UBSan. Full BlackMarket production object compiled and bounded review approved. See `docs/SHUFFLE_BLACK_MARKET_LIGHTING_20261108.md`; GPU/screenshot result remains unverified.

Bounded boss-key review approved `ff6df34c`: no new material findings after failed-dump fallback correction. Hint production translation unit at `d3df0106` compiled successfully with the actual generated Release command. Full native game build remains in progress; this is not gameplay acceptance.

Published WIP source milestone: local `8da378bfdc4fdeb0ccd941b29bb54a6b6b4aa28d` -> public `b58faf80139f745dca1eb0fcf80aec46bbe3a66e`, exact tree `5dc6e094ac061512f74a7f2a5d4414f78a4f0b3b`. Draft PR #89 remains unmerged. Durable `ComboShip_Shuffle_WIP1_Source_20261108.zip` (`libfile_765b8588a91c8191b194170cd9cd97ae`), SHA-256 `a09b033126f7964ac229891f5c33e4f0e4907499cc6ca7f7d3847f1e57777c84`, independently applied from baseline to that exact tree, 62 changed files. Integrated 20 form audio cases pass; all three form TUs pass Combo object compilation plus original-guard probes. Final MM boss/hint/soul objects were recompiled successfully after refreshing the PCH; these supersede earlier PCH-ambiguous MM results. Launcher and headless-driver objects also pass. Full linking/seed gameplay remains pending.

Task 6 WIP integration: four selected MM key rings, native grants/oracle/ownership, pool/start-with/plentiful behavior, dungeon restriction/FC classification, alias/receipt and validated recipe implemented. Renderer, native/foreign callbacks and default-off per-dungeon UI have been recovered. Fresh both-host native renderer/callback sanitizer tests and actual export/cache/latch tests pass. Full native build, real linked seed validation and configured GPU gameplay remain unfinished. See `docs/SHUFFLE_MM_KEY_RINGS_20261108.md`.

Workspace recovery: baseline and WIP1–4 packages were checksum verified and each restored exact recorded full tree. Public recovery checkpoint `6b8653389675c8fcc779808986bb2bdb857b7ea4` has exact WIP4 tree `e45b9ef56a0577f711adf23b71a8754152aa3e47`; restored local `a5d58c11232a9c4650791ceb04b0b344c69560fb`. WIP4 ZIP `libfile_c5b6b142ef188191a4f57a241c037c93`, SHA-256 `16c55c74e0178226cd1ac142161dfe0cd18dff411d7e0b353625506fefc7d4c8`. Newer unuploaded commits did not survive; they are reconstructed rather than claimed exact. Lost native build outputs are not reused as fresh build proof.

Reconstructed shop source `3f2f7f59` integrated as `2aeb89b9f9d0501f584482180ab466d19e94a43a`, exact tree `bbab7195b5b26135132175fafa88100e0aab1ceb`. Fresh native runtime/dependency fixtures pass. Durable ZIP `libfile_af00b98a222c819187322dbaf96ceccd`, SHA-256 `9d4fe856b99afb9eed5b56f6c71cfcc9edd51f36883c83a3156d6db0cbd236b1`, independently restored. Shared source `1edd7116` integrated as `727cfcf10ea53f8f6ccfa4007e64f57e870d83a3`, tree `04c832456a5e2335263f0e4726b3e449946572a0`; durable ZIP `libfile_d8075dc6111c81918f8a9279e48e160a`, SHA-256 `07b7b5943b62101ad159a7f872d3ce234ca0252a1244a4357266356e1f9523c6`. Fresh shared fixture/32-fill/scalar regression evidence is in the soul report; independent review found a stale-slot receipt issue now under correction. Incomplete NPC/shared UI remains hidden.

Additional durable milestones: lighting local `6aa61d810edb9b66a48d0d692a34db1736233e84`, tree `ff928d0fc2b7051251e974f8766ff1eb28c7b8fd`, ZIP `libfile_851a10e5ebd8819193e812b0be8792e4` SHA-256 `0357bf3326f8758ef9e99265e8f19d9ba7887c83652ad4024d9cfd70a3be54b4`. Soul runtime/dependency layers through local `14ed2e523507b1b3cae1762dafa9ff9eba0e596b`, tree `f0749aa98ca3aa1b1c7a1f9bff891adff5ce3373`, ZIP `libfile_b9667b22ef34819188fa2e1b220a3f4c` SHA-256 `74f750ad5a344690d07017e39210d073f79b5504f59cdfa18e3604251de4f8ed`. Each bounded patch independently restored its exact tree.

Public WIP1 CI run42529433408 failed only the clang-format14 fast source check (BenPort, GiveItem and both SoulShuffle headers); native build/test matrix skipped consequently. Windows compile preflight42529433401 passed. Formatting is corrected in the current candidate; the new soul grant dispatch now occupies its ordinary default switch arm, preserving post-grant shared notification and avoiding unrelated switch reindentation. No unchanged CI rerun is claimed.
