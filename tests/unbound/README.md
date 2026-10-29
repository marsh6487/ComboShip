# Unbound native regression checks

Run `python3 tests/unbound/run_native_tests.py -v` for format, width, registry, archive-order,
legacy routing, ownership and checked-growth boundaries. The four count/allocation fixtures use
UBSan and refuse large allocations through their fixture allocator.

Run `python3 tests/unbound/run_savestate_tests.py` for ASan/UBSan checks of the actual save/load
methods, collision allocation owner, snapshots and immutable SceneDB save dispatch. It covers
growth and repeated rewinds, multiple slots and contexts, original actor-held polygon addresses,
queue rebinding, allocation failure before world mutation, room/scene epoch rejection, collisionless
setup/query/teardown and delayed worker serialization. Audio and overlay services are fixture
boundaries; native rendering and configured game behavior are not exercised.

The tests require C/C++ compilers, sanitizer runtimes and nlohmann JSON headers. LSan cannot inspect
process tasks in the managed runner; the savestate fixture instead accounts for every external
native malloc/realloc/free and requires zero remaining allocations at teardown.
