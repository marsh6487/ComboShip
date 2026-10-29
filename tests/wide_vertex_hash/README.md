# Immutable hash vertex packets

Run `python -B tests/wide_vertex_hash/run_tests.py --sanitize`.
Optional individual cases: `--case offsets|mm|packet|lifetime|bounds|dso`.

The fixture extracts the current production `gfx_vtx_hash_handler_custom` and
links the actual engine `Resource.cpp`, `Vertex.cpp` and MM `Array.cpp` under the
shared wide runtime GBI ABI. It checks:

- Serialized Fast vertex records of 16 and 22 bytes, including offsets above
  one MiB and the legacy 16-byte offset versus 24-byte runtime stride.
- Real MM vertex arrays, with bounds from the actual vector even when the
  declared array count is stale, and rejection of scalar/vector/untyped arrays.
- Unchanged command, hash and trailing packet words after every accepted or
  rejected execution.
- Resource replacement within one owner and owner replacement between executions,
  expiration of the retired objects, and submission of the newly selected data.
- Count/destination checks, missing owners/resources, nonvertex provenance,
  unknown record strides, misaligned/out-of-range serialized offsets, partial
  runtime records and null payloads.

The default run also builds two real ELF shared libraries and a separate host.
The engine library links `Resource.cpp`/`Vertex.cpp` with default visibility; the
MM plugin links `Array.cpp` with `-fvisibility=hidden -fvisibility-inlines-hidden`
and exports its fixture factories through the real `COMBO_EXPORT` macro. The
unlinked host opens both with `RTLD_NOW | RTLD_GLOBAL`, then checks engine-side
`dynamic_cast` and virtual dispatch on MM vertex/scalar resources and destruction
through engine `IResource*`. ELF symbol checks and distinct `type_info` addresses
verify that the MM interface RTTI really is hidden. Run only this proof with
`python -B tests/wide_vertex_hash/run_dso_tests.py [--sanitize]`.

The ResourceManager cache/selection boundary and vertex submission are explicit
test doubles. These checks exercise the native resource implementations and
production handler; they do not execute archive factories, the complete renderer,
configured Alt archive loading, a graphics backend or game runtime.
The shared-library proof covers the native Linux compiler/C++ runtime used by the
fixture; it does not establish Windows/macOS or another C++ runtime's RTTI rules.
LeakSanitizer is disabled because the managed environment denies its process
inspection; ASan/UBSan and explicit retired-resource/owner lifetime checks run.
