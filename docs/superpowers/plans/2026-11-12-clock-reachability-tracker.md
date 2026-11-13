# Clock reachability and tracker implementation plan

Spec: `docs/superpowers/specs/2026-11-12-clock-reachability-tracker-design.md`.
Baseline: `a7cac0b273fe7a3c6608d0312afcf291c1ab556d`.
Candidate: `candidate/clock-reachability-tracker-20261112`.

## Global constraints

Preserve the supplied save, seed, placements and entrance mappings. Do not use
authored bridges or unmodeled Song of Time resets to certify reachability.
Keep solo/combined guards, source identity and discovery visibility intact.
Tracker resolution is read-only and applies each routing phase at most once.
Keep exact checkpoint and verification evidence; distinguish compiled fixtures
from configured gameplay acceptance. No public publication is part of this plan.

## Task 1: Clock return proof

Owned production files: `combo/rando/CrossWorldRando.h` and, if needed,
`combo/rando/CrossDungeonReachability.h`. New fixture:
`tests/cross_dungeon/clock_return_test.cpp`.

1. Exercise production combined fill with a deterministic native-oracle fixture.
   Demonstrate the false logical MM-start success when cross shuffle is enabled
   and the Clock is reachable only from the freely rooted MM world.
2. Separate bootstrap gating from the required return proof. Preserve actual
   cross topology in closed probes, credit only earned placements, and expose
   the native portal result independently of MM-world rooting.
3. Place required OoT prerequisites within closed-probe reach and enforce the
   final MM-start return check. Keep OoT-start cross behavior and No Logic's
   documented exception.
4. Verify negative and positive cases, shared-item views and oracle cleanup;
   commit only the owned paths with red/green evidence.

## Task 2: Final tracker destinations

Owned production files: `soh/soh/Enhancements/randomizer/randomizer_entrance_tracker.cpp`,
its header, `combo/rando/CrossDungeonOotRuntime.inc.h`, and a small new pure
`combo/rando/CrossDungeonTracker.h` helper. New fixtures:
`tests/cross_dungeon/tracker_destination_test.cpp` and, as needed,
`tests/cross_dungeon/tracker_behavior_test.py`.

1. Add a failing exact regression: Ice `0x88` to native GTG `0x08` to Fire `0x165`;
   do not continue through Fire's separate overlay to Pirates Fortress.
2. Add a const game-aware endpoint resolver and a host metadata boundary. Cover
   BeforeNative priority, the Fire-to-fountain return, MM endpoints, unknown
   metadata and native fallback.
3. Apply final metadata consistently to tracker labels, filtering, destination
   grouping and sorting. Keep source/discovery keys and native tables unchanged.
4. Refresh derived metadata on seed load; verify queries do not mutate gameplay
   state or unlock spoiler visibility. Commit only the owned paths.

## Task 3: Integration, review and recovery

Root owns test runner registration, documentation and recovery packaging. Tasks
1 and 2 have disjoint file ownership and may run in parallel; neither edits the
other's paths or the shared test runner. New C++ fixtures are auto-discovered by
`tests/cross_dungeon/run_tests.py`. Root adds any Python fixture explicitly.

1. Verify the untouched baseline and preserve its selected source checkpoint.
2. Review each bounded change for spec compliance and quality, then run the
   combined relevant suite and a whole-change review.
3. Record exact commits/trees, accepted input hashes, dependency identity, test
   output and remaining runtime limits. Create a deterministic recovery archive
   with original and candidate source plus patches and offline fixture inputs.
4. Extract into a fresh directory, check hashes and patch application, and save
   the recovery artifact durably. Present changes and verification accurately.
