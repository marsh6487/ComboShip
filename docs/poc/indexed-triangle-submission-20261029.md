# Indexed triangles in the cumulative Unbound source

Implemented real indexed triangle submission for DX11 and OpenGL in both games.
This source candidate preserves authored geometry, full texture resolution,
materials, triangle order, animation and gameplay. It adds no diagnostic scope.
Full application compilation, GPU execution, visual acceptance and gameplay
performance remain unverified on this host.

## Source and implementation

Preservation parent: `b43e86eaad1b3507367cc84b5d405f7b9f4aca1e`.
Renderer milestone: `dfc259f727d5f7aab927327da7e7c756359ca4fa`.
Local candidate branch: `candidate/indexed-triangle-submission-20261029`.

The existing morning RenderFlight evidence identified substantial CPU command
work; it did not prove this optimization's game-frame savings. The earlier
investigation and texture-pipeline findings remain in
`docs/poc/renderer-investigation-20261029.md`. Its proposal/status sections
describe the preimplementation checkpoint; this document records the new state.

| Area | Production source | Result |
| --- | --- | --- |
| Triangle packing | `libultraship/src/fast/interpreter.cpp`, `GfxSpTri1Impl` | Reuse complete packed attributes for an unchanged loaded RSP vertex slot. |
| Batch submission | Same file, `Interpreter::Flush` | Send unique records and ordered uint16 indices when sharing occurred. |
| Lifetime | `UcodeHandler::invoke`, Run/reset/microcode/Flush and shadow replay boundaries | Retire slot reuse before mutable vertex/material commands can change attributes. |
| API | `libultraship/include/fast/backends/gfx_rendering_api.h` | Capability query, indexed entrypoint, exact expansion fallback. |
| DX11 | `gfx_direct3d11.cpp`, `InitializeTriangleIndexBuffer`, `SubmitIndexedTriangles`, `DrawTrianglesInternal` | Dynamic index buffer, R16_UINT, DrawIndexed; original state preparation shared. |
| OpenGL | `gfx_opengl.cpp`, `InitializeTriangleIndexBuffer`, `SubmitIndexedTriangles`, `DrawTrianglesInternal` | Element buffer, GL_UNSIGNED_SHORT, glDrawElements; original state preparation shared. |

Each batch starts with the original expanded records. Only an actual shared slot
promotes it to indexing; the prior prefix receives identity indices in place.
Two compatible shared-edge triangles submit four records and six indices rather
than six records. A batch with no sharing retains the original GPU draw and no
index upload. Existing CPU transformation, lighting, clipping and culling still
run; this change reduces subsequent attribute packing/copying, vertex uploads
and potential GPU vertex work. It does not reduce triangle or draw counts.

Reuse is command scoped and batch local, independent of the OoT-only prepared
state optimization. Slot reloads, material changes, explicit Flush and rollover
cannot reuse stale records. Rectangles, generated shadow vertices, triangle LOD
and two samplers sharing one mutable texture-cache node retain the direct path.
Unsupported backends retain the direct path. Material passes and translucent
triangle order are preserved; there is no cross-actor content deduplication.

The maximum batch remains256 triangles/768 references. New interpreter storage
is about1.7KiB per interpreter, chiefly1536 bytes of indices and128 bytes of slot
IDs. DX11's additional dynamic buffer is1536 bytes; OpenGL uploads at most that
many index bytes per batch, with its element buffer following existing context
lifetime. Existing vertex capacity is unchanged. No resource ownership,
archive precedence, CPU texture cache or persistent texture memory policy changes.

## Verification and measured limits

The production-function fixture expands indexed output and compares every float
bit and ordered backend event against the old path. It passes16384 material,
clipping/culling and texture variants in each preparation mode, plus the prior
16384-variant suite. Focused cases cover shared edges, nontriangle mutations,
slot63, toggle, explicit Flush, rollover, shared samplers, full768 unshared
records, indexed/direct/indexed transitions, shadow capture and active toon
normals. Review found the first toon test had omitted G_LIGHTING. A new assertion
failed on that test; enabling the actual mode and checking exact transported
normal attributes passed ASan/UBSan. No production correction was needed.

