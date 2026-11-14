# OoT solver inventory isolation

## Authorized goal

Cor requested a fix/guard on top of the ammo-bug source after the uploaded seed
listed Goron City's left maze chest before its strength, hammer, or Goron Mask
requirements were earned. The solver must derive reachability from configured
starting inventory and the items supplied to the current search.

## Baseline and boundary

- Isolated branch: `fix/oracle-inventory-isolation-20261114`.
- Ammo source plus fixture repairs: `95383b7ee50de52540281495b65084fa45f74b35`.
- Preserve existing location requirements, sphere numbering, ammo fixes, runtime
  save formats, original/Combo guards, and accepted master branches.
- A fresh solver inventory must neither read nor write the player's global
  extended inventory. Independent solver instances and successive searches must
  not share earned custom items.

## Design

`Logic::NewSaveContext()` selects an isolated inventory with 48 extended slots,
each holding a 16-bit item ID and initialized to `ITEM_NONE`. Explicit private
custom-item grant counts cover ownership currently held in runtime-only flags
(equipment, wand modes, Grace, Sensor, Mirror, Rito Mask, Bomb Arrows, Pendant).
Narrow, enumerated item effects make legitimately supplied custom rewards visible
without consulting live state. Duplicate grants and removal preserve remaining
ownership. Shared identities and progressive items retain their actual semantics.

`SetSaveContext()` explicitly selects the existing live inventory view, including
when the caller supplies a copy of the native save for a hint search.
`Reset(false)` preserves the selected view and inventory. Copying/restoring a
`Logic` object also copies/restores its private inventory.

Invalid inventory slots read empty and reject writes. Extended item IDs are not
truncated to eight bits. No clearing, temporary swapping, or mutation of global
NEI data is used for simulation.

## Required evidence

1. Reproduce the defect against the baseline with complete production methods.
2. Empty -> Goron Mask -> empty returns the actual maze predicate to false.
3. Dirty live ownership cannot grant solver capabilities; solver work preserves
   the live save, while live and copied-save hint bindings still read it.
4. Legitimate grants, starting grants, duplicates/removal, independent instances,
   partial resets, 16-bit items, and invalid slots behave correctly.
5. Production combined fill/replay rejects a mask self-lock and an item cycle
   after a contaminated query, with a valid early-mask placement as a control.
6. Relevant ammo and progression regression gates remain green. Register the new
   regression in CI and obtain independent review of the bounded change.

The fixture is native behavioral evidence, not full-game or historical-seed
certification. Replaying the uploaded seed requires a matching rebuilt pair of
native modules and the headless driver; none was available at baseline.
