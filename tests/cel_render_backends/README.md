Cel backend verification covers the production renderer code without loading a game.

Run the native layout, transport, stencil and encoder-lifetime checks:

```sh
python3 tests/cel_render_backends/run_tests.py
```

The runner compiles production code against deterministic GL, D3D11 and Metal API providers. It checks 5,120 packed layouts, including a legacy Prism template that omits the normal; all 40 depth/write/decal/stencil cache combinations on D3D11 and Metal; wrapping two-sided z-fail; the self-clearing nonzero composite; invalid-mode neutralization; GL frame resets and full/regional clears; and Metal color-only versus depth clears across encoder restarts.

Run template expansion against the Prism version pinned by the host's `libultraship/cmake/dependencies/common.cmake` (`KiritoDv/prism-processor`, commit `1de054450e7b3c5f777d2e3dfcb228ad120c329d`):

```sh
python3 tests/cel_render_backends/run_shader_tests.py /path/to/prism-processor /tmp/cel-shaders
python3 tests/cel_render_backends/compile_glsl.py /tmp/cel-shaders
```

Template checks use the production combiner callbacks and contexts. They expand 256 feature combinations per backend, including toon on/off, two textures, clamps, masks, blend textures, fog, grayscale, alpha and primitive depth. Neutral shader output is compared with host commit `badc4e09a5176b5d423926eed21d5d19a67ee87e`, ignoring whitespace/comments and Metal's enlarged uniform declaration. The GLES test compiles and links the generated vertex/fragment pairs using a surfaceless ES 3 context; it needs `libEGL.so.1` and `libGLESv2.so.2`.

These checks passed during integration, including the shader compiler test on Mesa llvmpipe. Full Windows/D3D11 and Apple/Metal builds, driver output, configured OoT/MM transitions, scene/feature toggles and user acceptance remain separate checks. No game runtime or performance result is implied.

Transport/layout decisions:

- Toon uses shader option bit 33, while backend shader caches retain all 64 bits of both IDs.
- Normal3 follows texture/clamp/fog/grayscale fields and precedes combiner inputs. Legacy custom shaders keep their input offsets and existing output; Metal descriptors skip an unconsumed CPU normal.
- D3D11 keeps primitive depth at b2 and puts the 64-byte toon buffer at b3. Metal keeps the primitive-depth offset at byte 24 and starts toon fields at byte 28; compile-time layout assertions accompany the fixed 84-byte buffer.
- D3D11 uses D24S8 with a matching depth read view and restricts MSAA to counts supported by both color and depth/stencil. GL retains its existing depth/stencil attachments. Metal uses separate Stencil8 attachments with matching sample counts and retains stencil between encoder restarts.
- Every backend returns to stencil Off at frame start. Full and regional depth clears also clear stencil. Color-only clears preserve pending stencil volumes.

The Unbound backend delta was inspected separately: its native D3D11 change replaces the selected custom shader path with the default shader, so the host's custom path is preserved. Its only GL change is an Emscripten GLEW guard, outside this native integration scope.
