# Woods of Mystery visual POC4 — wall and tunnel fit

This additive candidate changes the nine native Woods of Mystery room meshes.
All four existing openings in each room receive a donor entrance rim and a
cropped interior sleeve (36 openings total). It does not choose a daily route.
The first clearing also receives the separate OoT bridge tunnel, with green
hollow hex particles supplied by the matching native source change.

The 246-face / 321-source-vertex donor was recovered from the unchanged
`zzzz_MM3D_Lost_Woods_Black_Market_POC14.prelude.o2r`, SHA256
`7b8f2b820ba0bf4441889693f820d280f8dace6f4664387f847bf0fdeecbdce9`.
Both donor room halves remain in the reusable GLB. Room passages use only the
external room-0 mouth and the required length of its interior, with interpolated
UVs at the cut. The long donor body is not compressed into the short passages.

The supplied grass, rock, vine and ruin textures are scoped to this scene.
Recovered donor bark textures cover tunnels and added upper tree/root geometry.
Both native floor texture stages now use the supplied grass. Original Henriko
ENVIRONMENT crown and forest-edge RGBA artwork replaces all 36 native overhead
foliage display lists, including translucent lists and room 2's distinct edge.
Original alpha, foliage positions and normal bytes are retained; logical UVs
are adapted to 32×32 or 32×16 tiles with texture-edge coverage.
Native wall lighting and normal bytes are retained; donor bark uses its original
unlit vertex colors.

## POC4 correction from the play-renderer clip

cor accepted POC3's sky coverage and previously approved the custom fitted
OoT bridge shape. The latest clip and interior close-ups show native stone
walls burying the maze rims and cutting through their wood sleeves, plus a
thin native-wall lip across the bridge crown. POC4 preserves all 194 POC3
bridge wood/continuation/cap triangles and attributes exactly. That includes
the 110 approved bridge positions and its 24 exterior bark-rim triangles.

The inward wood uses continuous axial UVs within the plank portion of the donor
atlas. Shared positions receive a consistent circumferential UV phase and
baked vertex color, removing the isolated bark samples and bright/dark wedges.
This correction applies to all 36 maze sleeves and the bridge sleeve. Original
texture pixels and the donor source resources remain unchanged.

All 36 maze mouths move another 20 units into their clearings (195 → 175),
with short sleeves cropped to 275 so the far joins stay at 450. Their scale is
unchanged. Wall cutouts follow the complete projected bark rim, including the
sloped native shoulders that were hiding the lower wood. Walls above the
wood remain closed. The long donor remains cropped rather than compressed.

The opaque native lining becomes hidden backing outside the wood: its sides
sit at ±116 and its roof at height 145, from 210 to 450. The original eight
grass floor triangles per room remain exact. Sixteen added grass wing
triangles join their edges to the widened backing, and eight tail triangles
extend grass to the end join. The backing feet stay below the grass. An end
surround shares every quantized wood terminal edge and seals only the space
outside that opening; the 450 passage remains open. This closes exposed side,
foot and end strips without changing collision or the original walked floor.
The expanded lit backing draws through the appended visual entry's 1600-unit
cull sphere, with the old native lining list empty. Native room bounds stay
unchanged; separate material state keeps donor RGB unlit and backing normals lit.

The bridge corner uses the existing wall surface with a fitted aperture cut
through it. Its stone-panel material is replaced with the neighboring forest
wall materials. The lower/side bridge cutout retains POC3's coverage; only its
upper edge gains clearance to remove the crown lip. Clipping preserves UV
interpolation and signed lighting normals. Verification accounts for integer
XYZ quantization when recovering original-face attributes from UVs.
Clipped wall edges share every subdivision before integer serialization, closing
the tiny rounding cracks visible from oblique approaches. A 30-unit continuation
is stitched to every vertex of the approved sleeve's end loop, keeping that
cross-section through depth 150. Its cap shares every terminal edge rather than
using a rectangle or dropping collinear cut vertices.
The native effect mouth and gameplay/collision boundaries do not move.

## Loading and rollback

Use `zzzzz_Woods_of_Mystery_OoT_Makeover_POC4.prelude.o2r` as an additional mod archive,
with Alt Assets enabled. It must win load order over another pack replacing
`Z2_26SARUNOMORI` room headers or display lists. Reload the scene after changing
packs. Keep the accepted POC14 and other forest/shop packs unchanged.

In Prelude, load the native MM archive, add this override, and choose
**Continue editing**. Open **Woods of Mystery** and select **Room 1** for the
OoT bridge entrance. All nine rooms contain their four short tunnel mouths.
The geometry is a baked scene layer and can be inspected in the scene viewport;
it is not an unplaced recipe waiting to be instantiated. Remove the failed
POC1/POC2/POC3 overrides before loading POC4. Export as a **Prelude project** to retain the
editable layer. Canonical project entry names support both the original alt/
placement and Prelude's bare-path placement for newly introduced resources.

