# Final polish integration verification

Published baseline: `1c29c83f5d676d02ececa7302c138f7aa46b952a`, draft PR #34. Recovered original root source: `9994843f`. Existing completed GI worker `282b0090` was integrated as `7df56cb8`; the implementations were preserved rather than recreated. Existing Wolf, editor/wallet, receipt/icon and preservation reports remain applicable to their stated source/fixture scope.

## Independent final review

One independent final integration review inspected `7df56cb8` against the published baseline. It reported no Critical findings, two Important findings and no Minor findings. The implementer completed one focused correction pass and added production-boundary regressions; this is not a claim of a second reviewer rerun.

| Finding | Reproduction before correction | Correction and verification |
|---|---|---|
| Wolf skin capacity rejection did not reach Player_Draw; native continuation could open exhausted arenas | Real native OpenDisps advanced all three full heads after host returned submitted=1 | Skin supports opt-in caller reserve; host returns selected/skipped=2; exact native Wolf draw branch returns before continuation. Frozen scroll/matrix and native marker capacity are reserved before skin/pose mutation. Host and sanitized skin runners pass. |
| Intrinsic weapon flames were absent from model reservation | Actual Somaria flame's twelve-Gfx scroll plus matrix exhausted OPA tail before later model matrix | OoT reservation includes actual flame and later model; MM bridge preflights native flame and all subsequent model/restore matrices. New ASan/UBSan fixture runs real OoT/MM flame, scroll and marker bodies, checks short-capacity no-op and normal submission for Somaria and selected True Master Sword. |

The combined gate also includes the new flame fixture and the restored production dependencies of the song/icon bridge fixture. No missing dependency was bypassed by replacing the production decision logic.

## Current verification

Targeted commands and results are recorded in `docs/recovery/2026-10-04-final-polish.md`. The complete 43-command `bash scripts/diagnostics/run_combo_nei_regressions.sh` and exact-head platform builds remain pending at this checkpoint. Formatting, collision, conflict-marker and whitespace gates pass.

## Runtime limits

Native matrices, deferred resource submission, GPU interpretation, real Wolf binary/assets and external model appearance require game verification. The synthetic Wolf rig and engine boundary fixtures prove source/lifecycle/allocator behavior, not runtime acceptance. Arbitrary inherited native get-item, active trail, equipment and third-party mod callbacks retain their existing allocator contract; these fixes do not prove whole-frame headroom. Actual interactions, enemy damage tables, camera/focus, freeze visuals, selected-mod appearance and Wolf passive equipment/anklet attachment remain runtime checks. Full seed-placement correctness also requires its separate existing acceptance workflow.

Keep PR #34 draft. Both game modules must build together. No merge or master promotion is authorized.

## Final harness publication review

An independent read-only publication review of `65145de1` found no Critical or Minor findings and one Important issue: the quest-held fixture was the remaining transitive consumer without native graph helpers or complete graphics arena initialization. The full gate reproduced that exact compile failure. Its runner now shares the same production helper extraction and temporary include path as the main/held fixtures; all three native arenas are initialized with explicit bounds. The quest-held runner passed after correction, covering five MM scenarios and seven OoT scenarios plus loaders/geometry/dispatch checks. The final complete gate and exact-candidate platform builds remain pending. No production behavior was changed for either harness repair.
