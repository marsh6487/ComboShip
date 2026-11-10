# MM environmental checks — bounded source candidate in progress

Not complete or exposed. First vertical slice: the seven Snowhead Temple entrance Bg_Icicle placements. Exact pinned source: official OoTMM `0c0f8a8e1412f3bc979e77a2563293770d58a439`, `data/checks/mm/dungeons/snowhead_temple.xml:119–125`, `data/world/mm/dungeons/snowhead_temple.yml:1–38`, `data/macros/macros_mm.yml:296`. Room 0 actor-list indices: entrance 1=0, 2=1, 3=4, 4=5, 5=6; after-block 1=2, 2=3. Native existing pot/snowball adapters confirm scene/room/actor-list-index identity conventions; no coordinates are invented.

Owned planned source: new `mm/2s2h/Rando/MMEnvironment.h`, `MMEnvironmentIcicles.inc`, `ActorBehavior/BgIcicle.cpp`; scoped Types.h, StaticData/{Checks,Options}.cpp, Logic/GeneratePools.cpp, Menu.cpp, Logic/Regions/SnowheadTemple.cpp, ActorBehavior/{ActorBehavior.h,ActorBehavior.cpp}; exact native break hook in `mm/src/overlays/actors/ovl_Bg_Icicle/z_bg_icicle.c` and appended GameInteractor vanilla behavior enum. Test root `tests/shuffle_parity/environment`.

Reason for exact hook: native BgIcicle_Destroy also executes on unload; only BgIcicle_Break is positive break evidence. Native stalactite regrow also passes through Break, requiring active-drop suppression and permanent obtained ownership across cycle reset. Normal collectible drops must be vetoed only for active shuffled checks. Default off pool must contribute no check/item slots.

Task9 plan/spec and progress ledger read. Existing original research source pin reused. Full family scope (remaining icicles, rocks, boulders/red-boulders, soil), native seeded fills, executable and configured gameplay proof remain outstanding. No test pass claimed yet. No staging/commits/publication by this worker.

## Bounded implementation (seven checks only)

Source candidate now includes all owned paths listed above plus the appended `RCTYPE_ICICLE` icon row in `mm/2s2h/Rando/CheckTracker/CheckTracker.cpp` and the required allocation-null guard in `mm/2s2h/CustomItem/CustomItem.cpp`.

- New permanent, append-only check IDs are generated from `MMEnvironmentIcicles.inc`; existing enum values are preserved. Each static check has the real Snowhead scene and its own native before/after-block region CHECK. Check names derive from the same IDs for native UI, save/import/exclusion strings and Combo friendly names. Seven optional junk slots enter the native pool when enabled; disabled and excluded controls add neither those checks nor those item slots.
- Runtime uses the native authored-placement ObjectExtension index, not a live actor list index or guessed coordinate. Actual `z_actor.c` placement loop sets `currentActorListIndex = i` at line 2949; allocation snapshots it through `SetActorListIndex` at 4114 and immediately resets it to -1. Dynamic spawns cannot inherit the previous index. The positive hook is registered only for `ACTOR_BG_ICICLE` and exact scene/room/index matches; unmatched tuples and off/unshuffled actors preserve native behavior. No Actor ABI change.
- Actual `BgIcicle_Break` invokes the added VB hook. Existing particles/audio remain; only identified, still-unobtained shuffled placements suppress their native random/fixed collectible. No reward hook is attached to actor destruction/unload. Existing regrow and falling-break transitions invoke the same hook.
- An unsaved `pendingDrops` map records the physical break position separately from saved eligibility. Actual `CustomItem::Spawn` allocates an EnItem00, with a narrow null check before callback writes. Failed allocation retries on player updates. `activeDrops` prevents repeated/regrow breaks from allocating duplicate items. Only physical pickup sets saved `.eligible`; this feeds the existing native CheckQueue path. The permanent `.obtained` guard remains authoritative when Song of Time clears `.cycleObtained`. The fixture exercises pickup eligibility plus the real saved JSON/cycle functions; it simulates the standard receipt queue's `.obtained` update, and does not claim an end-to-end GameInteractor item-give execution.
- Room/scene reload may clear active/pending pointers. An uncollected check remains physically renewable because the custom-drop veto never consumes the native collectible flag. It can be broken again after reload. No unique one-shot state is silently treated as renewable.
- Conservative WIP CHECKs use native sword/explosive/form/bow/nut predicates. No weapon denies; individually owned bow/nuts/Deku/Goron/Zora routes admit; removing the item denies again. Native nut-bag ownership gates nuts. Existing region ingress/block access is retained. No soul is invented for an icicle; unrelated enemy requirements remain in the existing graph.
- New category label and bounded tracker icon index are present. Disabled saved exclusions are retained but hidden from exclusion display and group metrics. The WIP option is deliberately not exposed by a menu control or What's New entry.
- Two explicit handle casts in the touched C TU preserve the exact existing `__OTR__` resource addresses: `CollisionHeader*` for `gIcicleCol`, `Gfx*` for `gIcicleDL`. The production CMake already permits these legacy pointer conversions; these casts allow strict GCC14 standalone syntax verification, and are not a claimed baseline gameplay fix. ImGui syntax flags reproduce `libultraship/cmake/dependencies/patches/imgui-fixes-and-config.patch:112`'s actual `ImTextureID=void*` interface, rather than weakening diagnostics.

## Verification and reproducible commands

From repository root, use:

```
export CPATH=../recovered-tooling/deps:../firstdrops-tooling:../firstdrops-tooling/imgui-1.91.9b-docking:../firstdrops-tooling/SDL2:../firstdrops-tooling/spdlog-1.16.0/include
export CC=gcc-14 CXX=g++-14
```

