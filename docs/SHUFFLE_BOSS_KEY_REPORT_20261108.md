# Own-dungeon boss-key regression — 2026-11-08

## Candidate and scope

- Accepted source baseline: `ced18542387d5ac8879e0f1cb303e790e4e3a1a8`.
- Authorized scope/spec checkpoint: `b3280ce58e9993ff9180d8b83096984c7961aa8a`.
- Candidate branch: `candidate/ootmm-shuffle-parity-20261108`.
- Bounded production changes: `MM_DumpRandoStaticData` in
  `mm/2s2h/BenPort.cpp`, and fixed-placement parsing/final validation in
  `combo/rando/CrossWorldRando.h`, with GUI dump-error rejection before fallback.
  No enum, option identity/value, asset,
  native solo fill, original-game guard, save format, or master changed.
- Implementation source checkpoint:
  `fa0478de099d35005d9099b9200b1e2fabc1abc2` (ordinary native gate verified;
  configured runtime remains unverified).

## Confirmed cause

The reported leak is reproducible at a real native generation boundary.
`MM_DumpRandoStaticData` copies restored configuration into a fresh local
`RandoSaveInfo`, and passes that object to `GeneratePools`. It previously
called `PreplaceConfinedItems` without supplying that generated state.
Confinement reads `RANDO_SAVE_OPTIONS` and writes `RANDO_SAVE_CHECKS`, which
refer to the unrelated live save. A cold or stale live save containing
Anywhere therefore leaves all four MM boss keys in the free cross-game
pool despite the persisted generation option being Own Dungeon. The
combined fill does not have item/dungeon restrictions for that free pool.

This is the existing GAP-9 described in `docs/COMBO_FILL_PARITY.md`, now
reproduced by compiling and executing the real native adapter and pool
functions. The older `FleetComboRando.cpp` already has origin-game
restrictions; changing it would not repair the launcher combined-fill
export boundary used here.

Pre-fix diagnostic execution printed:

```text
cold-live-save: boss keys fixed=0 free=4 live restored=1
seeded full-catalog pool/combined fills=64 MM boss keys in OoT=26 Stone Tower normal=0 inverted=0
```

The last two orientation counts are zero because none of the keys were
exported as confined placements, rather than because those checks were
absent. The diagnostic was run with the new test files against the
unchanged production boundary at the scope checkpoint.

The reproducible historical-adapter diagnostic specifically places the
Stone Tower Boss Key in OoT on fixture seeds **12, 18, 33, 38, 41, 42**.
For example seed **12** puts it at **`OOT fixture 3`**. Those are modeled
OoT check identities in the actual combined-fill test, not names of
configured in-game checks. This separates evidence for the reported
specific key from the aggregate of 26 foreign boss-key placements.

## Correction

Native confinement now borrows the generated rando state in a local RAII
transaction. It copies the generated placements back to the local
snapshot, restores the live rando state on normal return or exception,
and exports fixed placements from the local snapshot. It rejects an item
that remains confined after all legal checks were exhausted (for example
by exclusions), instead of quietly exporting it as a free item. That
failure returns explicit error-bearing JSON from inside the transaction;
the common combined-fill parser rejects it before reading the pools.
This avoids an exception escaping the export to headless callers, while
the early return still restores the live save.

Bounded review found that the legacy no-oracle fallback did not consume the
new error marker. A new failing regression reproduced its valid-looking empty
MM spoiler. Both generation consumers now use the same diagnostic-aware parser,
and the GUI validates both dumps immediately after acquisition, before Hunt
counting, entrance work or fallback. The fallback rejection is tested for both
source games, with a valid-dump control. This review correction does not change
generation retry budgets or normal placement policy.

The MM random confined placements explicitly request combined final
validation with `validate: true`. This is necessary because native
preplacement chooses a matching physical dungeon check without testing
whether that check requires the same key. The combined solver already
collects fixed items only when reached; its final advancement validation
now checks these opted-in fixed placements too. An unreachable own key
rejects the fill. Existing vanilla/other fixed placements without the
metadata retain their previous validation behavior. The existing outer
whole-fill attempts re-roll native preplacements on rejection; the
attempt budgets are unchanged.

