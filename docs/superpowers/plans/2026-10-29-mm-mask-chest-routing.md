# MM mask chest routing implementation plan

> **For agentic workers:** Use superpowers:executing-plans for this single bounded change.

**Goal:** Show the existing 3DS major-item chest for mask rewards.

**Architecture:** Alias only the visual `RITYPE_MASK` model entry to the existing major body/lid. Preserve item classification, reward delivery, sizing, opaque-pass guards, and pairwise resource fallback.

**Tech Stack:** C, C++, existing native production-draw harness with UBSan.

**Spec:** User explicitly requested masks use the major-item chest. Fairy routing is undecided; lesser-item styling is a recommendation only.

## Global Constraints

- Parent: repaired ComboShip `3cbb22d1f09e85deb206d2b085fc62105bc09fad`.
- No archive changes, fairy changes, gameplay changes, or publication. Keep the actor/music combined candidate's publication hold.

## Review Focus

- Every native mask slot selects both major model parts, with original reward/actor state preserved.
- Randomizer and foreign mask categories select the same model without changing their category.
- Missing or failed body/lid retains the native pair; never mix model halves.
- Existing default-off, fading/Lens, boss-key, minigame, and six-category behaviors remain covered.
- Stray fairies keep their current native fallback until the user selects their appearance.

### Task 1: Add the mask visual mapping

**Files:** `mm/2s2h/Enhancements/Graphics/ChestContents.c`, `mm/tests/chest_contents_test.cpp`.

**Interfaces:** `MMChest_PrepareDraw(EnBox*, PlayState*)` produces the optional body/lid resource names consumed by the real native and randomizer chest draw functions. No signature changes.

- [x] Extend the existing draw tests for native masks, randomizer masks, and missing/failed mask-model resources.
- [x] Run `python3 scripts/diagnostics/run_mm_chest_contents_tests.py`; the new mask draw check failed at `ExpectModel` on the unchanged production code.
- [x] Add `[RITYPE_MASK] = { sBodyMajor, sLidMajor }` to the visual model table.
- [x] Run the complete MM chest harness and the adjacent OoT chest-size harness; both passed. MM emitted 57 existing controller-macro/const warnings; OoT emitted none.
- [x] Independent review found no issues. Commit the bounded candidate and retain its exact source changes for the held combined-source candidate. Runtime validation remains pending.

## Verification ledger

The existing MM harness passed before test changes. The new regression then failed on the missing model routing; after the one-line production change the entire MM harness passed with UBSan and full C actor syntax compilation. The adjacent OoT chest-size harness passed. `git diff --check` passed. An independent reviewer found no issues.

No archive or runtime configuration changed. The source still classifies masks as `RITYPE_MASK`; only the optional display-list pair aliases the major-item art. Native and randomizer stray-fairy rendering are unchanged. A full executable build and configured in-game test are pending.

### Task 2: Follow-up fairy appearance and size

The user accepted the Minor model and asked whether the chest should be small. Implementation choice: use small visual size when the existing size-matches-contents option is enabled; retain authored size when it is disabled. Keep the actual `RITYPE_STRAY_FAIRY` category and reward intact. This supersedes Task 1's temporary exclusion of fairy routing; unrelated behavior and publication hold remain protected.

- [x] Add production-draw tests for native/randomized/foreign fairy rewards, both authored sizes, independent style/size controls, and missing Minor resource parts.
- [x] Verify the tests fail on the absent model mapping, then on the old large-size rule.
- [x] Map only fairy art to Minor and include `RITYPE_STRAY_FAIRY` in the visual small-chest rule.
- [x] The complete MM chest harness passes, including full C actor compilation. Review found no issues. Integrate the verified source into the same held combined candidate without archive changes or publication.

### Task 3: User-requested OoT parity

The user additionally requested the same classification on OoT. Preserve OoT's existing Major routing for masks. Identify the five concrete fairy rewards and foreign fairy checks only within chest cosmetics; use Minor art and optional small size without changing item-table categories, fill/progression flags, other container cosmetics, foreign delivery, or trap handling.

- [x] Add a failing production-actor regression for all five concrete fairy identities across authored chest types and independent size/style settings.
- [x] Add a narrow C-callable fairy classifier using the existing foreign lookup, guarded by `COMBO_BUILD`, and use it only in OoT chest cosmetics. Keep invalid checks and foreign traps on their existing paths.
- [x] Exercise the real classifier and actor for ordinary OoT and ComboShip builds, including foreign fairy/mask/trap/missing-category cases and partial model fallback. A foreign-branch mutation failed the expected size assertion; restoring the classifier passed.
- [x] Both engine suites pass. OoT's two-mode C/C++ bridge and complete actor compile without warnings; MM retains its 57 existing warnings. Independent review of the combined follow-up found no issues. Retain both changes in the same held source; runtime remains unproven.
