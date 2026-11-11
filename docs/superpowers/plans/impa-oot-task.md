# OoT startup receipt implementation brief

Read `../specs/2026-11-10-impa-startup-receipt.md` and `combo/rando/StartupCheckReceipt.h` first. These are the requirements and exact shared ABI. Work only in this isolated checkout.

## Owned files

New `soh/soh/Enhancements/Gameplay/ComboStartupReceipt.cpp` and `.h`; `soh/soh/Enhancements/randomizer/savefile.cpp`; the bounded native receipt guard in `soh/src/overlays/actors/ovl_player_actor/z_player.c`; named/frozen foreign renderer support in `combo/menu/ComboForeignDrawOOT.h` and its existing source wrapper if necessary; tests with `oot_` prefix in `tests/startup_receipt/` and their `run_oot_tests.py`.

Root owns shared ABI/state, launcher, CI, plan and recovery; MM implementer owns all MM files. Do not edit those paths. Report an interface issue to root instead.

## Required exports and source seam

- Export `SOH_SetStartupReceiptHost(CwStartupReceiptHost)`.
- Add `ComboStartupOot_BeginFile()` at the beginning of randomized save initialization. Only call the host BEGIN_FILE for combined randomizer files; preserve standalone/solo behavior.
- Add `bool ComboStartupOot_Capture(const GetItemEntry*, int32_t check)` immediately before the existing Impa `StartingItemGive`. It returns true ONLY when the host accepted an MM-owned WAIT_MM request, in which case skip that immediate foreign grant and mark the source check saved. For a native OoT item it captures the resolved pregrant entry but returns false, preserving the existing grant. Invalid/no callback/noncombined/other checks return false.
- Host CAPTURE is called with game=0, actual file slot, version=1 and concrete metadata. Host assigns generation. READY for native; WAIT_MM for foreign. WAIT_MM carries the raw MM `fi->itemName`, category/importance and flags; MM fills concrete name/id after its save exists. Native `itemName` must be a concrete owner identity usable by MM's existing foreign draw/receipt routines. `displayName` is the real visible awarded item.

## Presentation

On safe foreground gameplay frames, call host PEEK. Reconstruct/freeze the actual entry; never run a progressive conversion again. Existing native GI skip policy decides whether to use `GiveItemEntryWithoutActor`; skipped animation still emits item+source notification and acknowledges once.

Use `CW_STARTUP_RECEIPT_TOKEN_BIT` in the existing `comboMarketReceipt` namespace with a bounded local token. Introduce C-compatible helper(s) for native player code. For a token receipt, the native player must retain model/camera/fanfare but skip inventory/BGS grants, `Player_SetPendingFlag`, generic receive hooks, AskToEquip/equip mutations, delayed trap and Nabooru capture. Handle stale tokens as presentation-only too. Never call `Return_Item_Entry` merely to show this receipt. Only acknowledge after visible text/toast begins. Cancel before acceptance leaves the durable receipt pending.

Text/toast names the actual item and `CW_STARTUP_SOURCE` (`Song from Impa (OoT)`). A custom receipt message can use simple item and source pages; no authored native item tutorial replay is needed. Preserve normal messages for all other receipts. Foreign MM draws must use the frozen concrete name, bypassing current check lookup/latch and progressive re-resolution.

The root will finish MM-owned deferred grants after MM creation, atomically save MM+READY metadata, and persist acknowledgements. Both host adapters are strictly presentation-only.

## Tests and report

Write/run failing native production-boundary tests before implementation, then demonstrate: eligibility/noncombined preservation, native pregrant identity capture, foreign deferred capture, all OoT GI policies/classes, one notification/ack and zero grants from repeated presentation callbacks, cancellation/slot changes, and all suppressed native side effects. Exercise original/combined guards where affected and compile the actual changed native player boundary if available.

Do not spawn agents or publish. Do not commit until root coordinates the index. Write a concise report to `/workspace/scratch/2a4e504bb390/oot-startup-report.md` with files, red/green commands, remaining limitations and concerns; return only status and report path. Preserve existing user work and stop optional test expansion after concrete risks are checked.
