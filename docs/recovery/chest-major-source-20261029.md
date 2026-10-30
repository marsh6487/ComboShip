# Combined source recovery checkpoint — Major mask/fairy chests

Recovery branch: `checkpoint/combined-chest-major-17748a4-20261029` in `marsh6487/ComboShip`.

This is a source-only recovery checkpoint. It preserves the committed held actor/NPC fixes, completed missing-music repair, resource-entry fixture correction, and corrected chest source. It does not publish a game build, merge accepted masters, or establish in-game acceptance. Local worktrees and prior local commits are retained.

## Exact sources

- Repaired gameplay parent: `3cbb22d1f09e85deb206d2b085fc62105bc09fad`.
- Published full actor-source parent used to reconstruct the complete repository: `4ca64d89b6a1fce834f8b234518ab227c5f92aa7`.
- Committed held combined source snapshot before this recovery note: `17748a4924a44a40986dd64b5c58e482c3539222`.
- Reviewed standalone corrected chest source: `8605126c29f2b4ce733b1790f6cf174d38f2a508`.
- Superseded Minor/small experiment: retained only in prior local history; the current source uses Major chests for masks AND stray fairies in both engines.

The remote checkpoint starts from the complete published actor-source parent and overlays every tracked file from the committed held source snapshot, plus this note. Exact Git blob identities are checked during upload and independent restore verification. Uncommitted work from other tasks is not swept into this checkpoint.

## Restore independently

```sh
git clone --branch checkpoint/combined-chest-major-17748a4-20261029 https://github.com/marsh6487/ComboShip.git ComboShip-recovered
cd ComboShip-recovered
python3 scripts/diagnostics/run_mm_chest_contents_tests.py
python3 scripts/diagnostics/run_chest_size_tests.py
```

The branch is a full source checkout, rather than the narrow local reference snapshot. Follow normal platform dependency/build instructions for a complete executable. Preserve the existing mod/archive configuration; the chest correction requires no new pack. Existing chest-pack installation details remain in `docs/poc/2026-09-23-combo-chests-midna.md`.

## Validation and remaining gates

Both final chest suites passed with UBSan. OoT's full C actor compiles for ordinary OoT and COMBO_BUILD, and its real foreign-category mapper remains Major for masks/fairies. MM's final production chest diff is only the two model-table aliases; its size predicate remains native. Independent review found no issues.

A full combined executable link/build and configured in-game validation remain pending. Earlier actor/music verification and donor/restoration notes are retained under `docs/poc/`. Gameplay publication remains on hold.
