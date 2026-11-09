# Actual native first-drop seed regression

This runner loads the **built game module**, calls its native pool and graph oracle exports, and runs the **real comborando combined fill plus playthrough replay**. No replacement graph or fake module is supplied. It is a prepared integration runner, not a locally verified native seed result: the current workspace has no built game DLLs/driver.

```sh
python3 tests/shuffle_parity/enemy_drops/native_seed_regression.py \
  --runtime-dir /absolute/build/bin \
  --driver /absolute/build/bin/comborando \
  --output /absolute/evidence/first-drops-native
```

The default twelve configurations cover all-Vanilla, all-MQ and complementary mixed layouts, each with drops Off, On, and On with every active drop check excluded. Add `--case vanilla-on` (repeatable) for a bounded run. The twelve names/counts are source-derived by `native_seed_config.py`: Vanilla 0/43/43, MQ 0/40/40, both mixed selections 0/41/41. The generator output must independently match these identities.

Each configuration runs in fresh native processes:

1. **Solo native graph**: `SOH_SetGameMode(COMBO_GAME_OOT)`, headless native initialization, real CVar restore, no tricks, selected saved layouts, real pool dump and native `ReachabilitySearch` oracle.
2. **Combined native graph**: same process with `COMBO_GAME_BOTH`. Both graph modes require each active first-drop check to be reachable with full native item metadata supplied. Removing its exact source soul must make that same check unreachable. All starting souls must be absent. Native inventory application and graph traversal supply every result; source profiles supply only expected check identity.
3. **Combined full seed**: actual CLI `--settings ... --master-seed 20261109 --count 1 --starting-game oot --shared-items none --triforce-required 0`. The saved consolidated spoiler must contain exactly the expected drop checks and requested layouts with no enabled tricks. Excluded first-drop checks must resolve to the exact native GetJunkItem default ID set, whose native item-table categories are ITEM_CATEGORY_JUNK, both in fixed output and final placements. Native exported pool category is checked where present; health upgrades and traps are rejected even when non-advancement. Trap and conditional Bombchu-junk settings are pinned Off. Recovery Heart remains allowed because the native table explicitly classifies that refill as junk.
4. **Combined replay**: actual CLI `--playthrough spoiler.json` must exit 0 (configured first pass), not the all-tricks-only exit 3.

The full-inventory graph control gives generous copies of native item names for key/token counts; it is a graph ownership negative control, **not a replacement seed fill** or an ordering proof. Full combined progression ordering is judged by the driver itself. Saved evidence includes effective settings, resolved layouts, native pool dumps, reachable check names, every own-soul negative identity, command logs, consolidated spoilers, driver/module hashes and a passing-case manifest.

The runner requires a **fresh, nonexistent --output path** and refuses existing directories or dangling symlinks without deleting evidence. Its new runtime directory receives current symlinks to the supplied real modules/assets, so a prior run cannot retain obsolete assets/modules. Every child explicitly receives SHIP_HOME equal to that isolated runtime, overriding an inherited home. It does not edit the configured runtime or existing seed outputs. Native MQ layout validation requires the real `oot-mq.o2r` and mixed layouts require original quest availability; absence is a setup failure, never a skip or placeholder asset. Vanilla-only headless fallback remains available for `--case vanilla-on` without quest archives, matching the native headless ctor. Do not silently reduce the default matrix when recording full acceptance.

## Concrete remaining limitation

The current `ComboRandoHeadless.cpp` exposes **combined fill only**. It has no solo generation flag, and the existing SoH C export surface has no standalone native `Fill`/`Playthrough_Init` entry point. Therefore this runner proves a **solo native graph**, not a standalone solo filled seed. `manifest.json` explicitly records standalone solo fill as unsupported. Complete solo-fill acceptance needs a reviewed native driver entry point or another existing real executable path; this script must not relabel combined generation as solo.

The ctypes native-loading path and actual output validation still need to execute against configured build artifacts in CI. Local checks cover Python syntax, source-schema configuration derivation, explicit missing-module failure, and native_seed_scaffolding_test.py: fresh-only output rejection, stale-asset isolation, actual CLI normalization rejection for existing/dangling output paths, complete staged native-module hashes including versioned `.so` dependencies, inherited SHIP_HOME override, exact source junk IDs/categories, and health/trap negative controls. These are not native graph/fill evidence.

```sh
python3 tests/shuffle_parity/enemy_drops/native_seed_scaffolding_test.py
```

The current production fixed[] and items[] exports omit native RG IDs/categories. The runner resolves their exact names through the production item table and GetJunkItem initializer, checks native items[] advancement/trap metadata, and additionally checks pool[].category wherever that item occurs. It does not invent an ID/category field in the exported schema.