- `python3 tests/shuffle_parity/environment/run_tests.py`: native original + Combo PASS before the last added category/rule assertions; final sanitizer rerun below covers the added assertions.
- `python3 tests/shuffle_parity/environment/run_tests.py --red-spawn`: verified behavioral RED against the exact pre-feature `0c2e93bb` `CustomItem::Spawn` body. A dedicated allocation-only case must fail specifically with SIGSEGV; this runner rejects unrelated compile/assert failures as RED evidence. Current null guard made the same actual Spawn allocation path GREEN.
- `python3 tests/shuffle_parity/environment/run_exclusion_tests.py`: PASS original + Combo, all 128 subsets through actual native name-keyed exclusion writer/reader and JSON reload, plus actual menu enabled/disabled helper and its display/metric integration.
- `run_tests.py` compiles production catalogues, full native GeneratePools/placement constraints and new runtime callback bodies against actual game headers. It executes actual `CustomItem::Spawn`, actual native break/attacked/fall bodies (the C parameter name `this` is mechanically renamed to `self` to compile those function bodies in the C++ fixture), actual JSON conversion, actual cycle-save functions, actual native CHECK lambdas and the native authored-index snapshot/reset statements. Engine allocation, hook registration, drawing and ObjectExtension storage are explicit service boundaries; no fabricated production header is used.
- The fixture uses catalogue-membership regions for pool enumeration, not the whole reachable world graph. The seven CHECK expressions are separately executed with actual inventory/form/bag predicates. This is not a full native solver/fill or configured-gameplay proof.
- `git diff --check` on owned changed paths and Python syntax compilation PASS.

Final sanitizer and whole-TU status is appended after the running checks complete.

## Remaining gates

The seven-check slice remains hidden WIP. Remaining native full graph/seeded Glitchless fill, real item-give/cross-game receipt integration, executable link and configured gameplay acceptance are not available in this local fixture. Custom scene layouts that alter authored placements have not been independently audited; no custom-layout identity compatibility claim is made. Do not expand before reviewing this bounded vertical slice. Remaining icicles outside the entrance, rocks, boulders/red-boulders and soil remain unimplemented. No full environmental-shuffle, family-completion or user-acceptance claim is justified. Root owns checkpoint, CI, publication, ledger and promotion.

## Final bounded verification result

- Final `run_tests.py --sanitize`: PASS original + Combo with AddressSanitizer + UndefinedBehaviorSanitizer, no recovery, leak detection disabled only for host process-inspection limits. This includes the last actual CHECK and category-index assertions plus exact 0/7/0 disabled/enabled/excluded item-slot deltas. No weapon/form blocks all seven; actual accepted bow/nut/form inventory admits individually; missing native nut bag blocks nuts.
- Final `run_syntax.py`: PASS all 20 complete translation units (9 C++ + 1 C in each original/Combo guard), including the actual native C actor, ActorBehavior registration, CheckTracker, CustomItem, real scene region, full Checks/Options, GeneratePools and Menu. The pre-existing ignored anonymous-enum `typedef` warning in CheckTracker.h:37 remains; no diagnostic was downgraded.
- Exact unchanged baseline Spawn SIGSEGV control and all 128 exclusion subsets PASS as described above.

Bounded seven-check source is ready for independent review/checkpoint; not ready for menu exposure or a full-family completion claim. No staging, commit, index operation, asset/ROM upload or publication performed.

## Review follow-up: pending-drop room ownership

The review found a real stale-coordinate defect: `pendingDrops` originally held only a position and was reset on scene changes. A failed allocation in Snowhead room 0 could retry after the player entered room 1, spawning at the old room's coordinates. The actual production callback fixture was extended first; it failed at `native_test.cpp:143`, `allocations == beforeRoomChange`, after a failed room-0 allocation followed by room-1 player update. This is behavioral RED, not a source-string check.

Pending records now bind the exact source scene, actor room and position. Before allocation, a record whose scene or room differs from the current owner is discarded. Returning to room 0 does not resurrect the stale request. A new break of the renewable physical source queues a fresh request, and same-room allocation retry still works. Saved `.eligible` remains false until physical pickup. Off/unidentified native-drop and permanent obtained guards are unchanged.

### Physical access limitation clarified

The pinned check XML establishes placement IDs, names, scenes, rooms and authored indices; it does **not** establish each placement's actual actor parameter bits. The native asset XML in `mm/assets/xml/N64_US/scenes/Z2_HAKUGIN/Z2_HAKUGIN.xml` is an extraction manifest, not decoded actor parameter metadata. Actual `BgIcicle_Wait` changes to Shiver when player distance is below 60, and Shiver/Fall can break a stalactite without a weapon. The seven source-macro combat predicates therefore represent conservative WIP access, **not exact physical access parity**. Per-placement actual params/proximity access, potential conservative self-locks, and full Glitchless seeded fills remain hard gates before any exposure/family-completion claim. The runtime fixture's generic random/fixed/regrow mode tests do not prove which mode is authored at each catalogue placement.

Post-fix verification status is appended after execution below.

Post-review GREEN: `run_tests.py --sanitize` PASS original + Combo, including the new failed-room-0-allocation → room-1-player-update negative, stale-request discard, return-without-autoaward, fresh break and successful same-room allocation retry. AddressSanitizer/UndefinedBehaviorSanitizer remain enabled with no recovery and only leak detection disabled. Full changed `ActorBehavior/BgIcicle.cpp` translation-unit syntax PASS again in both guards using `run_syntax.py`'s same real-header flags, targeted to this newly modified TU. `git diff --check` PASS for the follow-up paths. Exclusion/other TUs were unchanged by this fix; their prior successful verification remains applicable. No index or commit operation performed.
