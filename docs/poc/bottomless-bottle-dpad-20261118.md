# Bottomless Bottle D-pad charge parity

Both native bottle-use paths already reached the Bottomless counter driver.
The missing step was projection: `BottleItems_SyncSlot` only restored C-button
bindings after a use, leaving a D-pad binding as an empty single-use bottle
while the inventory and remaining counter still contained the original fill.

SoH now refreshes all seven non-B button bindings. MM refreshes its separate
four-entry D-pad equipment row and reloads the correct D-pad icon. Both paths
match the inventory slot, not the content ID, so an ordinary bottle with the
same potion does not acquire Bottomless charges. Projection and re-equipping
never mutate the counter. The native counter driver still spends one use.

HUD charge lookup now accepts D-pad bindings as well as C buttons. It rejects
B/invalid indices and unowned/other bottle slots. Bottle counters are selected
before ordinary ammo-array lookup, avoiding an incidental bottle-item index
lookup in SoH. Depletion shows the native empty bottle and zero remaining uses.
No fill limits, Chateau buff behavior, ownership or save format changes.

`CPATH=/path/to/deps/include python tests/bottomless_dpad/run_tests.py` executes
the actual native inventory-update functions, production counter hooks,
counter math and slot/HUD projection against both games' real native layouts.
ASan/UBSan pass for all C/D-pad directions, potion 5-to-0 and Chateau 3-to-0,
duplicate-content ordinary bottles, repeated projection, re-equip/no-reset,
unowned state and invalid button indices. Leak detection is disabled for this
environment. Both changed native source units in each host pass full real-header
syntax checks. Configured gameplay/UI remains untested.
