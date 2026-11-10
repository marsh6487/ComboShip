# Authored Hylian Clock get-item

This candidate is original deterministic geometry and procedural material art. No ROM, donor mesh, generated image, or icon extrusion is used. Both hosts contain exactly the same nineteen resources beneath `objects/object_nei_clock/`. The fourteen accepted body resources from `5cd05ecf71a05e80030a199a58974a1184641ed3` remain byte for byte unchanged; five added resources provide a shallow front lens and a native scrolling reflection. `body-baseline.json` pins the accepted body hashes and metrics independently of regeneration.

## Reproduce and inspect

Run from the repository root with Python 3, NumPy and Pillow:

```sh
python -B tools/authored_clock/build.py
python -B tools/authored_clock/build.py --check
python -B tools/authored_clock/verify.py
python -B tools/authored_clock/preview.py --output /absolute/path/outside/source/clock-previews
```

The preview command generates only static PNGs, hashes and an evidence record. It requires an explicit destination outside the checkout: preview binaries are excluded from the source handoff. The sheet shows the accepted body alongside three glass scroll phases, matched body/glass depth views, the finished reverse, and 96px/64px readability samples. There are no GIFs, videos or animation outputs.

The tool shares stable mesh, native serialization and texture helpers from `tools/authored_door_keys/build.py`. Its Clock-local `clock_render.py` extends the existing door-key material evaluator without changing that shared tool. It evaluates the actual unlit glass combiner, authored UVs, runtime tile offset, alpha texels and translucent depth policy. No preview-only reflection or extra geometry is added. Preserve the referenced repository tools when restoring this source.

## Model interface

- Opaque entry point: `objects/object_nei_clock/gNeiClockDL`.
- Glass texture preparation: `objects/object_nei_clock/gNeiClockGlassTexDL`.
- Glass translucent draw: `objects/object_nei_clock/gNeiClockGlassDL`.
- **Required paired interface:** load `GlassTexDL`, set tile 0's scroll offset, then draw `GlassDL`. The draw entry point deliberately does not reload the texture or reset tile size. A renderer calling it alone has not satisfied its material dependency.
- No dynamic segment dependencies. The host supplies a direct `gDPSetTileSizeLerp` command between the two glass entry points.
- Dial centered at X/Y origin, front facing +Z; suspension loop above +Y.
- Accepted body bounds: `[-403,-403,-112]` through `[403,601,147]`. The lens changes only the front bound to Z170; body geometry and silhouette are preserved.
- Suggested uniform GI scale: `0.04`, yielding 32.24 width × 40.16 height × 11.28 total depth with the lens.
- Accepted body: 2,532 unique triangles; 4,068 submitted triangles, including 1,536 brass triangles drawn twice.
- Lens: 120 triangles. Combined: 2,652 unique / 4,188 submitted triangles.
- Body groups remain brass 1,536; blue enamel 728; ivory 172; dark minute divisions 96.
- 30 vertices maximum per cache load, valid signed 16-bit positions/UVs and signed byte normals.

The accepted case, enamel waist, ivory dial, raised hands/hour lozenges, minute divisions, sun/crescent inset, crown, open suspension ring and finished rear are unchanged. The added lens has 24 sectors and radial rings at 0/120/240/335 native units. Its shallow dome runs from Z170 at the center to Z106 at the edge, inside the bezel's inner lip. The actual serialized lens planes clear the raised hand surfaces by at least 6.628342 native units. Narrow inlay faces and the lens intentionally omit hidden backs; this is a render mesh, not a watertight collision or manufacturing mesh.

## Native materials and state

Brass first draws an opaque normal-generated reflection map with native primitive/environment interpolation and lighting. Its exact same geometry then draws a static UV weathering coat using native XLU decal mode. The blue enamel, ivory and minute divisions draw opaque textures. A 16×16 RGBA16 reflection map uses 512 bytes; each 32×32 RGBA32 material uses exactly 4 KiB. Only one texture is loaded at a time. All accepted body art remains fixed on the object.

The glass uses a 32×32 RGBA32 cool-white raised-cosine alpha ramp. Peak alpha is 70/255; 71.875% of its texels have exactly zero alpha, so there is no uniform face haze. The diagonal direction comes from the actual authored UVs. The native two-cycle combiner multiplies TEXEL0 by PRIMITIVE for both color and alpha, with lighting disabled; the preview evaluates that same material rather than adding a specular effect. Back culling keeps the reflection on the front, and `G_RM_AA_ZB_XLU_SURF2` tests opaque depth without writing depth.

Tile 0 wraps in S/T with mask 5 and a 32×32 span. The runtime uses `uint32(gameplayFrames) & 127` as the X offset in quarter-texel units, interpolating toward `phase + 1`; Y remains zero. The full texture repeats every 128 gameplay ticks. The static sheet evaluates offsets 96, 0 and 32. Glass is intrinsic to the model and remains drawn when optional ambient NEI shimmer is disabled.

The body root retains its original state setup and cleanup. The glass draw root establishes its own two-cycle unlit material state, clears fog/front culling/texgen, and sets back culling. Exit disables texture/texgen and restores lighting, white primitive/environment colors and opaque render mode. Host draw wrappers remain responsible for restoring inherited render state and handling optional NEI effects.

## Evidence limits

`verify.py` checks regenerated bytes against both hosts and the manifest; accepted body hashes against the pinned baseline; native texture headers/TMEM; declared paired-entrypoint dependency closure; cache batches and triangle indices; signed ranges; nondegenerate triangles and outward normals; separate body/lens counts and bounds; low alpha and transparent clear area; front culling/material/depth policy; and root exit state. It computes clearance from intersections of actual serialized body/lens triangle planes, verifies that changing the native tile origin moves the sampled reflection and wraps seamlessly at 128 quarter texels, and checks pixel-identical reverse output against the body-only preview.

The previews are **static resource/material evaluations**. Their canonical look-at axes, diffuse lights, rasterization, coverage, filtering and coplanar depth rule are offline approximations. They do not show optional NEI shimmer or prove GPU behavior, host route selection, Alt/base resource resolution, native/original/combined configurations, playback cadence or effects-off preservation. Those require host integration checks and configured runtime review. No runtime acceptance, installation package, publication or master promotion follows from the preview alone.
