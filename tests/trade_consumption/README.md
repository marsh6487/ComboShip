# Trade consumption regressions

Run the focused suite with a C/C++ compiler and nlohmann JSON headers:

    NLOHMANN_JSON_INCLUDE=/path/to/include python3 -B tests/trade_consumption/run_tests.py
    ASAN_OPTIONS=detect_leaks=0 NLOHMANN_JSON_INCLUDE=/path/to/include python3 -B tests/trade_consumption/run_tests.py --sanitize

The default JSON include directory is /usr/include. CC and CXX select compilers.
The suite is registered in scripts/ci/regressions.json. All generated sources
and executables stay in temporary directories.

The runners exercise production code rather than copied algorithms:

- core_run_tests.py compiles both ports' trade helpers against native save and
  item headers, in standalone and Combo modes.
- oot_run_tests.py extracts native accepted NPC transitions and randomizer
  consumption/acquisition branches. Trade APIs and engine boundaries are
  controlled here; the core runner separately exercises their implementation.
- mm_run_tests.py links the production trade core with native grant/deletion,
  script/toilet, scrub and cycle functions. mm_README.md details boundaries.
- sync_run_tests.py compiles the shared production wire policy against both
  real FC item tables and checks both FleetSync call sites.
- persistence_run_tests.py compiles the complete production OoT NeiSave_Save /
  NeiSave_Load and MM NeiSaveData JSON conversion functions against real native
  save layouts. SaveManager's JSON boundary and unrelated services are controlled.

The suite covers all 23 wheel identities; exact inventory/button removal;
permanent Pendant and Cojiro perks; canceled/unaccepted trades; randomized
reward ordering; real reacquisition; randomizer cycle reuse; old migrations;
legacy save defaults; stale/partial shared state; and historical FC replay.

The existing tests/unified_trade_slots suite separately verifies native pause
input, age switching, hatches, page mapping, and normalization before C equip.

The source candidate's behavior, provenance, limitations and gameplay matrix
are recorded in docs/poc/trade-consumption.md. These CPU fixtures do not replace
a full engine build or configured gameplay acceptance.
