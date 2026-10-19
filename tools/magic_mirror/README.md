# Native Magic Mirror visuals

The gold oval handheld mirror is generated from `combo/gameplay/MagicMirrorGeometry.h`.
Both engines and the software preview use this geometry. There are no downloaded
images, Time Gate models, actor resources, texture lookups, or external art inputs.

## Native interface

Include `combo/gameplay/MagicMirrorDraw.inc.h` after the native `global.h`.
MM defines `COMBO_MAGIC_MIRROR_MM` before including it; OoT leaves it undefined.
Call `ComboMagicMirrorVisual::Draw(play)` after setting the current object matrix.
The function preserves the caller's CPU matrix and reloads that matrix into both
display streams, then restores the native opaque/translucent setup state. Before
allocation it verifies space for the complete draw: aligned data, opaque and
translucent commands, and the overlay scratch cells used by native debug brackets.
Insufficient room queues nothing and leaves native arena cursors untouched.

The object is approximately **25.4 units wide × 49.3 units tall × 3.25 units deep**:
X −12.7..12.7, Y −27.8..21.5, Z −1.2..2.05. The oval center is at Y=3. Caller
scale 1 uses these ordinary game units; the draw function internally uses a
1/16-unit vertex precision to retain slender bevels and highlights.

The mesh has 950 vertices, 860 triangles, and 90 vertex batches (at most 16
vertices per batch). The gold rim, handle, and emerald accents use the opaque
pass. The recessed double-sided reflecting pane and confined shimmer use the
translucent pass. The 960-frame color/depth ripple is deterministic and wraps
exactly; there is no positional bobbing or rotation inside the visual renderer.

## Rebuild the native texture files

Requirements: a C++17 compiler, Python, NumPy, Pillow, and the existing bundled
`soh/assets/custom/fonts/CenturyGothicBold.ttf`. Preview captions use the runtime's
DejaVu Sans font.

```sh
python tools/magic_mirror/generate.py
python tools/magic_mirror/generate.py --preview-dir /absolute/scratch/preview
```

This writes identical 32×32 RGBA32 inventory icons and identical 128×16 IA4-exact
`Magic Mirror` name textures to both native asset trees. Name rendering imports
the existing item-name generator without altering its item list. Registering
these texture identities in asset headers/archive recipes belongs to the native
inventory integration.

The optional front PNG, contact sheet, and animated GIF rasterize the actual
mesh after the same 1/16-unit native quantization. Their object geometry, colors,
alpha, and animation phase match the shared builder. They are software previews;
configured game rendering and get-item scale still require runtime verification.

## Verify

```sh
python -m unittest discover -s tools/magic_mirror -v
python tools/magic_mirror/run_draw_tests.py
```

The standalone C++ test checks all 960 animation phases plus the wrap boundary,
large frame values, finite coordinates, scale bounds, deterministic results,
local triangle indices, nondegenerate triangles after native quantization, opaque
metal, and translucent/transparent shimmer. Asset tests check identical game
copies, native dimensions, icon coverage, and exactly representable IA4 names.

The native draw fixture requires the engine's external include dependencies
(`nlohmann/json.hpp` and spdlog, supplied through `CPATH` when needed). It compiles
both real native `GraphicsContext`/GBI headers and executes verbatim production
allocator, setup, and debug bracket bodies. Guarded storage proves rejection
before any mutation when data, OPA commands, XLU commands, or overlay scratch are
short, and proves the exact accepted boundary stays in all three arenas. The
current draw needs 15,328 aligned data bytes, 336 opaque commands, 229 translucent
commands, and two overlay scratch commands. Optional sanitizer flags can be
passed with `MIRROR_TEST_CXXFLAGS`; the draw fixture also links the real C-named
interpolation boundary, so namespace/linkage regressions fail at link time.
