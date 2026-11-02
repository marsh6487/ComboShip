# OoT empty-cell cursor repair candidate

## Baseline and scope

- Repository: `marsh6487/ComboShip`.
- Parent: `377e8191fb389ad8aec28f89879f0dd148925673` (PR #77), tree `b42f00013cddb567cdf134fee6c469c879f36e7b`.
- Integrated candidate branch: `candidate/crash-hints-market-magic-trace-20261102` (PR #78). The original isolated proposal was `candidate/nov01-oot-item-cursor-20261101`.
- Original item-selection blob: `d97c2abf7fa1932ac5ef4b99af06587b0b317cc2`.
- Scope: OoT item-page cursor navigation and geometry, focused regressions and their CI registration.
- Existing cumulative assets, renderer settings, MM code, item ownership and equipment storage remain unchanged.
- Publication status: cor explicitly authorized adding this repair to the cumulative GitHub checkpoint on November 2. It is integrated into PR #78. The earlier automatic rejection belongs to the original November 1 donor investigation and does not describe the current authorized integration.

## Cause and expected behavior

With Pause Any Cursor active, navigation can select an empty cell. The recent stale-item repair correctly resolves that cell to `PAUSE_ITEM_NONE`; however, the older fallback then replaces the next frame's horizontal input with rightward input. A move left from Light Arrow at visual cell 16 reaches empty cell 15 and immediately snaps back. The same interaction blocks traversal from the net at 20 across empty cell 19 to the bottle at 18. Vertical navigation also rejects an already selected empty cell.

The displaced three-corner cursor has a related rendering cause. The inactive quest page draws first and initializes the shared cursor vertices around quest cell zero at `(74,38)`. The empty-item branch hides only the first four vertices, leaving three corners at the quest position. The clip's offset upper-right cursor and missing top-left corner match this path.

The repair makes intentional empty cells stable, permits vertical continuation through them, and sets all four cursor corners at the selected item-grid cell. When empty-cell selection is disabled, all sixteen corner vertices are hidden on an empty selection. Empty cells remain unnamed and cannot equip a stale previous item. Switching Pause Any Cursor to **Never** was reported to restore navigation in the existing executable; this is the immediate workaround.

## Verification

The new `tests/oot_pause_inventory/run_cursor_tests.py` extracts the real item-selection input block and the real `KaleidoScope_SetCursorVtx` / `KaleidoScope_UpdateCursorSize` implementations. It uses the native headers and structures. Graphics submission, audio and external effects are fixture boundaries.

Before the production change, both standalone and combined variants failed all five affected cases: idle stability, owned-item reachability, vertical movement, cursor alignment and complete hiding after an empty-page switch. The disabled-policy control passed. These failures were observed before editing production code.

After the repair, all six cases pass in both variants under AddressSanitizer and UndefinedBehaviorSanitizer. Cases cover analog and D-pad navigation, all three subpage offsets, Always / randomizer-only policy behavior, Never preservation, twenty neutral frames, the bottle/sticks/nuts/ocarina/net/bugs path, correct bottle equip identity, all sixteen cursor coordinates and stale equip rejection.

The following existing checks also pass under sanitizers:

```sh
python3 -B tests/oot_pause_inventory/run_tests.py --sanitize
python3 -B tests/nei_controls_oot/run_selection_tests.py --sanitize
```

All three focused runners also pass through `scripts/ci/run_regressions.py` with its combined graphics ABI compiler adapters (`GBI_FLOAT_MTX` and `GBI_S32_VTX`). The new runner is registered in the equipment CI group. An independent read-only reviewer reran the three direct runners and found no Critical or Important issue.

Full executable compilation and configured gameplay of this candidate were not performed locally. The supplied clip proves the original visual symptom; the user's setting change proves the workaround, not runtime acceptance of the new source.

## Remaining checks and boundaries

- In the candidate executable, enable the previous Pause Any Cursor mode, traverse from Light Arrow through empty cells to the bottle, sticks and nuts, and equip the bottle.
- Check pause close/reopen and side-arrow exit/reentry. Enabled-policy side-arrow reentry is not explicitly covered by the new focused tests; the existing arrow tests cover the disabled policy and those branches are unchanged.
- The separately discovered `KaleidoScope_SetDefaultCursor` opening-time synchronization and last-cell wrap defects are outside this candidate. They do not explain the repeated snap addressed here.
- No accepted-master promotion, protected-branch merge or game release is part of this candidate.

## Original isolated recovery and preservation

The six-file scope below describes the original donor patch. The cumulative PR #78 also contains the other authorized fixes listed in `docs/checkpoints/20261102-crash-hints-market-magic.md`; its complete diff is not limited to these six paths.

The local investigation used a bounded source projection because direct Git transport was unavailable. Its local root snapshot is not a replacement repository history. Remote publication must construct the candidate tree from the full parent tree above and modify only:

1. `soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c`
2. `tests/oot_pause_inventory/run_tests.py` (fixture cursor storage matches the native twenty-vertex allocation)
3. `tests/oot_pause_inventory/run_cursor_tests.py`
4. `scripts/ci/regressions.json`
5. `docs/superpowers/plans/2026-11-01-oot-item-cursor.md`
6. `docs/publication/2026-11-01-oot-item-cursor.md`

Before publication, restore a complete ComboShip checkout at the exact parent and apply the supplied `comboship_cursor_fix_20261101.patch` with `git apply --check`, followed by `git apply`. After approved publication, the candidate commit or branch can be checked out directly. Reproduce the decisive checks with the three commands above, adding `python3 -B tests/oot_pause_inventory/run_cursor_tests.py --sanitize` for the new regression. Compare the candidate to the exact parent: these six paths must be the only changes and there must be no deletions. Returning to the parent discards only this isolated repair. The existing PR #77 branch is retained unchanged.
