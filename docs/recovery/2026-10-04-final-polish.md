# Final polish recovery checkpoints

Published PR #34 baseline: `1c29c83f5d676d02ececa7302c138f7aa46b952a`.

The historical final candidate disappeared from the transient workspace. The supplied partial recovery ZIP contained 14 exact blobs from GitHub staging tree `dc4171c3587e52e62359ab6ee1f06ecaf03a870a`; their Git hashes and archive SHA-256 checks passed. It did not contain the complete gameplay source or tests. The reconstruction plan names the accepted requirements and file ownership.

| Checkpoint | Content | Verification | Remaining work |
|---|---|---|---|
| `c54ca14d3555324731412c35a0242e5dd334afcc` | Exact recovered headers and accepted reconstruction plan | Remote tree `34ea8dc7c7a5c4ccd49ce13c42fc4622eb58004d` equals the local tree; fetched branch verified | Reconstruct gameplay implementations and fresh tests |
| `3f6ed4d42d7cb759ee1bb5f06cd97d3af4388fdf` | Tested skin, yellow magic and independent shimmer sections; checkpoint instructions | Remote tree `7068c516dd6b5488407ce7b91f25d86cc97cf1ab` equals the local tested tree; fetched branch verified | Wallet/editor, Wolf core/host, GI fitting/song effects, remaining receipts/icons and combined review/builds |
| `6d5902fb8ada8dd8b5ce9fedec4a1fa6e93a07c9` | Safe wallet digits and descriptive localized MM songs | Remote tree `e4f335a0d6e1b3288b904ec6f058e621274083d3` equals the local tested tree; fetched branch verified | Editor, Wolf core/host, remaining GI/text presentation and combined review/builds |
| `9d4886c54847b0eb267a8985d99bd55140011170` | Wolf loader, GI fit, traditional/exact-color receipts, Pikachu compatibility and WIP editor fixture | Remote tree `9e03696c4aa44929e0516d04360a4fa4f72a2b8c` equals local tree; fetched branch verified | Host/editor implementation, final presentation, review and builds |
| `a79093a7c4c6a601c334c44e7578cb6ca945f83b` | Live editor grants, Wolf combat, themed songs and sanitized skin fixtures | Remote tree `4832641839853c68f577c2108f48312c174db1c7` equals local tree; fetched branch verified | Host, sword/receipt integration, preservation review and builds |
| `167aa37c33c3f5be46de342287772100de5633fa` | Integrated Wolf host, upright swords, colored clefs and MM-owned Ikana icons | Remote tree `6942eb50ff1c26561b7624863a8d34abd1f16b62` equals local tree; fetched branch verified | Final edge cases, independent review and builds |

## MM skin section

Local worker commit `12d055506bbdb49280cd30e8502672b9a10e91e7` adds opt-in root motion, fractional animation interpolation and definition-only scale while retaining zero-default Pikachu behavior. Production pose computation and native MM matrix code are exercised by `python3 -B tests/mm_wolf/run_skin_tests.py`. The fixture failed on the previous renderer's packed vertex position and passed with the change. Queries reject another character's pose and destroyed skins.

## Other completed sections

- Yellow magic receipts retain localized yellow identity and RPG remaining-upgrade counts; saturated tiers no longer wrap. `python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py` passed on the combined checkpoint, including native/foreign receipt catalogs, grants, latch state, Skulltula totals and dungeon-information guards.
- Custom/legacy GI selection retains independent shimmer identity without attaching authored mesh-local energy. `python3 -B scripts/diagnostics/run_nei_identity_tests.py` passed all 17 owner themes and 19 native MM themes through selection and toggle cases. `python3 -B scripts/diagnostics/run_nei_gi_tests.py --held --combo` passed the combined renderer and engine-header checks, including foreign-host shop fitting.
- Native MM and Tycoon rupee digits safely display every nonnegative signed16 balance without changing saved currency, wallet capacity or accumulator state. `python3 -B tests/mm_wallet/run_tests.py` passed all 65,536 balances across five paths under ASan/UBSan/bounds and the real native HUD translation-unit syntax check. The regression reproduced the native index16 crash at 1667/wallet2 and Tycoon10000 masking to1296. Only leak detection is disabled because the fixture allocates no heap and this runner cannot perform LeakSanitizer's process inspection.
- All 15 MM song variants retain descriptive localized receipt bodies with a cold donor. The combined `run_mm_item_receipt_tests.py` runner passed again after this section, including EN/DE/FR song cases and existing grant/dungeon-information guards.
- Wolf's binary loader rejects malformed bounds, counts, trees, names and NaN/Inf using serialized float bits. `python3 -B tests/mm_wolf/run_core_tests.py` passed UBSan and optimized fast-math fixtures. Host activation was integrated after the scaffold checkpoint; see the next section.
- Traditional OoT dungeon/song receipts survive the donor export; enabled map/compass and MQ information appends as a separate page. The combined receipt runner passed again with these changes.
- Generated catalog bounds now fit all 61 authored GI models under native/OoT/MM routes and pickup/shop/freestanding matrices. The production GI runner passed; Four Sword's actual top edge now fits while retaining its shelf scale. Source/held asset bytes are unchanged.
- Exact boss-soul name RGB is applied by both Latin text renderers only in recognized localized Soul-name spans. `python3 -B scripts/diagnostics/run_receipt_soul_color_tests.py` passed five souls across EN/DE/FR, wrapped/malformed/page/unknown cases, ordinary palette and red Goht Remains, plus both real textbox translation-unit syntax checks.
- Independent review found an existing Pikachu metadata layout with 48 skeleton nodes and 47 weighted bones. The strict equality guard was corrected. A fixture now links the actual generated Pikachu mesh/skeleton/registration and passes production draw submission, legacy shared scale and bounded pose queries. `run_skin_tests.py` passed again after the fix; review follow-up is in progress.

