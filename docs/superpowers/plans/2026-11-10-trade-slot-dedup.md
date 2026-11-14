# Adult framing and trade-slot repair candidate

> Use one implementation worker for the coupled trade correction. The parent owns this plan, CI registration, independent verification, review and source recovery.

**Goal:** Deliver the existing adult GI framing together with the correction for duplicate non-mask trade selections, and audit all 23 unified trades.

**Spec:** `docs/superpowers/specs/2026-11-10-trade-slot-dedup-design.md`

**Starting commit:** `47a0bee04163fd2a43562cc5d4290fbfa7dffe00` on isolated branch `candidate/adult-gi-trade-slots-20261110`.

## Preflight and ownership

| Boundary | Owner | Constraint |
| --- | --- | --- |
| Trade normalization, mask eligibility, slot bindings and new trade fixtures | Implementation worker | Preserve adult GI files; keep the native slot map and quest/gate policy intact |
| All-23-row grant/use/sync audit | Read-only audit worker | Distinguish visible duplicates from necessary MM native backing state |
| Baseline neighboring regressions | Read-only baseline worker | Record exact commands and unchanged input hashes |
| Documentation, CI, final review and recoverable delivery | Parent | Stage only completed owned paths; no public push or merge |

The existing worktree is already isolated. No nested worktree is needed. The user explicitly authorized implementation; implementation decisions can proceed without another design-approval round. Source fixtures and syntax checks are the available local gates; unavailable full-game dependencies are recorded rather than replaced with invented stubs for a build claim.

## Task 1: Trade normalization and direct regression coverage

Owned production paths are the narrow trade/mask/inventory and registry consumers in `soh/`; new fixtures live under `tests/unified_trade_slots/`. Existing fixture changes are allowed only when needed to exercise the changed dependency honestly. Root owns `scripts/ci/regressions.json`.

- [x] Verify the adult checkpoint and baseline adjacent tests without changing their inputs.
- [x] Write real behavior assertions for duplicate child-slot migration; observe the expected failure on the baseline before production edits.
- [x] Implement idempotent normalization and masks-only eligibility in the intended combined randomizer mode.
- [x] Preserve trade ownership, selected unrelated trades, live/age equipment identity, native item replacement, masks and original build/mode guards; configured quest exchanges remain a runtime check.
- [x] Verify actual hatching followed immediately by age restoration, and normalize before same-frame pause equip resolves its item and slot.
- [x] Cover all 23 entries and both hosts' inventory boundaries, with sanitizer checks where supported.
- [x] Self-review and report exact changes, commands, failures and limitations.

## Parent integration

- [x] Confirm audit coverage and inspect the production diff against the specification.
- [x] Add the bounded regression to CI and replay the relevant adjacent runners.
- [x] Re-run adult GI framing and verify its production files remain identical to the adult checkpoint.
- [x] Obtain a fresh independent review of the final cumulative candidate; fix material findings and verify the fix.
- [x] Prepare the complete candidate and cumulative restoration recipe from the published baseline.

Post-checkpoint actions are recorded in the delivered artifact: exact commit/tree identities, independent restoration proof and durable delivery. Keeping those final identities outside this committed plan avoids changing the tree being verified.

## Verification status

Before implementation, all six adjacent runners passed: OoT Rito ownership/migration; Rito randomizer behavior; OoT page navigation and same-frame checks; OoT cursor checks in solo and combined variants; MM Rito inventory; and native MM pause navigation/entry. Their 26 recorded direct inputs remained unchanged. Final results are recorded below when the implementation is complete.

## Final verification outcome

The exact new CI entry passes normal and AddressSanitizer/UndefinedBehaviorSanitizer runs. It covers all 23 identities in both hosts, the three overlapping child trades, legitimate masks, idempotence, live and saved equipment banks, native hatch followed immediately by age restoration, registry folding, and actual same-frame pause input. Native C and C++ objects use matching explicit ABI flags. Leak detection is disabled for the managed environment's ptrace restriction; no leak-check claim is made.

All six neighboring runners pass. The cursor-only fixture needed a new no-op external normalization service because it has no trade cases; the new trade fixture separately executes the real normalizer against the actual native pause input block. Both complete changed native translation units (`z_parameter.c` and `z_kaleido_item.c`) pass syntax checks in original and combined modes. The latter text-includes the changed `extended_inventory.c`; compiling that fragment separately without its enclosing types was an inapplicable preliminary attempt, retained in the evidence.

Adult GI's exact CI entry passes all nine groups normally and under sanitizers. Its five production files and two test files remain identical to `47a0bee`. Fresh independent review found no remaining Critical or Important defect after the hatch/age and early-pause ordering corrections.

Full C++ translation-unit/build verification remains blocked by the previously established absent real dependencies, including spdlog. No complete game executable, configured adult Din visual acceptance, real save transition or guard/Talon exchange was run. Native/editor writes may use the old child backing slot transiently; save-load and early page-zero pause reconciliation repair completed writes. Gate/starting-Letter policy and MM hidden native quest homes are unchanged.
