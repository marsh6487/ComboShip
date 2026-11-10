# MM Saria and Desired Items — source milestone

Baseline: cumulative PR #88 at `ced18542387d5ac8879e0f1cb303e790e4e3a1a8`.

The actual MM item catalogue and both menu candidate functions were reproduced
in a native-header ASan/UBSan fixture. Before correction it fails because the
real catalogue has no Defense Upgrade entry. The baseline already contains
Progressive Magic and Magic Stat Upgrade, but the family availability filter
does not recognize a generic progressive wish when only concrete tiers are
in the configured pool.

This milestone appends six selector-only RPG identities (defense, speed, power,
crawl, climb and push) without changing any previous item/option/flag value.
The existing Magic Stat Upgrade selector and native Progressive Magic remain.
All seven RPG names map to genuine non-trap OoT-owned foreign placements, and
the Saria selection path now accepts those verified RPG identities. Existing
native-tier ownership conversion still excludes duplicate/obsolete tiers.
Saria skips RPG stats whose mirrored seed rules mark them disabled or whose
current level meets the configured cap, and falls through to useful progression.
Unknown mirrored rules in legacy paired saves retain their previous guidance.
Desired Items keeps OoT's explicit wish for an uncollected raw upgrade copy.
The generation menu offers an RPG family only when its configured OoT pool
option is enabled, including at the title screen without a loaded save.
Generic progressive wishes recognize concrete-tier pools without offering a
specific unavailable tier. Selectors never add synthetic RPG pickups to a pool.

Verification compiles the actual full item-table initializer, actual Saria
candidate and Sensor menu functions, saved/spoiler serialization, actual Saria
priority selection, actual desired-item outstanding-check search and native
placement identity matching. It covers both original and Combo guards, all
seven RPG names, native single/double magic, traps, wrong-game foreign items,
obtained/unreachable targets, priority round trips and 665 frozen baseline
serialized IDs. The review correction additionally covers all seven families
below/at/above a cap, adjusted required levels, a real native Hookshot fallback,
disabled and legacy unknown rules, and enabled/disabled pool configuration.
Both review issues were reproduced as failing assertions before correction.
Reachability traversal and item conversion are controlled
fixture boundaries; this is not a full-world solvability or configured UI run.

Commands (official JSON 3.12.0 supplied by the environment):

```sh
CPATH=../build-deps/include python3 -B tests/shuffle_parity/hints/run_tests.py
CPATH=../build-deps/include python3 -B tests/magic_hints/run_tests.py
python3 -B -m unittest discover -s tests/ci -v
git diff --check
```

The new native fixture is registered in the items CI group. Existing hint-policy
regressions remain intact. Source compilation under these fixtures is separate
from complete game linking, full-world cross-game hint selection and in-game
appearance/acceptance. The broader shuffle integration remains WIP.

## Actual seed-location resolution follow-up (2026-11-09)

The runtime path now carries a hint target rather than treating every source as
an MM check ID. Saria preserves the ordered wishes across both worlds before
falling back to useful reachable progression. Desired Items resolves the actual
outstanding source and renders the current native upgrade (for example, the
native `RI_DOUBLE_MAGIC` / "Magic Upgrade" after the first meter). Explicit
wishes for capped raw copies retain their original item name, matching OoT.
Native skipped/excluded and cycle-obtained checks no longer win either search.

`OOT_GetMMHintCandidates(fileNum, reachableOnly)` is a declared, exported C ABI
shared through `combo/MMHintBridge.h`. It enumerates actual loaded native OoT
placements and the pushed foreign seed map, excluding obtained/saved, excluded,
skipped and trap placements. All seven native RPG upgrades and foreign MM
placements retain their actual OoT check name. Shared ordinary magic is admitted
only by genuine shared placement metadata / the seed's `sharedItems.magic`
setting; an unrelated OoT magic meter never becomes an MM wish.

For Saria, the provider uses the native check tracker's live-inventory traversal
with no-logic disabled. It clones inventory and restores the complete Logic
value and pointer, region access flags, exit pool flags, native ItemLocations
and the prior no-logic option, including on exceptions. This is a read-only hint
query, with no generation-oracle reset and no item grants. Desired Items does
not impose Saria's reachability requirement.

Cold native saves do not require the generation-only `allLocations` pool:
loaded native placements remain searchable. Saria additionally needs the native
region graph established by save-load; if the native save/graph is unavailable
or belongs to a different slot, the provider returns no unverified target.
The fixture checks an empty generation pool and an absent graph separately.
A real cold-launch/play-session acceptance run remains outstanding.

Validation adds the complete production MM `ConvertItem`, native RPG magic-tier
helpers, Desired Items rune preparation and item-name rendering to the native
fixture. It verifies upgrade identity plus exact source check/game, all seven
RPG families in both source worlds, shared/nonshared magic, native magic tiers,
filters, disabled/capped/legacy RPG rules and exception-safe traversal state
restoration. Native construction, obtainability services, region traversal and
scene/location rendering are declared fixture boundaries; this does not prove
a complete generated world's reachability or GPU/UI acceptance.

The full MM `SariasSongHint.cpp` and `Rando.cpp` translation units also pass
C++20 syntax compilation under original and Combo guards with official restored
dependencies. The full OoT `OTRGlobals.cpp` syntax check stops at the existing
missing `zip.h` dependency; its new complete provider/snapshot functions compile
and run with actual native OoT storage types in the focused fixture.
