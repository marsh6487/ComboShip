# Solo MM entrance shuffle candidate

Implementation, focused regression gates, Linux native linking and the complete
14-profile repeated native generation/replay matrix have passed. Cor explicitly
authorized publication to the existing draft PR, but automatic approval review
still blocks the source upload. Configured gameplay acceptance and a current
Windows build remain separate checks.

## Scope and baseline

This implements cor's Solo Majora's Mask entrance-shuffle request and the
follow-up screenshots: dungeon, boss, overworld, interior and grotto entrances,
decoupled returns, and selectively mixed entrance pools. The native menu is
**Rando > Entrances** in Solo MM. All new settings default off and are saved
with the generated seed.

The implementation starts at recovered R22
`191794e8c7534c30a55ee4ab4554e49356c6ca75`, on isolated branch
`candidate/solo-mm-entrances-20261109`. The existing draft PR #89 was still at
published R21 `0063920e18145299afc1682aab98407ebd366fc5` when inspected for this
work. Publication must carry forward R22 as well as this feature. Concurrent
Rito mask and Clock glass candidates remain separate. Master promotion is not
part of this task.

## Implemented behavior

| Control | Native MM scope |
| --- | --- |
| Dungeon Entrances | Five temple entrances, counting normal and inverted Stone Tower separately |
| Expanded Dungeon Pool | Seven additional entrances: Spider Houses, Ikana Castle, Secret Shrine, both Well entrances and Pirates' Fortress |
| Boss Entrances | The four temple bosses |
| Interiors, Shops, Door Interiors | Existing classified native catalogue; 42 unique entrances when the full interior and shop pools are enabled |
| Grottos | 26 logical holes with native content, return and void metadata |
| Overworld Entrances | 40 paired connections, independently matched to all 80 native graph directions |
| Decouple Entrances | Independently permuted forward and return destinations |
| Mixed Entrance Pools | One shared pool for the enabled categories selected under Mix Dungeons, Bosses, Interiors, Shops, Grottos and Overworld |

All categories together contain 124 canonical portal identities. Unselected
mixing categories retain their own pools. Door Interiors is a subset of
Interiors; enabling both does not duplicate a portal. All endpoints remain in
MM. Native song/owl travel, authored private-scene links, and scripted scene
reloads retain their existing behavior.

The native generator installs the entrance map before item placement and uses
MM's existing region predicates, time slices, item requirements and physical
door ownership. Structural map selection and final all-check replay use the
existing ten-second attempt deadline and ten-attempt limit. No all-items or
goal-only fallback is added. Nearly No Logic and No Logic retain their native
semantics; Vanilla clears the entrance layout.

In separate boss shuffle, regional completion follows the source boss door.
When bosses actually mix with another populated category, all four regional
completion effects follow the physical boss. Blue-warps and death recovery
still follow the saved routing policy. This avoids duplicate or missing clears
when only some links in a mixed permutation happen to connect two boss doors.

The native save contains strict schema-2 forward/reverse permutations,
settings/seed identity, the active recovery checkpoint and completed-boss mask.
Selected-save installation occurs after the normal save-load hooks. File
preview parsing cannot install another slot's layout. Legacy saves clear the
new state; foreign, duplicate, malformed or mismatched maps are rejected.

Saved maps also drive map and compass information. Existing positioned-warp
features preserve their captured shuffled-entrance ownership. This includes
Farore's Wind and the candidate's already-existing optional OoT Spiritual Stone
items in MM; these optional items are not prerequisites for entrance shuffle.

## Regression evidence

The following complete gates passed with exit code 0 using the actual source
boundaries, native GBI ABI definitions and pinned real dependency headers.

| Gate | Observed result |
| --- | --- |
| `tests/solo_mm_entrances/run_tests.py` | Nine suites: explicit runtime, layout, native overworld catalogue, logic, codec, original/Combo save JSON, native runtime/lifecycle, actual Spiritual Stone capture/recall, and production retry RNG |
| `tests/cross_dungeon/run_tests.py` | All 19 C++ probes and 29 Python suites, including affordability (48 total) |
| `tests/solo_games/run_tests.py` | All three compiled mode/options probes and 20 production-boundary suites (23 total) |
| `scripts/diagnostics/run_mm_item_receipt_tests.py` | Complete receipt, queue/latch/grant, settings/save, donor export, catalogue, map/compass, altar and loaded-tracker gate |

The new runtime suite covers all four bosses' first and repeat blue warps,
mixed and decoupled boss recovery, native grotto identity and respawn metadata,
ordinary seasonal aliases, scripted reload protection, cycle/moon-crash/teardown
state, selected-file ownership, Farore recall, and positioned grotto recall.
The original combined entrypoints retain their combined-session guards.

