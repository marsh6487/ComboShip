# Demise's Destruction: violet medallion discharge

| Field | Record |
|---|---|
| Baseline | ComboShip `bridge/audit-merge-20261008`, commit `7511aedad2e21ce22e9c3fea1f01782d475e04f0` (PR #41 source). Both original Demise fixtures passed before editing. No new baseline runtime claim is made. |
| Candidate | `poc/demise-medallion-discharge-20261014`, isolated from that source parent. |
| Scope | Replace Demise's thirteen bomb sprites and red dust with a violet seal, pale-core branching ground electricity, an expanding electrical ring, and fading motes. Share the bounded renderer between SoH and MM. |
| Preservation | Existing damage flags, damage amount 40, radius 400, height 200, five attack ticks, magic cost 12, 70-tick windup, animation, camera sequence, audio, and native lightning schedule. Shared explosion helpers and animation assets remain byte-identical to the parent. |
| Configuration | No new external asset pack or Alt asset mapping: a private native 8x16 I8 strip is embedded in the spell renderer. Draw from the local player's world path before replacement-model forks; suppress pause preview draws. |
| Evidence | Both actual-item/native-lightning fixtures pass with AddressSanitizer, UndefinedBehaviorSanitizer, and float-cast-overflow checks. Leak checking is disabled because this environment blocks it. Shared geometry is identical in both exports; peak 357 quads under a 384-quad cap. A code review found no remaining blockers. |
| Verdict | Implemented and fixture/static verified. Offline preview reviewed with the seal corrected to violet. Full game builds and in-game visual acceptance remain untested. The cumulative NEI gate is blocked by a missing local CMake executable at the reward asset-packaging check. No master promotion. |
| Recovery | Discard this candidate's isolated diff or return to the exact parent above. No accepted source branch is overwritten. |

The collider already owns the damage independently of `EffectSsBomb2` sprite spawning. The candidate removes only those visual spawn calls. It does not change damage type to a different mask or extend the attack window: the 28-tick afterglow is draw-only. Native interrupted-cast cleanup and MM damage-mask compatibility are existing behavior and outside this visual change.

The renderer reserves aligned vertex and matrix space plus native debug-marker headroom, checks XLU command capacity, uses explicit translucent render state, and restores the host setup afterward. Allocation failure skips this visual layer without changing damage. Scene/play changes, clock rewind, cancellation, and unequipping invalidate its state. Foreign player draws cannot clear or duplicate the local effect.

## Checks

```sh
python3 -B scripts/diagnostics/run_demise_lightning_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_demise_lightning_tests.py --sanitize
python3 -B scripts/diagnostics/run_mm_demise_lightning_tests.py
ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_mm_demise_lightning_tests.py --sanitize
git diff --check
```

The additional vertex-batch and Alt segment-binding fixtures passed. The cumulative NEI gate passed its initial hint, crash-owner, four Demise, tunic/Mario, held/common GI, elemental-arrow and reward-renderer checks. It then stopped at `tests/reward_gi/run_asset_packaging_tests.py` because this environment has no `cmake` executable; later checks did not run. This is a blocked gate, not a full-suite pass. The gate requires the fixed historical sources and pinned dependency headers documented in `.github/workflows/build-artifacts.yml`.

## Preview and next decisive check

Set `DEMISE_FX_EXPORT` to a JSON path when running either Demise fixture, then run:

```sh
python3 -B scripts/diagnostics/render_demise_spell_preview.py geometry.json output-directory
```

The offline preview uses actual emitted vertices, colors, UVs, and the native strip with ordinary alpha blending. It omits the character animation and retained native lightning sprites; it is not an engine capture.

In each game, compare parent and candidate from the same flat scene/save with an enemy in range. Confirm the violet seal/discharge, unchanged enemy hit and magic cost, no bomb sprites, and a fading afterglow after the five attack ticks. Then check a slope, pause/unpause, unequip/cancel, scene transition, and a replacement player model. Repeat the affected rendering paths with Alt Assets on and off before accepting the visual result.
