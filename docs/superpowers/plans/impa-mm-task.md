# MM startup receipt implementation brief

Read `../specs/2026-11-10-impa-startup-receipt.md` and `combo/rando/StartupCheckReceipt.h` first. These are the requirements and exact shared ABI. Work only in this isolated checkout.

## Owned files

New `mm/2s2h/Enhancements/Gameplay/ComboStartupReceipt.cpp`; a minimal helper in `mm/2s2h/BenPort.cpp` if existing dormant grant exports cannot preserve native sharing; named foreign renderer wrapper in `combo/menu/ComboForeignDrawMM.h`, `mm/2s2h/Rando/DrawItem.cpp` and `Rando.h`; tests with `mm_` prefix in `tests/startup_receipt/` and `run_mm_tests.py`.

Root owns shared ABI/state, launcher, CI/docs; OoT implementer owns OoT files and `ComboForeignDrawOOT.h`. Do not edit those paths. Send interface issues to root.

## Required exports

- `MM_SetStartupReceiptHost(CwStartupReceiptHost)` stores the launcher callback.
- `int32_t MM_FinishStartupReceipt(CwStartupReceipt*)` completes an accepted WAIT_MM reward after the launcher guarantees the matching MM save and starting inventory are resident. Validate before mutation. Resolve raw `itemName` exactly as `GrantMmItemByName` does (`ConvertItem`, RI_JUNK fallback). Fill the same record with frozen concrete `getItemId` (MM RandoItemId), `itemName`, `displayName`, importance/category/dungeonInfo, and READY state. Grant exactly once through the existing dormant-resolved/native sharing behavior and save the native MM section. This is the ONLY adapter operation allowed to grant; all presentation callbacks below are no-grant.
- Set the output record to READY immediately once the native award succeeds, before the save callback can throw. The launcher stages its native MM writes and commits the resulting native save and READY receipt together; it will not repeat a successful in-memory grant if persistence must retry.
- Do not use a live previous slot or create an invented MM check. Parent owns the call ordering.

## Presentation

On safe foreground gameplay, ask host PEEK and offer an actor-owned CustomItem receipt, reusing the existing native lifecycle pattern. Register after load; the regular MM CheckQueue reset may clear transient events, so never use them as the durable record.

For MM owner, draw `DrawResolvedItem(frozen id, RC_UNKNOWN, actor)`. For OoT owner, add a small named/frozen wrapper around the existing foreign renderer; preserve all static, animated, NEI and model-fit behavior. Never resolve the next progressive tier after a grant.

Use existing native category skip settings (0 never skip, 1 junk, 2 junk+minor, 3 all). The payload's frozen importance is for foreign owner; native owner may use its concrete ID and existing `ShouldShowGetItemCutscene`. Always emit actual item + `CW_STARTUP_SOURCE`, including when GI is skipped. For full GI, show simple item/source receipt text with the native CustomItem presentation; actor callbacks must never call GiveItem/cross-delivery/mark an MM check. Acknowledge exactly once after visible feedback begins.

Maintain actor/token/slot/generation identity, cancel on invalidated context, keep pending if canceled before acknowledgement, preserve unrelated events. Original/solo/vanilla paths must stay unchanged.

## Tests and report

Write/run failing production-boundary tests first. Cover deferred completion after starting inventory, exactly one grant, all native skip modes and three classes, both item owners, frozen progressive identity, item/source feedback, repeated no-grant callbacks, cancellation/reload and unrelated events. Reuse relevant native CustomItem fixtures without weakening assertions.

Do not spawn agents, publish, or commit until root coordinates the index. Report to `/workspace/scratch/2a4e504bb390/mm-startup-report.md` with files, red/green commands, limitations and concerns; return only status/report path. No broad refactoring or unrelated work.