Concrete failures were observed before fixes: absent native entrance settings;
Solo runtime ignoring a saved route; native JSON dropping the layout; a grotto
return being reinterpreted as an overworld edge; inherited grotto respawn mode
on a mixed ordinary destination; and a positioned warp using a subsequently
visited boss's death checkpoint. Failed logs are retained separately from the
successful runs.

Legacy fixture repairs provide new real headers or newly introduced dormant
Solo interfaces. They retain the original assertions. The native Solo-flow
fixture links the actual `SoloEntrances.cpp` implementation, and combined-mode
fixtures assert that Solo clear handling is not called.

The final nine-suite gate also passed directly with GCC 13 and its explicit
native ABI flags, with ambient C/C++ include variables cleared. The aggregate
supplies its declared dependency directory. This closes the new registered CI
row without relying on the legacy fixture compiler wrapper.

## Independent review

A separate read-only reviewer examined generation, strict serialization,
runtime source guards, mixed boss completion, grotto materialization, lifecycle
and positioned warp behavior. The review ended with no unresolved material
source finding after its native reproductions passed. In particular, the
reviewer's actual Goht-to-Gyorg-to-Goht Spiritual Stone recall restored the
captured Goht checkpoint and the corresponding mapped death return.

This source review does not establish in-game camera, collision, actor loading
or player acceptance. Those require configured gameplay with the intended
assets and settings.

## Linked native verification

The fresh GCC 13/Ninja Release build passed: `libultraship`, `soh`, `2ship`,
`comboui`, and `comborando` compiled and linked. The initial pass compiled a
precompiled header before the new warp-save fields were finalized and failed
when a later source used those fields. Refreshing that header closed the
failure, and the complete build exited 0. The subsequent native retry fix also
compiled and linked successfully.

`tests/solo_mm_entrances/native_seed_regression.py` runs the actual linked MM
generator in a fresh native session per profile, installs the exported layout
through the real oracle setter, and independently replays native reachability.
The 14 profiles are Off, each of eight individual categories, all
separate, decoupled, selected dungeon/boss/grotto mixing, all-category mixing,
and all-category mixed/decoupled. It checks every shuffled check, settings,
canonical endpoint identity, separate/mixed pool ownership, both bijections,
malformed input rejection, and isolation from poisoned combined access roots.
Deterministic repeats compare the saved map, placements and prices. Expected
check coverage comes independently from the native exported pool and hintable
fixed checks. The stale-state probe first installs an actual nine-portal
combined temple/boss map, then switches back to Solo with poisoned access data.

The final complete matrix exited 0 with seed `20261110` and `--repeat`: 28
actual fills, 14 independent normal replays and 14 stale-state replays. All
profiles visited all 284 shuffled checks in this configuration. Every enabled
profile exercised moved entrances and rejected malformed forward permutations,
return permutations and foreign endpoints.

| Profile | Portals | Moved forward routes | Total reachable checks | Repeat/replay |
| --- | ---: | ---: | ---: | --- |
| Off | 0 | 0 | 2823 | PASS |
| Temples | 5 | 5 | 2823 | PASS |
| Expanded dungeons | 12 | 12 | 2823 | PASS |
| Bosses | 4 | 2 | 2823 | PASS |
| Interiors | 35 | 35 | 2823 | PASS |
| Shops | 7 | 5 | 2823 | PASS |
| Door interiors | 24 | 24 | 2823 | PASS |
| Grottos | 26 | 24 | 2823 | PASS |
| Overworld | 40 | 39 | 2804 | PASS |
| All, separate pools | 124 | 119 | 2823 | PASS |
| All, decoupled | 124 | 115 | 2823 | PASS |
| Selected dungeon/boss/grotto mixing | 124 | 121 | 2804 | PASS |
| All categories mixed | 124 | 122 | 2823 | PASS |
| All categories mixed and decoupled | 124 | 122 | 2823 | PASS |

The difference in total reachable checks does not represent missing shuffled
checks: all 284 independently enumerated shuffled checks were collected in
every case. Other optional item shuffles retain their default settings; native
small and boss keys are configured Own Dungeon. This is evidence for these
profiles and seed, not an exhaustive claim over every seed and option combination.

Exact final modules:

| Module | SHA-256 |
| --- | --- |
| `lib2ship.so` | `f9c1a6a6585020d9d484d6fb1c5e94995d556e30db218b8339db9291cf27fb6d` |
| `libsoh.so` | `2a59f525383a0baec2f332e1bea587163f8df1f98982abf5926c88d11bbe9241` |
| `libultraship.so` | `23c8818237699df16d90fd37d088d62a5eda2f72a342391d008bed41c9fb4015` |
| `comboui.so` | `6b3b0359e06b949e19a2efbbd3f6fbec5aaa9aead4fc51bccbe53a0062d7596c` |

