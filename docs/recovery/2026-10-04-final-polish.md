# Final polish recovery checkpoints

Published PR #34 baseline: `1c29c83f5d676d02ececa7302c138f7aa46b952a`.

The historical final candidate disappeared from the transient workspace. The supplied partial recovery ZIP contained 14 exact blobs from GitHub staging tree `dc4171c3587e52e62359ab6ee1f06ecaf03a870a`; their Git hashes and archive SHA-256 checks passed. It did not contain the complete gameplay source or tests. The reconstruction plan names the accepted requirements and file ownership.

| Checkpoint | Content | Verification | Remaining work |
|---|---|---|---|
| `c54ca14d3555324731412c35a0242e5dd334afcc` | Exact recovered headers and accepted reconstruction plan | Remote tree `34ea8dc7c7a5c4ccd49ce13c42fc4622eb58004d` equals the local tree; fetched branch verified | Reconstruct gameplay implementations and fresh tests |
| `3f6ed4d42d7cb759ee1bb5f06cd97d3af4388fdf` | Tested skin, yellow magic and independent shimmer sections; checkpoint instructions | Remote tree `7068c516dd6b5488407ce7b91f25d86cc97cf1ab` equals the local tested tree; fetched branch verified | Wallet/editor, Wolf core/host, GI fitting/song effects, remaining receipts/icons and combined review/builds |
| `6d5902fb8ada8dd8b5ce9fedec4a1fa6e93a07c9` | Safe wallet digits and descriptive localized MM songs | Remote tree `e4f335a0d6e1b3288b904ec6f058e621274083d3` equals the local tested tree; fetched branch verified | Editor, Wolf core/host, remaining GI/text presentation and combined review/builds |

## MM skin section

Local worker commit `12d055506bbdb49280cd30e8502672b9a10e91e7` adds opt-in root motion, fractional animation interpolation and definition-only scale while retaining zero-default Pikachu behavior. Production pose computation and native MM matrix code are exercised by `python3 -B tests/mm_wolf/run_skin_tests.py`. The fixture failed on the previous renderer's packed vertex position and passed with the change. Queries reject another character's pose and destroyed skins.

## Other completed sections

- Yellow magic receipts retain localized yellow identity and RPG remaining-upgrade counts; saturated tiers no longer wrap. `python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py` passed on the combined checkpoint, including native/foreign receipt catalogs, grants, latch state, Skulltula totals and dungeon-information guards.
- Custom/legacy GI selection retains independent shimmer identity without attaching authored mesh-local energy. `python3 -B scripts/diagnostics/run_nei_identity_tests.py` passed all 17 owner themes and 19 native MM themes through selection and toggle cases. `python3 -B scripts/diagnostics/run_nei_gi_tests.py` passed the native renderer and engine-header checks. Foreign-host shop fitting and song effects are still being reconstructed.
- Native MM and Tycoon rupee digits safely display every nonnegative signed16 balance without changing saved currency, wallet capacity or accumulator state. `python3 -B tests/mm_wallet/run_tests.py` passed all 65,536 balances across five paths under ASan/UBSan/bounds and the real native HUD translation-unit syntax check. The regression reproduced the native index16 crash at 1667/wallet2 and Tycoon10000 masking to1296. Only leak detection is disabled because the fixture allocates no heap and this runner cannot perform LeakSanitizer's process inspection.
- All 15 MM song variants retain descriptive localized receipt bodies with a cold donor. The combined `run_mm_item_receipt_tests.py` runner passed again after this section, including EN/DE/FR song cases and existing grant/dungeon-information guards.
- Wolf's binary loader rejects malformed bounds, counts, trees, names and NaN/Inf using serialized float bits. `python3 -B tests/mm_wolf/run_core_tests.py` passed UBSan and optimized fast-math fixtures. The loader/core scaffold is not yet activated in the MM host.
- Traditional OoT dungeon/song receipts survive the donor export; enabled map/compass and MQ information appends as a separate page. The combined receipt runner passed again with these changes.
- Generated catalog bounds now fit all 61 authored GI models under native/OoT/MM routes and pickup/shop/freestanding matrices. The production GI runner passed; Four Sword's actual top edge now fits while retaining its shelf scale. Source/held asset bytes are unchanged.
- Exact boss-soul name RGB is applied by both Latin text renderers only in recognized localized Soul-name spans. `python3 -B scripts/diagnostics/run_receipt_soul_color_tests.py` passed five souls across EN/DE/FR, wrapped/malformed/page/unknown cases, ordinary palette and red Goht Remains, plus both real textbox translation-unit syntax checks.
- Independent review found an existing Pikachu metadata layout with 48 skeleton nodes and 47 weighted bones. The strict equality guard was corrected. A fixture now links the actual generated Pikachu mesh/skeleton/registration and passes production draw submission, legacy shared scale and bounded pose queries. `run_skin_tests.py` passed again after the fix; review follow-up is in progress.

## Editor fixture in progress

The production grant fixture in `tests/mm_editor` is checkpointed as work in progress. The editor implementation is still being reconstructed; this fixture is not yet included in the combined passing gate. It exercises real catalog/FC/grant/flag handlers through headless engine boundaries so the missing Slate/Hourglass/Crystal/Seasons grants can be reproduced before changes.

This is a partial reconstruction checkpoint. Wolf host/combat integration, editor grants, song effects/standalone sword orientation, clef and Ikana icon handling, combined review and complete Windows/Linux builds remain in progress. In-game acceptance is not claimed.

## Resume

Fetch `recovery/final-polish-20261004`, inspect this note and the reconstruction plan, and continue the uncompleted sections. The checkpoint commit's message records its exact tree and tests. Do not treat the 43-command gate reported by the lost session as fresh proof. Keep the final PR draft and preserve its published ancestry. Existing authorization permits the final push to PR #34; no merge or master promotion is authorized.
