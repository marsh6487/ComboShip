# SDD ledger — plan: docs/superpowers/plans/2026-11-08-shuffle-parity.md

Baseline: `ced18542387d5ac8879e0f1cb303e790e4e3a1a8`, tree `ab1a683f2c91fd14a039cc58800a112ae92966b9`.
User authorized the full spec and incremental public WIP source checkpoints. Dedicated candidate branch exists. No source implementation was performed before the final authorization.

Baseline checks: `python3 -B -m unittest discover -s tests/ci -v`: 25/25 pass. Initial attempted path `tests/ci_policy` did not exist; corrected discovery path without changing tests. One chained source-read attempt stopped at a nonexistent DesiredItemHint.cpp; no test result is inferred from it.

Baseline recovery: `../deliverables/ComboShip_Shuffle_Baseline_20261108.zip` created from the exact Git commit, source-only paths. Validation and durable saving pending.

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

- Task 1: own-dungeon boss-key regression requested by user; trace combined placement and native ownership.
- Task 2: source tracing found Rando candidate helper already includes native/fractional magic; actual exported Desired Items and Saria UI path remains the reproduction target.
- Task 11: related chat reports a committed implementation and 58 controller cases/six object compilations, but exact source handoff has not yet been returned. Do not recreate or claim that work from a summary.
- Tasks 3–10,12: pending implementation, retained in the plan.
