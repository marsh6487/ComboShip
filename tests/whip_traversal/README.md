# Whip traversal regression fixtures

Run `python tests/whip_traversal/run_tests.py --sanitize`. Sanitizers are always
enabled; the optional flag also matches the repository CI invocation. The runner
compiles MM and OoT with and without `COMBO_BUILD`, using each engine's actual
native structs. Run through `scripts/ci/native_test_abi.py`'s compiler adapters
for the combined game's `GBI_FLOAT_MTX` / `GBI_S32_VTX` ABI.

The fixture executes verbatim production `WhipStateEquip`,
`WhipStateExtending`, actor/enemy detection and responses,
`Grapple_AnalyzeSurface` and its geometric helpers, the complete reticle section
of `CustomItems_OverrideDraw`, `FirstPerson_DrawReticle`, and the fallback snake
rope renderer. Unrelated meshes are omitted from the extracted dispatcher.
Graphics, camera entry, audio, pose setup and collision queries are engine
boundaries; the fixture supplies a thin beam, a broad wall, and actor lists.
It does not replace the surface classifier or the reticle readiness logic.

Checks cover native MM equip aim with a zero legacy timer; hidden, inactive,
released and unready aim; preserved legacy readiness; a marker clipped to 1040;
an eligible beam at 900 and exactly 1040; rejection beyond 1040 and of broad
walls; torch grips; Keese stun/pull and disarm responses; visual segment density;
and preservation of the different focus-origin marker and hand-origin lash.
The marker and lash have the same distance budget and manual aim angles, but
their origin offsets remain as in the baseline. A red marker describes aim,
not a promise of attachment. Z-target flight still homes toward the target.

`--flight-only` runs the flight/geometry/combat/visual checks independently,
which allows the original 520-unit flight and original 40-segment fallback to
fail even while the missing native reticle is still broken. The first failing
reticle, reach and visual runs are retained separately in the handoff evidence.

The behavioral MM fixture retains the same `linearVelocity` to `speedXZ`
compatibility seam as existing MM item fixtures. It independently exercises the
selected production behaviors and does not substitute for native compilation.

The runner also compiles each complete production `z_player.c` into a native C
object, for OoT and MM with and without `COMBO_BUILD`, using the combined game's
`GBI_FLOAT_MTX` / `GBI_S32_VTX` ABI and MM's CMake C precompiled header order.
Run that check alone with `python3 -B tests/whip_traversal/run_native_compile.py`.
It supplies no player field aliases or generated headers and retains the actual
camera helper → common initializer → object code → item logic include order.
This catches the original early `item_whip.h` include: its state aliases rewrite
later designated fields and member accesses into invalid nested member names.
Camera helpers now consume only the pure state/reach constants header and read
explicit state fields; aliases remain in the later item implementation header.
Existing native warnings are counted in the pass output. These object builds
prove full translation unit compilation, not linking or configured runtime.

No locally retained native Woodfall collision archive was available. The
authored poisonous-water crossing, configured assets, actual graphics and
camera behavior require a game run. The 136-unit fixed swing, release momentum
and attachment rules are unchanged; the existing `tests/nei_whip` suite retains
the aimed lash-pose regression control.
