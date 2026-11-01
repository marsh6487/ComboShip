# Built-in Temple / Sages Chamber Implementation Plan

> **For agentic workers:** Use native execution for asset preparation and one delegated, isolated runtime-layer task. Steps use checkbox syntax for tracking.

**Goal:** Ship a coherent, reachable Temple annex in ComboShip without an external chamber archive, with Alt Assets on or off.

**Architecture:** Retain the existing 4,096-resource native-art bundle. Add native Unbound JSON room/collision controls derived from its exact binary controls, then mount an indexed, read-only view of only these port-owned chamber resources after the clean base and before supplemental mods. Keep the legacy room controls for return-pad discovery.

**Tech stack:** Python standard-library asset preparation; existing Unbound JSON/bulk schemas; native C++ Archive/ArchiveManager; current CMake and regression registry.

**Spec:** User's final-addon request in this session plus tools/temple_sages_bundle/README.md (native artwork and optional visual layering).

## Global constraints

- Parent candidate: 46227a45ca2f18dea3907b108c7514c0152728ef; keep its startup and shop repairs.
- Preserve original 4,096 resource payloads, native texture bindings, vertices, triangle routing and collision/audio fields.
- No optional visual archive is required for preparation, building or the default runtime path.
- Keep native room/setup actor records, five portal markers, child/adult Zelda and legacy return discovery.
- Promote only chamber-owned resources; base assets outside this scope and later optional mods retain normal priority.
- Preserve the active MM resource-manager context and Combo/original build guards.
- Record static/native evidence separately from unperformed Windows gameplay checks.

## Review focus

1. Standalone Unbound versus native-ROM conversion startup: same scoped layer order.
2. Alt switching: JSON controls, collision and OPA/XLU graph remain coherent.
3. Story and age/night setups: exact legacy actor/object/time/behavior parity.
4. Optional overrides and malformed/missing indices: normal priority, no partial mount or broader resource promotion.
5. Recovery: same cumulative base, all prior repairs and independently reconstructible new source tree.

### Task 1: Native JSON controls and prepared index

**Files:** tools/temple_sages_bundle/unbound_controls.py; build.py; manifest.json; parts/*; scripts/assets/prepare_temple_sages.py; tests/temple_sages/test_unbound_chamber.py.

**Interfaces:** unbound_controls.apply(resources: dict[str, bytes]) adds canonical scenes/tokinoma/scene.json and canonical/Alt {rooms/0.json,collision.json,collision.bin}. The scene patch inherits canonical base story/spawn setups in either mode. prepare_temple_sages.verified_resources() also supplies custom/temple_sages/builtin_index.json with {version:1,resources:[exact owned paths]}.

- [x] Add failing checks that require native JSON controls, preserve every binary room setup and reconstitute collision geometry/flags/audio/cameras/water.
- [x] Observe the missing-JSON failure on the parent candidate.
- [x] Implement deterministic conversion of only the known binary Temple command families and collision layout; reject unsupported/truncated data.
- [x] Repack the existing pool with only the seven new control resources; verify all old payload hashes remain identical.
- [x] Generate the scoped runtime index during preparation and verify both normal port trees without source archives.
- [x] Run the new tests and existing packaged/native-art material checks; commit owned paths.

### Task 2: Scoped runtime mount

**Files:** soh/soh/unbound/BuiltinTempleSages.h; soh/soh/OTRGlobals.cpp; tests/unbound/archive_startup_fixture.cpp; tests/temple_sages/{run_builtin_layer_tests.py,builtin_layer_test.cpp}.

**Interfaces:** bool SOH::Unbound::MountBuiltinTempleSages(shared_ptr<Ship::ResourceManager>); reads the index directly from the original normal port archive. Calls only under COMBO_BUILD after base/cache eviction and before supplemental archives.

- [x] Observe a failing actual archive precedence regression before implementing the filtered view.
- [x] Implement read-only, source-retaining, idempotent view indexing exact validated chamber members.
- [x] Verify base -> builtin chamber -> optional mod order, unrelated resources, hash access, JSON merges, source lifetime and foreign MM owner.
- [x] Verify missing old index skips cleanly and malformed/incomplete indices cannot partially mount.
- [x] Run startup fixtures and real-API Combo/original header checks; attempt full OTRGlobals.cpp compilation and record its missing SDL_net.h dependency. Commit owned paths.

### Task 3: Combined verification and recovery

**Files:** scripts/ci/regressions.json; tools/temple_sages_bundle/README.md; docs/publication/2026-11-01-temple-sages-builtin-unbound.md; cumulative source recovery archive.

- [x] Integrate Task 2 onto Task 1 without replacing the earlier repairs.
- [x] Register the new scoped regressions and review the final change independently.
- [x] Verify prepared resource closure, canonical/Alt texture graph, room/collision parity and both startup paths.
- [x] Generate an updated recoverable source checkpoint and independently verify patch/tree restoration. Deliver it through the existing checkpoint file's version history.
- [x] Report runtime-untested status and the focused no-external-archive Temple entry, chamber traversal, Alt switch and return-portal acceptance checks.
