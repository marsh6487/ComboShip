# MM adult Rito foot contact

The portable gate compiles the **actual** C `Player_AdjustSingleLeg` seam and
C-compatible production solver, the actual read-only Rito query, real native MM
structures, matrix code, sine/cosine tables and Euler conversion. It does not run
the game or claim configured visual acceptance.

```sh
python3 -B tests/mm_rito_feet/run_tests.py --sanitize --mode original
python3 -B tests/mm_rito_feet/run_tests.py --sanitize --mode combo
```

The POC applies only to local Human Rito, adult root factor.7036, render scale.01,
render YOffset0, actor XYZ scale.01, normal Rito draw offset-1059 and zero native
root sinking. The real renderer must have neutralized age-root scaling for its
draw. The current native walk/run resource must match one of the seven explicit
native gait names, including free-hand and fighter variants. Flight/bow,
paused/message/cutscene/dead/damage/swim/climb/item poses, peer presentations,
unknown clips, binary headers, child, other scales and offsets retain native
behavior. Only valid **horizontal static scene polygons** are enabled; slopes,
dynamic floors and missing hits keep the original native result.

The original native raycast, leg solver and lava-flame side effect run once. A
private true3D solve then uses actual Rito bone translations and foot geometry to
raise only a sinking gait foot. It preserves foot world orientation and transports
the knee pole. The minimum vertical lift plus a geometry-derived native
quantization allowance must remain reachable. Actual native binang recomposition
must pass complete110-vertex contact, orientation and knee-side checks before the
three leg rotations are committed. Root, other joints, actor physics and camera
focus are unchanged.

The fixture contains actual67-word MM frames for **all seven enabled gait clips**
and both native idle clips. Original source archive identity:
`f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`.
Member/header/packed-frame hashes are retained in `native_gait_fixture.json`.
The deterministic extraction recipe opens the supplied archive read only:

```sh
python3 -B tests/mm_rito_feet/extract_native_clips.py /path/to/mm.o2r --verify
```

The regular gate needs no game archive. It hashes the current46 tracked Rito
skeleton/limb/foot-DL/vertex inputs (working copy, or `git show HEAD` for sparse
files), reparses native flex matrix ownership, and regenerates the exact110
triangle-referenced vertices per foot. It compares the production104-point
foot-owned hull, six shin-owned seam points and authored bone offsets against
that actual geometry, and recomputes the native reference idle minima. An asset
change fails with regeneration guidance; updating hashes alone is insufficient.

Current gate:2,004 actual gait-foot poses across normal/yaw±45°/mixed-axis
perturbations/translated-world controls, plus2,136 exact idle cases. It checks the
complete rendered mixed-owner foot, independent world orientation and knee bend
hemisphere, callback rotation/joint-table consistency, unchanged unrelated/root
bytes, player state, matrix state and native ray/flame call counts. Query and
actual-seam controls cover unsupported modes, binary/null/odd/custom animation
pointers, slopes, dynamic floors and missing hits. Invalid reflection, shear,
singular, unreachable and nonfinite trials cannot partially write a pose.

Earlier scalar/ankle-clearance hypotheses and broader scale/slope experiments are
retained outside this regression gate as diagnostic evidence. Scale.005 and
sloped-plane behavior are not included in the production correction. Runtime
review is still required for the exact executable/packs, idle→walk→run→stop,
left/right planting and bow/flight/landing transitions.
