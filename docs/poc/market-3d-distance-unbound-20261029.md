# Market geometry distance regression after Unbound

## Baseline and evidence

Parent: `22b6569a8f9f13880311609734e6c549f22d7ede`, isolated branch
`candidate/market-distance-unbound-20261029`. Accepted masters stay unchanged.

Input: `!Market Suite FinalMaster - R15.o2r`, SHA256
`b0ee965767c0f332af54ee23ca0f4ec8d4db5b86e24b8e71a6327b799939e281`.
The user's decisive clarification: the missing geometry appears as they approach.
This is distance-dependent visibility; the archive's work has not disappeared.

Static audit: ZIP CRC clean, 7,864 unique members, 3,863 valid XML resources,
including 3,639 display lists. The Entrance day/night boundary-patch vertex
resources and normal/Alt host display lists remain present. Core-scene XML
vertex loads resolve within bundled vertex counts in both modes. No cached
vertex XML operations were found. This does not prove active runtime ownership.

The initial JSON-header hypothesis was not established: this fork has a legacy
scene-routing compatibility fallback. A small native-header diagnostic was
created and independently reviewed, but is superseded by the user's distance
clarification and is NOT part of this fix or required for testing.

## Root cause and change

`Environment_Update` derives the new `lightCtx.zFar` from legacy `fogFar` for
these scenes. R15 uses 2,000 for Market day/night and Entrance night; Entrance
day uses 3,000. `Register3DPreRenderedScenes` runs its OnPlayDrawBegin hook and
sets `view.zFar` and the old fog fields to 12,800. However, `Play_Draw` then
rebuilds the projection using **lightCtx.zFar**, restoring the shorter plane.
Unbound's type-2 room culling also reads that new field.

The hook now extends **lightCtx.zFar** to at least 12,800 and synchronizes the
view with it. An authored Unbound distance larger than 12,800 remains intact.
The existing scene set, checkbox, fog policy, debug bypass, sky selection,
geometry, collision, actors, assets and load order are preserved.

## Verification

`python3 -B tests/graphics/run_3d_background_distance_tests.py -v` compiles and
executes the actual production draw-hook expression, actual following
projection-call expression and actual room-cutoff expression under UBSan.
The untouched baseline fails the projection and 6,000-unit visibility controls.
All six controls pass after the fix: affected scenes, room visibility, an
authored 250,000-unit far plane with a 25-unit near plane, feature disabled,
unrelated scene and environment-debug bypass. CI's Unbound group runs it.

No configured game runtime or new Windows build has been demonstrated.
Final native/related checks are recorded with the recovery package.

## Runtime test and recovery

Build the candidate source normally; use the existing R15 and Djipi packs,
with the same settings and ordering. Keep Alternate Assets and 3D scene
rendering enabled. Compare the same stationary camera from the far end of
Market Entrance and the Market square in day and night; walk closer and away;
exit and re-enter. The boundary geometry should remain visible across the
previous cutoff. Inspect the Temple exterior because it shares the existing
3D-background hook. Runtime success and user acceptance remain pending.

Reverting this isolated source commit restores the prior implementation.
The Market archive is unchanged, and there is no replacement Market archive
or header overlay to install for this repair. Retain the exact input and the
source patch/checkpoint for the next combined executable build.

Additional native-suite attempt was blocked by missing nlohmann/json.hpp in
the local test include paths; it is not recorded as a full-suite pass.
The six focused native UBSan controls pass. No full executable build ran.