## Native regression fixture and evidence boundaries

`tests/shuffle_parity/boss_keys/run_tests.py` compiles these production
bodies unchanged, with actual MM game/rando/save types and the combined
float-matrix/s32-vertex ABI:

- Complete `GeneratePools.cpp` and `PlacementConstraints.cpp`.
- Complete native check, item and option catalog initializers.
- `MM_DumpRandoSettings`, `MM_RestoreRandoSettings`, and
  `MM_DumpRandoStaticData`.
- Actual computed starting-item and save starting-item functions, native
  reversible item naming, and `FleetCombo_UnifiedPoolActive`.
- Complete `CrossWorldCombinedFill` from the production header.

Configuration storage, RNG service, check-label/hint rendering, region
membership, OoT confined input and reachability oracle callbacks are
fixture boundaries. All catalog checks are assigned to a fixture region;
the real pool function selects the checks enabled by the actual options.
Reachability in the seeded placement checks is intentionally synthetic.
The circular-key negative case makes a confined check reachable only
after owning the exact item placed there, exercising the actual combined
fixed point and final validation. These are real native adapter/pool and
combined-fill executions, **not full dual-native Glitchless seeds**.

The physical-owning-dungeon assertion uses an independently specified
key-to-scene contract for Woodfall, Snowhead, Great Bay and Stone Tower.
Both Stone Tower scene orientations and its boss scene map to the same
owning dungeon. Inverted chest catalog entries intentionally retain
`SCENE_INISIE_N` for shared chest flags, so orientation coverage uses
their actual `_INVERTED_` check identity rather than assuming every
inverted check stores `SCENE_INISIE_R`.

## Commands and results

From the repository root, with GCC 14 and the official nlohmann/json
3.12.0 single-header dependency prepared at `../build-deps/include`:

```bash
CXX=g++-14 python3 tests/shuffle_parity/boss_keys/run_tests.py --json-include ../build-deps/include --adapter-ref b3280ce58e9993ff9180d8b83096984c7961aa8a
CXX=g++-14 python3 tests/shuffle_parity/boss_keys/run_tests.py --json-include ../build-deps/include --adapter-ref b3280ce58e9993ff9180d8b83096984c7961aa8a --observe
CXX=g++-14 python3 tests/shuffle_parity/boss_keys/run_tests.py --json-include ../build-deps/include
ASAN_OPTIONS=detect_leaks=0 CXX=g++-14 python3 tests/shuffle_parity/boss_keys/run_tests.py --json-include ../build-deps/include --sanitize
CXX=g++-14 CPATH=/workspace/scratch/198fa7c65e91/build-deps/include python3 tests/dungeon_rewards/run_tests.py
CXX=g++-14 CPATH=/workspace/scratch/198fa7c65e91/build-deps/include python3 tests/magic_mirror_shuffle/run_pool_tests.py
git diff --check
```

`--adapter-ref` substitutes only the actual historical BenPort function
bodies, leaving the current native pool/catalog and combined solver in
place. Its strict run must fail the four-confined-keys assertion.
`--observe` is a deliberate pre-fix diagnostic: it requires a leak and is
expected to fail against the corrected adapter. Use ordinary mode without
an adapter reference for the regression gate.

Verified ordinary-mode coverage:

- Persisted native MM settings round trip, then cold/stale live save:
  **4 confined boss keys, 0 free**, exact live rando-state restoration.
- Seeds **1–64**: 64 generated full-catalog native MM pools passed through
  the complete combined fill; **0 MM boss keys crossed to OoT**. All four
  keys stayed in their physical owning dungeon. Stone Tower selected
  **41 normal and 23 inverted check identities**.
- Seeds **65–128**: 64 native pool/export cases with imported exclusions,
  including original Woodfall/Stone Tower boss-key chests and a quarter
  of dungeon check IDs; all four keys remained in their own physical
  dungeon and avoided excluded checks.
