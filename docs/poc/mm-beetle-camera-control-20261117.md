# MM Beetle camera and horizontal controls

Baseline: `d3cf01741084c188eb0a01a263018fa3de23fbe1`; additive unpublished bugfix candidate.
The user reports a failed camera return in the Swamp Skulltula House as adult
Link and camera glitches as child Link, plus horizontal steering opposite OoT.

Confirmed source defects:

- Launch always waited MAIN_CAM and teardown always activated MAIN_CAM, losing
  an already active native subcamera. Native camera creation returns CAM_ID_NONE
  when slots are exhausted; that ID aliases the active camera in status APIs,
  but the old path used it without checking allocation success.
- Numeric subcamera handles were not scoped to a PlayState/Camera lifetime.
  Native slot reuse could cause later Beetle cleanup to clear someone else's
  camera. Item-action initialization also discarded a live handle, and the
  existing Player initialization/destruction reset did not reset the Beetle.
- Flight advertised a thrown Zora boomerang and assigned Link as its target.
  The native player camera selector consequently changed MAIN_CAM to
  FOLLOWBOOMERANG while the Beetle view was active.
- The actual MM flight helper added stick X to yaw; OoT subtracts the same input.
  This reverses horizontal flight with equivalent inversion/mirrored settings.

The candidate owns its allocated Camera pointer plus native unique `uid`, captures
and returns to the previously active native camera, validates slot availability,
and retires only its own camera. It preserves a newer cutscene camera's priority
and never writes to a recycled slot. Losing camera ownership, a cutscene or a
transition cancels piloting. Native Player initialization/destruction and item
reinitialization reset the transient Beetle camera/collider state without
reading an old PlayState or grabbed actor. Main/native camera settings and targets
are not force-reset on return.

The existing native `CustomItems_BlocksMovement` bridge now holds Link during
piloted flight. Autonomous flight and return leave control with Link. The fake
Zora boomerang flag/self-target are no longer created, so the item cannot force
the main camera through that native boomerang path. Existing leaf/shovel movement
rules remain. Horizontal flight now follows OoT's sign while retaining MM's
own invert-axis and mirrored-world options. No OoT source or assets changed.

Verification:

`python3 -B tests/mm_beetle_camera/run_tests.py` passes ASan/UBSan with actual
MM game structs and unchanged native Play camera create/get/status/clear bodies.
Camera initialization/status and collider teardown are explicit service boundaries.
The fixture checks return from a native subcamera without changing its settings,
mode or target; exhausted slots; a newer cutscene; slot/address UID reuse; a
changed return-camera UID; player replacement; and fresh/destroyed Play contexts.
It compiles the actual movement predicate and verifies piloted versus autonomous
and returning control, plus preservation of leaf/shovel locks.

The same fixture against the immutable baseline (`--ref d3cf01741084c188eb0a01a263018fa3de23fbe1`)
compiles the old camera functions and fails its native-camera-return assertion.
Both actual OoT/MM steering helper bodies are compiled and agree for left/right,
up/down/diagonals, both invert axes and mirrored-world combinations. The complete
MM Beetle item translation unit passes real-header C syntax checks with and
without COMBO_BUILD using the native unity-build prelude.

Not established: a full executable build, exact Skulltula House replay,
controller feel or adult/child configured runtime proof. The camera/item path
has no age-specific condition, but these component tests do not substitute for
both reported gameplay configurations. No publication or acceptance is claimed.
