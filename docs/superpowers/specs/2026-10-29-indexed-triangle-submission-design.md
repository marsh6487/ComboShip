# Indexed triangle submission

User authorization: implement indexing, then add it to the waiting cumulative ComboShip source. Preserve full scene/model geometry, texture resolution, animation, effects, ordered material passes and gameplay. No new diagnostic build, protected merge or release.

Parent: `b43e86eaad1b3507367cc84b5d405f7b9f4aca1e`. Renderer is identical to `1b3f16dc`; parent preserves the cumulative actor/music/Major chest fixes, equipment provenance and restored loader ABI checks. Preserve Market and Chamber SFX through bounded additive integration after the renderer is verified.

## Architecture

Build the index stream while packing triangles, not by deduplicating the expanded buffer afterward. Start a batch with ordinary records and promote it in place only when sharing is observed; identity indices preserve the existing records. Batches with no reuse retain the original GPU draw and avoid an index-buffer upload. Eligible consecutive triangle commands may reuse a packed output vertex for a loaded RSP index. Existing nontriangle command boundaries, explicit flushes, microcode changes, rejected/unknown commands and each Run invalidate that reuse. A new load or material therefore cannot reuse old attributes for the same index.

The current CPU clipping/culling, shadow capture and triangle order remain. Indices refer to complete packed attributes, not only positions. Keep the direct path for rectangles/S2DEX, triangle-dependent LOD and shared sampler nodes. Mode transitions flush before changing representation. Maximum batch size remains256 triangles; uint16 indices cover the maximum768 vertices.

The shared backend API gains a capability query and indexed draw entrypoint. DX11 and OpenGL implement real indexed buffers/draws while sharing their existing state preparation. Other backends advertise unsupported and retain the old path. A default expansion implementation provides a correctness fallback for the new API. Indexing applies to both games independently of OoT-only triangle-preparation reuse.

Enable on supported backends at Run start, with `gEnhancements.IndexedTriangles=0` as a source-candidate rollback control. This is an optimization option, not diagnostic instrumentation; existing tracing remains unchanged and defaults remain respected.

## Verification

- Compile the actual interpreter functions and API in the existing recording-backend fixture.
- Expand indexed output and compare every float bit, triangle order and observable state boundary against the legacy stream for16384 material/clip/cull variants, both game-style preparation modes, shadows, explicit flush, rollover, modified vertex slots and material changes.
- Assert two shared-edge triangles use four packed records and six ordered indices; preserve all triangles and fallback behavior.
- Exercise actual backend indexed upload/draw functions at controlled API boundaries; compile real backends where the toolchain permits. A fixture is not a platform build.
- Run meaningful synthetic CPU comparisons with indexing on/off, including repeated and disjoint vertices. Report unfavorable results and limits; do not convert to claimed game FPS.
- Preserve the complete cumulative source, test inventory and existing required binary resource bytes except the explicitly accepted Chamber emitter addition. Independently verify a durable source checkpoint restores the exact tree.

Source/build verification, actual runtime proof and user acceptance remain separate.
