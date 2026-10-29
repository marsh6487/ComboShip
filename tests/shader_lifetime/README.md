Run `python3 tests/shader_lifetime/run_tests.py --sanitize` from the repository.
The native variant omits `--sanitize`. A GNU-compatible C++20 compiler/linker and
the repository's TinyXML2 sources are required; `--tinyxml2-root` can override
their location. The configured `build-cmake`/`build` ImGui dependency is discovered
automatically, or can be supplied with `--imgui-root`.
The fixture supplies the pointer-valued `ImTextureID` configuration from the
production `imgui-fixes-and-config.patch`, so raw pinned and patched ImGui
dependency trees exercise the same engine interface.

The fixture compiles the real XML display-list factory and destructor with the
production shader push/pop/lookup functions and actual interpreter containers.
Only the GPU-free interpreter construction, address resolution, resource-name
signature boundary, and logging are supplied by the fixture.

It exercises XML document teardown, display-list teardown followed by persistent
cache lookup/deduplication, interpreter retirement, rejected pushes followed by
pops, missing/empty XML attributes, and an injected shader-copy malloc failure.
The original borrowed XML pointer and persistent borrowed cache pointer each
reproduced a separate ASan heap-use-after-free before the fixes; the unguarded
pop also violated the empty-stack invariant.
