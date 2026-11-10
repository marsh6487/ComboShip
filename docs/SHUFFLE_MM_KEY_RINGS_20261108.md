# MM key rings — WIP integration and recovery

Four append-only native MM rings replace the selected dungeon's complete small-
key supply: Woodfall 1, Snowhead 3, Great Bay 1, Stone Tower 4. A default-off
bitmask option preserves the selected dungeons. Vanilla key placement retains
vanilla keys; Start With computes selected rings instead of individual keys.
Plentiful does not duplicate rings, and configured starting rings consume their
normal pool supply. No existing item, flag, setting or FC ordinal is reordered.

The native grant and headless oracle use the same save-only key writes and
native 16-bit RandoInf flag APIs. Each counter's -1 sentinel is handled
independently. Repeated receipt does not refill already-spent keys; actual MM
before/after cycle-save hooks restore the persistent found totals. Ring removal
removes its full key quantum and ownership, with nonnegative counters. This is
the existing removal-policy model, not a transactional undo of older grants.

Rings are native SMALL_KEY items with owning-dungeon classification, including
normal/inverted Stone Tower. The older FC restriction path also recognizes the
KEY_RING suffix. Four append-only foreign aliases/FC rows preserve MM grant
ownership in OoT. Their fresh alias GI IDs are 0x1b1 through 0x1b4. Shared receipt
markup names each ring, colors it by dungeon and explains its complete key supply.

Verification so far:

- The pre-change native catalogue test fails for missing Woodfall Key Ring.
- Full native catalogues, GeneratePools, placement and computed starting-item
  bodies pass all 16 selections x four placement modes x two densities in
  original and Combo guards, including 665 frozen legacy IDs.
- Native grant/removal, independent sentinel combinations, repeated receipt,
  save-state copying and actual cycle-save hooks pass under ASan/UBSan in both
  guards. The initial byte-oriented flag prototype was rejected by source
  layout verification; the final implementation calls native 16-bit flag APIs.
- The actual native flag save codec passes current, short legacy, reused, empty
  and future-tail tests in both guards.
- Real MM palette/export -> OoT resolver/cache/latch tests now carry each
  authored metal/emblem plus ring path and correct count through a validated
  append-only draw kind. The recipe regression failed before the new producer.

Fixture service/config/region-membership boundaries are controlled. Initial
fixture setup failures (missing private declarations, cycle-service linkage,
C++ preinclude used for a C helper, header search paths) were corrected before
recording final results. ASan/UBSan disables leak detection due managed ptrace;
no leak-check result is claimed.

The renderer and native/foreign receiving-host callbacks now use the exact
bundled MM metal/emblem meshes and existing main ring. Counts are1/3/1/4;
separate native metal and emblem colors remain live. There is a thin static-
lifetime procedural connector per key. Missing ring resources return NOT_READY
for retry; native unavailable-resource fallbacks receive their own arena check.
Ready rings denied arena space do not enter an unsafe fallback.

Default-off per-dungeon controls are under Small-Key Rings. Vanilla placement
disables controls without clearing selections. They retain one saved option
identity and its existing bitmask values.

The execution workspace reverted after the original renderer commit
`3b546721c6774513503770457e283b710c3d9ddd` was created but before its recovery
package was saved. That complete commit/tree is not claimed restored. The core
`combo/ComboMmKeyRingDraw.h` was recovered byte-for-byte, SHA-256
`6a36ea41c4e9ac49efef8467be5e48facaff4b645d2f8e88c7f166664d180887`.
Callbacks, UI and regression fixtures were reconstructed and tested anew.

Fresh receiving-host tests execute actual native matrix, graph and setup bodies,
real GBI and structures, with controlled GPU/resource/matrix-allocation services.
Both hosts pass ASan/UBSan: counts, palettes, authored scale, fan anchors, seams,
nondegenerate connector triangles, pose/material restore, scarce arena, safe
fallbacks, missing resources, late module availability and Alt retry. Real MM
export -> OoT cache/latch tests also pass live palette edits and missing-ring retry.

Still unfinished: fresh complete production object checks, full native builds,
real linked seeded Own Dungeon/exclusion/self-lock validation and configured
gameplay. No full Glitchless or visual acceptance is claimed.

The user additionally requested OoT-style visual treatment using the authored
MM key meshes, live metal/emblem palettes and visibly correct key counts. Exact
bundled meshes are retained; an offline triangle/attachment study established
a compact thin connector without modifying the assets. Independent review before
the workspace reset checked the actual quantized24-segment/four-sided connector.
Stone Tower's broader heads intersect at40degrees; its final spacing is47degrees,
with a recorded0.2154-unit full projected-mesh gap. Other dungeons use40degrees.
The reviewed core renderer's exact hash is preserved above. Scratch geometry
reports were lost in the reset; fresh CPU tests do not substitute for native
material, lighting, camera or GPU gameplay verification.