- Seed **129**, circular fixed-key oracle: rejected after all three
  existing inner validation passes; **4 unreachable MM advancement
  checks** were reported. Removing the explicit validation metadata is a
  passing legacy-behavior negative control.
- Injected exception after a first confined native placement, exception
  during later hint emission, and fully excluded dungeon checks all
  restore the live rando state; exhausted confinement returns explicit
  error JSON and the real common GUI/headless combined-fill parser rejects
  it without reporting an apparently valid empty pool. A focused failing
  test was run before replacing the initial throwing implementation with
  this error return.
- Native Anywhere/Start With/Vanilla controls retain respectively
  **4 free / 0 placed / 4 vanilla** boss keys. Anywhere seeds **1–16**
  completed combined fill and crossed **6 boss-key copies**, proving the
  fix does not disable legitimate cross-game placement.
- Reverse stale-state control (live Own Dungeon, snapshot Anywhere)
  exports all four keys free and restores the live state.
- Existing native dungeon-reward pools pass **32 configurations in each
  of ComboShip and standalone MM**. Existing Mirror pool regression
  also passes.

Final-source ordinary suite exits 0. The strict historical-adapter run
exits 1 at `FAIL: VerifyFixed(dump) == 4`, with `fixed=0 free=4`; this is
the expected regression failure, distinct from fixture setup failures.
The complete source/tests checkpoint was created after ordinary native
verification, before waiting on the slower final sanitizer invocation.
The final full **ASan/UBSan run exits 0** with all 64 Own Dungeon seeds,
64 exclusion exports, 16 Anywhere seeds, circular-key rejection, legacy
fixed control, explicit exhaustion/parser rejection, settings controls,
and failure restoration passing. There are **no ASan/UBSan diagnostics**.

Fixture setup findings were corrected before assessing production: the
existing extraction utility did not handle templated reference return
types, and the synthetic oracle initially omitted the required placement
callback. Neither was a native-generation failure. The first sanitizer
run reached all 64 own-dungeon fills without a sanitizer diagnostic but
hit the fixture's initial 120-second execution budget before the remaining
cases. The sanitizer budget was raised to 360 seconds, without changing
the engine's generation attempts or test assertions.
An ensuing full sanitizer run completed all assertions but LeakSanitizer
failed at process exit because the managed runtime uses ptrace. The final
ASan/UBSan invocation disables only leak detection with
`ASAN_OPTIONS=detect_leaks=0`; leak checking is not claimed.

Review-fix validation: `ASAN_OPTIONS=detect_leaks=0 python3 -B tests/shuffle_parity/boss_keys/run_tests.py --json-include ../build-deps/include --sanitize --failure-path-only` exits 0 for both failed-source fallback rejections, valid fallback, common fill rejection and live-state restoration. The initial new negative test failed at `REQUIRE(rejected)` before the parser correction. An initial corrected sanitizer invocation passed its assertions but hit the already documented ptrace leak-check limitation; the final invocation disables leak detection only. CI policy discovery remains 25/25 passing after registering the full native suite.

## Remaining evidence and recovery

No `libsoh.so`, `lib2ship.so`, or `comborando` build artifacts were
available in this workspace when this task began. The complete MM BenPort
production translation unit has since compiled under the generated native build
command. Full game linking, actual native OoT restricted fill/settings execution,
real dual-native oracle generation, cross-dungeon entrance permutations,
and configured in-game pickup/load/reset behavior remain unverified by
this fixture. OoT's existing confined input is modeled and its fixed key
is asserted unchanged by combined fill; this must not be reported as a
native OoT seed test.

The next decisive test is linked headless dual-native generation with
**both** native boss-key settings Own Dungeon, including a cross-dungeon
entrance map and imported exclusions, followed by configured Stone Tower
normal/inverted pickup and paired-save replay. These results do not
establish runtime acceptance or master promotion. No external assets,
ROMs, saves, external writes or push were used in this bounded task.

The controller retains responsibility for durable recovery packaging.
Restore the implementation checkpoint on top of the scope checkpoint;
the tests need only GCC 14 and the JSON header above, with no ROM or
resource archives.
