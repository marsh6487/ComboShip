# Mirror stump scene input

`LostWoods-MirrorStump.prelude.o2r` is cor's unchanged original editable
`1 - lost woods 2ship granny shop and stump moved.prelude.o2r`, retained under a
portable filename. Its Prelude project includes the stump's visual vertex edits
and the matching collision, alongside the Granny shop work from that export.
The complete original bytes and their SHA-256 are preserved.

The current complete scene is the separately retained
`zzzz_MM3D_Lost_Woods_Black_Market_POC14.prelude.o2r` identified in `manifest.json`.
It already contains the same stump and newer Black Market work. Use that complete
scene for combined-mode gameplay. Do not layer this older source edit over it:
the older whole collision would replace later scene improvements. The compiled
scene's existing Library identity and exact hash are part of the recovery manifest.

The source edit is for recovery/editing against its original forest donor. Future
Prelude edits should begin with the latest complete scene and export visual and
collision changes together. Keep one winning forest scene in the MM mod load
order. Solo modes continue using their private forest addon; recall resolves the
stump in whichever forest collision is actually loaded in that mode.

The current cap is centered at `(-377, 80, 416)`; its clear base landing is
`(-377, 0, 296)`, facing the stump. These are verification snapshots, not hardcoded
runtime destinations. Both the pickup and recall use the same loaded 12-triangle
stump cap. Translation/rotation preserve that identity; resizing or duplicating
the stump requires revisiting the locator. Missing/ambiguous geometry or no clear
supported landing retains the scene's ordinary entrance spawn.

```sh
python3 -B tests/magic_mirror/check_scene.py
python3 -B tests/magic_mirror/check_scene.py --compiled /absolute/path/to/zzzz_MM3D_Lost_Woods_Black_Market_POC14.prelude.o2r
```

These checks compare visual and collision placement, execute the production cap
locator on all loaded horizontal faces, and inspect static collision support and
clearance at the current landing. They do not establish configured game rendering,
archive priority, player/camera behavior, or user acceptance.
