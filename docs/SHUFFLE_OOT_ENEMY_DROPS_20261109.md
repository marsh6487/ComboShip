# Native OoT first enemy-type drops — verified WIP, not ready to expose

Status: native runtime/check/pool/exclusion plumbing and 39 source-audited witness families are implemented. The complete 43-family Glitchless feature is **not complete**. The setting is default off, default hidden, absent from both menu groups, and skipped by RandomizeAllSettings. Do not describe it as a completed shuffle or expose its widget yet.

## Implemented paths

- Append-only 43 RC identities, 43 receipt RandoInf flags and 43 separate defeated/eligible RandoInf flags, plus RCTYPE_ENEMY_DROP and RSK_SHUFFLE_ENEMY_DROPS. Catalogue variants deliberately share the native enemy-soul family identity. Gold Skulltulas, inert Armos, composer ghosts, child Skull Kid roles, Gohma arena children, bosses and viewer/shop actors are excluded by the native role classifier.
- Real OnEnemyDefeat registration. No Actor_Kill or damage-trigger reward. Existing native ordinary-death and special-victory functions emit the event; no actor C files were changed. Actual C Actor_Kill, EnGoma_SetupDead and EnGeldB_SetupDefeated bodies execute in the native regression.
- Real EnItem00 dummy reward, native check-to-final-item bridge and RandoInf receipt. Pending actor pointers prevent duplicate callbacks and duplicate same-type actors. Defeat eligibility is saved before allocation. Player-update redelivery retries allocations and recovers uncollected unique rewards after room/scene change or save reload; it never requires defeating the absent unique actor again. Item destruction only clears the active pointer; scene change clears active pointers. Both eligibility and collected ownership use native saved randomizerInf bits. The original ordinary enemy drops remain intact; the first-type check is an additional reward.
- Native location-pool exclusion when off; 43 filler slots when enabled in solo/unified modes. Native check names, RC string maps, serialization/import-compatible exclusions, check-to-flag reverse path. Existing excluded-fill algorithm assigns junk. Original/Combo guards exercised.
- Truthful global classification: SCENE_ID_MAX and no owning dungeon. The existing Hyrule Field tracker/exclusion bucket contains explicitly named `Global Enemy Drop: <type>` rows; this bucket is not a claim that each enemy occurs in Hyrule Field. Logical witnesses retain their real regions. Native RandomizeOwnDungeon explicitly rejects RCTYPE_ENEMY_DROP even if a witness contributes a dungeon hint area.
- Tracker hides the category when disabled, including the non-rando reset path.
- Real option identity `CVAR_RANDOMIZER_SETTING("ShuffleEnemyDrops")`; controller owns What's New registration. English name/description landed. No supported widget is exposed yet.

## Verification

With `CC=gcc-14 CXX=g++-14`:

- `python tests/shuffle_parity/enemy_drops/run_tests.py`: PASS original + Combo. Real native types, production registration/callback body, actual C normal/special death transitions, actual Actor_Kill negative control, duplicate callbacks/type variants, receipt and eligibility bitfields/reload, unique special-reward redelivery without a second defeat, scene/item destruction, allocation retry, off/vanilla and actor-role negatives.
- `ASAN_OPTIONS=detect_leaks=0 python tests/shuffle_parity/enemy_drops/run_tests.py --sanitize`: PASS AddressSanitizer + UndefinedBehaviorSanitizer, original + Combo.
- `python tests/shuffle_parity/enemy_drops/run_logic_tests.py`: PASS original + Combo. Full production CanKillEnemy and Deku Baba combat bodies execute with the actual witness lambdas. Own missing soul blocks every audited route; no weapon blocks; all tricks disabled; source witness/combat identity checked. This is a combat-predicate fixture, **not** a full graph reachability/fill proof.
- `python tests/shuffle_parity/enemy_drops/run_pool_tests.py`: PASS original + Combo with solo/unified session controls. Compiles real EnemyFirstDrops.cpp, location.cpp, randomizerEnumStrings.cpp. Executes full native GenerateLocationPool, PlaceItemsForType, AddFixedItemToPool, InitHashMaps, exclusion Parse/Serialize, FillExcludedLocations and exact native Own Dungeon filter. Context/dungeon services and UI-only LocationOption construction are deliberate isolated boundaries. Checks 0 off / 43 on, filler counts, names/global ownership/flags, exclusion round trip, all excluded slots junk, own-dungeon rejection despite inherited hint area, ordinary-check positive control. **Not** a full seeded solver fixture.
- Real-header syntax PASS: EnemyFirstDropsRuntime.cpp (0 warnings), EnemyFirstDrops.cpp, location_list.cpp, RCToRandInf.cpp, SeedContext.cpp, location_access.cpp, 3drando/item_pool.cpp, 3drando/fill.cpp, settings.cpp, randomizer_check_tracker.cpp. GUI units have the diagnostic runner's pre-existing IMGUI_DEFINE_MATH_OPERATORS redefinition warning, not a downgraded error.
- `git diff --check` on owned paths PASS.
- Initial RED was the missing native catalogue assertion. A later meaningful behavioral RED caught lost unique rewards: after EnGeldB_SetupDefeated, scene/register reset and reload without a second defeat produced no delivery (`runtime_test.cpp: spawned>beforeRecovery`). Separate saved defeated flags plus player-update redelivery made this GREEN in both guards. Do not claim a pre-implementation behavioral-red history beyond this recorded evidence.

