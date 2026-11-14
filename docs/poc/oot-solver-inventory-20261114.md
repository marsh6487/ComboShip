# OoT solver custom inventory isolation

## Scope and source

Cor requested this guard on top of the ammo repair source after finding early
checks whose physical requirements had not been earned. The isolated candidate
starts at `95383b7ee50de52540281495b65084fa45f74b35`, which includes the ammo,
Bowling, magic, and pool fixes plus their subsequent fixture repairs. Publication
continues on `candidate/seed-fill-regressions-20261110`; PR 90 remains draft.

## Defect

OoT's solver creates a private native `SaveContext`, but its custom inventory
methods previously read and wrote global NEI slots. `NewSaveContext()` did not
clear those slots. An assumed/full-inventory search could therefore leave the
Goron Mask available to a subsequent empty-inventory search. `HasStrength(2)`
accepts that mask, so the real Goron City maze rule could be satisfied without
earned strength, Hammer, or Goron Mask. The same oracle serves placement,
forward playthrough, and final validation; the effect is not confined to display.

Other `HasItem` cases read live custom ownership flags directly. Clearing the
global save would damage gameplay state and would not supply earned solver
grants for the custom items whose item effects were missing.

## Implementation

- Each solver instance owns 48 extended slots with 16-bit IDs, initialized empty
  whenever it creates a new simulated save.
- Explicit custom grant counts isolate equipment, Grace, Sensor, Rito Mask,
  Mirror, Bomb Arrows, wand modes, and Pendant ownership. Enumerated inventory
  grants supply legitimate custom tools and masks without resolving their GI
  presentation against another context.
- Counts retain duplicate grants and remaining progressive Roc levels after
  removal. Pendant identities share ownership; the base wand aliases Sand, and
  modes follow the current seed's single-item, elemental, or medallion setting.
  Automatic Bomb Arrows use the private Bomb Bag and seed setting.
- `SetSaveContext` borrows the supplied native context and selects live custom
  ownership, including copied native saves used by hints. `Reset(false)` retains
  that view and inventory. A new simulated save does not delete borrowed saves.
- Invalid slots read empty and reject writes; extended writes preserve 16 bits.
  Shovel and Demise grants no longer fall into the native bottle allocation arm.

Location requirements and sphere numbering are unchanged. Runtime NEI save
formats and global ownership helpers are unchanged. Existing seeds retain their
placements; this source correction does not retroactively repair a seed.

## Verification record

The initial native regression compiled complete production inventory/reset
methods against the real `Logic` declaration in standalone and Combo modes.
Both baseline runs failed the same two assertions: simulated grants modified
global NEI, and empty -> mask -> empty retained the mask. Both assertions pass
with the candidate. Full `logic.cpp` syntax compilation also passes in both
modes against actual native headers, without rewritten production code.

The expanded fixture passes 128 behavioral assertions per guard under ASan/UBSan.
It executes complete production ownership, grant, reset, and starting-inventory
methods, the actual GC maze-left predicate, and production combined fill/replay.
Both a mask behind itself and a two-item cycle reach final validation and are
rejected after a deliberately priming full-inventory query. The valid early-mask
control passes and collects the maze prize in a later sphere. Running the same
fixture against the baseline compiles successfully and fails 65 semantic
assertions per guard, including acceptance of both invalid placements.

The inventory matrix also covers dirty live flags, global-write call counts,
complete live save-byte preservation, duplicate removal, separate instances,
copy/restore, live and copied-save bindings, partial reset, legitimate starting
grants, 16-bit IDs, invalid slots, wand treatments/private medallions, Pendant
aliases, progressive Rocs, and Shovel/Demise bottle controls. The new command is
registered in the items CI suite. The fixture README records its native service
and small-world boundaries.

Adjacent ammo sync, legacy network ammo, RPG magic supply/oracle/replay, shared
progression fill/replay, wallet receipt, seed-setting, and cross-game hint-provider
regressions pass. The wallet fixture covers 768 receipt/order/delivery/switch
schedules under sanitizers. The hint
fixture exercises the changed `SetSaveContext` and graph restoration under
ASan/UBSan. Missing local dependency headers were supplied outside the source
tree from their exact pinned or tracked versions.

The initial production milestone was checkpointed before the expanded tests.
Final review, exact-source CI-runner verification, and publication identities are
recorded separately in the final recovery receipt.

Independent review found no new blocking defects. It identified two inherited
ownership limitations for follow-up: Chateau Romani's bottle-content identity
has no extended inventory slot, and removing one of a mixed pair of OoT/MM
trade-mask identities can clear their shared native trade flag. Neither case is
claimed as fixed or covered by this isolation regression. Chateau has no current
reachability predicate; both limitations predate this repair.

No matched rebuilt native modules or headless driver were available locally.
These checks do not certify the uploaded historical seed, a linked game build,
or configured gameplay acceptance. Use the corrected native generator/replay
to regenerate or revalidate before treating old sphere output as trustworthy.
