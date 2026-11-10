# R23 cumulative Solo MM, Rito, Clock and spring integration

## Status and scope

The existing R22 candidate now includes the three requested donor checkouts:
Solo MM entrance shuffle, optional Rito Mask integration, and the authored Clock
GI with the revised spring blossom textures and intrinsic scrolling glass.
Publication remains on cor's hold. This work makes no remote branch update, CI
run, merge, accepted-master promotion, or configured gameplay acceptance claim.

The production source checkpoint is
`04943a8cbb873b171dd6e04900fde9409e6d4cdf`, tree
`800525fe3dabf29c7c677a6b1e14504fc39088fa`. Later bounded changes make the Rito
identity test independent of unpublished Git history and record this report;
they do not change production source. The final recovery package's manifest is
the authoritative final commit/tree record.

## Exact inputs

| Input | Preserved donor commit | Integration |
| --- | --- | --- |
| R22 without optional key animation | `191794e8c7534c30a55ee4ab4554e49356c6ca75` | Preserved ancestor, including shared Magic Mirror, authored keys, soul miniatures and their prior fixes |
| Solo MM entrance shuffle | `df89a2e347a3aa85d5476ae38c33e9bba256d7e0` | All 24 commits after R22 retained as the integration branch base |
| Clock, spring blossoms and glass | `46f0a52811c76c8a352e89e0149cf2924b5c0983` | All 14 additive commits after its older common base `e22bcb22` applied; no full-tree overlay |
| Rito Mask | `cf457d7096f3de1997b8f7aeb36df6726cde8949` | All six commits after R22 applied |

The isolated branch is `integration/r23-solo-mm-rito-clock-20261109`. The original
donor branches and the R22 recovery branch remain intact.

### Resulting behavior

- Solo MM retains native entrance pools, mixed pools, decoupling, saved entrance
  layouts, seed/replay validation and lifecycle routing. Its guards isolate it
  from Combo entrances. Native boss recovery, grotto identity, Farore's Wind and
  the project's Spiritual Stone checkpoints remain in the donor implementation.
- Rito is a default-off optional reward in the existing mask controls. It has
  shared Combo preferences, independent solo preferences, canonical ownership,
  save/editor integration, existing inventory-wheel placement, native MM form
  support, authored ivory/gold GI, and gold shimmer. The solver does not require
  it, Plentiful does not duplicate it, and Combo keeps one shared reward.
- The authored OoT Clock renders through native and foreign routes in both
  hosts. Intrinsic scrolling glass is independent of optional item effects;
  optional gold shimmer retains its control. The six MM half-day clocks retain
  their existing routes. The 14 accepted Clock body resources remain exact;
  the full authored object has 19 resources per host.
- Spring uses the authored transparent blossom and petal surfaces, cupping and
  soft detail while retaining motion, particle counts and fade behavior. Both
  hosts retain matching packaged textures.

## Reconciliations

The independently developed MM options both started appending at the R22 tail.
The cumulative enum preserves Solo's 16 options at **100–115**, then appends
`RO_SHUFFLE_RITO_MASK` at **116** (`RO_MAX` is 117). Every earlier identity remains
in place. The independent Rito donor's historical option `100` is not the
cumulative serialized value. The compatibility fixture pins the complete
accepted prefix, the explicit Solo suffix and Rito's final ordinal.

MM `Menu.cpp` retains all 119 prior refresh keys and adds the three Rito keys:
`gRando.Generation.ShuffleRitoMask`, `gCombo.Rando.ShuffleRitoMask`, and
`gRando.Options.RO_SHUFFLE_RITO_MASK`. Shared Mirror and both simpler soul model
controls remain. OoT's draw file retains both the Clock and Rito presentation
headers. The CI registry retains all 328 Solo-base rows unchanged and adds three
rows, for 331 unique rows.

The Clock compositor fixture now includes the actual
`ComboNeiGiResourceOwner.h`, matching the production compositor's dependency.
The Rito identity fixture was first corrected for the cumulative option suffix,
then made independent of `git show 191794e8` so a new CI or recovery checkout does
not require an unpublished donor commit. These are fixture repairs, not
substitute production helpers or weakened identity checks.

The optional key GIF stays absent. Preview ignore rules are combined, and
source archive attributes exclude GIF, APNG and video formats. Recovery-package
validation also checks PNG/WebP animation chunks and nested ZIP contents.
Required native player, skeleton and material animations remain included.

## Fresh verification of the integrated source

