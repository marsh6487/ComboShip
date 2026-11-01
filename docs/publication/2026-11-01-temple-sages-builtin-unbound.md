# Built-in Temple / Sages Chamber for native Unbound loading

## Candidate scope

This addon extends the cumulative source candidate on
`fix/initialization-shop-receipt-20261031-poc1`, parent
`46227a45ca2f18dea3907b108c7514c0152728ef`. The cumulative public base remains
`628e891c683f5cd5b02a5570b1572ad62d42dd37`, tree
`31d6341a9135b1d3180760118a45b4715282a0e2`. The current startup Blob repair,
MM Black Market receipt repair, and all eight newer companion/enemy commits
remain in the candidate. Accepted masters are unchanged.

The exact final commit, tree, file hashes, patch and bounded recovery bundle
are recorded in the accompanying source checkpoint. This document describes
source/native evidence; Windows gameplay acceptance is still pending.

## Missing boundary

The source already stored 4,096 chamber resources, including the annex
geometry, native artwork, binary room controls, collision, and portal markers.
Ordinary CMake preparation restored them into both port asset trees. However,
Unbound selects `scenes/tokinoma/scene.json` and `rooms/0.json`, and the bundle
contained no corresponding JSON controls. Normal port assets also mount below
the clean ROM/Unbound base. Thus the clean scene's actor/collision controls or
native room display-list root could win over the authored annex resources.

This is a proven source/data-selection mismatch. It explains how different
asset modes could select inconsistent scene data; the reported texture
appearance has not been reproduced in a Windows renderer here.

## Change

The authoring-only `unbound_controls.py` exports the existing binary room and
collision controls into seven native Unbound JSON/bulk resources. It retains
all 13 effective room headers, child/adult Zelda, five route markers, OPA/XLU
roots, object lists, behavior, time, skybox, echo, collision vertices/polygons,
surface flags/audio, cameras and water state. Ordinary builds decode the
committed bounded pool and need no authoring or converter archive.

The canonical scene document patches only collision and room 0. It inherits
the clean base's room 1, spawn points, entrances and story setups. Both modes
have complete room-0 and collision controls. There is deliberately no partial
Alt scene document: exact-path JSON merging cannot inherit canonical story
data into an Alt-only partial document. The preparer retires that exact stale
Alt path if an earlier generated tree contains it.

Preparation derives an index of exactly 4,103 payloads; the index is the
4,104th prepared file in each port tree. All original 4,096 resource payloads,
native texture bindings, native dependency list and collision/audio metadata
are unchanged. The default keeps native OoT artwork and the same geometry in
both modes. The separate visual addon remains optional.

After clean-base setup and scene/text cache eviction, ComboShip mounts a
read-only, exact-member view of its original `soh.o2r`. It runs before optional
supplemental archives. Only indexed chamber paths receive the later priority;
unrelated base resources retain their priority and optional mods can override
the chamber afterward. The view retains its source archive, preserves its
exact source path for native/mod provenance, and does not unload its source.
It uses the explicit OoT owner without changing the active MM Context.

Malformed, incomplete, missing and foreign indices cannot partially mount a
layer. Older port packages without an index retain their previous behavior.
The index is read directly from the first normal port archive. No ROM version
identity is duplicated and repeated mounting is idempotent.

## Verification

The pre-change controls test failed on missing JSON/index data. The archive
precedence regression failed before the scoped layer existed. Further red
checks exposed synthetic source provenance and the partial Alt scene's lost
room-1/story fields; those checks pass after the integration corrections.

Final focused checks:

| Check | Result |
| --- | --- |
| Native JSON controls, all room setups and exact collision roundtrip | 5 passed |
| Existing packaged chamber storage/material/dependency checks | 11 passed |
| Independent native artwork/geometry checks | 4 passed; optional visual-donor case skipped |
| Actual Archive/ArchiveManager and JSON merge methods, ASan/UBSan | 15 core cases plus actual prepared-assets case passed |
| Existing Unbound native/startup suite | 17 passed |
| Original payloads and both prepared trees | All 4,096 originals identical; exact 4,104-file trees |
| Real API header syntax, Combo and original guards | Both passed |
| Production formatting and diff whitespace | Passed |

The archive fixture uses real archive class state/method bodies, binary
readers, hash lookup and JSON merging. Its top ResourceManager, Context and
disk backend are adapters. The Alt selection boundary is an explicit file
probe adapter. It uses the actual prepared payloads and a canonical-only clean
scene fixture, asserting spawn/story data and room 1 survive in both modes.
It is not a full resource factory, cache, renderer or game executable.

The real-header check uses cached engine/third-party headers. A complete
`OTRGlobals.cpp` syntax check was attempted in both modes and stopped at the
missing `SDL2/SDL_net.h` dependency. No replacement SDK header was invented.
The earlier startup repair's two complete translation units remain separately
compiled and verified. Full project linking and a Windows build are unproven.

The optional visual-donor test requires its separately pinned visual input;
it was not supplied for that check. This candidate does not claim that visual
archive's appearance or runtime compatibility has been validated.

## Runtime acceptance and recovery

Build this exact cumulative source with its regenerated normal port assets.
With no external chamber archive, enter as child and adult, traverse the
Temple-to-chamber connection, switch Alt Assets off/on, check floor/wall
collision and native textures, then exercise all five destinations and return
arrivals. Verify the original Temple room, story transitions and cameras.
An optional visual archive should still win when subsequently installed.

Repeat the startup crash's supplied MM-to-OoT Lost Woods handoff and both
Black Market seasonal receipt choices in the rebuilt executable. Those
repairs have source/native evidence but still require this gameplay proof.

The Kakariko dog-path and native-pot-identity archive candidates are separate
asset deliverables. They do not alter this source's loader or randomizer keys.
Their archive hashes, exact field changes and restoration recipes accompany
those candidates. Nothing here promotes an untested archive or executable.
