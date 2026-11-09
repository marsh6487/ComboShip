# Linked native Boss Keys Own Dungeon and integrated parity seeds

Run after the real modules, comborando and port resources have built:

```sh
python3 -B tests/shuffle_parity/boss_keys/native_boss_key_regression.py \
  --runtime-dir /absolute/built/runtime --driver /absolute/built/runtime/comborando \
  --output /absolute/new/evidence-directory
```

The default required Linux CI matrix runs eight cases: own dungeon, native boss-chest exclusions, forced Stone Tower normal and inverted checks, rusty doors, MM capacity/infinite/ring/lottery items, new features off, and all new parity features together. Every case uses both games' Boss Keys Own Dungeon and native Glitchless logic, actual standalone fills and independent native replay, and a real combined fill followed by the configured CLI playthrough. Fixed native metadata supplies each check's physical scene and exclusion/source identity; every boss key must occur exactly once in its own game and dungeon. Cross-game non-key placements must also exist. MM rings stay in their native dungeon.

The all-parity case simultaneously enables both games' five soul families, all four shared soul masks, native OoT first-type enemy drops, all five MM environmental families (211 combined checks, and the exact 181 solo checks eligible without configured Iron Boots), both song event shuffles, OoT Clock, rusty door keys, and MM capacity/infinite/ring/lottery features. The all-parity preset also enables real native OoT token, pot and freestanding checks to provide item capacity. Unrelated Skijer custom items are explicitly off in these scoped presets. Expected item and environmental check identities come from production catalogues and the modules' actual native pool metadata. Shared soul counterpart edges are merged into semantic components before asserting one placement per component. No donor count stands in for the native pool. The all-off case asserts absent new item/check families and souls.

Before filling any case, an actual overfull native configuration must report the native item budget error, export empty pools, and fail native standalone generation. The adapter must never continue with a partial pool or switch to a full vanilla fallback.

The own case additionally moves the real Stone Tower key behind its actual boss warp and requires configured native replay to reject that circular seed. The fresh output contains per-process native settings/static metadata, exact solo placements, MM independent collection spheres with every exact generated per-check price (including zero) installed before traversal, consolidated settings/spoilers, logs, module and driver SHA256, and a manifest written only after each full case passes. Failure retains evidence and fails CI. The evidence artifact excludes its runtime symlink tree; the built runtime is uploaded independently before validation or AppImage packaging.

These are genuine Vanilla layout native fills. Full MQ/mixed first-drop cases remain a separate opt-in matrix in `enemy_drops/native_seed_regression.py` and require the caller's actual configured MQ asset archive. Nothing fabricates an availability marker or substitutes a fixture graph. The small validator and entrypoint tests check schema/lifecycle boundaries and are not linked seed proof. No gameplay acceptance is claimed by these commands.

For complete translation-unit guard checks with real installed headers, use:

```sh
python3 -B scripts/diagnostics/run_native_syntax.py --repo "$PWD" \
  --game mm --mode original mm/2s2h/Rando/Logic/SeedGeneration.cpp
```

The helper reads native CMake PCH and CVar prefix definitions, honors `CC`/`CXX` and inherited `CPATH`, and optionally accepts a pinned real-header `--toolkit` directory. It retains the existing native GNU compiler policy. Syntax success is separate from native linking, configured seed proof, and gameplay acceptance.

The MM solo export snapshots the caller's price cache and restores it with the live save/seed state on success or failure. Its price payload comes directly from the completed native save; replay does not rerun GeneratePools or reuse pre-generation probe prices.
