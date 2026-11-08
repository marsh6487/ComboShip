# SDD ledger — plan: docs/superpowers/plans/2026-11-08-shuffle-parity.md

Baseline: `ced18542387d5ac8879e0f1cb303e790e4e3a1a8`, tree `ab1a683f2c91fd14a039cc58800a112ae92966b9`.
User authorized the full spec and incremental public WIP source checkpoints. Dedicated candidate branch exists. No source implementation was performed before the final authorization.

Baseline checks: `python3 -B -m unittest discover -s tests/ci -v`: 25/25 pass. Initial attempted path `tests/ci_policy` did not exist; corrected discovery path without changing tests. One chained source-read attempt stopped at a nonexistent DesiredItemHint.cpp; no test result is inferred from it.

Baseline recovery: `../deliverables/ComboShip_Shuffle_Baseline_20261108.zip` independently restored 5,349 source files with exact Git blob identities. SHA-256 `9b1ca6bec5362c61e0536a76219ec800e5bbcce7f6d22ed14de29908f5b3ce1b`; saved as `libfile_2362c5944e2c81919ca100f8a97e1743`.

Published scope checkpoint: local `b3280ce58e9993ff9180d8b83096984c7961aa8a` -> public `f6d92583ff8b136ecfec81b4b08d29e26e38db63`, identical tree `aad3d16c3719e0ef93410ad57081b8a9ed305b3e`. Dedicated draft PR #89 created against PR #88's candidate. This initial public checkpoint contains the scope/plan only.

## Preflight shared interfaces

| Tasks | Shared boundary | Finding and handling |
|---|---|---|
| 1, 3–10 | Native item/check/options and combined fill | Preserve all baseline IDs. Append new families; controller integrates sequential commits and resolves new-ID ordering before accepted seeds. |
| 2, 3–10 | Hint candidates and native item identity | Candidate selection must reflect the actual exported UI and dynamic conversion, not only Rando helper functions. |
| 3, 4 | Enemy soul ownership and kill reachability | Task 4 consumes actual soul-aware kill logic; no runtime-only gating. |
| 5, 6, 7 | Capacities, small keys, door interaction | Keep native dungeon key ownership distinct from overworld door keys and foreign OoT rings. |
| 8, 10, 11 | Playable songs, time and form instruments | Seed event assignment and solver must agree with each form's actual songs and bank ownership. |
| 1–11, 12 | CI fixtures and shared enums | Register checks after native production verification; never weaken existing assertions for convenience. |

Ruling: Retain MM enemy immunity for initial parity rather than silently changing to OoTMM spawn suppression — preserves native behavior; changing later requires an explicit runtime milestone.
Ruling: Treat upstream equivalent-check candidates (egg hatching/diving/fairy/bush subfilters) as research rather than quietly asserting missing functionality — approved implementation targets the named confirmed gaps; costs a follow-up if deeper parity proves useful.

## Active work

- Task 1: reproduced and fixed stale live-save confinement. Native MM catalogs/pools and real combined fill passed 64 Own Dungeon seeds, 64 excluded-check exports and 16 Anywhere controls; randomized confined self-locks reject. Review-found failed-dump fallback publication now rejects before GUI structural work. See `docs/SHUFFLE_BOSS_KEY_REPORT_20261108.md`; linked dual-native generation and configured gameplay remain pending.
- Task 2: actual MM catalogue reproduction failed for missing RPG entries. Six append-only RPG selectors and matching native Saria/Desired Items paths implemented; source evidence in `docs/SHUFFLE_HINT_PARITY_20261108.md`. Native original/Combo sanitizer tests pass; review corrections skip capped/disabled Saria RPG stats and filter menu availability by configured pool. Bounded fix review approved `d3df01069fceba8dfa83c4441c600e663f32e1cf`. Complete cross-world/game/runtime verification remains pending.
- Task 11: exact original form-instrument candidate `76a1fee1be88aa8c01cc06f44d20f485031987ad` recovered from its source-only ZIP; target tree `5d13866641bcc62066a2cf8195171c55e2381a95`, baseline tree matches PR88. Package SHA-256 `11a7b7db1977f191fecf0dc189ca406bc3fccf1c7f8a9fa8371b1d9f3999c3b7`, saved as `libfile_7426260765b08191830915b66e3ec70c`. Independent verification matched all 45 archive checksums, 20 changed-file blobs/modes/lengths, 21 evidence records and reconstructed the exact full tree from PR88. The verified patch is integrated without overlaying the shared CI manifest. Integrated controller tests pass 58 cases across original/Combo; CI policy 25/25. Native object builds, integrated audio and configured runtime remain separate gates. Required MM Soundfont 0/samples and existing model resources are not included or published.
- Task 3: append-only MM soul ownership save codec integrated; 154 new native catalog/grant/pool/oracle foundations integrated as `3e688310ce7a04550a9f36c3997ce5364c0bb311` from isolated worker commit `78aa221e9d3b06d73206a7ce447bb79450423c60`. Runtime/solver/shared behavior remains WIP and menus stay unexposed.
- Tasks 4–10,12: pending implementation, retained in the plan.
- Additional screenshot regression: Chateau Romani shelf renders black. Source audit identified missing per-actor shelf light setup as the leading hypothesis; no visual correction or gameplay result claimed yet.

Bounded boss-key review approved `ff6df34c`: no new material findings after failed-dump fallback correction. Hint production translation unit at `d3df0106` compiled successfully with the actual generated Release command. Full native game build remains in progress; this is not gameplay acceptance.