The pinned CI formatter14.0.6 passes for every changed CI-gated source file.
Five inherited chest/fixture formatting gaps were normalized with an explicit
comparison confirming unchanged non-whitespace bytes.

Real DX11/OpenGL initialization, upload and draw methods compile in native GPU
API boundary fixtures. They verify vertex/index bytes, uint16 format/order,
original draw behavior and DX11 map-failure handling. Existing cel/stencil backend
fixtures pass. Native caustic binding/UV/cache checks, MM graph/loader tests in
both runtime ABIs and wide-vertex hash/two-DSO ownership tests pass; the latter
uses a private no-op destructor trace header because spdlog headers are absent.
That suppresses logging only, not resource storage or production handlers.
Unbound equipment ownership checks pass in both games. The existing missing-audio
sanitizer fixture is run with LeakSanitizer disabled because this host cannot
inspect threads through its process boundary; ASan and UBSan remain enabled.

Synthetic CPU results below are median10 timed iterations after warmup,32768
triangles, recording backend with output expansion disabled. They exclude game
actors, collision, resource I/O, driver and GPU work. The shared host is noisy;
these are mechanism checks, not frame times or an FPS forecast. Disjoint cases
include command barriers and expose bookkeeping overhead. All measured cases
are included, including regressions.

| Material variant | Shared legacy ms | Shared indexed ms | Disjoint legacy ms | Disjoint indexed ms |
| --- | ---: | ---: | ---: | ---: |
| 0 | 1.31801 | 1.10848 | 2.06345 | 1.77978 |
| 1 | 1.63128 | 1.16753 | 2.51090 | 2.73982 |
| 3 | 2.91164 | 1.91358 | 5.70146 | 5.42055 |
| 15 | 1.95859 | 1.19709 | 2.11026 | 2.41476 |
| 127 | 2.84509 | 1.77594 | 3.90229 | 4.16923 |

Confirmed: exact tested stream parity, reduced records for sharing, real indexed
API submission at controlled boundaries. Suspected: detailed shared-vertex
models benefit during gameplay. Unmeasured: actual DX11/GL driver cost, GPU time,
game frame-time percentiles, stutters, VRAM effects and full-visual scene parity.
There is no measured before/after game-frame result and no promised target.

Full DX11/GL translation units and the executable were not compiled: this host
lacks the platform toolchain and application dependencies, CMake and a GPU/display.
Native backend fixtures are not a substitute for that build. The old vertex-batch
baseline script also needs historical commit9c084d74, absent from this shallow
store. An adjacent collision/audio-reader check needs nlohmann/json.hpp. These
are recorded limitations, not passes. No new performance instrumentation was added.

## Cumulative preservation

The candidate retains the complete newer morning source, final MM Major masks
AND fairies policy, actor/NPC/music integrations, equipment provenance repair and
native MM loader ABI repair. It does not replace the source with an older donor.
All260 existing regression groups and commands survive; two groups are appended,
plus the Market command in the existing Unbound group.

Market is exactly the bounded `cb24e1a6a6c2e7a1c57143d0ba91aa99084aea5a`
production/test/documentation delta: synchronize projection and room culling with
`max(lightCtx.zFar,12800)`, preserving larger authored distances and existing fog.
Six actual-expression UBSan controls pass. No Market archive changed.

Chamber SFX uses POC14's narrow COMBO_BUILD guard and appends exactly one native
emitter to each current canonical/Alt child/adult-day header. The guard affects
only the authored Temple enclosure's exact emitter, room and player bounds.
Current child Zelda `(0,-110,-2400)`, Y rotation0, is retained. All4092 other
decoded pool resources are byte-identical to the parent. The four edited headers
change only actor count13→14 and the appended16-byte actor record:

