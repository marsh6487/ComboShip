# MM capacities: WIP implementation evidence

Scope remains the four authorized Coming Soon options. This checkpoint is the
first native bag vertical; it does not claim Child Wallet, Infinite Upgrades,
final UI reorganization, linked seed validation or configured gameplay.

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
Child Wallet, Infinite Upgrades and the rest of the full item-pool UI reorganization.
