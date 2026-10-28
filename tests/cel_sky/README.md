# Cel/sky game-side integration checks

Donor runtime: `roborich/Shipwright` `9.2.3-celshade0.11`,
`85ab8273114fc57b40f7a0641d77692311c9625b`. Its runtime matches branch
head `cbcd3e719d2d089a0a30e797ab8878cb40fe9066`; the two alpha-bleed authoring
scripts come from that later head. Cloud PNGs match the donor blob SHAs.

The owned modules/menu retain donor defaults: cel enabled, shadows/light
casting/sky master disabled. The native core/renderer insertions are owned by
the integration controller.

```sh
python3 tests/cel_sky/run_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 tests/cel_sky/run_tests.py --sanitize
python3 tests/cel_sky/run_tests.py --compile-commands /absolute/build/compile_commands.json
```

The first two commands execute actual production function bodies, with only
native state or UI registration supplied by the fixtures. They cover weather
sampling once per frame, final-lightning overcast, noon/night/rain palettes,
wide horizon/vertex/light coordinates, safe optional shadow floor transport,
actual menu registration/defaults/visibility/reset isolation, and cloud override
format/size/lifetime validation. The optional compilation-database gate checks
the eight full new Graphics translation units, new menu translation unit and
current `SohMenu.cpp` using the native build's exact include/define options.
Fixture success is not a substitute for that compile gate or runtime proof.

Changes from the runtime donor:

- Optional signpost floor-clamp transport returns its no-clamp sentinel for
  nonfinite or out-of-s16-range world Y; it never saturates to a different floor.
- Dynamic sky vertex writers use the shared s32 geometry ABI. Texture coordinates
  keep their native s16 format. Held-stick point lights retain f32 world positions.
- Raw cloud overrides require power-of-two RGBA32/image data, valid bounds and a
  complete pixel buffer. Resolved variants retain their resource owners for
  deferred display-list use across archive cache replacement or Alt switches.

## Required native interfaces/insertion points

`ToonLighting.h` supplies C-callable:

- `int ToonLighting_FeaturesActive(void)`
- `int ToonLighting_CelEnabled(void)`
- `int ToonLighting_ShadowsEnabled(void)`
- `int ToonLighting_SuppressVanillaShadows(void)`
- `int ToonLighting_IsShadowReceiver(struct Actor*)`
- `void ToonLighting_LensBracketBegin(struct GraphicsContext*)`
- `void ToonLighting_LensBracketEnd(struct GraphicsContext*)`

`WorldLighting.cpp` supplies
`extern "C" void WorldLighting_ApplyFlameFlicker(LightInfo*, u8*, u8*, u8*)`
for the source-light choke point in `z_lights.c`.
`WWSkyFileSelect.h` supplies
`void WWSky_DrawFileSelect(GraphicsContext*, View*)` for the file-select draw
after its vanilla skybox.

Required GI hook declarations/bridges are `OnActorDraw(void* actor)`,
`OnPlayDrawWorldLights(void* play)`, `OnPlayDrawSkyGradient(void* play)`,
`OnPlayDrawSky(void* play)`, and `OnPlayDrawSkyClouds(void* play)`.

The actor hook runs after native lights have been emitted and before actor
geometry. Toon brackets cover OPA/XLU actor draws, including the separately
bracketed lens pass. OPA shadow capture must arm/disarm at every actor boundary,
including excluded actors and the final actor when cel is off. Receiver prepass
uses the host's complete existing cull/draw/audio/NEI path exactly once per actor;
world light pools and previous-frame shadow flush follow that prepass before
remaining actors draw. Donor's five bespoke shadow suppressions and native
feet/circle suppression use the cached predicate without removing special
telegraphs. Preserve host MM-transform audio, Hourglass grayscale and post-draw
hooks while applying those core edits.

Sky order after native skybox: gradient, stars, clouds/wisps, then native sun/moon
and world. Keep native weather/season control and close graphics state before
seasonal particles, UI and cross-game renders. The new sky modules are OoT
policy; generic renderer transport does not add MM sky/shadow policy.

The shared renderer must provide public `gSPToon`, `gSPToonKey`, `gSPStencil`,
`gSPToonShadow`, `gSPToonShadowArm`, `gSPToonShadowFlush`, and
`TOON_SHADOW_NO_CLAMP`. The controller assigned unused opcodes 0x41/0x4c/0x4e/0x4d
respectively while preserving host scroll/interpolation opcodes.
`Fast::GfxRenderingAPI` needs `SetToonLighting`/`SetToonRamp` and
`Fast::Interpreter` needs
`SetToonShadowParams(float,float,float,float,int,bool)`.

Regenerate/package cloud and shader resources through the host's current
archive builders. Existing accepted archives are inputs and remain unchanged.
