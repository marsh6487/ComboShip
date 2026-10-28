# Combined cel-shading, Unbound and collision integration

**Goal:** Deliver one ComboShip candidate containing Rich's cel-shaded presentation, Unbound 9.3 support and expanded OoT actor collision queues. The user explicitly chose one combined integration; comparative merge-performance experiments are outside this task.

**Host:** `marsh6487/ComboShip` cumulative crash-repair candidate `badc4e09a5176b5d423926eed21d5d19a67ee87e`, tree `7cf77119f87361102e9de1464c2c20adab925205`. Preserve all existing OoT/MM/NEI integrations, rendering/material/seasonal changes, game bridges, saves and Keaton/Pictograph repairs. Existing accepted archives remain unchanged.

**Donors:** Rich cel release `9.2.3-celshade0.11` at `85ab8273114fc57b40f7a0641d77692311c9625b`, based on `cb71e22a79bc5d1f688fa881795bbd93094895fc`, with engine `4698a3d6310d90900cec81e7e93a8337f4e6938f` / base `fdcaf6336776d24a6408d016b0a52243f108f250`. Unbound `9.3.0-unbound0.11` at `ace55888e181fabf7dfccd60f30d3f7fb2cef46b`, based on SoH `ecd889c2019b87e78b0a3d942a6a3fe98b7efaed`, with engine `7e96dc7b8265e2a80db91fe9cc005baa54c065ca` and Torch `2ab12fe9660aec04e02ee89fe81baed304a1a1d6`.

## Design decisions

- Port final donor deltas into the current host; preserve the host's vendored engine, Prism/shader stack, custom graphics opcodes, cross-resource-manager ownership and existing fixes.
- Use one float-Mtx/s32-Vtx runtime ABI across the shared engine and both game modules, with native MM writer adapters and legacy resource readers. Cross-game models can appear in the same frame, making a global active-game format switch unsafe.
- Preserve legacy authored scene-header mods: Unbound's JSON scene routing must not silently bypass existing actor/setup/collision overrides. Use explicit compatible routing/structured-layer conversion while leaving original input archives untouched.
- Keep cel policy and sky hooks in OoT. Shared backend transport must return to neutral state for MM, menus and existing effects. Preserve Rich's release defaults.
- Allocate unused host opcodes for cel transport: Toon `0x4c`, ToonKey `0x4d`, ToonShadow `0x4e`, Stencil `0x4f`. Preserve Unbound float fog `0x41` and existing `0x46`, `0x4a`, `0x4b` commands. Verify uniqueness across producers and interpreters.
- Replace OoT's fixed AT50/AC60/OC50 admission ceilings with growable queues, preserving native order, damage/contact policies, frame-advance/second-update guards, Champion/Din/TimeCtl behavior and proper teardown. The donor's uncapped static geometry does not expand these queues.
- Preserve runtime proof as distinct from source/native/build checks. Test correctness and feature preservation in this combined candidate; do not make a comparative performance study a prerequisite.

## Owned work and progress

- [x] Pin host and donor references; create isolated branch and remote baseline checkpoint `integration/cel-unbound-collision-20261028`.
- [x] Independent Rich and Unbound donor audits, with exact pins, changed paths and compatibility constraints.
- [ ] Shared engine renderer/GBI, backend/shader transport, float/s32 ABI and build/archive wiring (root).
- [ ] Unbound OoT native structs, factories, registry, scene and text systems, preserving host seams (Unbound worker).
- [ ] New cel/sky/light policy modules, menu and cloud resources (cel worker).
- [ ] MM native matrix/vertex producer compatibility (MM worker).
- [ ] Host NEI/custom geometry/dynamic-collision consumers and shared OOT/MM guards (custom-collision worker).
- [ ] Growable OoT actor queues with native admission/contact/damage and teardown tests (queue worker after header handoff).
- [ ] Central game hooks, OTR/SaveManager/legacy archive compatibility, recovered resource capture, package/build integration (root).
- [ ] Focused native tests, full build checks, independent final preservation review and recoverable source checkpoint; one combined candidate/PR.

## Recovery and acceptance

The initial host checkpoint is already on GitHub and does not trigger the develop-only push build. Commit and publish bounded source milestones before long builds or handoff; local scratch alone is insufficient. No accepted master is promoted by source/build checks. Record exact source trees, tests, unresolved issues and runtime requirements in the final handoff.

## Review focus

Legacy scene archives and Alt paths remain effective; MM and foreign models use the correct common resource ABI; native pickup and randomizer progression remain intact; no cel opcode/shader state corrupts existing materials; all queues and dynamic collision consumers respect their actual capacities and lifetimes.
