# Black Market disabled-seal capture, 2026-11-07

Parent: `2054eef2f12aeedfb2d2d0f5519cc23272227e4e`, candidate branch
`candidate/night-music-dialogue-20261107`. The user's reported severe MM shop
slowdown remains a runtime investigation; this is a bounded, proven redundant
work correction, not a demonstrated cure for that slowdown.

Both hosts created `BlackMarketVisual::Capture(locked)` before item drawing and
checked `gCombo.BlackMarket.AnimatedItemSeals` only afterward. Hiding animated
seals therefore still queried selected model bounds for locked offers. The
generic resource-backed query creates a new `ModelBoundsReader` and walks its
display lists and vertices on every call (`combo/NeiAssetPriorityResource.h`,
`GetGiModelBounds`; `combo/NeiGiModelBounds.h`, `Read`, `Walk`, `Vertices`).

The candidate reads the existing setting once per market draw and enables
capture only for locked offers with seals enabled. Stock geometry, prices,
availability, eight native item positions, native updates, room/shelf rendering,
effects, camera behavior, and enabled-seal fit remain unchanged. No asset edits,
new cache, default-setting changes, or broad renderer changes are included.

## Verification

The existing native lock-draw regression compiles and executes both production
`DrawMarket` and `DrawSeal` bodies with real host GBI packets and native arena
allocation under address/undefined sanitizers. The new assertion failed in both
unmodified host bodies because capture remained active with seals hidden.

After the correction, both hosts pass: all eight offered item identities still
draw with seals off, with zero seal-bounds collection and zero seal allocations.
Turning seals back on for the next draw resumes two locked-item bounds queries
and the existing fitted seal packets. Sold Out, renewable offers, selected-item
positions, state restoration and short-arena guards retain their existing tests.

Commands passed:

```sh
python3 tests/black_market/run_lock_draw_tests.py
python3 tests/black_market/run_frame_capture_tests.py
python3 tests/black_market/run_stock_tests.py
python3 tests/black_market/run_shop_camera_mm_tests.py
```

Local test environment used `CPATH=/workspace/scratch/a725ee441d3d/publication-deps`.
For the existing fixtures' native C pointer conversions, the local CC wrapper
supplied `-Wno-error=int-conversion` and `-Wno-error=incompatible-pointer-types`
to GCC 14. The MM camera fixture's C++ compilation used `-Wno-narrowing`, also
present in the real MM CMake target. No production source or test body was
rewritten for those compiler issues. The stock tests
cover 4,096 inventories plus replacement/persistence policy. No complete game
executable build, configured FPS comparison, or user runtime acceptance is claimed.
The native MM shop/camera suite also passed its real EnOssan/EnGirlA initialization,
all-eight-stock checks, native camera/receipt ownership tests, and all-five-form,
all-eight-selection clearance against 7,577 room and 144 shelf triangles.
Whole production-unit compilation was attempted, but both hosts stopped at the
environment's missing `SDL2/SDL.h` before compiling the shop bodies.

## Other static findings

- MM stock actors deliberately have `draw = nullptr`; `DrawMarket` submits each
  existing native item's custom GI once. No duplicate native/custom draw was
  established. Native EnGirlA and keeper updates preserve their native behavior.
  The conversion from `3dde94f9` to `f7e8338f` retained the same eight-slot GI
  renderer and locked-item capture; it replaced dummy display actors with real
  native stock actors. That conversion did not change the GI model renderers or
  bound readers. Native stock update performs scale/focus/rotation with a no-op
  item action, and native keeper initialization finishes by switching to Idle.
- Room magic's resource wrapper calls `Prepare` each draw, but the prepared
  room graph is reused while owner, selected root, Alt state and path match.
  Its material trace emits only on changed canonical snapshots. Repeated room
  graph copying or per-frame material log flooding was not established.
- Model-bound queries with seals enabled still traverse selected geometry.
  `ResourceManager` exposes per-resource dirty/reload/unload operations, but no
  shared resource-generation counter suitable for a narrow cache was found.
  Caching only by manager, root pointer and Alt state can miss a dirty/unloaded
  child vertex/list whose root remains alive. No speculative cache was added.

The parent source's RenderFlight recorder is wired to OoT, including its
hard-coded `OoT-RenderFlight` filename and scope. MM's `BenPort::RunCommands`
does not begin/end the recorder or emit its render reports. Existing material
and door diagnostics are not MM RenderFlight coverage. Real MM capture needs
its own correctly labelled tick/render integration; this narrow correction
does not imply that coverage has been added.

In coordination with the separate MM recorder integration, this work also adds
`MMFrameTiming::NamedScope` stages for keeper update/draw, the complete custom
market draw, shelves, effects, item GI draws, seals, Granny room and room magic.
The actual new MM adapter supplies these spans; standalone native fixtures use
its real header and disabled bridge functions. These CPU construction stages do
not measure the later GPU execution of the submitted display lists.
