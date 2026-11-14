# Shared progression with relaxed OoT logic

This regression compiles the production combined fill and forward replay. Its small oracle graphs expose only explicitly listed free checks and possession-gated edges; they do not make every check reachable.

The main graph confines a native MM boss key to one check requiring a shared capability. Both games initially contribute that capability, and sharing retains one OoT-owned copy. The only free pickup locations are in MM. Generation must place the retained source logically, preserve its OoT item identity through foreign-item serialization, and reach the MM key from empty inventory. The test runs native shared Bow and a distinct NEI Whip alias with either starting game.

Additional cases verify that:

- OoT-only advancement can still occupy an unreachable check in No Logic, while All Reachable rejects the same world.
- Beatability-only cannot discard the logical obligation of a retained source serving MM.
- Final validation rejects an unreachable fixed shared source when its native adapter explicitly requests validation.

These focused graphs test the shared-source placement policy. They complement a reproduction using the actual native game modules; they do not prove all native region rules or configurations.

## Native rewards after entrance shuffle

A second graph models native End-of-Dungeon rewards preplaced before the final entrance topology is known. The original assignment wins by forward collection in the initial topology. The changed topology puts one reward behind its own possession gate, and the same assignment becomes self-locked. Adding `validate: true` to the locked rows detects the failure but cannot repair it. Supplying `allowedChecks` for the two eligible native boss checks lets the production combined fill relocate the rewards; forward replay must then collect the moved reward before the newly accessible boss reward.

The regression preserves every item and its game of origin, keeps the forced Light Medallion at Link's Pocket, and requires movable rewards to remain in their native boss domain. A reachable OoT overworld chest and a same-named MM check cannot rescue a completely blocked native domain. An accidentally included Pocket check is rejected as an ineligible domain member. These are tests of the native dump contract consumed by the combined solver; the native integration sweep separately exercises the adapter that exports that contract.

Portal-prerequisite placement has its own domain regression. A boss reward needed to open the OoT-to-MM portal must still land at its legal native boss, even when an ordinary OoT chest is reachable. If that boss requires the same reward, generation must reject the cycle instead of moving the reward to the ordinary chest. A separate unconstrained-prerequisite control remains valid, and the reachable boss case must win by forward replay through the closed portal.

### Scarce boss capacity and topology feasibility

A three-boss graph gives two stones only two early reward slots, while a medallion can use a third boss behind both stones. Seed `9` exhausts all three fill passes under the original random ordering by spending early capacity on the medallion. The regression requires the production fill to preserve that capacity and forward replay to collect the stones before the late reward. It asserts the completed behavior and item identities, not a particular retry count.

Direct behavioral tests of `NativeRewardTopology.h` use the same possession-gated oracle with exactly the earned reward subset and any fixed Pocket item. They reject three stones behind one early boss, accept three early bosses, and accept two early bosses when the third stone is already in Pocket. They also reject a cycle with no first pickup and exclude a reachable boss that is outside the movable reward domain. An eight-reward graph with only seven usable early slots checks that infeasibility is detected within 256 reachability queries; fully reachable eight-reward, empty-domain, and unsupported-count cases cover the boundary. These tests establish the feasibility helper's contract, not the native adapter's treatment of keys, prices, entrance reloads, or RNG state.

## Run

Requirements: a C++20 compiler and the nlohmann JSON headers.

```sh
python3 tests/shared_progression_logic/run_tests.py
```

To check a preserved source tree or provide a non-system JSON include directory:

```sh
python3 tests/shared_progression_logic/run_tests.py \
  --source-root /path/to/source \
  --json-include /path/to/include \
  --output-dir /path/to/evidence
```

The runner records the compile command, exit codes, and complete compiler/test output when an evidence directory is supplied. It returns a failing status if compilation or any behavioral assertion fails.

## Native settings interaction sweep

`native_sweep.py` runs the actual combined generator and both native game modules. Its default matrix contains **32 configurations, each tested with master seeds 20261109 and 20261110**, for 64 native runs. This is bounded interaction coverage, not an exhaustive Cartesian product or a guarantee for every seed.

The original 28 configurations remain unchanged. Four additional profiles cover native OoT entrance shuffling combined with cross-game entrances under all-reachable and mixed-logic policies, plus native entrance shuffling with vanilla and anywhere dungeon-reward modes.

The matrix includes individual and combined shared Bow/Magic/Light Arrow families, NEI aliases, relaxed OoT logic, both No Logic settings, native key-placement modes, MM key rings, OoT/MM/random starts, several cross-game entrance pools, rusty keys, soul families, clocks, song events, and selected all-parity combinations. Large item-family cases explicitly enable additional native check capacity. The two base settings snapshots come from public native CI evidence; `configs/provenance.json` records their origin and hashes. They contain no uploaded user configuration, save, or gameplay log.

