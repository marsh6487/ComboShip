# MM editor and wallet reconstruction

Task 2 of `docs/superpowers/plans/2026-10-04-final-polish-reconstruction.md` is source-verified on isolated branch `poc/reconstruct-editor-wallet-20261004`, worktree `/workspace/scratch/68d44ffc4c98/takeover-editor-wallet`. Base is `f8cf8292` (accepted reconstruction plan), following recovered-header checkpoint `d1dfbc7b` and published baseline `1c29c83f5d676d02ececa7302c138f7aa46b952a`. The missing editor/wallet implementation is reconstructed source, not byte-for-byte recovery of the unavailable historical candidate.

This worker does not push, merge, promote master or claim in-game acceptance. Parent integration, independent review, durable publication and platform package gates remain separate.

## Checkpoints

| Commit | Completed section |
| --- | --- |
| `c6dc3efdde7c042155475c40eb23848c7fb3b239` | Bounded decimal wallet drawing and exhaustive production draw fixture |
| `da281887a2a59a29615165ff7320b3f43f19f9d2` | Explicit WIP checkpoint of the real production editor fixture; editor restoration was still incomplete |
| `005360b57014da7ac96bbc269e25399c622edf1a` | Live NEI catalog, individual grants, grant-all and real grant/header verification |
| This report's commit | Clear/regrant host restoration, enabled editor controls after clearing, historical RED control, production pool/GI characterization and final evidence |

The parent publishes these coherent sections to `recovery/final-polish-20261004`. A local commit is not represented here as a verified remote checkpoint.

## Wallet behavior and cause

The native MM HUD decomposed balance 1667 into three digits, producing a first digit of 16 and indexing a ten-entry counter texture table out of bounds. The original production draw fixture failed at that exact index under UBSan before the fix (`wallet-red.log`). The Tycoon path masked balances outside 0..9999 with `0xDDD`; its original production draw function rendered 10000 as 1296 (`wallet-tycoon-red.log`).

Both paths now call `RupeeCounter_BuildDigits`, a C/C++ static-inline helper with a five-digit buffer. Every nonnegative signed16 balance displays its full decimal value. Native wallet0 keeps a two-digit minimum; native wallet1/2 keep three; Tycoon keeps four. Native wallet3 also has a safe three-digit minimum if an override does not handle it. Negative corrupt balances display zero without writes. Currency, accumulator, wallet bits, wallet capacity, colors, draw origin, spacing and HUD override routing remain in the existing paths. Additional necessary digits extend to the right using the existing spacing.

The fixture extracts the actual native rupee draw block and the complete production Tycoon counter function, with real MM headers. It records digit textures from the draw calls against a ten-entry texture table. Expected output comes independently from decimal `printf` formatting. All 65,536 signed16 balances run through native wallets0/1/2/3 and Tycoon: 327,680 draws. It checks exact decimal output and preservation of the save balance, wallet bits and accumulator, including literal 1667/wallet2, 999, 1000, 9999, 10000, 32767, -1 and -32768 cases.

## Editor behavior and cause

The old Give All Custom Items control used the actor/action registry and debug ownership helper. That registry cannot describe the current full-width appended items or all sibling power identities. The real old editor button block, debug helper and registry ownership fields fail the canonical Slate39 assertion in the RED fixture. The final fixture replays this failure from the published `1c29c83f5d676d02ececa7302c138f7aa46b952a` baseline using `--control`. Its editor and registry blobs match the initial local `f8cf8292` control; the published baseline survives a fresh checkout.

The new `NeiEditorItems.h` catalog enumerates the current `Rando::StaticData::Items` metadata, selecting `RI_OOT_NEI_` and `RI_OOT_EXT_` identities and excluding retired Hylia's Grace. There are 61 active identities on this baseline. `SaveEditor.cpp` calls its grant-all helper and displays individual grant buttons with current item names. New ownership goes through actual `Rando::GiveItem` production handlers, including FC recording, rather than filling icons directly.

