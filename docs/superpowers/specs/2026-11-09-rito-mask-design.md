# Rito Mask: optional shared reward and existing mask-wheel integration

## Authorized outcome

cor requested a real randomizer Rito Mask, an authored NEI get-item model with shimmer, its own place with the masks in kaleido, a What's New entry, and functional use, icons and receipts in both games. No progression logic is required. Implementation proceeds under that request and the existing isolated-candidate/recovery workflow; no master promotion is authorized.

Baseline: local R22 `191794e8c7534c30a55ee4ab4554e49356c6ca75`; published PR89 was `0063920e18145299afc1682aab98407ebd366fc5` at trace time. Existing Rito code is identical in the relevant paths. Original source and assets remain preserved.

## Design decisions

- Add an independent, default-off **Shuffle Rito Mask** control immediately below the masks shuffle controls. Use the existing Combo shared-setting pattern, with standalone host settings retained. Add the What's New accent and a visible What's New entry. This grouping is the implementer's recommendation, consistent with cor's proposed Shuffles placement.
- Append randomizer and shared identities without renumbering old values: `RG_RITO_MASK`, `RI_OOT_NEI_RITO_MASK`, `FCI_RITO_MASK`. One optional native copy in either solo game, one shared copy after Combo deduplication. No world-access, solver, action or progression requirements consume this item. No automatic Farore's Wind grant in new shuffled files.
- Use only existing inventory cells. In solo OoT and both sides of Combo, add Rito to the bottom-right mask-selection wheel on the first inventory page. In solo MM, cycle Rito with Bremen in Bremen's existing mask slot, preserving each mask's independent ownership. No extra page, bank, grid cell or rearrangement. Preserve existing Rito ownership and restore only legitimately owned Farore's Wind during migration.
- Expose native ownership functions in each `mods/extended_inventory.h`: `uint8_t RitoMask_IsOwned(void)`, `void RitoMask_Grant(void)`, `void RitoMask_Remove(void)`. Both functions and native item identities must be usable by the corresponding native/foreign randomizer adapters.
- Add Rito ownership and selection to the updated save editors in both games, using the same host grant/remove APIs and preserving the other shared-slot masks.
- Keep OoT's existing Rito transformation and implement native MM Rito through its existing custom-form subsystem, including the Rito body and its flight/bow behavior. Do not substitute a latent save bit for usable functionality.
- Author a real 3D ivory-and-gold Rito bird mask, following the existing icon, with a sculpted beak, eye openings and feather crest. Use the established NEI renderer and shimmer in all four native/foreign receipt combinations. Asset path: `__OTR__objects/nei_gi_redesign/rito_mask/gi_dl`. Preserve the existing icon path `__OTR__textures/icon_item_custom/gItemIconRitoMaskTex`, ship it for both hosts, and repair the actual runtime resolver.
- Preserve fixed Combo Vanilla acquisition, native mask behavior, existing controls and unrelated integrations. A save without the new shuffle option must remain loadable. Existing old ownership is migrated safely; a new seed does not inherit unrelated flags.

## Approaches considered

cor explicitly rejected an extra page and an extra fixed grid slot. The accepted placement is the existing corner mask wheel for solo OoT and Combo, with Bremen sharing reserved for solo MM. MM's current first inventory page already uses the OoT layout in both modes, so the mode-specific Rito placement must be explicit. Extending MM's existing custom forms preserves its established player renderer and lifecycle.

## Evidence and handoff

Require exact-ID icon tests, enabled/disabled pool counts and shared dedup, grants/removal/save/migration, existing corner-wheel and solo-MM Bremen equip/HUD routing, native/foreign authored GI with shimmer, MM form entry/exit and gameplay controls, plus native compilation of changed translation units. Asset display lists must be checked with the resource pipeline and static renders. Distinguish those checks from configured in-game visual acceptance. Source checkpoints exclude GIFs, animated previews, private game assets, saves and logs.
