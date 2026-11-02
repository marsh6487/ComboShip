# Dialogue Icon Repairs Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Stabilize boss-soul dialogue icons and show the correct authored lantern icon for each captured fire state.

**Architecture:** Capture receipt image identity when constructing the message, so rendering is independent of transient get-item actor state. Use the existing value-owned receipt presentation, with the verified RGBA32 32x32 assets; retain text, gameplay, GI models, and native inventory behavior.

**Tech Stack:** C/C++, Python native extraction fixtures, ASan/UBSan.

**Spec:** cor approved `docs/superpowers/specs/2026-11-02-combo-vanilla-nei-design.md` and added these two repairs on November 2. This plan is an independently testable prerequisite milestone; Combo Vanilla implementation follows it.

## Global Constraints

- Inherit full remote `ac19dda4a7c580988c7641c7b8bf93faaba1790e`; preserve existing bugfixes and input archives.
- Work on `candidate/combo-vanilla-icons-20261102`; unchanged source imports are recovery-only and must not be published as changes.
- Four lit lantern resources plus unlit fallback exist, all verified 32x32 RGBA32 PNGs.
- Preserve all three OoT tutorial languages and native/MM boss receipt routes.

## Review Focus

- A get-item actor can expire before dialogue finishes; receipt must retain its image.
- Captured fire can differ from live equipped fire; tutorial uses the captured value.
- Empty/unknown fire values need the unlit fallback.
- Literal item-ID bytes can drift when inventory changes; use the named item constant.
- Existing native/foreign soul routes and long-hint formatting must stay intact.

### Task 1: Captured lantern icons

**Files:** Modify `soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp`; inspect MM's `lantern_message.cpp`; create `combo/menu/ComboLanternReceiptIcon.h` and `tests/item_receipts/run_dialogue_icon_lifetime_tests.py`.

**Interfaces:** `ComboLanternReceiptIcon(unsigned fireType)` returns the existing local resource path: 0/unknown unlit, 1 Fire, 2 Blue, 3 Poe, 4 Green. Existing receipt metadata carries the path and 32x32 RGBA format.

- [x] Write and run a native fixture extracting the actual tutorial builder; assert each captured state, all languages, no Minish Cap operand, and independence from subsequent live-state changes. Observe the old path fail.
- [x] Remove raw `0xB4` operands, use `ITEM_LANTERN` and capture the correct authored path in the value-owned presentation. Inspect and repair MM's equivalent only where its source needs the same correction.
- [x] Run the fixture with sanitizers and existing receipt-layout tests; commit the independently verified repair.

### Task 2: Soul receipt lifetime

**Files:** Modify the soul branch in `soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp`; extend `tests/item_receipts/run_dialogue_icon_lifetime_tests.py`; retain existing `tests/item_receipts/run_soul_icon_tests.py` coverage.

**Interfaces:** Existing boss-soul message construction snapshots its custom texture with 32x32 RGBA32 metadata. It uses the ordinary receipt renderer even after `getItemEntry` changes.

- [x] Extract the actual soul branch into the native fixture and assert the emitted message owns the correct image after actor invalidation. Observe failure before implementation.
- [x] Stage the image in `receiptPresentation` and use the safe icon sentinel, without truncating the RG identity into an inventory operand or changing GI art.
- [x] Run native soul-format, receipt-layout, tutorial and message-replacement tests with sanitizers; retain explicit pending status for configured game runtime.
- [x] Append the lifetime gate to `scripts/ci/regressions.json`, preserve every previous gate, review the bounded diff and publish a recoverable GitHub checkpoint.

## Execution ledger

Authorization: cor approved implementation and these two bugfixes; proceed inline without another design/permission round. Runtime acceptance remains separate.
