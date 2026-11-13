# MM semantic trade handoffs

Run from the repository root:

```sh
python3 tests/trade_consumption/mm_run_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 tests/trade_consumption/mm_run_tests.py --sanitize
```

The runner builds separate C and C++ translation units in a temporary directory,
then runs standalone and `COMBO_BUILD` variants. Each variant currently executes
124 behavioral assertions. AddressSanitizer and UndefinedBehaviorSanitizer are
enabled by `--sanitize`; leak detection is disabled in the example because the
managed test environment uses ptrace, which LeakSanitizer does not support.

To reproduce the missing semantic hooks while retaining the current trade-state
implementation:

```sh
python3 tests/trade_consumption/mm_run_tests.py --source-ref 28d4fe4eed1e44150223a8247be41c4c21b6cfca
```

That baseline fails 44 of 124 assertions in each build mode. It is expected to
exit with status 1. The core's independent historical failure probe is in
`core_run_tests.py`.

## Executed production code

The runner extracts unchanged production enums, inventory slot tables, C/D-pad
macros, trade-state implementation, and these semantic paths:

| Boundary | Executed source |
| --- | --- |
| Actual grant and deletion | `Item_Give`, `ItemGrantAudit_ItemGive`, the native trade/mask arm of `Item_GiveImpl`, `Inventory_DeleteItem`, `Inventory_ReplaceItem` |
| Moon's Tear | `func_80ADBAB8`, `func_80ADBBEC`, the randomizer's Sellnuts update callback |
| Four deeds | `func_80BEF20C`, `func_80BEF360`, `func_80BECFBC`, the randomizer's Akindonuts update callback |
| Script and toilet hand | `MsgEvent_DeleteItem`, `EnBjt_TakeItem`, `MiscVanillaBehaviorHandler`, the Keep Express Mail callback |
| Cycle boundary | Native SRAM trade-slot cleanup, randomizer `AfterEndOfCycleSave` through trade restoration |
| Legacy Pendant migration | The old-boots migration condition and body from `ExtEquip_Init`, with native ownership-bit lookup |

The assertions distinguish waiting or uncommitted dialogue from the accepted
transition, ordinary scrub purchases from deed handoffs, exact-item deletion
from shared-slot replacement, and a shuffled reward from the submitted item.
They check real reacquisition, vetoed grants, historical registry projections,
all seven active C/D-pad bindings, repeat deletion, permanent Pendant ownership,
Express Mail retention and recipient progression flags. Cycle tests distinguish
native dummy-ID clearing from explicit randomizer restoration, including
unacquired entries and consumed OoT entries carried in MM's wheel.

Followup regression cases verify that positive and negative out-of-range deletion
IDs cannot alias a native trade byte. They also cover old Pendant ownership and
equipped-only migrations with revision zero or one, preserving the permanent
perk, effect toggle, layout version and existing boots cleanup. Current Climb
Boots are not mistaken for legacy Pendant history. These followup cases failed
10 assertions in each mode before their fixes.

## Scope of the evidence

This is an extracted-production source-function fixture with a minimal field
shape for the save and actor structures. It is not a full native engine build,
binary ABI check, or in-game acceptance test. The separate core fixture compiles
the trade helpers against native save headers. Animation, text input, item-offer
scheduling, hook dispatch, and the equipment removal called by the migration are
controlled external boundaries here.

The whole `Item_GiveImpl` is not compiled: only the production trade/mask arm is
needed for these cases. The randomizer cycle fixture stops before unrelated
check-flag clearing, and it directly invokes the registered callback. Its
`IS_RANDO` registration remains in production `MiscBehavior.cpp`. A Bunny Hood
grant stands for an unrelated queued shuffled reward; reward queue scheduling
is outside this fixture.

Runtime acceptance still needs actual native/NEI and randomizer configurations:
hand in each supported item, reopen Kaleido, try its former C/D-pad binding,
save/reload or switch games, and verify the configured cycle/reacquisition
behavior. Keep Express Mail and the permanent Pendant combat perk should be
included in that configured run.
