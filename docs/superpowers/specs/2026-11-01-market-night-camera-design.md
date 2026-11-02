# Market night transition camera crash

## Intent and evidence

Prevent the supplied nighttime back-alley crash while preserving the accepted
scene packs, geometry, camera behavior, and progression. Investigate the missing
Spirit Temple key separately; no save editing is justified without the save.

The uploaded log identifies build `c8cebeaf1d861549adf0d23624509768d7da9b10`,
tree `b42f00013cddb567cdf134fee6c469c879f36e7b`. The crash dump records a read
from `0x10000000e` at `soh.dll+0xd63e73`. Its captured instructions match the
inlined background camera lookup in `func_80096A74`, called unconditionally by
`func_80096B6C`. The active camera setting is `0x1c` (`CAM_SET_DOOR0`), with
camera index 3. The room renderer draws image backgrounds only for setting
`0x19` (`CAM_SET_PREREND_FIXED`), yet performs this lookup first for every camera.

This is instruction/source correlation, not PDB-backed symbolication. The dump
and log agree on the fault and state. No game runtime has been run here.

## Approach

Implementation choice: preserve the existing background hook invocation, then
interpret image camera metadata only when the active camera is eligible to draw
that image. The entrance-shuffle hook performs camera backup/restoration even
on nonfixed frames, so the guard belongs inside `func_80096A74`, after the hook.
Tolerate no returned image in the caller. Keep the original valid fixed-camera
ID remapping and player-parameter update. Preserve opaque and translucent
geometry submission.

A global collision-camera lookup change would affect unrelated callers and
requires broader validation. Changing the accepted Market archive would modify
assets without evidence that those bytes need replacement. Neither is part of
this candidate. The exact archive is identified in the log but unavailable here.

## Boundaries and verification

- Production scope: `soh/src/code/z_room.c`, multi-image room drawing only.
- Do not alter camera settings, entrances, collision, actors, assets, save data,
  cross-game receipts, the pending cursor fix, or repository publication state.
- Extract and compile the actual room functions with native project types.
- Prove the original fails with inaccessible image metadata under the observed
  door camera; check a free camera and valid fixed-camera remapping as controls.
- Check hook veto and both geometry submission paths under sanitizers.
- Review and restore the exact source delta independently.
- Full Windows build and the user's configured game runtime remain untested.

## Recovery

Use a separate candidate worktree based on the exact source tree. Preserve all
original uploads. A durable delta package will include baseline identity, patch,
tests, evidence, and restoration instructions. This bounded local projection is
not the full upstream history and must never be pushed as replacement history.

Historical publication status: the original November 1 investigation did not
authorize upload. On November 2, cor explicitly requested integrating the fixes
from that chat into the cumulative GitHub source. This repair is now included
in PR #78; the earlier publication block is no longer the current status.
