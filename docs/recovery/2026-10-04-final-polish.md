# Final polish recovery checkpoints

Published PR #34 baseline: `1c29c83f5d676d02ececa7302c138f7aa46b952a`.

The historical final candidate disappeared from the transient workspace. The supplied partial recovery ZIP contained 14 exact blobs from GitHub staging tree `dc4171c3587e52e62359ab6ee1f06ecaf03a870a`; their Git hashes and archive SHA-256 checks passed. It did not contain the complete gameplay source or tests. The reconstruction plan names the accepted requirements and file ownership.

| Checkpoint | Content | Verification | Remaining work |
|---|---|---|---|
| `c54ca14d3555324731412c35a0242e5dd334afcc` | Exact recovered headers and accepted reconstruction plan | Remote tree `34ea8dc7c7a5c4ccd49ce13c42fc4622eb58004d` equals the local tree; fetched branch verified | Reconstruct gameplay implementations and fresh tests |

## MM skin section

Local worker commit `12d055506bbdb49280cd30e8502672b9a10e91e7` adds opt-in root motion, fractional animation interpolation and definition-only scale while retaining zero-default Pikachu behavior. Production pose computation and native MM matrix code are exercised by `python3 -B tests/mm_wolf/run_skin_tests.py`. The fixture failed on the previous renderer's packed vertex position and passed with the change. Queries reject another character's pose and destroyed skins.

## Other completed sections

- Yellow magic receipts retain localized yellow identity and RPG remaining-upgrade counts; saturated tiers no longer wrap. `python3 -B scripts/diagnostics/run_mm_item_receipt_tests.py` passed on the combined checkpoint, including native/foreign receipt catalogs, grants, latch state, Skulltula totals and dungeon-information guards.
- Custom/legacy GI selection retains independent shimmer identity without attaching authored mesh-local energy. `python3 -B scripts/diagnostics/run_nei_identity_tests.py` passed all 17 owner themes and 19 native MM themes through selection and toggle cases. `python3 -B scripts/diagnostics/run_nei_gi_tests.py` passed the native renderer and engine-header checks. Foreign-host shop fitting and song effects are still being reconstructed.

This is a partial reconstruction checkpoint. Wolf loader/core and MM host integration, editor and wallet fixes, GI fitting/song effects, remaining receipts/icon ownership, combined review and complete Windows/Linux builds remain in progress. In-game acceptance is not claimed.

## Resume

Fetch `recovery/final-polish-20261004`, inspect this note and the reconstruction plan, and continue the uncompleted sections. The checkpoint commit's message records its exact tree and tests. Do not treat the 43-command gate reported by the lost session as fresh proof. Keep the final PR draft and preserve its published ancestry. Existing authorization permits the final push to PR #34; no merge or master promotion is authorized.