The last behavior change and compiled source are
`38a95028df17645057ad4c1b11800176801d1826`. Subsequent controller changes register
the retry test, choose the exercised control seed, and record evidence. Final
publication preflight also applied pinned clang-format 14.0.6 to 14 production
files. Independent review verified that every changed file is exactly the
formatter's output from the compiled source, all logical preprocessor directives
match, and the C++ tokens match after normalizing adjacent string literals.
Include sorting is disabled. All 69 changed source paths covered by the CI
formatting gate are now byte-idempotent under that formatter. The native module
hashes and matrix above identify the pre-format compiled code; the final source
is its verified formatter-derived equivalent.

After formatting, the nine-suite entrance gate passed again with the explicit
GCC 13/native ABI environment. The complete cross-entrance gate (48 probes and
suites), Solo-game gate (23 probes and suites), and item-receipt gate also exited
0. Separate post-format logs and the reproducible read-only formatter/token
verification are retained with the source package.

### Native retry failure and correction

The first matrix used seed `20261109`; its boss profile generated an identity
permutation and was correctly rejected as insufficient shuffle coverage.
Seed `20261110` exercised moved bosses and passed that profile, but its Off
repeat exposed an existing native retry bug. Both fills timed out on attempt
one and succeeded on attempt two, yet the placements differed. An independent
native reproduction observed 7010 versus 7046 search probes in the failed
attempts. Their differing RNG consumption carried into the successful retry.

The nine-line correction derives each retry's native RNG stream from the
native seed and attempt number. It preserves the first-attempt stream, fresh
per-attempt item/check pools, ten-attempt limit and ten-second deadline.
The production-body regression failed against the old code and passes against
the correction using the real native PRNG. Independent review found no blocking
issue. The exact native Off/20261110 repeat then passed: both attempt-one
timeouts were retained, and both attempt-two results matched.

This isolates a given retry from failed work. Retained wall-clock deadlines can
still cause different hardware or load to select a different successful attempt;
unconditional cross-machine determinism across different winning attempts is
not established by this fix.

### Reproduction

After building the native modules and configuring their ordinary shared-library
search path:

```sh
python3 -B tests/solo_mm_entrances/run_tests.py
python3 -B tests/solo_mm_entrances/native_seed_regression.py \
  --runtime-dir /absolute/path/to/native-modules \
  --output /absolute/path/to/a-new-evidence-directory --repeat
```

The native directory must contain current `libsoh.so`, `lib2ship.so`,
`libultraship.so` and `comboui.so` (or the platform equivalents). The SoH module
initializes the shared headless Context; generation and graph replay in this
driver use the MM module. No ROM or replacement graph is used by this gate.
The outer 300-second process timeout allows two native fill budgets plus replay
overhead; it does not increase either native generation limit.

## Recovery and evidence limits

Recoverable source checkpoints include a binary patch from the named published
base, a Git bundle, exact commit/tree/hash identities and restoration commands.
Each patch is independently applied to the base in a temporary Git index and
its restored tree must exactly match the snapshot. Archives omit ROMs, private
runtime inputs, generated build products and animated previews. Clean snapshot
commits are directly parented to published R21: the older local history contains
a deleted preview GIF, so bundling that intermediate ancestry would violate the
requested archive policy despite the clean final tree. Both earlier saved
checkpoints were corrected without changing their restored source trees.

The final source package records the exact local candidate commit, clean
snapshot commit, source/restored tree, hashes, native evidence and failed
controls. The publication audit confirms all 35 changed binary paths relative
to R21 are byte-identical to accepted R22; Solo adds only source/tests/docs.

The publication target is the public `marsh6487/ComboShip` repository, existing
branch `candidate/ootmm-shuffle-parity-20261108`, and open draft PR #89. Read-only
verification confirmed the repository destination and that its R21 tree matches
the local R21 equivalent. Cor replied, "yes you may, I authorize it," to the
request to publish this candidate. Automatic approval review nevertheless
rejected the resumed source-tree upload, stating that the authorization must
explicitly name both the source payload and the public destination. The rejection
was respected; no alternate upload route was attempted. No GitHub branch ref,
PR body, merge or master was updated by this task. Publication remains blocked
on that more specific authorization, rather than a missing implementation.

Configured in-game traversal, camera/collision/actor-loading behavior, user
acceptance, and a current Windows compile have not been established by these
Linux builds, native generation/replay runs and production-boundary fixtures.