## Newly verified sections

- MM editor grants now enumerate the live NEI/EXT item table and use production grants for all four canonical slots, powers, seasons and cane/wand variants. `python3 -B tests/mm_editor/run_tests.py` passed actual catalog/FC/grant/flag behavior, ordinary inventory byte preservation, three wand rules, repeat safety, partial cane and Roc Cape cases, plus real SaveEditor/GiveItem translation-unit syntax. The previously WIP fixture is now green.
- Wolf combat keeps native MM damage and freeze actions authoritative and releases attack ownership. `run_core_tests.py` passed native damage flags, freeze with zero invincibility, bounce, dash/wall rebound and cleanup in UBSan and fast-math modes.
- Themed song geometry includes actual Soaring feather vanes/rachises. `run_song_gi_tests.py` passed all 24 profiles, native/imported mappings, early MM dispatcher and wraparound frame cases.
- The skin fixture's actual graphics arena now initializes both head and tail pointers. The null-tail regression reproduced under UBSan and was corrected. `run_skin_tests.py` now passes ASan/UBSan/bounds for synthetic and actual Pikachu data; leak detection alone is disabled for the runner's process-inspection restriction.

## Integrated host and presentation checks

- `python3 -B tests/mm_wolf/run_host_tests.py` passed the production Wolf host/core and real full-width extended-button accessors. It links the unchanged native MM freeze and thaw bodies, exercises periodic damage once, A/B thaw input, native completion without reacquiring the Wolf action, C/D-pad toggles, tool/custom-form/PAK/O2R priority, masks, death/scene/destroy cleanup and asset fallback.
- `python3 -B tests/mm_wolf/run_skin_tests.py` and `run_syntax_tests.py` passed again with the host integration; all four changed production translation units use real engine headers.
- `python3 -B scripts/diagnostics/run_sword_pose_tests.py` passed actual standalone/native and producer sword bodies: +X blades become upright +Y through all spins. `run_nei_gi_tests.py --combo` passed again, including native selection priority, foreign shelves, custom models, all 61 serialized GI bounds and authored effects.
- `python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py` passed on the integrated tree with colored native IA8 song clefs and MM-owned Ikana shield icons, including cold aliases, foreign ownership and aspect/reset behavior. `run_receipt_syntax_tests.py` passed all seven full production translation units, including the jointly edited MM DrawItem dispatcher.
- The combined regression gate now includes these host, syntax and sword tests.

## Final catalog and mod checks

- After integrating the final editor section, `tests/mm_editor/run_tests.py` passed all eight groups: actual current grants, clear/regrant host recovery without recounting earned powers, repeat/native inventory preservation, complete production Hourglass pools and Grace194/FireRod192 bindings, plus both real translation units. The extracted binding fixture now uses the production `Kind` alias and a declared effects-off CVar boundary. Its historical RED control reads published baseline `1c29c83f`, whose editor/registry blobs equal the original local control, so it survives a fresh checkout.
- The completed direct OoT exporter fixture passes all 48 traditional catalog rows, including the Chest Game key's native text ID `0xF3`. `run_mm_item_receipt_tests.py` passed again on the integrated root. The tracked receipt report preserves the worker's sanitizer and source evidence.
- `run_nei_gi_tests.py --combo` and `run_nei_identity_tests.py` passed again with arbitrary external OPA/XLU mod replacements. Those selected meshes retain independent shimmer while excluding authored fit and model-local energy.
- CI now provides BS thread-pool v4.1.0 and stb headers at the project's exact pins. Actual SaveEditor compiler dependency expansion proved they are required alongside the already-provided ImGui/JSON/spdlog/SDL headers. The accepted fixed historical source commits used by preservation tests have been fetched unchanged.

## Open independent review findings

The checkpoint is deliberately incomplete. Independent review reproduced Wolf responding to raw A/B when MM suppressed or overrode effective input; the Wolf worker is fixing effective input capture and remote-tool/textbox ownership with the actual native selection block. GI review found selected standalone/Din sword recipes return before assigning their per-item shimmer identity. Receipt review found local concrete MM OoT small keys skip the donor because their FC count-chain length exceeds one. These are assigned narrow implementation/production-fixture fixes; do not treat the passing older fixtures as proof of these omitted cases.

Final malformed-transform/renderer guards, the above review corrections, the complete combined gate and Windows/Linux builds remain in progress. In-game acceptance is not claimed.

## Resume

Fetch `recovery/final-polish-20261004`, inspect this note and the reconstruction plan, and continue the uncompleted sections. The checkpoint commit's message records its exact tree and tests. Do not treat the 43-command gate reported by the lost session as fresh proof. Keep the final PR draft and preserve its published ancestry. Existing authorization permits the final push to PR #34; no merge or master promotion is authorized.
