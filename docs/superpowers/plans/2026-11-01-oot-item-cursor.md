# OoT item cursor repair implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement this bounded regression repair and obtain an independent final review.

**Goal:** Let cor navigate from Light Arrow across empty cells to the corner bottle, Deku Stick and Deku Nuts, with a correctly aligned cursor.

**Architecture:** Preserve the existing 6 by 4 item grid and ownership resolver. Honor the existing Pause Any Cursor policy in automatic movement, vertical navigation and cursor geometry, while keeping empty cells non-equippable and unnamed.

**Tech stack:** Native C pause code; Python-driven extracted native C tests, ASan and UBSan.

**Spec:** Cor's supplied November 1 clip and report; current source `377e8191fb389ad8aec28f89879f0dd148925673`, PR #77, tree `b42f00013cddb567cdf134fee6c469c879f36e7b`.

## Global constraints

- Change only the OoT item selection path, focused regression runner, CI registration and evidence documentation.
- Preserve item ownership, equipment IDs/slots, page switching, item wheels and MM behavior.
- Keep accepted masters and existing candidate branches intact; this is a separate candidate.
- The local checkout is a verified source subset. Publication must apply only named changes to the full upstream tree, never publish the subset as a full replacement.
- Native regression evidence does not establish full executable or configured gameplay success.

## Review focus

- Neutral frames must not move an intentionally selected empty cell.
- Vertical movement must continue from an empty cell for analog and D-pad navigation.
- All four cursor corners must follow the selected cell, including after an inactive quest/map draw.
- Empty cells must retain no item name and must not equip stale items.
- Disabled and randomizer-only cursor policies, page offsets and page-arrow handoffs must retain their existing behavior.

## Task 1: Repair the empty-cell navigation/render boundary

**Files:** `soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c`, `tests/oot_pause_inventory/run_cursor_tests.py`, `scripts/ci/regressions.json`, and publication evidence.

**Interface:** Keep `KaleidoScope_DrawItemSelect(PlayState*)` and existing settings unchanged. Exercise its actual input block and native cursor vertex helpers.

- [x] Add native regressions for the supplied sparse inventory, idle stability, vertical traversal, ownership/equip, all three subpages, both input methods and cursor geometry.
- [x] Run against unchanged source and record the expected navigation and geometry failures.
- [x] Gate forced movement by Pause Any Cursor, admit empty-cell vertical navigation, and assign complete cursor geometry independently of item ownership.
- [x] Run the new runner and existing pause/empty-selection runners under sanitizers, then obtain independent code review.
- [x] Register the focused runner in CI and record preservation and runtime limits. Candidate recovery uses the full upstream parent plus a tested delta patch containing the six named changed paths. GitHub publication is pending explicit approval after automatic review rejected the first blob upload.

## Decision record

The reported behavior has one concrete cause: the stale-empty-item correction sets `PAUSE_ITEM_NONE`, which the older movement path treats as a reason to synthesize rightward input even when empty-cell selection is enabled. The empty rendering branch hides only the first four of sixteen cursor vertices, leaving three corners supplied by an inactive page. Fix the interaction without altering the user's setting or redesigning navigation. Separate opening-time cursor synchronization defects found during exploration remain outside this candidate because they do not cause the repeated snap in the supplied clip.
