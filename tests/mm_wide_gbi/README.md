# MM runtime GBI compatibility

Run `python -B tests/mm_wide_gbi/run_tests.py --sanitize` from the repository root.
The runner compiles actual MM C translation units with the actual shared GBI and
MM headers in both legacy and `GBI_FLOAT_MTX`/`GBI_S32_VTX` modes. It does not
duplicate matrix writers or transition geometry builders. Resource lookup,
frame-interpolation recording, sleeping and the system allocator are boundary
fixtures; the graphics wrappers and MM trigonometry are production code.

The checks cover:

- Signed/fractional matrix roundtrips and large float-mode translations, both
  conversion APIs, native identity, the static identity, translate/scale writers,
  and the existing general/skin matrix conversion wrappers.
- Oblique-axis rotation and nonuniform scale/rotation/translation, preserving
  the existing documented `axisY` signs. Legacy mode pins existing packed
  word/halfword outputs rather than changing their endian-dependent behavior.
- Native transition vertex/UV initialization, row offsets, two independent
  frame-specific display lists in wide mode, alternating draw/list selection,
  allocation sizes, and one matching free for each allocation. Legacy mode
  retains its single segmented list.

An initial native run passed the legacy checks and failed the wide-mode doubled
display-list allocation check before the transition adapter was added. The
pre-adapter `guMtxF2L` was separately observed emitting fixed-point bytes under
the float-mode flag. To check preserved legacy behavior against the pinned host:

```sh
python -B tests/mm_wide_gbi/run_tests.py --mode legacy --sanitize \
  --source-ref badc4e09a5176b5d423926eed21d5d19a67ee87e
```

The sanitizer run disables signed-shift instrumentation for unchanged legacy
N64 packing operations. LeakSanitizer cannot scan `/proc` in this managed runner,
so its thread scan is disabled; the transition fixture separately verifies every
allocation and free. Existing MM controller macro redefinitions and unrelated
stubs warnings are emitted by the production headers/sources.

## Native writer audit

The scoped changes cover all live packed matrix writers found in `mm/src`,
`mm/2s2h`, `mm/mods` and the tracked `mm/expansions` sources: `gu_pc.c`,
`mtxuty-cvt.c`, `su_mtx.c`, and the `sys_matrix.c` static initializer. General and
skin conversions already funnel through the gu boundary. `src/libultra` copies
are excluded from MM's CMake source list.

`z_fbdemo.c` was the only native-built segmented vertex command found computing
an offset with `sizeof(Vtx)`. Wide mode now uses per-frame native pointers.
Skin/SSBB/cape/pause/interface/NameTag and effect buffers use typed runtime vertex
fields, typed element pointers and `sizeof(Vtx)` allocations/copies. SSBB's
authored segment-8 vertex commands still contain legacy 16-byte offsets and
therefore depend on the shared renderer's legacy-offset remapping.

MM's unchanged `SOH::Array` v0 factory reads signed 16-bit positions field by
field into runtime `Fast::F3DVtx`; `Array::GetPointerSize` reports runtime vertex
bytes. Its BenPort bridges and complete scalar-reader handling remain intact.
The renderer must preserve the 16-byte resource-record fallback for these MM
arrays and convert archived fixed-point matrix resources when loading them.

No additional MM source adapters were identified by this audit. These are native
source/ABI checks, not a full executable build or configured runtime acceptance.
