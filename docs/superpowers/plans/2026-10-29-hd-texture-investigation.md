# Renderer investigation handoff

Spec: `docs/superpowers/specs/2026-10-29-hd-texture-investigation-design.md`.

- [x] Recover all four morning scene/model RenderFlight captures and distinguish Alt states, cold work and static-only archive results.
- [x] Trace the actual vendored texture lifecycle and exact pinned upstream differences.
- [x] Preserve the combined source baseline; reproduce and separately restore the native MM loader ABI compile regression.
- [x] Remove the experimental new diagnostic probe completely.
- [x] Provide concrete renderer changes, priorities, code locations and correctness requirements in `docs/poc/renderer-investigation-20261029.md`.
- [ ] Implement an actual renderer optimization candidate from those proposals; do not substitute new instrumentation.
- [ ] Verify identical ordered geometry/material output and compile affected real backends before gameplay acceptance.

Current renderer code is unchanged from source parent `1b3f16dc`. Local loader repair is `029bcfc6`. The proposal handoff does not claim a built or runtime-proven performance candidate.
