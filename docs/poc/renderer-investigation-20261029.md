# ComboShip renderer investigation — concrete changes

## Outcome

Use the existing October 29 captures and the actual vendored SoH/LUS renderer to change CPU rendering work. Preserve full models, texture resolution, scene geometry, effects and gameplay. No new diagnostic build is proposed. The experimental texture-only probe was withdrawn; no probe hooks, new probe tests, CVar or CI entry remain in the source candidate.

The captures report executable commit `226b9f4`; source review is pinned to the available combined checkpoint `1b3f16dcc27fcb940521c311b73b825e8153d9c0`. They are different provenance records. The renderer tree is identical to the accessible older scene-suite checkpoint; this is not an assertion that the user's running executable has been replaced.

## What the existing evidence already establishes

| Condition | CPU renderer median | Matched GPU median | Interpretation |
|---|---:|---:|---|
| Latest Field, Din POC3 present, Alt on, 488 warm samples | 32.96 ms | 5.32 ms | CPU renderer exceeds GPU in 476/488 matched samples. Din self time averages 16.13 ms. |
| Earlier Field, Din absent, Alt on, 616 samples | 27.07 ms | 1.91 ms | Residual CPU burden is real; authored NPC paths were still present. |
| Same earlier capture, Alt off, 475 samples | 8.66 ms | 2.06 ms | Different visual workload; do not mix with Alt-on samples or call this texture-only causality. |
| Kakariko, Anju / object_im | 3.31 / 5.31 ms resource self medians | Not a per-actor GPU measurement | Independent NPC command work. Cold scene-entry skewed the old Impa mean. |
| Fountains, 256 slower audited samples | 0.301 ms own-command median | Unmeasured per fountain | Zero water uploads/misses; persistent water reupload is contradicted. |

These are instrumented CPU/GPU scopes, not uninstrumented FPS or controlled optimization deltas. Cold loads and episodic GPU stalls remain: latest GPU maximum 202.3 ms; 60/488 samples exceed 16.667 ms. The Din-absent capture is not a clean Djipi-only control. No extra broad model-removal run is needed to establish the above.

Retain the morning lossless NPC/model results and the user's stable 30–40 FPS observation with the later hair/hoodie POC1, without presenting static POC2/MM verification as runtime proof. Traditional Saria remains excluded.

## Concrete renderer changes, ranked

### 1. Add indexed triangle batches: main structural recommendation

Current source: `libultraship/src/fast/interpreter.cpp:184–195,2638–2663`; `libultraship/include/fast/backends/gfx_rendering_api.h:78`; `libultraship/src/fast/backends/gfx_direct3d11.cpp:1000–1029`; `libultraship/src/fast/backends/gfx_opengl.cpp:686–772`.

Today every triangle appends three complete packed vertices to `mBufVbo`. Existing packed-vertex reuse avoids recalculating some attributes but still memcpy-copies them for every reference. DX11 then maps/copies that expanded stream and calls `Draw(3 * triangle_count)`. The OpenGL path likewise uses `glDrawArrays`. This is directly relevant to the high TRI2 CPU cost and detailed repeated-vertex models.

Change the batching interface to carry a unique packed-vertex buffer plus a triangle index buffer. For a compatible prepared-state run, reuse a batch-local emitted vertex index rather than append its attributes again. Flush both buffers at the existing state/texture/framebuffer/material boundaries. Use DX11 index-buffer upload plus `DrawIndexed`, and an OpenGL element buffer plus `glDrawElements`. Other backends must retain an explicit compatible fallback until implemented.

Preserve clipping/culling and triangle order; never merge distinct material passes. Keep fallback emission for rectangles/S2DEX, triangle-dependent LOD and any case where the same loaded index has different complete packed attributes. Invalidate on changed vertices, UV/color/fog/shader state, explicit flush and buffer rollover. Keep the toon-shadow capture path before rejection unchanged. Do not assume a raw N64 vertex index alone identifies an output vertex.

Proof before a gameplay candidate: expand indexed fixture output back to the old triangle stream and require bitwise-identical full attributes and ordered triangles across the existing 16,384 material variants, clipping/culling, both clip conventions, shadows, LOD and rollover. Then compile the real DX11 backend. Reduced copies/uploads are the intended mechanism; whole-game FPS savings are not yet measured. This is a substantial renderer change, not an asset rewrite or simple flag toggle.

### 2. Stop redoing matrix work for identical loads

Current source: `interpreter.cpp:1776–1835,1840–1848,1917–1928,2921–2958` and custom matrix/toon handlers. The inspected Anju command graph contains 809 identical body/umbrella matrix reloads and 289 in the skirt. These are source/asset opportunities, not measured time savings.

For decoded `G_MTX_LOAD` with no push or multiply, compare against the current matrix stack top. Avoid the redundant copy and matrix-product rebuild only when the combined matrix is known to represent the current modelview/projection pair. Track matrix-product validity across projection/modelview loads, stack operations and frame/reset/custom commands. The checked G_MW_MATRIX/force-matrix constants have no implementing cases in this renderer; preserve that behavior, and require any future direct-product writer to invalidate the cached product. Never use address equality alone: interpolation replacements and mutable matrix memory can change values at the same address.

Do not blindly suppress `lights_changed`. Light/look-at setters and toon transitions currently have invalidation gaps, so an identical matrix command can be the event that refreshes lighting. A first narrow patch retains that side effect while skipping safe matrix work. A complete revision-based path must invalidate transformed light/look-at/toon coefficients at every relevant input mutation and preserve preexisting dirty state.

