# Enemy and NPC soul miniature checks

The detailed GI uses the owning game's actual model resources, a representative
native pose or animation, and the soul flame. Canonical Alt replacements are
selected by that owner's resource manager. A TP model is used only when the
selected pack supplies a compatible replacement for that resource. The two live
preferences are `gRandoEnhancements.SimplerEnemySoulModels` and
`gRandoEnhancements.SimplerNpcSoulModels`; both default to zero and are shared
between the Combo game tabs. Checked selects the category's token/flame fallback.

## Public regression gate

```bash
export IMGUI_DIR=/path/to/the/project-pinned/imgui
python3 -B tests/soul_mini_models/run_tests.py --sanitize
```

Registered in `scripts/ci/regressions.json`, render suite. Requires Python 3.11+ and a
C++20 compiler with ASan/UBSan support. The existing CI workflow supplies the
project-pinned ImGui source at `IMGUI_DIR`. No game archive is needed.

| Check | Evidence it provides |
|---|---|
| Production settings header and both menu bindings | Detailed default; all four independent enemy/NPC combinations; same-frame updates from either tab; boss preference independence |
| Real native checkbox widgets and ImGui | Both controls receive the gold What's New accent in both tabs; turning the global accent off restores ordinary styling; click state, IDs, layout and surrounding styles remain correct |
| Independent owner catalogs | Exact 43 OoT enemies, 46 OoT NPCs, 47 MM enemies, 53 MM NPCs; representative resources; native/foreign dispatch and original-build guards |
| Both host renderer adaptations | All 189 production recipes through actual native limb traversal and the shared renderer; body geometry required; resource ownership, matrix restoration and Alt cache behavior |
| Owner DLL boundary | Two real shared libraries with deliberately different C++ skeleton layouts; only validated POD metadata crosses the boundary |
| Native frame recorder transactions | Nested failure restores the recording path and actor inverse state; earlier successful draws survive rollback |
| Failure controls | Invalid resource kinds and layouts, malformed rigs/animations/poses/materials, failed loads, safe fallback and retry; focused cases are in the renderer fixtures |
| Archive validator negative controls | Wrong mesh types, missing/hash dependencies, unbound segments, matrix palette overruns, truncated animation tables, invalid XML/Alt selection, undersized material arrays |
| Actual MM material factory and native interpolation | Declared tables fit allocated storage; malformed/truncated inputs clean up; the last frame of linear color clips has valid terminal data |

The all-catalog renderer sweep runs each recipe in native/Alt/native phases at
four frames: 2,268 draws per receiving host, 4,536 total. Resource I/O is a fixture
boundary; recipes, dispatch, shared rendering, and native limb traversal are
production code. These are command and memory-safety checks, not GPU screenshots.
The material boundary preserves the five catalog resources' actual handler types,
segment lists, periods and declared counts, using invented colors. In particular,
Gomess exercises its type-3 linear color material and final frame 23; replacing
every material with a generic scroll would hide this required handler.

The eight formerly incomplete foreign MM enemy cases are Bad Bat, Boe, Chuchu,
Freezard, Like Like, Dexihand, Gomess and Iron Knuckle. Their exact native geometry
is represented, including non-skeletal parts, palettes, materials and post passes.

## Read-only checks with local game and replacement archives

Dump the actual compiled owner recipes:

```bash
python3 -B tests/soul_mini/run_oot_catalog_tests.py --dump /tmp/oot-souls.json
python3 -B tests/soul_mini_mm/run_recipe_tests.py --dump > /tmp/mm-souls.json
```

Check original assets:

```bash
python3 -B tests/soul_mini_models/check_native_resources.py \
  --oot-archive /path/to/oot.o2r --mm-archive /path/to/mm.o2r \
  --recipes /tmp/oot-souls.json --recipes /tmp/mm-souls.json \
  --output /tmp/soul-original-resource-report.json
```

For a selected pack configuration, append `--oot-mod /path/to/pack` or
`--mm-mod /path/to/pack` for every selected archive, in increasing load priority.
The gate resolves canonical and `alt/` paths, including XML skeleton/display-list
replacements. It validates the selected hierarchy, animation ranges, body and
fallback dependency closure, material segments and matrix addresses. Native
segmented-address tags are decoded as the interpreter does.

ZIP archives use Python's standard library. Legacy MPQ `.otr` archives use
StormLib, opened read-only. If the available StormLib build hides its symbols,
build the provided read-only bridge against that installed static library:

```bash
c++ -shared -fPIC -I/path/to/StormLib/src \
  tests/soul_mini_models/stormlib_read_bridge.cpp /path/to/libstorm.a \
  -lz -lbz2 -o /tmp/libstorm-soul.so
```

Pass `--stormlib /tmp/libstorm-soul.so`. No downloaded or private resource bytes
are included in source, reports, or recovery files. Reports contain identities,
paths, archive hashes, selection metadata and validation counts.

The OoT semantic head-swap compatibility check is separate:

```bash
python3 -B tests/soul_mini/run_oot_tp_compatibility.py \
  --stormlib /tmp/libstorm-soul.so \
  --mod /path/to/TwilightPrincessEnemies.otr \
  --mod /path/to/TwilightPrincessEnemiesTextures.otr \
  --mod /path/to/TPNPC_No_Darunia_No_Nabooru_POC1.otr
```

In the supplied configuration, selected replacements cover 26 OoT enemy and
36 OoT NPC soul rigs, plus 13 MM enemy rigs. Composer Bros uses a Djipi model in
the supplied mixed MM pack. Other souls use their native resources unless a
selected pack supplies another compatible replacement. The six forced OoT head
substitutions have matching TP replacement resources. These specific packs do
not use optional NativeNpcV1/PoseAnimations/private-skin adapters; compatibility
with a different custom adapter requires a new check.

## In-game acceptance matrix

Run this against a build that contains the source revision under review and record
its commit, executable hashes, save/seed, selected pack hashes, and settings.

| Source of soul | Receiving game | Required presentation |
|---|---|---|
| OoT | OoT | Native Combo pickup and placed/shop GI |
| OoT | MM | Foreign pickup and placed/shop GI |
| MM | MM | Native Combo pickup and placed/shop GI |
| MM | OoT | Foreign pickup and placed/shop GI |

For each route, capture representative skeletal and composite souls with original
assets and with the selected replacements. Inspect recognizable complete body,
correct face/eyes, attached props, transparency, scrolling materials, flame,
centering and scale. Exercise the eight MM cases, OoT Freezard and Stalfos,
Composer Bros' multiple lantern passes, and Kafei/Dark Link's native player poses.

Change the enemy checkbox and NPC checkbox independently from both tabs while
the GIs remain visible. Check all four combinations, immediate updates on cached
shop/placed items, and preservation of the separate boss setting. Repeat Alt
off/on/off. A missing, unsupported or malformed selected resource must yield
token/flame and allow a later successful retry. Check a following unrelated item
for inherited matrix, resource-owner or material contamination.

Repeat the relevant native route in Combo OoT-only and MM-only modes. Original
standalone ports retain their existing behavior and do not expose these new
Combo-only controls.

Passing the public and archive gates does not mark this visual matrix complete.
Record actual observations; GPU appearance and user acceptance remain separate
from source, compile and command-level evidence.
