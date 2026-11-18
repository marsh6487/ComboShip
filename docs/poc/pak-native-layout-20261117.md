# PAK models: native vertex and matrix layout

Baseline: ComboShip `d3cf01741084c188eb0a01a263018fa3de23fbe1`.

The combined runtime enables `GBI_S32_VTX` and `GBI_FLOAT_MTX`. The two PAK
loaders bypass resource factories: their translators pointed `G_VTX` directly
at in-place byte-swapped 16-byte N64 records, although native vertices are
24 bytes, and pointed embedded `G_MTX` commands at fixed-point words interpreted
as floats. Both are concrete layout defects consistent with the reported
scene-spanning geometry. The old swap tracking also double-swapped vertices
when body/equipment lists loaded overlapping ranges.

The PAK boundary now decodes immutable N64 vertex/matrix records into cached
native allocations. The model retains their owners through deferred drawing
and model-container moves. Body, raw ZOBJ, adult/child and equipment-only loading
retain those allocations. Runtime segmented matrices (including skeleton
segment 0x0D), equipment aliases, textures and skeleton selection are unchanged.
Truncated vertex/matrix spans become no-ops instead of out-of-bounds pointers.

Verification: `python3 tests/pak_native_data/run_tests.py --sanitize` compiles
and executes each production `TranslateDL` with actual native libultraship GBI
types in legacy and wide modes. Four combinations pass ASan/UBSan, including
signed positions, UVs, colors, overlapping loads, nested lists, fixed-to-float
matrices, deferred lifetime, caching, segmented matrix slots and invalid ranges.
The same fixture against the exact baseline fails on the decoded vertex
position in wide mode. This is an actual translator failure, not a visual
acceptance result.

Full executable compilation, the user's exact PAKs/load order, both game
renderers and configured gameplay remain untested. No model or animation pack
was altered; no publication or runtime acceptance is implied.
