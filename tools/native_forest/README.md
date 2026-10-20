# Native stock forest source

`resources.zip` is the verified geometry-only source bundle generated from the
stock MM archive supplied on 2026-10-20 (SHA256
`2f44bafe1a00f44ddaf7eb5e5c99e73a3d23771038529d6d1748819ad575dc80`).
`manifest.json` records all 39 generated resources and 88 external stock
dependencies. Controlled test graphs are never packaged as game assets.

The source contains only private exploration headers, changed native
stump display-list ancestors and vertices, building geometry and collision,
the existing native MM passage arch, and stock material-animation parameters. It contains no texture pixels,
MM3D forest meshes, masks, poster, bench, or authored cosmetic effects. Building
meshes use an untextured native lighting material. Their original positions,
vertex data, loads and triangles are retained. Existing procedural portal
effects remain owned by the runtime.

The arch is the existing MM `BuildPassage` geometry at (-695, 0, -313), yaw
12288, exposed as `custom/comboship/native_forest/portal_root`. Its native
matrix and local vertices are preserved exactly, with a separate type2 culling
entry in room 0. Its material references stock bark through the MM owner;
the bundle contains no bark pixels.

Each building mesh calls its own
`custom/comboship/native_forest/building/{recipe}_material` display list.
The receipt's `building_materials` map records the exact source recipe and
native material resource. A later optional visual archive can replace those
material lists and supply their texture bindings without replacing meshes,
vertices, scene headers, or collision. Default material lists contain no images
and reference no custom texture resources.

The retained FINAL POC19 full-scene archive uses legacy combined scene and
MM3D mesh identities. It does not override these native material lists or the
stock texture identities, so its former combined presentation does not activate
with this native scene. It needs a separate visual-only retargeting; no such
texture/UV migration is included here. Existing converted legacy solo scenes
retain their own FINAL material path.

`custom/comboship/native_forest/stock_materials` is a version-zero LUS Blob
with the normal 64-byte header, little-endian payload length at offset 64, and
payload at offset 68. The payload contains byte version 1, byte binding count (0–8),
then each byte segment and byte layer count followed by each layer's signed
X/Y velocity and unsigned width/height bytes. Y is already negated to match
native MM. The Blob factory adds 16 zero padding bytes to its in-memory Data;
they are not part of the serialized payload. The runtime must account for
that padding when checking trailing data. The supplied stock forest has a
zero-segment Empty TexAnim sentinel and needs no scene scrolling. The empty
parameter Blob is valid; optional visual materials can replace it with tracks.
For matching MM effects, the same visual-only pack must also override
`custom/comboship/native_forest/texanim` with equivalent MM AnimatedMaterial
tracks. The OoT Blob and MM TexAnim are distinct host formats; supplying only
one does not establish animated-material parity between the games.

MM retains its native Demo_Kankyo ambient fairies. OoT private rooms use
Object_Kankyo params0, the host equivalent, instead of dropping the particles.
Both native draw routines bind their stock sun texture to segment8 themselves;
no particle images or global scene scroll override is introduced.

Each host has a complete readiness descriptor at
`custom/comboship/native_forest/dependencies_{combined,mm,oot}_v1`. These use
the same standard LUS Blob framing. Their payload is byte version 1, then
little-endian uint16 core count and uint16 stock count, followed by all core
paths and then all stock paths. Each path is a little-endian uint16 UTF-8 byte
length and the bytes, without a terminator. Both lists are sorted, contain
1–4096 paths, and share no duplicate path; each path is safe and 1–512 bytes.

Core includes every generated shared/private resource, including all changed
native display-list ancestors, plus only that host's scene, collision and
three room headers. Dependency descriptors themselves are excluded; the
runtime must check its selected descriptor before loading it. Stock contains
the complete receipt `external_resources` inventory, including the arch's
bark. The runtime checks core through the host asset owner and stock through
the native MM owner after initialization. `native_core_header()` exports the
three descriptor constants; its fixed common resource list alone does not
describe the donor-dependent graph. Bundle validation checks exact core
membership, consistent stock membership and, when given the receipt, exact
external membership before writing.

Unchanged geometry and all stock textures retain their original MM resource
paths and are supplied by the user's normal `mm.o2r`. OoT must initialize its
existing `MmAssets` base owner before enabling/loading this forest. Mod and
Alt texture lookup remains ordinary host resource resolution. No extra forest
archive is mounted at runtime.

OoT solo uses private scene `0x6F` through entrance `0x618`. Its pink-tunnel
hook selects that entrance only after the native resource gate passes. The
previous Great Fairy Fountain floor exit can still run when the gate fails.
The MM forest and native MM keeper require the normal `mm.o2r` base owner;
OoT solo's general setup permits running without it, but this MM area is then
unavailable. The supplemental Lost Woods archive cannot satisfy that owner.
Neither source checks nor asset preparation establish an installed game's route.

To reproduce the source, use a verified immutable donor and new output paths:

```sh
python3 -B tools/black_market/native_forest.py \
  --native-mm /absolute/path/to/mm.o2r \
  --final /absolute/path/to/zzzz_MM3D_Lost_Woods_FINAL_avatar_POC19.prelude.o2r \
  --output /temporary/native-forest/resources.zip \
  --manifest /temporary/native-forest/manifest.json \
  --contract /temporary/path/NativeForestResources.h
```

The CLI requires new output paths and verifies all immutable donor SHA-256
identities before writing. The original authoring donor SHA remains supported;
the supplied newer stock donor was verified against all retained stump vertex
records (positions, UVs and colors) and the three native room headers. Inspect
the resulting dependency inventory before replacing checked-in source. The normal
`GenerateSohOtr` and `Generate2ShipOtr` targets verify the bundle and prepare
both custom-asset trees before packing. Missing/corrupt input blocks archive
generation. Ordinary executable builds do not require a completed bundle.

Run focused generator/preparation checks with:

```sh
python3 -B -m unittest discover -s tests/native_forest -p 'test_*.py' -v
python3 -B tests/black_market/run_solo_forest_tests.py --native
```

These tests exercise the actual retained native edit, native binary layouts,
resource rebinding, culling bounds, texture exclusion, deterministic packing,
complete per-host dependency closure, strict Blob decoding, hash-checked
restoration and the real CMake dependency. The controlled stock
graph only verifies mechanics. The real 39-resource bundle is also consumed by
the runtime dependency gate for all three modes. The `--native` route check also
runs both production entry/return/save bodies with the real bundled dependency
payload and host-specific resource index, instead of a boolean availability
stub. It verifies private entry without a visual archive. Native scene generation
and both-tree preparation pass; full game/port archive builds, configured
rendering/collision and user acceptance remain unverified. Local full CMake
configuration currently stops at the missing SDL2 development package.