The matching ComboShip source must be built for the animated green hexes and
suppression of the older temporary bark arch. **The archive alone does not add
the animated effect.** The effect follows the loaded custom bridge mesh,
current Alt/resource availability, gameplay/cutscene state and existing
combined-game / solo-addon availability guards. The existing purple intro
portal treatment remains separate.
The POC2 native ownership guard accepts both bare and alt custom resources,
because Prelude exports newly introduced resources at their bare path. Alt
Assets and ownership of the loaded bridge room mesh remain required.

To roll back, remove this additive archive and reload the scene. The matching
source falls back to the older entrance treatment when the custom mesh is
inactive. To revert source too, revert only this candidate's commit or patch.

## Preservation boundary

The candidate contains no collision resources, actors, paths, scene headers,
triggers or arrivals. Its nine room headers differ only by one appended visual
mesh entry. Verification removes that entry and reproduces each native room
header byte-for-byte. Daily maze data, vanilla collision, audio, unrelated
forest/shop assets and accepted masters remain input-owned.

## Rebuild

Python 3, NumPy and Pillow are required. From this source root:

```sh
python -B tools/woods_mystery/build_candidate.py \
  --native /path/Woods-of-Mystery-Native-Resources.zip \
  --poc /path/tunnel-source-resources.zip \
  --donor-archive-sha256 7b8f2b820ba0bf4441889693f820d280f8dace6f4664387f847bf0fdeecbdce9 \
  --textures /path/textures --foliage /path/foliage --output /path/output
python -B tests/woods_mystery/verify_candidate.py \
  /path/output/zzzzz_Woods_of_Mystery_OoT_Makeover_POC4.prelude.o2r \
  /path/Woods-of-Mystery-Native-Resources.zip
python -B tests/woods_mystery/test_tunnel_fit.py /path/tunnel-source-resources.zip
python -B tests/woods_mystery/run_hex_test.py
python -B tests/woods_mystery/run_active_pack_test.py
python -B tests/lost_woods_entrance/run_tests.py
```

The original full POC can replace the exact donor-resource subset; omit
`--donor-archive-sha256` in that case. The provenance value records the original
archive identity; the report also hashes every donor resource actually used.
Archive entries have fixed timestamps and sorted names for exact rebuilding.
The recovery package's `rebuild.py` restores a small, complete asset-builder
snapshot and verifies an exact archive rebuild without the full game checkout.
Its `--base-recovery` option also restores the full source from the unchanged
POC1 recovery archive, applies this bounded repair patch, and verifies the exact
final Git tree. `--native-tests` uses that mode and POC1's header dependencies.

## Verification and remaining proof

Checks cover all nine room layouts, native wall material semantics/normal bytes,
unlit donor materials, exact header restoration, absence of collision/actor/path
overrides, retained entrance rims, clipped sleeve lengths, green hollow-hex
geometry, live Alt changes, scene/room ownership and native/solo fallback guards.
The existing forest entrance suite compiles the production renderer and checks
boot, save, arrival, camera and existing forest behavior across days/forms.

POC2 checks additionally cover exact room-path decoding (without trailing NULs),
both floor stages, raw RGBA byte strides, all original Henriko pixels/alpha,
and Prelude's actual manifest validator and layer-restoration code. The latter
must recover all 162 POC4 payloads byte-for-byte in both original and canonical
ZIP layouts. POC2's live Prelude import/export was exercised separately.
POC3 seam tests additionally check solid lining and noncollapsed tiling, all 36
mouth/far-join placements, the retained upper wall, unchanged approved bridge
wood positions and exterior rim attributes, continuous interior UV/color joins
in all 37 sleeves, centered/oblique aperture rays, a closed serialized cap mesh,
and byte-identical main floor, foliage, textures and room headers compared with POC2.
POC4's wall-fit tests check all 36 wood interiors and visible lower rims, the
bridge crown's first visible material, exact POC3 bridge preservation, and
opaque backing feet with a negative control that removes their grass wings.
They also check that the far surround keeps the wood passage open and every
backing vertex lies within the actual appended visual cull sphere.

POC4 contains no new native C++ changes; native compilation evidence belongs to
the inherited POC2 effect/ownership implementation. POC4 has fresh static/editor
checks and an independent archive rebuild. In-game appearance,
live room culling, performance and traversal of each daily route remain untested
for POC4. The user accepted POC3's sky coverage; the new wall fit and hidden
backing still need play-renderer inspection. The cloud browser cannot create a WebGL context, so its Prelude
viewport could not be visually checked. The earlier POC1 software PNGs omitted
native two-stage canopy behavior and are not acceptance evidence for this
repair. No master promotion is included.
