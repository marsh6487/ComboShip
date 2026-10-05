# GI polish reconstruction, October 4

Candidate: `poc/reconstruct-gi-20261004`, based on recovered header checkpoint
`d1dfbc7b` and published baseline `1c29c83f`. Reconstructed host source is new
implementation, not byte-for-byte recovery. No authored or held asset bytes
change. Native forms, player/mod selection, save layouts and grants are outside
this probe.

## Checkpoint 1: model-independent shimmer identity

The OoT producer now emits `neiShimmer` and live appearance metadata before
archive/model selection. Declining the authored mesh leaves its local energy
unset. Native MM selects the same identity for its fallback overlay, including
rods, spells, Slate powers, Byrna and Four Sword; the fallback restores the
incoming GI matrix before sampling. Mario's legacy drawer still owns its sole
mandatory mask overlay.

Verification:

- `run_nei_identity_tests.py`: production owner descriptor failed on discarded
  identity, then passed 17 themes through authored/mod/selected/missing paths
  with effects off/on. Native MM descriptor failed on discarded identity, then
  passed 19 exact sampler themes through owner/native model selection.
- `run_nei_gi_tests.py`: native production renderer, serialized shelf checks and
  common/shop/overhead C translation units passed using actual engine headers.
- Real `mm/2s2h/Rando/NeiGiPresentation.cpp` C++ syntax check passed. Existing
  engine controller macro redefinition warnings remain.

The recovered foreign recipe consumers still require the missing shop fitting
and themed song implementation. Full candidate build, native/foreign in-game
appearance, Alt parity and buffer headroom remain unproven.

## Checkpoint 2: serialized catalog fitting

`NeiGiFrameBounds.inc` derives all 61 bundled models plus optional Cojiro's
metadata from actual opaque/translucent vertices and N64 scale matrices. The
same host correction encloses body, energy and shell in native/foreign space.
Accepted sizes stay when they fit; only overflow changes scale/pivot. Held
geometry, authoring exports and materials remain untouched. The incoming caller
still owns actor/pickup/world scale. Foreign shop recipes reject an identity
that mismatches the catalog mesh.

The new Four Sword upper-edge assertion failed before the fit: its old shelf
pose reached world Y=20.43 despite passing a height-only check. The corrected
pose reaches Y=19, clears the shelf by 3.11 and retains its .85 shelf scale.
All 61 model bounds passed the real shared renderer under native/OoT/MM routes
and pickup/shop/freestanding caller matrices. Transparent cores and shells share
their fit and every draw restores its CPU matrix. The native suite and real MM
presentation translation unit passed again. Eight occupied potion-shop slots
use 72,784 vertex bytes and 2,075 XLU commands in the capture fixture.

Still outstanding: themed songs, selected standalone sword orientation and full
foreign dispatch/sanitizer verification. No in-game acceptance is claimed.


Themed song checkpoint restores `NeiGi_DrawSongOverlay` for all 24 recovered
profiles. Soaring uses six curved triangulated feathers with vane, barb and
rachis geometry (1296 vertices), rather than recolored generic shimmer. Healing
uses hearts; Time/Double/Inverted use clocks; Epona uses horseshoes; Sonata/Saria
and Minuet leaves, Nova/Serenade ripples, Sun/Prelude light, and the other songs
retain their distinct recovered palette. Storms remains weather-only. The native
MM early dispatcher resolves exact song identity before shared table aliases.
OoT native IDs and imported/progressive aliases resolve the same profiles.

RED: native Soaring renderer emitted the generic colored shimmer; MM early
production dispatcher declined a native song. GREEN: `run_song_gi_tests.py`
executes real MM DrawSong + early dispatch for every native/imported mapped song,
checks all profile bounded/finite geometry at wraparound frames, exact recovered
colors and actual feather geometry. `run_nei_gi_tests.py` and `--combo` pass the
real renderer, all equipment fits, owner descriptors and both foreign dispatch
paths. Real MM `NeiGiPresentation.cpp` and `DrawItem.cpp` syntax pass, with the
latter using the production dependency headers `nlohmann/json.hpp` and
`ship/window/gui/GuiWindow.h` forced in for the standalone compile.


Sword checkpoint removes the native Four Sword helper's extra X quarter turn,
restores native selected standalone Kokiri/Master/longsword submission with one
Z=1.8 rotation, and uses the same protected Din priority as the recovered
exporter. True Master Sword's existing RGB(120,180,255) flame is retained as a
separate helper, with no geometry tint or authored-mesh particles on the
selected sword. `run_sword_pose_tests.py` transforms a +X donor blade through
actual native and exporter bodies: RED on zero upward component, GREEN at
>0.95 normalized Y across six spins. The native selected-path fixture also
failed before direct selected submission was restored.

`run_nei_gi_tests.py --combo`, `run_sword_fallback_tests.py --sanitize`, and
`run_mm_static_gi_bridge_tests.py --sanitize` pass. Preservation fixtures
`run_fairy_bottle_tests.py`, `run_boss_soul_model_tests.py` and
`run_morpha_tentacle_tests.py` pass: partial shell priority, bouncing pink fairy,
exactly one pink shimmer, native boss flame, and no live Morpha tentacle resource
requests/submissions. Old isolated fixtures were brought up to the exact
restored ABI/sword operation count; their production bodies remain unmodified.


Direct redesigned-path mod overrides are now classified as external geometry.
If either opaque or translucent pass is selected from a mod, native OoT and the
owner descriptor retain that selected mesh and independent identity shimmer,
but withhold authored fitting, model-local energy and upgrade flame. The
owner emits `CUSTOM_GI`; native MM submits the same raw passes through
`NeiGi_DrawExternalPresentation`. The recovered foreign custom dispatch already
uses this kind without authored effects. Mario's mandatory shimmer remains
exactly once, including this selected-mod route.

RED: the actual native renderer attached authored energy to an arbitrary mod
replacing a redesigned GI pass. GREEN: every bound model's OPA and available
XLU overrides pass both base/Alt and effects off/on in native/owner/MM renderer
fixtures; no generated fit or local effects, exact separate identity metadata.
Native and `--combo` renderer suites, identity fixtures and real MM syntax pass.


Renderer validation checkpoint rejects incomplete/oversized triangle counts,
non-finite and unrepresentable position/UV values, absent graphics contexts,
invalid scales/centers, and unknown/mismatched authored paths before allocation
or GPU commands. IEEE-bit checks remain effective under fast-math. RED: one
vertex was submitted as a full triangle. GREEN: the actual native/MM/foreign
renderer fixture passes `--combo --sanitize` and `--combo --sanitize --fast-math`,
including the maximum 1536-vertex mesh, all fits, paths, owner scopes and effects.
Acquisition fixtures now initialize the simulated N64 object segment rather
than passing null through the legacy VIRTUAL_TO_PHYSICAL macro.

Independent review found the selected-sword producer returned before writing
identity. `CwAltSwordGi` now emits mandatory per-award `neiShimmer` for all seven
concrete sword themes, preserving selected/Din geometry, protected priority and
True Master flame fields. Full producer+resolver tests reproduced the loss first,
then passed for both selection modes; sanitized actual foreign renderer dispatch
receives each exact identity sampler and no generic overlay.
