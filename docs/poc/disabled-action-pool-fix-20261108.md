# Disabled OoT action pool correction

The MM `Add OoT Items` category no longer adds its own `Open Chests` ability to
ComboShip's pool. OoT's settings-scoped native pool owns that ability, including
its normal and progressive counts. Enabled abilities can still be placed in
either game by the combined fill. The remaining gear and the Add-all button keep
their existing behavior. No item IDs, name mappings, grant paths or save formats
change, so existing seeds are not rewritten.

## Confirmed trigger

`Add all NEI content` calls `NeiRando_SetAll(true)`, which turns on MM's
`RO_SHUFFLE_OOT_GEAR`. In ComboShip, `FleetCombo_UnifiedPoolActive()` is false and
`Combo_IsCombinedGame()` makes `hasOotGameplay` true. The separate gear branch
therefore added one `RI_OOT_ABILITY_CHESTS` even when OoT's
`RSK_SHUFFLE_OPEN_CHEST` was off. The native OoT pool correctly excluded it, but
the MM dump exported its extra copy and combined placement retained it. Sharing
cannot remove that lone copy when the OoT pool supplies none.

The fix removes that extra insertion. Pure solo MM already excluded it through
the old `hasOotGameplay` gate. The legacy separate-process unified-pool branch is
unchanged; this correction targets the ComboShip launcher build. Climb was not
reproduced as a separate leak and its native setting gate is unchanged.

## Regression proof

The regression failed against the unchanged source before implementation. Its
720 seeded fill cases produced 320 chest-count contract failures. The test uses:

- Actual native OoT/MM enums and the native `AddItemToPool` implementation,
  including the separate plentiful reservoir.
- Unmodified MM Add-all menu functions and their selected native option rows,
  followed by the real MM dump's settings-copy loop.
- The exact relevant MM/OoT pool branches and the ComboShip unified-pool flag.
- Actual MM item catalog names, native name-collision mapping, category logic and
  the MM dump's free-pool emission loop.
- The complete production combined assumed-fill implementation, including its
  shared-pair resolution and foreign placements.

The matrix covers default options, manual gear enable and Add-all; chest shuffle
off/on/progressive; Climb off/on; sharing absent/present; and twenty seeds for
each combination. Twenty-four native pool cases separately exercise all four
abundance modes and enabled/disabled action counts. Gear-chain counts and a
pure-solo-MM control are checked too.

The fixed-source results and extraction hashes are included in checkpoint
evidence. The strengthened regression must pass with zero chest-count failures;
reinstating the removed insertion makes it fail. This protects both exclusion
when disabled and continued native supply when enabled.

## Running and limits

From the full matching source tree, with nlohmann/json headers installed:

```sh
python3 tests/action_pool/run_tests.py --output /tmp/action-pool-results
```

The runner compiles with GCC C++20 and ASan/UBSan. It accepts
`--observe-leak` only for recording unfixed behavior; the ordinary regression
command rejects every count mismatch. Checkpoint evidence includes a bounded
source fixture and the JSON dependency so these tests can be recovered without
the transient checkout.

Checks, reachability oracles, CVar storage, UI persistence and the unused fields
of native structs are fixture boundaries. Twenty synthetic checks per game and
all-reachable oracles isolate pool membership and placement. OoT action names in
the test dump are explicit fixture strings; MM names/emission are production
code. This is not a complete native seed-generation replay or gameplay run.
Sample JSON outputs are test fixtures and must not be loaded as playable seeds.
The complete matching game libraries and full repository build setup are absent
from this bounded checkout; native compilation/linking and runtime remain
pending. Other POC5 source and asset limitations remain in force.

## Recovery

Apply cumulative POC6 with its checksum-gated installer to the matching source.
POC5 inputs and its optional Marin archive remain unchanged; POC6 retains all
earlier voice, blink, winter, UI, shovel, pedestal, masks and hint changes.
Only pool generation, its tooltip/comment, this note and the regression are new.
Use the checkpoint's original affected baseline plus `source.patch`, or upgrade
the POC5 overlay using the installer. No publication or master promotion is
included.