Proof: compare final loaded vertices, lighting, texgen, toon outputs and stack effects against the old path for repeated loads, same-address changed matrices, changed projection/direct MP insertion, animated/interpolated actors, light/look-at changes and pop/push boundaries. No pose, animation, skeleton or triangle reduction.

### 3. Remove avoidable work in triangle and DX11 submission paths

`TraceAutumnForestDraw` at `interpreter.cpp:2127` is called from every textured triangle at `2631`. For ordinary nonforest textures, a zero cached layer falls back to resource metadata and path classification repeatedly. Move classification to texture-state setup, distinguish unclassified from classified-not-forest, and invoke these existing observations only when their relevant draw state changes. Retain alias/private-resource correctness and existing target trace coverage. This removes work from ordinary triangles; it does not introduce another trace.

DX11 `DrawTriangles` at `gfx_direct3d11.cpp:927–953` checks cached resource views, but still calls `PSSetSamplers` for every used slot on every draw. Move the sampler comparison outside the resource-view-change condition and issue the API call only when that slot's sampler changes. Preserve cache invalidation after `ClearState`, framebuffer/GUI/context handoffs and device recreation; same-resource sampler changes must still bind. This is secondary CPU/driver cleanup, not the likely entire Din cost.

Proof: unchanged ordered GPU-state events after eliminating only identical repeats; coverage for the same texture with changed wrap/filter, resource changes with unchanged sampler, GUI restore, framebuffer and device reset. Preserve all rendered pixels and effect behavior.

## Lower-priority or rejected shortcuts

- **Vertex-view caching:** hashed vertex commands repeat RTTI/stride/size work, but resource vectors/stride are public and mutable, and existing tests require prompt resource/manager expiry after replacement. Preserve current owner-aware RM resolution every execution. Cache only safe provenance with lifetime/version validation; keep current pointer/size/offset/count checks. No persistent pointer-only cache or strong retention. The resource getter wrapper is not another archive load. This proposal is less direct than indexed submission and needs a shape-stability contract before caching whole views.
- **Scrolling material reuse:** a guarded default-off implementation already exists. Finish/preserve that path rather than claim it as new. UV-only changes should not repeat texture import/binding when pixels/sampler state are unchanged, but keep UV calculations and palette/mask/framebuffer fallbacks. Fountain costs are small in the available evidence, so this is secondary.
- **Enlarging the texture cache:** it already has 8192 entries. No measured eviction churn justifies more entries; this is an entry count, not a VRAM byte budget.
- **Repacking legacy headers:** a cold-load envelope skip and zero-copy texture body do not explain a repeated warm-frame CPU cost. Expanded Unbound limits are runtime limits; do not confuse 1024 object banks with cache entries or assume MM has OoT's expanded gameplay capacities.
- **Opcode-table rewrite:** the renderer already uses fixed 256-entry constexpr tables; it is not doing unordered-map opcode dispatch. Do not propose replacing a structure that is already present.
- **Automatic texture downsizing, scene cuts, model decimation or deleting paired material shells:** not part of this renderer work.

## Texture pipeline, traced from source

Last mounted archive wins same-path resolution. O2R cold entry reads/decompresses into a buffer. ResourceManager resolves path/owner/archive and Alt precedence, then caches the deserialized CPU resource. The loader skips a legacy envelope through BufferOffset; native texture factories retain the pixel buffer. Both games add narrow spin/effect normalization wrappers, once per cold load.

SETTIMG commands carry resource/pixel metadata into emulated TMEM state. ImportTexture checks the separate GPU cache, keyed by image/palette addresses, format/size and logical load bytes. On a GPU hit it returns before decoding/uploading. On a miss it converts where required or uploads physical RAW/IMG pixels. DX11/OpenGL then sample the existing GPU texture at draw time. CPU resource ownership and GPU-cache lifetime are separate; current invalidation/Alt/manager behavior must survive any optimization.

Exact upstream comparison: vendored LUS tree `c539cfd9e2f2737c055298e48a1ceac492e09723`, recorded Kenix3/libultraship pin `7f9b86a593c526fc42261d7fe197100cecf57178`; upstream texture cache1024 vs current8192. Zero-copy legacy readers already exist at that pin. Current synchronous LoadResource already bypasses ready-future allocation on a warm hit. Do not propose those existing changes as fresh work.

## Status and recovery

The source investigation and four-capture evidence audit are complete. The three renderer changes above are concrete proposals, not implemented or measured speedups. No diagnostic build is being delivered. No scene/model/texture archive or user preference was changed; accepted masters remain unchanged.

A separate source compile regression was independently reproduced and repaired in local checkpoint `029bcfc68275eee80a011fdbff498376c53cd683`: restore the native MM loader's direct GBI include/`sizeof(Vtx)` and its actual-resource fixture, preserving legacy16/wide22 archive offsets and runtime16/24 storage. Sanitized real-resource/graph tests passed independently twice. That repair is not presented as a performance fix. The renderer itself remains byte-identical to `1b3f16dc`.

This environment cannot establish DX11 gameplay FPS or visual acceptance. Existing evidence is sufficient to select and implement renderer candidates without another broad diagnostic round. Use ordinary gameplay/visual acceptance of an actual optimization candidate for the final result, with the existing source/archives retained as rollback.
