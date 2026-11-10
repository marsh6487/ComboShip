# Solo MM entrance runtime candidate

Final integration/build/native-matrix status is recorded in
`docs/poc/solo-mm-entrances-verification-20261110.md`. The sections below preserve
the earlier runtime milestones and their evidence boundaries.

Baseline: `191794e8c7534c30a55ee4ab4554e49356c6ca75` (R22). Work is isolated on
the Solo MM entrance candidate. Accepted masters and the original checkout are
unchanged.

## Coupled local routing milestone

Implemented a separate local MM route state, strict native seed binding,
physical respawn bypass, checkpoint ownership and native load restoration after
the normal `OnSaveLoad` hook chain. Cross-game installation, arrival queues,
fade completion and launcher handoff entrypoints retain their combined-session
guards. MM's native JSON save owns its entrance permutation; preview
deserialization has no runtime side effect.

Verified on 2026-11-09:

- `tests/solo_mm_entrances/save_test.py`: original and COMBO native JSON roundtrip,
  preview isolation, legacy clearing, checkpoint metadata and malformed/foreign
  input rejection passed.
- `tests/solo_mm_entrances/runtime_test.py`: actual native load hook and route
  adapter passed local dungeon ingress/paired return, physical respawn bypass,
  owl warp preservation and cross-game/session guards.
- Both tests were first observed failing against the baseline: the JSON adapter
  discarded the saved map and Solo MM ignored the saved Woodfall-to-Snowhead
  route.

These are native source behavior tests, not configured gameplay acceptance.
Full native build and in-game scene/respawn checks remain pending.

## Expanded requested scope

The requested candidate also includes overworld, mixed category pools and
decoupled returns. The coupled milestone is a recovery point, not a completed
deliverable. The generator owns a dedicated Solo route codec; the runtime will
consume its validated explicit routes while preserving combined route decoding.
Bosses mixed with any other populated category use physical boss world clears
for all four bosses. Recovery and blue-warp destinations still follow the
saved ingress/return policy.

## Expanded runtime milestone

The native save now stores schema 2's forward and reverse permutations, selected
pool/policy bits, recovery checkpoint, boss completion mask and the checkpoint
captured with Farore's Wind. The local adapter installs the core's validated
explicit routes; the original combined `Install/Decode` contract remains intact.

Verified on 2026-11-09 after observing the failing behaviors:

- `tests/solo_mm_entrances/runtime_test.py` exercises the actual native selected
  save-load bridge and extracts the registered Solo lifecycle hooks from
  `BenPort.cpp`. It passes restore-after-respawn-hook ordering, retained day/time,
  new-cycle town start, selected-file clearing, normal scene replacement,
  title/reset teardown, end-of-cycle save and moon-crash clearing.
- The same test verifies logical grotto content/position replay, paired hole
  exits, mixed temple/grotto/boss cycles, inverse-ingress boss death recovery and
  independent decoupled blue-warp returns. It also runs the real Giant escape
  queue transport for first/repeat warps from all four bosses, preserving queued
  item presentation. Giant/Oath bookkeeping itself is exercised separately by
  the existing production boss-warp test.
- Overworld seasonal aliases route through canonical native portals. The real
  Ranch practice function, Sharp reload event, authored Solo forest return and
  Pamela healing reload retain their native destinations; walking those same
  portal indices still shuffles. Both Stone Tower inversions remain shuffleable
  with their native cutscene index 0. Interior source-scene validation also
  accepts the spring smithy portal and return.
- The Farore test uses the native cast/save and recall code. A point captured in
  Goht's arena, followed by visiting Odolwa's arena, restores Goht's captured
  checkpoint without permuting its physical coordinates, room or temporary
  flags. A subsequent death returns to the source temple associated with Goht.
  The existing ban on casting new points inside grottos remains intact.
- `tests/solo_mm_entrances/save_test.py` passes actual native JSON conversions in
  original and COMBO builds, including schema 2, preview isolation, legacy
  clearing and the new bounded event-array loader. Short pre-Twinmold event
  arrays were first observed failing and now load with the new event cleared.
- `tests/solo_mm_entrances/explicit_runtime_test.cpp` passes mixed-category
  ingress, independently selected reverse returns, source-owned death recovery,
  transactional foreign-route rejection and unchanged combined category rules.
- `tests/farores_wind/run_tests.py` passes existing original/combined scene policy,
  launcher point persistence, both native bridges, both handoffs and slot
  isolation. `tests/cross_dungeon/mm_boss_warp_behavior_test.py` passes its real
  first/repeat all-four-boss bookkeeping and transition-layer checks.

The failing mixed-grotto cases exposed two distinct native respawn hazards. Boss
death previously cleared mode 4 after materializing a logical hole return; it
now clears before materialization. An ordinary destination selected when leaving
a hole previously inherited source mode 4; each selected local leg now clears
the source mode before allowing the chosen destination to reconstruct its own
respawn metadata.

The oracle starts Solo reachability from its native root and excludes combined
arrival inputs. Full native oracle/build verification is owned by the parent
integration workstream. The combined adapter and transition-fade fixture prefix
updates are owned by the generator workstream; their previous minimal fixtures
needed the new dormant Solo interfaces to compile.

These are source/native behavior results. Full executable verification and
configured gameplay acceptance remain separate parent-workstream gates.

## Positioned Spiritual Stone recall follow-up

The parent reviewer reproduced a Spiritual Stone point captured in one boss
arena recovering through a later arena's checkpoint. Spiritual Stones also
allow grotto casts, unlike Farore's Wind. A new Solo-only
`Combo_MM_SoloEntranceRestoreWarpCheckpoint` boundary restores the validated
captured owner and rebuilds logical grotto entry metadata while preserving the
caller's physical TOP point and respawn mode. The parent owns the actual stone
capture/recall call sites and the appended native point field.

The real JSON converter now roundtrips each stone's `soloEntranceCheckpoint`
and defaults missing fields/empty legacy data to -1 for all three points.
The native runtime test first observed stale grotto18 content after restoring
grotto19 ownership. It now passes logical grotto19 metadata reconstruction,
unchanged TOP coordinates/room/temporary flags, retained physical recall mode,
no second permutation on recall, and the correct mapped exit afterward.
Both JSON build modes and the full local runtime adapter suite passed after
this change. The runner accepts `COMBO_SOLO_MM_RUNTIME_TEST_SOURCE` so the
parent's actual Spiritual Stone production-body regression can reuse the same
native types, route adapter and extracted engine hook fixtures.

The parent reports that its production Spiritual Stone capture/recall regression
also passes independent points, boss source recovery, shared grotto content and
returns, physical coordinates and combined-session isolation. This bounded helper
checkpoint depends on the parent-owned appended field in
`mm/mods/spiritual_stones/spiritual_stones_save.h`; restore both component commits
from the final candidate branch when rebuilding this milestone in isolation.
