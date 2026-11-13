# Blast Mask and form combat implementation plan

> For agentic workers: use test-driven execution, explicit file ownership and verification before completion.

**Goal:** Restore native B-only shield-safe Blast Mask use and correct confirmed Zora/Goron melee defects.

**Architecture:** Preserve each game's native item and collision dispatch. Repair MM's eligibility boundary, OoT's imported mask input/collider policy, and only source-proven form hitbox defects. Deliver an isolated source patch against the pinned full upstream commit.

**Tech stack:** C/C++, native engine source, Python fixture runners.

**Spec:** docs/poc/blast-mask-form-combat-20261112.md

## Global constraints

- Upstream baseline: 267fc64d7ac6b0f0d6c14a3f108b1a2e7299a02e.
- A must never request a Blast explosion; B provides detonation and matching feedback.
- Shield protection must apply only to the mask's own blast, leaving ordinary bombs and enemy attacks unchanged.
- Preserve native disabled statuses, cooldowns, temporary B items, form gates, damage values, movement, audio, camera and assets.
- This is a partial inspection checkout; only a reviewed delta may be applied to the full repository. No publication or promotion in this task.

## Review focus

- Same-frame R+B versus already-raised shield; no shield plus raw R must not gain immunity.
- A-only / A+B, contextual A interactions, cooldown, failed bomb spawn, and blocked gameplay states.
- Imported swords versus native swords, unrelated temporary B payloads, disabled status, and nonhuman forms.
- Attack registration/geometry through form update/draw ordering, with native enemy collision controls.
- Preserve full source baseline and unrelated files while assembling recovery package.

## Task 1: MM mask-action eligibility

Owner: mm_blast_trace. Files: mm/src/code/z_parameter.c; tests/mm_mask_actions/*.

- [x] Add behavioral regression through the real interface setter and B readers; run and observe Master Sword failure.
- [x] Extend exact extra-sword eligibility only for human mask actions.
- [x] Verify Blast/Bremen/Kamaro, native swords, temporary B items, disabled flags and transformed controls.

## Task 2: OoT B-only activation and self-blast shield safety

Owner: oot_blast_trace. Files: soh/mods/transformation_masks/mm_mask_wear.cpp/.h, transformation_masks.c, soh/mods/extended_player.c, PauseItemDescriptions.cpp, tests/blast_mask/*. Parent applies agreed shared-player/UI hunks.

- [x] Add failing B-only/A-release and shield/self-blast collision regressions using native production functions.
- [x] Implement input, HUD/tutorial and blast-instance protection with source-verified update ordering.
- [x] Verify unshielded damage, raised shield protection, ordinary bomb/enemy damage, cooldown and native A actions.

## Task 3: Zora/Goron melee

Owner: form_combat_trace after diagnosis. Expected files: dedicated form combat/render helpers; parent owns shared player/renderer changes.

- [x] Identify source-proven geometry/timing/registration defects against native MM.
- [x] Add a failing source/geometry collision reproduction for each confirmed defect.
- [x] Apply the smallest native-compatible repair and verify attack/contact controls.

## Task 4: Integration and recovery

Owner: root.

- [x] Register focused regressions in scripts/ci/regressions.json and run relevant checks.
- [x] Inspect complete delta and obtain independent review of changed behavior.
- [x] Record exact source checks, native compilation limits and untested runtime configurations.
- [x] Commit bounded source delta and prepare the recovery package. Exact artifact restoration and persistence results are recorded in the delivered package and final handoff.
