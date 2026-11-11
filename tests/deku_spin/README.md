# Deku spin camera isolation

Run `python3 -B tests/deku_spin/run_tests.py`, optionally with `--sanitize`.
Add `--mutation-check` to confirm that realistic regressions are rejected by
behavioral assertions (each mutant must compile and fail its named case).

The runner extracts and compiles the current production spin entry/action,
`MmForm_IsDekuSpinning`, movement-input function, neutral-idle yaw tail,
skeleton draw span, and Deku hat-trail callback. It does not duplicate the
production spin equation. The action/form/state enums and spin fields also
come from production. Fixtures double external math, input, animation,
collision registration, graphics matrices, skeleton traversal, and audio;
the reduced structs are not an engine ABI check.

The cases catch spin leaking into actor/travel yaw, steering counted twice,
OoT idle feeding render yaw back into movement, missing visual rotation,
matrix leaks, wrong draw gating, lost trail rotation, and completion cleanup.
Attack arguments, animation ticking, floor/start sounds, and anti-reversal
behavior are protected at their existing service boundaries.

In-game proof is still required: stationary/moving/turning spins, releasing
the stick, target lock, manual camera, wall proximity, water-hop completion,
damage/detransform interruption, and native MM as an unchanged control.

The pre-fix production source failed stationary, steering, release-stick,
render-trail, draw-gates, and anti-reversal. Existing completion cases passed.
The render-only correction passes all nine cases in normal and ASan/UBSan
builds. The separate form-instrument runner is blocked in this partial source
snapshot by the absent `scripts/diagnostics/run_mask_transformation_tests.py`.
