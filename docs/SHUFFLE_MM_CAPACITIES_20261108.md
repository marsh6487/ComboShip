# MM capacities: WIP implementation evidence

## Infinite Upgrades foundation — unfinished

The default-off boolean option supplies one each of Infinite Quiver, Infinite
Bomb Bag, Infinite Deku Sticks, Infinite Deku Nuts, Infinite Magic and Infinite
Wallet. Unique endpoints preserve finite progressive identities and avoid
cross-game scalar-family trimming removing an infinity upgrade. Plentiful does
not duplicate them; starting selections subtract their supply.

Saved ownership requires its ordinary item/bag/wallet before becoming effective.
Native ammo/currency consumption and both native/common-NEI magic consumption
honor infinity without changing raw upgrade tiers, base ownership, RPG stats or
the Chateau Romani event flag. The player pre-update restores only usable owned
infinite families. Original and Combo sanitizer fixtures cover ammo extrema,
missing bases, native continuous magic states and independent Chateau behavior.
The common NEI affordability test failed before its production predicate was
extended and passes afterward. Audio, scene/cutscene and visual callbacks are
controlled boundaries; this is not configured gameplay proof.

Commands (GCC/G++ 14, nlohmann 3.11.3 include path through CPATH):

```
python3 -B tests/shuffle_parity/capacities/run_runtime_tests.py --infinite-only --sanitize
python3 -B tests/shuffle_parity/key_rings/run_tests.py --fixture tests/shuffle_parity/capacities/infinite_pool_test.cpp --sanitize
```

Still pending: starting-category/icons, real menu activation and gold accent,
dedicated receipt/save round trips, player and custom-equipment affordability
and direct-drain adapters, wallet HUD dispatch, full linked seeds and gameplay.
The old Infinite Upgrades placeholder intentionally remains disabled.

Scope remains the four authorized Coming Soon options. Native bags and Child
Wallet are integrated in source. Infinite Upgrades, final UI reorganization,
linked seed validation and configured gameplay remain unfinished.

## Native bags

- Append-only Stick Bag/Nut Bag item, option and saved ownership flags; defaults
  remain off. Real catalogue names, major-item classification, native GI models,
  receipt icons and starting-inventory categories are present.
- Actual GeneratePools inserts one enabled bag before starting-item subtraction;
  Plentiful does not duplicate it. No new check or exclusion identity is needed
  for these items: they occupy existing shuffled checks.
- Bags.h supplies shared native receipt/oracle grant writes and removal. A bag
  grants its slot, base tier and ammunition once. Repeats do not refill spent
  ammunition. Previously received capacity tiers are retained.
- Saved bag ownership independently gates native capacity, solver HAS_ITEM,
  ordinary single/five/ten refills, player use and the player update's ammo repair.
  Raw FleetSync capacity/slot imports do not satisfy bag ownership.
- Existing foreign progressive stick/nut IDs are unchanged. Bag-enabled seeds
  resolve their first capacity to tier 2 even before obtaining the bag. Disabled
  seeds preserve the old grant behavior. The oracle uses the same helper.
- Stick/nut ammo arithmetic clamps in an int before storing s8, including extreme
  signed refill values. Native direct capacity upgrades use effective capacity.
- Ownership uses the actual native 16-bit word-indexed RandoInf representation.
  No inventory sentinel, shared scalar tier, item identifier or old option key
  has been repurposed.

## Evidence

Pool regression initially failed at the absent native Deku Stick Bag catalogue
entry. Actual native GeneratePools/Items/Options now pass all four bag selections,
both densities and four starting selections under original and Combo guards:

```
CXX=g++-14 CC=gcc-14 python3 -B tests/shuffle_parity/key_rings/run_tests.py \
  --fixture tests/shuffle_parity/capacities/native_test.cpp \
  --json-include /path/to/nlohmann-parent
```

Runtime regression initially failed because actual HAS_ITEM accepted an imported
stick slot without its bag. Native save/logic/capacity macros, contiguous native
refill branches and complete Inventory_ChangeAmmo now pass original/Combo under
ASan/UBSan, including saved flag round trips, defaults, pre-bag capacity grants,
real shared grant/remove helpers and idempotent receipts:

```
CC=gcc-14 CXX=g++-14 python3 -B tests/shuffle_parity/capacities/run_runtime_tests.py --sanitize
```

Full production translation units z_parameter.c, MM z_player.c, GiveItem.cpp,
ConvertItem.cpp, RemoveItem.cpp and BenPort.cpp pass native CMake flag syntax
checks with current headers forced as text rather than stale compiled PCH.
These are syntax checks, not fresh linked objects or gameplay proof.

## Bag integration follow-up

The bag controls now replace their disabled placeholders in a separate Bags &
Capacities card, alongside the existing wallet controls. Their real saved CVars
are registered for the gold What's New accent and use the normal disabled/global
toggle scope. Existing option keys and selections are unchanged. The remaining
Child Wallet and Infinite Upgrades placeholders stay disabled while their own
implementations are pending.

Additional original/Combo ASan/UBSan coverage passes the actual starting-kit
traversal, starting spoiler/save list adapters and RandoSaveInfo JSON adapters.
It includes consumables with each missing/starting bag combination and old option
arrays loaded into a dirty context; legacy fields persist and absent bag options
default off. Saved flags use the actual native flag functions.