Canonical Slate39, Hourglass41, Shadow Crystal44 and Seasons47 are restored. Grant All includes all five Slate powers, all four seasons, six cane skill identities, base wand plus six wand variants, existing custom inventory, progression flags and extended equipment. The default wand treatment tests usability through `Wand_ModeOwned`; if a newly granted mode needs its medallion, the editor also calls the real corresponding medallion grant. This sets the existing NEI quest bits and leaves ordinary MM inventory/save fields unchanged.

Ownership checks prevent duplicate grants and preserve selected rune/season/tool state. Roc's Feather treats Roc's Cape as sufficient ownership, avoiding a downgrade during repeated Grant All. Cane's base identity remains progressive, and explicit siblings complete any skills missing from a partial save.

The existing Clear Custom Items control clears cells24..47 but retains separate power flags. A relevant self-review fixture first failed because an already-owned cane never recovered its host cell. A second RED showed its individual editor button also remained disabled after clearing. Empty retained hosts now recover via the canonical `ExtInv_GiveItem` setter before the ownership decision, without replaying already-earned grants. This is deliberately limited to previously owned empty cells: new powers still use production randomizer handlers. Replaying cane/rune handlers would respectively ignore an owned skill or change selection/recount shared acquisition. Cane requires its host for editor ownership; shovel/dominion accept either valid item in their shared cell and preserve an existing selection. The Clear control itself and the save layout are unchanged.

The production grant fixture executes the real current item metadata, save getters/setters/init, cane progression helpers, rune/season/wand grants, equipment grants, FC native reverse map and `Rando::GiveItem` prefix plus complete NEI/EXT/medallion switch arms. Native unrelated arms abort if accidentally invoked. Notification/audit/peer callbacks are headless seams; the real local FC obtained/applied recording remains active. The fixture checks all three wand treatments, every current catalog identity, four individual canonical grants, first-granted powers supplying their hosts, partial cane completion, retired/ordinary identity rejection, cape preservation, byte-exact ordinary SaveInfo preservation and complete repeated-grant NEI preservation. Clear/regrant checks host restoration, unchanged selected rune/season and unchanged cane FC obtained/applied counts.

## Hourglass and identity findings

The complete production `GeneratePools` function runs with genuine item/logic metadata and empty region/check input to isolate its option-driven additions before solving/placement. Starting/computed/excluded input seams return empty; solo/combo mode and all three wand treatments are exercised with NEI enabled and disabled.

- Hourglass is present exactly once iff NEI items are enabled, in all exercised modes.
- All five Slate powers and four seasons are added once when enabled; bare Slate/Seasons host identities are omitted as the powers supply their hosts.
- Retired Grace is never added; the existing global save remains byte-identical.

No pool implementation change or seed rewrite was needed. This proves new-pool construction on the recovered baseline. It does not run the placement solver, produce an actual seed or identify why an unspecified older seed lacked Hourglass.

The GI fixture copies the actual production binding table, legacy-mod priority check and `MM_DescribeNeiGi`, and uses the real symbol resolver to reach an exported draw-info test seam. `RandoItemId`194/0xC2 requests `hylia_grace`; 192/0xC0 requests `fire_rod`. A Grace-only MM legacy override blocks Grace's authored binding and leaves Fire Rod's binding available. These are randomizer identities, not the separate vanilla `ItemId` enum. The retired Grace ability remains excluded from the editor and pools; no GI/receipt implementation is edited by this task.

## Verification

All commands run from the worktree. Dependency environment is sourced using `/workspace/scratch/68d44ffc4c98/test-env.sh`, with actual upstream JSON v3.11.3, imgui v1.91.9b-docking, spdlog v1.15.3, BS thread pool v4.1.0, stb and SDL2 headers.

