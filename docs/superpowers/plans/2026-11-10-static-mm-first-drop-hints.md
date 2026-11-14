# Static MM first-drop hint implementation plan

> **For agentic workers:** Use the existing isolated checkout, focused failing-first regression workflow, and a final independent review. Root owns the production correction and integration; delegated workers own distinct regression files.

**Goal:** Static MM hints for an MM item placed on an OoT first-enemy drop identify the actual enemy source and retain the existing item/source colors.

**Architecture:** Correct only the `hints.mm.itemLocations` source fragment in `ComboRando::Generate`. Reuse the already-exported `enemyDropName` and English `enemyDropSource`; ordinary checks retain their region fallback. Existing MM message templates own color markup and encoding.

**Tech stack:** C++20, Python fixture runner, nlohmann JSON 3.12.0, existing native hint/message functions.

**Spec:** This document records cor's November 10 request and clarification: add the narrow static MM coverage, verify item/location colors, and keep Hyrule Field as the first-drop check's tracker/exclusion grouping rather than exposing it as a physical source.

## Global constraints

- Baseline: published `d8b4d4ca6e43010c3ea1efdc45cb43d8f41ba395`, tree `a0e243510f6c5dbb3d801e039c0364b0409dd9ad`.
- Preserve tracker/exclusion areas, check/item IDs, actor defeat behavior, reward placements, RNG, weights, saved schema and existing normal/solo hint paths.
- Keep source fragments plain. Static MM templates retain green item/yellow source and white resets; other host palettes retain their current conventions.
- Preserve the authored OoT sentence "defeating your first [enemy] rewards [item]". MM's heterogeneous drop triggers retain first-drop wording.
- No master promotion; evidence from native fixture execution is separate from configured in-game textbox acceptance.

## Review focus

- Native exported enemy source versus the organizational Hyrule Field area.
- Missing source fragment with a valid enemy identity, and old metadata without enemy identity.
- Ordinary and missing-check fallbacks, empty item names, and unchanged RNG/output outside the corrected map.
- The real static MM lookup and Moon callback consuming generated/saved hint data, with final encoded colors.
- Existing native/foreign gossip and representative environmental, lottery, and soul rewards retaining actual displayed identity and colors.

## Task 1: Reproduce and correct the static source export

- [x] Confirm the baseline gossip-routing fixture passes.
- [x] Extend `tests/cross_hints/gossip_routing_cases.inc` to fail when an MM mask at an OoT Stalfos first-drop check exports the Hyrule Field bucket; cover source/fallback and ordinary controls.
- [x] Observe the expected baseline failure before changing production.
- [x] Change `combo/rando/CrossHints.h` to export `at <first-drop source> (OOT)` for identified first-drop checks.
- [x] Run the complete gossip-routing fixture with ASan/UBSan and inspect its output.

## Task 2: Verify final static MM message colors

- [x] Extend `tests/cross_hints/run_gossip_routing_tests.py` and `mm_gossip_boundary.inc` to execute the actual static lookup, Moon helper and Moon message callback.
- [x] Feed the generated first-drop map through the actual saved-hint loader and assert the displayed item, enemy source, green/yellow bytes and white resets.
- [x] Add representative environmental, lottery and soul rewards through the existing generic gossip path; preserve foreign names/disguises.
- [x] Run existing dynamic hint-color and relevant soul regressions with the native source dependencies restored; results below.

## Completion

- [x] Independently review the bounded production change and real-consumer tests.
- [x] Verify the final diff and formatting. The recovery artifact records the final checkpoint commit, exact tree, changed-file identities and restoration proof.

## Verification results

- Unchanged baseline: the complete gossip-routing fixture passed with ASan/UBSan.
- Failing-first generation regression: the unchanged exporter produced `in Hyrule Field (OOT)` for the Stalfos first drop and failed the new assertion.
- Failing-first static consumer regression: the real Moon callback failed the literal final message/color assertion before the production correction.
- Corrected gossip-routing fixture: passed with ASan/UBSan, including the actual generator, saved-hint loader, static lookup and Moon callback. The emitted message is `They say Mask of Truth is hidden at the first Stalfos drop (OOT)...`, with green item, yellow source and white resets.
- Source controls: authored English fragments, missing/empty English metadata, missing enemy identity, ordinary areas, missing checks, empty areas and empty item names passed. Three fixed seeds produced identical output outside the corrected static map when only static export input was withheld.
- Existing dynamic hint-color fixture: all five groups passed with ASan/UBSan in combined and standalone MM configurations (sensor, remains, forms, selectors and picker).
- Existing soul-hint fixture: all 147 actual soul rows passed with ASan/UBSan, including authored multilingual names, all clarity levels and real combined generation.
- Independent read-only review: no Critical or Important findings. Updated the adjacent `MmHints::itemLocations` comment to describe source phrases as well as regions.
- Clang-format 14.0.6 accepted both changed production headers; `git diff --check` passed.

The primary fixture executes production algorithms and callback bodies. Inventory, region/item metadata inputs, RNG draw control, message wrapping and font submission remain controlled boundaries. The environmental/lottery/soul additions are representative shared-formatter tests, not exhaustive world-data traversal. No full game build or configured in-game textbox acceptance was performed. Existing saved seeds retain their stored hint strings; this correction applies when hint data is generated.

## Handoff

Preserve this bounded candidate as a commit and a recovery artifact containing the patch, changed source, baseline file copies, exact refs/trees, verification output and restoration instructions. The artifact's manifest supplies the final checkpoint identity without embedding a self-referential commit hash in this document. No game assets changed; runtime testing continues to require the accepted baseline assets. Master promotion and runtime acceptance remain separate.
