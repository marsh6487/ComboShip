# Native cutscene actor vertex offsets

Goal: repair native MM cutscene actor rendering in the repaired Unbound-integrated ComboShip candidate; extend the shared correction to all consumers.

Baseline: `marsh6487/ComboShip`, `candidate/cel-unbound-smoke-repair-20261029`, `3cbb22d1f09e85deb206d2b085fc62105bc09fad`.

Spec: cor's October 29 request in this conversation: native Anju standing/crying umbrella poses must render without replacements; extend the correction to the other cutscene actors. Performance diagnosis is excluded.

Architecture: carry serialized vertex record size and runtime stride through the existing strict MM graph resolver, then map archive byte offsets to runtime elements. Audit the ordinary actor path against the existing wide-aware renderer. Preserve skeletons, animations, draw routing, actor placements, behavior, texture ownership, and all prior collision/message repairs.

Tech: C++20, native unit tests, Python archive audit, GitHub candidate validation.

## Review focus

- Legacy 16-byte records expanded to 24-byte vertices; nonzero offsets must select exact elements.
- Wide 22-byte archive records, mixed resources, final valid ranges, and malformed metadata.
- Cached nested lists and props must retain the same corrected addressing.
- Shared strict and ordinary native actor paths; no Anju-only model replacement.
- Source/build verification does not establish in-game visual acceptance.

## Task 1: shared resolver correction

- [x] Preserve source and reproduce the offset regression in the production patcher.
- [x] Extend the vertex view with serialized/runtime strides; populate them from the actual resource.
- [x] Convert offset to an element index and validate ranges against runtime storage before patching.
- [x] Verify legacy, wide, mixed, malformed, boundary, and overflow cases under sanitizers.

## Task 2: actor coverage and delivery

- [x] Audit all discovered native vertex packets in catalogued MM object folders and verify both Anju animation lengths against native `mm.o2r`.
- [ ] Run the existing graph/actor regressions where dependencies are available; record any blocked checks.
- [ ] Review the bounded diff, preserve it on a related candidate branch, and trigger platform validation.
- [ ] Deliver source/build results separately from the pending in-game visual test.

Recovery: fetch the exact baseline above and apply the candidate diff; do not promote or overwrite the accepted repaired branch.


## Additional authorized parallel work

- [x] Zelda agent: synchronize cached matrix count in both skeleton-swap paths; seven production traversal cases pass ASan/UBSan.
- [x] Saria agent: authored-only fitted head pivots in ordinary/static Saria, Twins and Fado callbacks; 1,920 checks pass ASan/UBSan.
- [x] Root: preserve and repair accepted Twins copy; 545 hash payload changes, all other asset bytes retained, second pass unchanged.
- [x] Root: update existing ctypes archive diagnostic for the new resolver view; actual Skull Kid packet checks pass.
- [ ] Fresh whole-candidate review and platform validation.

All fixes are combined on `candidate/native-cutscene-vertices-20261029`; user continues smoke testing the unchanged repaired parent.