Combo coverage executes the actual friendly/enum-name lookup and
GrantMmItemByName receive-guard function for both bag names. The downstream
conversion/grant services are controlled bag-only boundaries backed by the real
bag helpers; duplicate conversion is tested without refilling ammo. This is not
a linked two-DLL receipt/gameplay test. No FC alias is needed: these are MM-owned
items delivered by the existing native MM-name route; OoT's own bag identities
remain separate.

The menu passes native CMake syntax checking with current text headers. Existing
real ImGui accent/input/layout/disabled/style tests pass; CI policy tests pass
25/25. Native pool/runtime regressions are registered in the items CI suite.

Remaining: actual native cycle/gameplay and linked Glitchless seed validation,
Infinite Upgrades and the rest of the full item-pool UI reorganization.

## Child Wallet foundation (WIP)

The Child Wallet implementation is in progress; its Coming Soon control remains
disabled. Append-only native item/option/flag identities and a real wallet GI/icon
are present. Missing base-wallet ownership means zero capacity at native tier 0;
a Child Wallet flag enables 99, and higher native wallet tiers remain unchanged.
Rupee grants and the existing dormant accumulator flush honor effective capacity.
The player update clears stale imported money while no wallet is owned.

Native grants/oracle share the same Child Wallet helper. Duplicate grants do not
refill money, removal clears missing-wallet money, and progressive conversion
resolves the base wallet before Adult/Giant/Tycoon. The pool adds one progressive
source and consumes either concrete or progressive starting selections.
Combined Shared Wallets normalizes the generated/oracle option snapshot off,
preserving existing scalar wallet tiers and the UI CVar selection; solo MM keeps
its requested Child Wallet setting.

The previous CAN_AFFORD admitted every price below 100 unconditionally. It now
uses effective capacity. Twenty paid minigame/race/lottery reward checks require
wallet ownership through the real CHECK adapter; unrelated props/conversations
retain their old conditions. Bean and potion refill sources require their native
fees (bean10, red20, green40). The swamp boat retains the free rescued-Koume route
and requires a wallet for the photo/payment route. Full source/scene/cycle audits
of those interactions and integrated seeds remain pending.

Evidence: the native pool test first failed at the missing Child Wallet catalogue
entry; current native GeneratePools passes original/Combo supply, Tycoon, starts,
solo/combined and Shared Wallet combinations. The runtime first failed at the
old CAN_AFFORD rule; native currency/flush and capacity tests now pass both guards
with ASan/UBSan. The actual CHECK adapter's paid-gallery rule then failed without
a wallet and now passes with a free-conversation control. Grant/remove/idempotence
coverage is included. The commands are:

```
python3 -B tests/shuffle_parity/key_rings/run_tests.py --sanitize \
  --fixture tests/shuffle_parity/capacities/wallet_pool_test.cpp
python3 -B tests/shuffle_parity/capacities/run_runtime_tests.py --wallet-only --sanitize
```

This is not a complete Task5 claim: full starting/save/cross-world wallet and
runtime/logic validation, menu activation and Infinite Upgrades remain pending.
The prior published bag source was restored after a second local workspace
reset, with exact tree b2d297fe3cd6eb325e27e0d778557211037b9d9a verified; no new
build evidence is inferred from lost build outputs. Existing Grace/editor pool
fixtures now include the actual ring/wallet helpers; the editor grant extractor
selects the top-level switch rather than relying on its first item label. Their
native regression commands pass locally.

## Child Wallet integration follow-up

The disabled Child Wallet placeholder is now a real gold-accented option in
Bags & Capacities. Shared Wallets disables this control in combined mode while
preserving its requested CVar; solo MM remains independent. The starting-wallet
selector allows the complete effective Child/Adult/Giant/Tycoon supply.

Actual progressive obtainability/conversion arms and starting-kit traversal
pass original/Combo sanitizer tests for zero through four starting copies,
Child on/off and Tycoon on/off. Native JSON/start-list adapters preserve wallet
ownership and the shorter legacy options default off when loaded into dirty
state. Actual foreign-name lookup and receive guard accept Child's Wallet and
RI_WALLET_CHILD, with duplicate conversion to junk instead of repeating the
base-wallet grant. Conversion/grant engine boundaries remain explicitly bounded
in the fixture; this is not a linked two-DLL gameplay claim.

Additional source regressions caught two secondary progression routes: the
native Poe Sister enemy-drop CHECK in Ghost Hut and the three Deku Playground
events that expose its rupee checks. Each failed without wallet ownership before
the correction and now passes under both guards, with the original native
day/clock filters, a free-conversation control and controlled combat capability.
The Ghost Hut's EnGb2 charges 30 (10 for a replay); the new gate uses 30, preserving
the existing 99-rupee base-wallet capability. Playground charges 10. Native
Rupee/flag/clock macros execute in these tests.

Six changed wallet production TUs pass fresh real-header syntax checks; Central
and East paid regions also pass. Menu passes with the actual native GNU
-fpermissive policy already declared in mm/CMakeLists.txt. The current BenPort
check is blocked by missing libzip zip.h after workspace restoration; it is not
claimed green. The bag checkpoint's CI Linux build has progressed past native
module/launcher linking into resource generation. Full current-wallet linking,
integrated seed/cycle/gameplay checks remain pending.

Existing real ImGui accent tests pass and CI policy remains 25/25. Wallet pool,
runtime, fee and starting/save fixtures are registered in the items suite.
CI follow-up also fixes the real MM debug renderer include order (native graph
declarations precede the counted-ring renderer); its real-header check passes.
The seasons pool and foreign-scale lifecycle fixtures now supply their actual
new helper/boundary dependencies and pass their native regression commands.
