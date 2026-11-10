# Unified trade selection repair

## Authorized result and source checkpoint

cor requested both the 2Ship adult Link/Din get-item framing and the duplicate trade-item repair, including a check of the other trade items. Adult framing is already implemented in `47a0bee04163fd2a43562cc5d4290fbfa7dffe00`; preserve its five production files and its regression fixtures unchanged. Build the combined source candidate on that commit, on isolated branch `candidate/adult-gi-trade-slots-20261110`. The cumulative delivery baseline remains published PR90 commit `fdbfb868a5d70d73a29ba79b1ac7701c122fff37`.

The new authorization covers repairing trade selections. It does not change the Death Mountain gate setting, starting-Letter policy, quest flags or seed configuration. No public push, merge or master promotion is included.

## Confirmed defect

The reported MM-start save has one Zelda's Letter ownership bit. In OoT, native raw slot 23 (visible cell 24) retains the Letter while the pause handler folds that item into the unified trade ownership and materializes it in raw slot 22 (visible cell 23). The corner mask wheel also accepts that non-mask item. This creates two visible selections from one grant.

The 23-entry trade table has three overlapping entries: Weird Egg, Cucco and Zelda's Letter (indices 20–22). The eleven adult OoT trades and nine MM entries do not belong to the OoT corner mask wheel. MM's separate native quest slots are backing state used by native quests; the visible inventory exposes the unified trade cell and a separate virtual mask cell. Those hidden native homes must remain intact.

## Repair

Use the existing save-load boundary and page-0 reconciliation to normalize legacy Combo randomizer selections. Pause reconciliation must run before navigation resolves the item and slot for a same-frame equip, including a switch into page zero. Before removing a child non-mask from the corner slot, preserve its unified ownership. Materialize a migrated item in an empty unified selection; retain an existing unrelated trade selection. Select an actually owned corner mask, or leave the corner empty. Exclude all non-mask trade items from that corner's cycling in the repaired mode.

Repair C-button slot bindings in the live, saved child and saved adult equipment sets while preserving their item identities. Preserve native item IDs, the global native slot table, mask ownership, receipt counters and all gate/quest flags. Ensure the shared registry can still observe trade items after their selected slot moves. Keep solo and native nonrandomizer behavior unchanged through explicit mode guards.

Native acquisition may still use its native temporary slot before reconciliation. The correction is selection normalization, not another award. Legacy randomizer ownership flags must not strand an unselected owned child trade item. Repeated reconciliation must be idempotent. Existing native inventory replacement must still find a selected migrated egg for hatching. Do not broaden this patch into a rewrite of unselected trade-item lifecycle semantics.

Two native compatibility consequences require explicit coverage. Age restoration must preserve an independently equipped migrated child trade when a different unified trade is selected. Actual Weird Egg hatching must register Cucco in the unified ownership and update affected migrated bindings in all three equipment banks, so an immediate age change cannot restore the historical Egg or substitute the other selected trade. These changes execute on the native hatch event; normalization itself must never hatch an item, change quest flags or manufacture another receipt.

## Evidence required

Run actual production-function behavior tests first against the unchanged implementation and retain the failing output. Cover the reported shared-bit plus child-slot state, already duplicated saves, child-slot-only and ownership-only states, all 23 table entries, all three overlapping items, eight native masks and Rito, preservation of another selected trade, repeated reconciliation and all three equipment sets. Check guarded solo/nonrandomizer controls and bounded native item replacement. Use real game IDs and structures where practical; identify controlled engine services and extraction boundaries.

Re-run affected Rito, randomizer and pause-inventory regressions. Re-run the adult framing regression on the combined candidate. Check changed native C translation units as dependencies permit. A native fixture or source syntax check does not establish a complete game build or visual acceptance with adult Din. Final delivery includes exact commit/tree identities, a cumulative source delta and independent restoration proof.
