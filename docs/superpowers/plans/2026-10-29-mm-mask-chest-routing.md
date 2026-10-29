# OoT/MM chest classification implementation plan

> **For agentic workers:** Use superpowers:executing-plans for this bounded source correction.

**Goal:** Preserve OoT's Major chest behavior for masks and stray fairies and give MM the same Major appearance.

**Architecture:** Add only MM visual aliases for `RITYPE_MASK` and `RITYPE_STRAY_FAIRY` to the existing Major model pair. Keep OoT production code and MM's size rule at the repaired parent; both categories are large when size matching is enabled.

**Tech Stack:** C/C++, existing production-function harnesses and UBSan.

**Spec:** User's final instruction: “if oot considers stray fairies as major then we should mirror that in 2ship”. This supersedes the earlier Minor/small appearance choice.

## Global Constraints

- Parent: `3cbb22d1f09e85deb206d2b085fc62105bc09fad`.
- Keep actual reward categories, item tables, delivery, flags, actors, packs and original settings intact.
- Preserve local source history and save an authorized remote source recovery checkpoint. Hold gameplay publication and accepted masters.

## Review Focus

- MM native/randomizer/foreign masks and fairies select a complete Major model pair.
- Size matching enables large size; disabling it restores authored size.
- Resource failures, transparency, traps and minigames retain their existing behavior.
- OoT production remains exactly the repaired parent's code; only test coverage expands.
- Actor and music repairs in the held combined candidate remain intact.

### Task 1: Final Major classification parity

**Files:** MM ChestContents.c and chest_contents_test.cpp; OoT chest test and diagnostic runner plus a small test-only C++ category bridge; POC/recovery documentation.

- [x] Update behavior tests for the final Major/large expectation.
- [x] Observe both revised regressions fail against the superseded Minor/small source.
- [x] Restore all three OoT production paths and MM's original size predicate; map MM mask/fairy models to Major.
- [x] Run both chest suites, including ordinary OoT and COMBO_BUILD actor compilation, and check the bounded diff.
- [x] Final independent review found no issues. Preserve the corrected source in new commits; record the authorized remote recovery checkpoint separately.

## Preservation

All earlier local commits remain in git history. This revision replaces current behavior without deleting prior checkpoints or clearing either workspace. A source recovery branch is separate from permission to publish a build or merge protected branches.
