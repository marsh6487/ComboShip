# Independent MM Bombchu capacity

Run from the repository root:

```sh
python3 -B tests/bombchu_capacity/run_tests.py --sanitize
```

Use `--json-include /path/to/include` or `NLOHMANN_JSON_INCLUDE` when the
nlohmann headers are outside the compiler's normal include paths.

The native test compiles MM's real save types and capacity header, unchanged
inventory tables and `Inventory_ChangeAmmo`, complete native obtainability
and Bombchu shop functions, and the production Bombchu refill branches. It
checks independent capacities 20, 30 and 50 against every native Bomb Bag
tier and both infinite flags. Controls preserve ordinary MM's 0/20/30/40
fallback, including native infinite Bombchus with no Bomb inventory item.
The three full-ammo predicates are extracted unchanged from C-button,
D-pad and pause drawing; rendering itself is outside this test.

The grant test executes the production foreign/native bag cases, the
randomizer's Bombchu obtainability cases, and the shared-family getter and
raise loop. It checks persisted independent ownership, native Bomb Bag
isolation, and repeated shared-tier raises without refilling spent ammo.
The editor test executes the real `NextItemInSlot`, native deletion function,
and Bombchu cheat refill branch, including independent and ordinary cycles.

All tests run with and without `COMBO_BUILD`. Item metadata, safe editor slot
lists, persistence notifications, the unrelated Powder Keg hook, and trade
or other-item replacement are fixture boundaries. The runner fails if its
production extraction anchors disappear. These are native source behavior
tests, not a configured gameplay or full transport test. In particular,
preserving exact ammo around deferred FC receipt replay is a separate sync
responsibility.