## Source provenance

- Current native 2Ship implementation inspected, unchanged: `mm/2s2h/Rando/ActorBehavior/EnemyDrops.cpp`. Audit `twoShip` / `combo` snapshot blob `c6e2e33fe703653f27c64b06f90c81b1f6d3196e`, SHA-256 `27159dc4150f58e86215fbf45ec3bfed655e6c67a9ebaf364f7e0b6a84dd38d4`. MM code was not modified.
- Full OoTMM commit `0c0f8a8e1412f3bc979e77a2563293770d58a439` fetched from the official OoTMM/OoTMM codeload endpoint. Archive SHA-256 `1c1e754513b7187535cf2ba051293a4c41849c6e98e24227e9dd76faa033ca23`. Additional witness comments identify exact source files/lines. Native actor IDs and native combat predicates are used, never donor numeric IDs.
- The audited native witness references are recorded alongside each row in EnemyFirstDropsWitnesses.inc. Additional pinned source evidence is in EnemyFirstDropsAdditionalWitnesses.inc.
- Official task-local dependencies: CMake-pinned ImGui v1.91.9b-docking (archive `466fdef9b18de15f0bb6e288e3d00ffa3d82200ec458ce5e4f724a161d9528a5`), spdlog v1.16.0 (`8741753e488a78dd0d0024c980e1fb5b5c85888447e309d9cb9d949bdb52aa3e`), SDL release-2.32.10 (`03f9d7c191a837525c9cda6406af2f2e48be02b5e7eb03d949cc9f1e9ca41c8b`), thread-pool v4.1.0 (`1da8bb1562433cb0759d774182d0d6fa3b87a04c26f64c6886f640445bf68efe`); official nlohmann v3.11.3 json_fwd.hpp restored alongside existing json.hpp. No fabricated production headers or warning downgrades.

## Remaining work / hard gates

1. Prove and add physical witnesses for GUAY, PEAHAT, SKULLWALLTULA, TAILPASARN. Do not replace these with fictitious Root checks or blanket CanDamage.
2. Audit **each selected V/MQ combination**, age, time and event conditions. Some current sourced witnesses exist only in one layout; 39 source-audited types does not mean 39 types are guaranteed in every configuration. The copied full native predicates deliberately preserve restrictions and may conservatively require unrelated source-check conditions.
3. Full native seeded solo/combined Glitchless fills, excluded progression negatives, actual graph soul self-lock and cross-game receipt/export verification. The harnesses do not replace those checks.
4. Full executable build and configured runtime verification. Relevant settings/tracker and all touched native adapter syntax checks now pass after restoring official pinned GUI dependencies; this is not a full executable or configured gameplay claim.
5. Evaluate category presentation: global type checks currently occupy the existing Hyrule Field exclusion/tracker bucket, clearly labelled global. A dedicated virtual group needs a deliberate tracker/area refactor rather than silently inventing a physical dungeon owner. Verify hint text for RHT_NONE fallback before exposing.
6. Once all gates pass: expose the option in actual menus, register the real-CVar gold What's New accent through controller, and obtain configured gameplay acceptance. No commit, staging, publishing, or master promotion performed by this worker.

## Owned files

New: EnemyFirstDrops.h/.cpp, EnemyFirstDropsRuntime.cpp, EnemyFirstDropsCatalog.inc, EnemyFirstDropsWitnesses.inc, EnemyFirstDropsAdditionalWitnesses.inc; tests/shuffle_parity/enemy_drops/{runtime_test.cpp,run_tests.py,run_logic_tests.py,run_pool_tests.py}; this report.

Adapters: randomizerEnums/{RandomizerCheck,RandomizerInf,RandomizerMiscEnums,RandomizerSettingKey}.h, RCToRandInf.cpp, location_list.cpp, location_access.cpp, SeedContext.cpp, settings.cpp, 3drando/{item_pool,fill}.cpp, randomizer_check_tracker.cpp, soh/assets/custom/lang/en_US.json. Only scoped first-drop edits; no SoulShuffle* or MM files changed.
