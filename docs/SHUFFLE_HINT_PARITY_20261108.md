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