| Command | Result and evidence |
| --- | --- |
| `python3 -B tests/mm_wallet/run_tests.py` | PASS: exhaustive five-path decimal drawing; full `z_parameter.c` and `TycoonWallet.cpp` real-header syntax; `wallet-final-verified.log` |
| `python3 -B tests/mm_editor/run_tests.py` | PASS: four production grant groups, complete pool and GI characterization, full `SaveEditor.cpp` and `GiveItem.cpp` real-header syntax; `editor-final-verified.log` |
| `python3 -B tests/mm_editor/run_tests.py --control` | Expected nonzero RED at missing Slate39 with actual historical editor path; `editor-control-verified.log` |
| `python3 -B scripts/diagnostics/run_mm_hud_cosmetics_tests.py` | PASS, including rupee icon cosmetics and native HUD header checks; `wallet-hud-regression.log` |
| `python3 -B scripts/diagnostics/run_mm_nei_tests.py` | PASS existing full NEI regression suite; `editor-nei-regression.log` |
| `python3 -B tests/seasons/run_tests.py` | PASS existing seasons input, grant, pool, shop, menu, save, icon, weather/water, native and cross-game fixtures; `editor-seasons-regression.log` |
| `python3 -B scripts/diagnostics/run_combo_grant_tests.py --json-include /workspace/scratch/68d44ffc4c98/deps/json/include` | PASS existing combo grant boundary suite; `editor-combo-regression.log` |
| `git diff --check` and clang-format14 with repository-compatible four-space/120-column style and preserved include order | PASS for this section |

RED evidence also includes `editor-red.log` (original Slate missing), `editor-clear-red.log` (missing cane host after clear) and `editor-clear-ui-red.log` (disabled cane control after clear). All listed logs are under `/workspace/scratch/68d44ffc4c98`.

The new production fixtures use address, undefined-behavior and bounds sanitizers with no recovery. LeakSanitizer alone is disabled because this runner cannot inspect processes through `/proc`/ptrace. The wallet fixture uses no heap. Whole C++ translation units use the repository's `-fpermissive`, real build defines and forced PCH-equivalent includes; existing controller macro, const qualifier, UIWidgets name and deprecated comma-subscript warnings remain, with no compilation errors. These are source/header checks, not a full linked game or platform build.

## Independent review correction: earned wand and usable mode

The initial clear/regrant repair used `Wand_ModeOwned` as evidence of an earned host. Independent production reproduction showed that a separately earned medallion makes a mode usable even when no wand has been obtained. The initial helper could then insert an empty wand cell and skip the actual randomizer grant and FC recording. It could also skip unearned siblings during Grant All when their medallions already existed. This was an omitted partial-save case in the original passing fixture.

Host restoration now requires valid retained `wandRodsOwned` bits. Editor ownership checks earned mode state as well as active-rule usability and the canonical host. The single-item treatment retains its native any-earned-rod semantics. Granting a new wand still uses `Rando::GiveItem`; repairing a bare previously earned wand grants only its missing medallion prerequisite and avoids awarding or counting the wand again.

The expanded production fixture failed first at the medallion-only canonical grant's missing earned rod bit (`wand-review-red.log`). It then passed medallion-only Elemental Wand and Sand Rod grants with actual FC obtained/applied counts, Grant All after all medallions were earned separately, and both retained/cleared bare wand prerequisite repairs with unchanged wand counts and complete repeat safety (`wand-review-green.log`). Both real translation units and pool/GI characterization also passed: nine PASS groups in total. Independent review reran the expanded fixture and confirmed the same tested blobs.

## Remaining acceptance and handoff

No game has been booted. Visual fit of the additional decimal digits, actual editor presentation, runtime item use and platform package acceptance remain for the parent/user candidate gates. No Wolf, GI/receipt source or root gate code is changed. The report's requested standalone copy is `/workspace/scratch/68d44ffc4c98/editor-wallet-rebuild-report.md`; the tracked copy provides durable recovery evidence when the parent publishes this section.
