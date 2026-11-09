# MM key rings — WIP native foundation

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

Still unfinished: the per-key native GI renderer and dispatch, pickup/shop
callbacks, UI exposure, complete production object checks, real linked seeded
Own Dungeon/exclusion/self-lock checks and configured gameplay. The default-off
option remains unexposed while these gates finish. No full Glitchless or visual
acceptance is claimed by this foundation checkpoint.

The user additionally requested OoT-style visual treatment using the authored
MM key meshes, live metal/emblem palettes and visibly correct key counts. Exact
bundled meshes are retained; an offline triangle/attachment study established
a compact thin connector and 40-degree fan without modifying the assets. This
geometry proposal is separate from native material/camera/runtime verification.