| Check | Result and scope |
| --- | --- |
| Solo MM nine-suite runner | PASS: runtime policy, layout, overworld, native walker, codec, actual original/Combo JSON serialization, lifecycle, Spiritual Stone and retry RNG isolation |
| Impacted native Solo flow | PASS: create-original, create-combo, reset-combo and owl-combo with ASan and UBSan |
| Rito randomizer runner | PASS: identities, grants, native/foreign descriptors, complete MM pool generation, solo/shared preferences, widgets, snapshots and actual Combo deduplication |
| Rito identity test portability | PASS with normal history and an isolated empty Git object store; the expected missing-history failure was recorded before the repair |
| Rito MM save conversion | PASS: actual native JSON round-trip, old-save defaults and invalid selection handling |
| Registered authored Clock row | PASS: reproducible resources, material/native command validation, callbacks, both complete foreign compositors, owner/Alt/effects routing, glass phases and wrap, arena/state cleanup, six MM half-days and original/Combo header configurations |
| Registered spring row | PASS: exact regeneration, both actual texture factories, host equivalence, dimensions/scales/alpha/lifetime, production combiner and fade |
| Six complete production translation units | PASS: real native object compilation, including the merged files listed below |
| Donor retention review | Solo 62/70 paths exact, with eight additive integrations. Clock 81/90 retained paths exact; eight reviewed unions and one fixture-header include account for the differences, and its deleted GIF stays absent. Rito 335/349 paths exact; 13 shared integrations and its identity fixture account for the differences |
| Whitespace and CI-row preservation | PASS; no duplicate CI rows and no existing row removed or altered |

The six complete native translation units are:

- `mm/2s2h/Rando/Menu.cpp`
- `mm/2s2h/BenPort.cpp`
- `mm/2s2h/Rando/ItemReceiptText.cpp`
- `mm/2s2h/Rando/StaticData/Options.cpp`
- `mm/2s2h/Rando/DrawItem.cpp`
- `soh/soh/Enhancements/randomizer/draw.cpp`

They compiled to objects with retained GCC13/sysroot and the real native CMake
flags, including the native GBI ABI definitions. All repository source/include
paths pointed to the integrated checkout. MM's forced PCH header was copied as
ordinary header text with its paths remapped; no stale donor `.gch` was used.
The exact source checkpoint and all six source hashes were unchanged throughout
compilation. This verifies complete units and native types, not whole-module
linkage.

The first sanitized Solo-flow attempt failed because LeakSanitizer rejects the
managed host's tracing restriction. Its original log is retained. The rerun
used `ASAN_OPTIONS=detect_leaks=0`; AddressSanitizer and UBSan remained enabled.
Leak checking is not claimed.

Evidence in the recovery package is organized under `evidence/solo/`,
`evidence/rito/`, `evidence/clock-spring/`, `evidence/native-tu/`, and the final
review folders. It records exact commands, source hashes, exit codes and logs.
Expected fixture failures are retained beside the passing repair evidence.
No claim is made that all 331 registered rows ran in this integration.

### Inherited donor evidence

The Solo donor's saved native matrix was separately checked: 14 profiles,
deterministic repeats, 28 native fills, 14 normal replays and 14 stale-state
replays; all reached 284 shuffled checks. The donor's four module hashes match
its manifest. These were not new executions of the final cumulative binary.

The Rito donor retains its 14-command final suite, six OoT and six MM native
object compilations, and native-resource/package checks. Resource conversion
used the actual available OoT/Torch tooling against MM inputs, not a complete
MM ZAPD package build. Unchanged form/controller suites were not rerun merely
because this integration appended options elsewhere.

## Recovery and publication continuation

A pre-build recovery ZIP was saved before the native unit compilation. It
contains 642 changed paths from the last recorded public commit
`0063920e18145299afc1682aab98407ebd366fc5`, plus source snapshots, binary patch,
manifest and evidence. A separate Git index applied that patch and reproduced
tree `800525fe3dabf29c7c677a6b1e14504fc39088fa`. Its SHA-256 is
`bbe13842ab45f0a748f239d68bd0937743cab7d123931ccaf683201e9d9a4f69`.
The final source ZIP adds the portable identity fixture and this report, and
records its own exact restoration hash. No historical Git bundle is included.

All 49 previously uploaded key-asset paths still match their original hashes;
the 45 unique blob receipts remain reusable. At the compiled source checkpoint,
the cumulative binary plan contains 83 paths and 33 additional unique binary
blobs. No new blob was uploaded during this held integration. The optional
animated key preview must stay out of every subsequent upload plan.

The last observed remote state is draft PR #89 at `0063920e`. The prior R22 source
upload was rejected by automatic approval review; this local integration does
not bypass that separate block. Before any later authorized publication,
re-read the remote parent, use the final cumulative tree, reuse verified asset
receipts, verify the published tree/commit, and advance only the candidate
branch with expected-parent protection. Keep the PR draft and do not merge or
promote an accepted master.

## Remaining proof and build requirements

Build both game modules and the launcher together from this same source;
Solo entrance/save structures and the prior soul drawing ABI require matching
modules. Repack the authored Clock, spring and Rito native resources with their
corresponding host assets. A donor's existing executable is not the complete
cumulative R23 application.

The exact final full-module link, Windows build/CI, configured in-game entrance
behavior, Rito inventory/form behavior and visual acceptance of Clock glass and
spring blossoms remain to be demonstrated. Source checks, object compilation
and inherited donor evidence do not replace those configuration-specific
runtime checks or cor's acceptance.
