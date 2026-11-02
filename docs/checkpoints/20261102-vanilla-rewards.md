# Combo Vanilla reward journal checkpoint

This source checkpoint follows public candidate `f382e50ea27a078558a6058c05043a01db60fa3e` (tree `d020ab24428487b81f6f010ade5de0c7391bbd2f`). The isolated candidate remains `candidate/combo-vanilla-icons-20261102` in the public `marsh6487/ComboShip` fork, draft PR #79. The fork relationship and accepted archive inputs are unchanged.

## Implemented source boundary

The version-one manifest names 69 exact checks, including the retained house Feather and Mirror identities. Payloads retain their full 16-bit identity; cane skills, wand modes, runes, seasons, equipment and upgrades are explicit grants. The Water Dragon Scale is explicitly deferred: its legacy equipment identity now grants Sage's Tunic, and substituting that reward would violate the approved design.

The launcher owns pending and collected receipts in the paired container. Begin writes an exact pending descriptor while native state remains unchanged. During the live transaction, receiving-game saves are staged. One atomic replacement commits the final native grant/payment together with the permanent check and cleared pending receipt. Failed writes retain the old native state and a recoverable pending descriptor. Existing ownership satisfies an alias/shop check without payment. Copy and MM cycle saves retain container metadata.

Native callers must drain OoT workers before Begin and after grants before Commit. Failure drains while staging is active, restores native/NEI RAM, and only then aborts. MM clears incidental staging before capturing its final native/owl snapshot. Native adapter enforcement and reward placement integration are separate work still in progress at this checkpoint.

New paired saves explicitly record `sharingProfile: "nei-only-v1"`. Early version-one files without that redundant field retain their fixed version-one policy; unknown explicit profiles are rejected. The launcher final-boss callback returns native completion independently for Vanilla files. The new mode receives a What's New highlight.

## Verification

The pure manifest/journal and actual launcher callback fixtures pass ASan/UBSan with leak detection disabled for this host. They cover real filesystem failures, a true cache/receipt restart, pending replay, receiving-game staging, peer writes, copying an active pending receipt, zero-charge aliases, fresh MM capture and native versus seeded endings. All four existing paired save/creation/serializer suites pass. The complete launcher translation unit passes C++20 syntax checking against real SDL and JSON headers.

Independent bounded journal review found no Critical/Important host issue. Its delayed-worker integration condition is recorded above and must be tested in the native adapter before feature completion. These checks do not establish a full game link, configured gameplay or user acceptance.

The preceding fixture repair's public CI run `41533237056` passed source/format checks. An additional regional MM boot extractor omitted the real opening helper; its dependency repair is in progress. Platform builds and remaining regression jobs must be checked against the final feature source, not inferred from these fixtures.

## Requested inherited fixes

The current candidate includes the Shadow Crystal crash and dialogue/font source fixes from checkpoint `33359ffce2a79e80d7f490d64c1462bfcaad6b14`:

| Source | Retained identity / behavior |
| --- | --- |
| `soh/mods/transformation_masks/mm_player_form.cpp` | Blob `d0522c039cf9e8f7ab3be0a2eec00225b4aa6fd6`; Wolf entry/removal use the flash path instead of ticking an unused MM skeleton callback. Outgoing form survives removal/reload. |
| `combo/menu/ComboItemReceiptPresentation.h` | Blob `ea243eddc0510e9efd27fe65d4aea27465b7cf85`; 8192-byte scratch capacity and destination clamping preserve long-message wrapping. |
| `mm/src/code/z_kanfont.c` | Blob `f307f80ca5cd8e0630345dd25d753c1cad730530`; native dialogue glyph bank/offset/identity/resource-path bounds. |
| `mm/2s2h/BenPort.cpp` | Optional GUI font resources load after archive selection. GUI selection remains separate from native dialogue glyph loading. |

Fresh Wolf transformation, receipt layout, native decoder/font bounds, MM mod-font and location-title suites all pass. Configured Shadow Crystal use, external font-pack overrides and visual acceptance remain unverified. The inherited magic refill trace remains diagnostic only.

## Remaining feature work

Dedicated NEI-only native adapters, all fixed reward bindings and exact native receipts, per-file earned-item availability, independent native endings, final regression registration, whole-branch review and platform build verification are still required. Runtime handoff must exercise paired creation, first MM visit/reset, travel, early/late and paid rewards, collection order, cancellation/interruption, reload/copy/reset, both native endings, Shadow Crystal use and stock/authored placement collision/visibility. No asset master is promoted by source publication.