Run against a built native runtime and a `comborando` driver supporting `--fill-only`:

```sh
python3 -B tests/shared_progression_logic/native_sweep.py \
  --runtime-dir /path/to/native-runtime \
  --driver /path/to/comborando \
  --output /path/to/new-evidence-directory \
  --jobs 3 --timeout 300
```

On Linux, the runtime directory must contain `libsoh.so` and `lib2ship.so`, plus their ordinary runtime dependencies. The CI workflow uses `_packages/stage` after the native runtime staging step. For a local environment that needs additional bundled dependencies, repeat `--library-dir /path/to/libraries` as needed. Each native run has a fresh process, writable directory, and local executable. `SHIP_HOME` points explicitly to that directory, which starts without a `comboship.json`; the native first-run settings restore creates it. Runtime libraries and assets are linked, and existing saves/configuration/log directories are not copied.

### Native array-setting restoration gate

Before any fill cases start, `native_settings_restore.py` loads the actual SoH and MM modules in an isolated directory prepared by the same `prepare_work` function as the cases. It calls both native headless initializers, settings restorers, and settings dump exports. The requested MM `gRando.StartingItems` and `gRando.ExcludedChecks` arrays must survive the round trip. Starting items retain exact order and multiplicity; excluded checks may be reordered by native check ID but must retain their names and multiplicity. The check is deduplicated by the requested array pair across the selected public cases. Both current base fixtures request empty arrays, so the default 64-run matrix needs one such native preflight.

This gate covers a configuration setup failure found during native verification: writing an artificial `{"CVars":{}}` config caused native JSON flattening to produce `CVars:null`. The native block writer then could not restore MM's array settings, and an empty requested starting kit silently became the default kit. A genuinely absent config follows the supported native first-run path. This is why the harness does not synthesize an empty config object.

Each preflight has a 30-second process limit and retains its input, command, native stdout and stderr, restored settings, result, and hashes under `settings-preflight/`. `settings-preflight.json` records the requested arrays and the cases covered by each probe. Any mismatch, native failure, timeout, or missing verdict fails the sweep before fill cases start; the summary records zero completed fill cases and cannot report `PASS`. This focused gate verifies the two array settings, not every scalar setting or saved-seed behavior.

Useful smaller invocations:

```sh
python3 -B tests/shared_progression_logic/native_sweep.py --list
python3 -B tests/shared_progression_logic/native_sweep.py --self-test
python3 -B tests/shared_progression_logic/native_sweep.py \
  --runtime-dir /path/to/native-runtime --driver /path/to/comborando \
  --output /path/to/new-single-case-evidence \
  --case mixed-native-nei --seed 20261109
```

`--case` and `--seed` can be repeated. The output directory must be new so stale evidence cannot satisfy a run. The manifest records fixture, harness, driver, and module hashes, requested settings, preflight groups, starts, entrance flags, seed inputs, and the accessibility contract for each case. Each case/seed directory retains its settings, command, native log, and result; command provenance records the isolated `SHIP_HOME` and initial config absence. Results distinguish `PASS`, `FAIL`, `TIMEOUT`, and `BLOCKED`; every nonpassing result makes the sweep fail. A timeout never becomes a pass, even if a partial log contains a success line. `results.json` is updated as runs finish, and `summary.json` records the final verdict. Native generation may use its ordinary deterministic retries; the result records successful resolved seeds separately from the requested base seed.

Cancellation or the outer CI step timeout can interrupt the sweep before a final summary is written. Partial logs, unfinished `RUNNING` entries, or a missing summary are incomplete evidence and do not establish a pass.

### What a native sweep pass proves

A pass requires every native array-setting preflight to pass, followed by a successful native process exit for every case, exactly one completed seed and one `1/1` PASS summary, acknowledgment of `--fill-only`, and the generator's final validation from empty inventory. The fill still enforces its configured accessibility rules. OoT No Logic permits OoT-only advancement off logic while retaining MM progression obligations; the both-No-Logic case is recorded as fill/inventory coverage rather than logical beatability proof.

`--fill-only` skips optional hint analysis and consolidated spoiler writing. The sweep therefore does not establish hint correctness, output/save serialization, independent spoiler replay, or in-game behavior. Those require their separate gates. The fast synthetic regression above additionally checks forward replay for its focused shared-item graphs.

### Native MM entrance limitation

This sweep runs **combined mode**. The `native-entrances` and `all-parity-native-allreachable` cases exercise native **OoT** dungeon entrances. Their saved Solo MM entrance option is intentionally retained in the inputs, but the Solo MM entrance implementation is inactive in combined mode; it is not proof of a shuffled native MM layout. Cross-game entrance cases exercise the separate Combo routing system. Actual Solo MM entrance generation/replay remains covered by the dedicated `tests/solo_mm_entrances/native_seed_regression.py` workflow step.