- Child canonical/Alt SHA256: `b076249094a9a52a6a97953c3d174b620834056cf6232a3c42264347b7974825`.
- Adult canonical/Alt SHA256: `127a9af4b2c8123219da9d5ae34134e30adf9051eb8c9bbc6dca03f748397fc8`.
- Protected4092-resource path/hash digest: `37e8c132a1f314add4be095a9c8e1fb83f48f05e6389aac0f3681c673c813d6f`.

The actual normal Chamber preparation followed by cumulative preparation retains
these exact headers in both port trees. Both CMake archive targets depend on
preparation; binary room resources are copied without reinterpretation. The
Chamber guard/emitter checks pass ASan/UBSan; existing packaged-Chamber and native
layer checks pass with their existing skip. Optional external visual archives
can override room headers: retain compatible emitter records in such overrides.
The older POC14 visual archive was not copied over the current scenes or masters.
Configured visual and audible acceptance remain pending.

## Reproduction, ordinary build and rollback

From the repository root, commands executed successfully for the new behavior:

```sh
python3 -B scripts/diagnostics/run_gfx_triangle_run_tests.py
GFX_TRIANGLE_TEST_CXXFLAGS='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie' ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_gfx_triangle_run_tests.py
python3 -B tests/indexed_triangles/run_backend_tests.py --sanitize
python3 -B tests/cel_render_backends/run_tests.py
python3 -B scripts/diagnostics/run_gfx_triangle_run_tests.py --index-benchmark
python3 -B tests/graphics/run_3d_background_distance_tests.py -v
python3 -B tests/temple_sages_bundle/run_chamber_sfx_tests.py --sanitize
python3 -B tests/native_mm_actor_vertices/run_tests.py --sanitize
```

The normal Windows commands below are verified against the repository README and
helper script, not executed here. They build the actual optimization candidate;
no separate diagnostic executable is required. On a machine with the documented
MSVC/CMake dependencies:

```powershell
cmake -B build/x64 -A x64
./scripts/build-comboship.ps1 --Release
```

Output is `build/x64/combo/Release/Fleet.exe` with the normal staged runtime and
port assets. The renderer virtual interface changed: rebuild the engine and both
ports together, retaining the complete staged DLL set rather than mixing old
binaries with this source. Preserve save, camera, scene setup, settings, archives and ordering
when comparing indexing on/off in the full authored configuration. Use existing
RenderFlight only if measurements are needed; validate final gameplay with
tracing disabled. No new capture is asserted to have happened here.

Console rollback, supported by the actual SetCommand parser:

```text
set gEnhancements.IndexedTriangles 0
```

Restore with value1. The setting takes effect at the next renderer Run and
flushes before a mode change. This disables the optimization while keeping the
cumulative integrations. Source-only rollback of indexing may revert the toon
test follow-up and milestone dfc259f7 on a new candidate; do not revert the later
Market/Chamber integration or overwrite accepted branches. The preserved b43
parent restores the entire pre-index source when a full rollback is intended.

Durable recovery branch:
`checkpoint/oct29-cumulative-indexed-triangles-20261029` in
`marsh6487/ComboShip`, published at `32680828cdd96fec1173d9cd95069857db12f149`.
Its source tree `e0fe8178202834acdcca45db02acd48d38e39b09` is exactly the
authorized local `f2ca8f249994a69caf5d13d47e4d020ee954888f` tree. Explicit
public-destination authorization resolved the earlier publication blocker.

The preservation parent, renderer milestone, active toon test and cumulative
integration are retained as mapped source-history checkpoints. See
`docs/recovery/oct29-cumulative-publication-20261029.md` and its JSON companion
for exact local/remote identities, focused re-verification, original-object
recovery, restoration instructions and remaining limits. Additional publication
records are on the related candidate branch; the checkpoint remains pinned.
Source publication does not establish a full platform build, GPU execution,
configured runtime acceptance or measured FPS improvement. No protected branch
merge or game build/release publication is authorized.
