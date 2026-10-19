# Magic Mirror candidate — 2026-10-19

The Magic Mirror is a permanent, fixed Lost Woods reward outside the shuffled
item pool. It returns the player to the opening-forest crossing from safe outdoor
areas. The elevated stump displays the spinning mirror; approaching its base
starts the native item presentation, so obtaining it does not require a seed's
equipment or reaching the inaccessible top.

## Source and inputs

- Candidate branch: `poc/magic-mirror-20261019`.
- Parent: `31ea4e711f13f1e5b2b1b9ac9b36ea43bc13b18b`.
- Parent tree: `bbc04bc7603296c9e8a5b69d5c0a2f81127ba45e`.
- This is an isolated candidate on the published cumulative integration. No
  accepted baseline or integration branch is promoted by this checkpoint.
- Placement was checked against the unchanged input
  `1 - lost woods 2ship granny shop and stump moved.prelude.o2r`.
  SHA-256: `7fde769411d21792f6bc6cde62ee934fb17b8e24d7a01b8d74f30069cce49a83`.
- That input's unique stump cap has center `(-377, 80, 416)`, dimensions
  `110 × 112`, and 12 triangles. The runtime locates the compact horizontal cap
  in the loaded collision; it does not edit the scene or collision archive.

## Behavior and integration

- Full item ID `0x0224`; independent saved ownership. Time Gate and Mirror share
  page-two cell 31 through the existing item selector. Old Time Gate saves heal
  into independent ownership, and receiving either sibling keeps the selection.
- Both games save/load and share the ownership flags. Full-width C/D-pad items
  survive cross-game transfers, including an owned tool behind its sibling.
- Combined OoT recall uses the existing clean-frame save/fade/handoff into MM's
  opening forest. MM recall uses its native forest entrance. Solo OoT/MM use the
  existing resource-checked copied forest and their native transitions.
- Recall prompts Yes/No and cancels with B. It requires a living, grounded,
  unmounted player in ordinary outdoor play. Dungeons, interiors, cutscenes,
  transitions, timed activities, and blocked item states reject use. There is no
  magic, currency, age, form, or seed-equipment requirement.
- Farore's Wind and grotto respawn structures retain their contents. The Mirror
  clears only its destination cutscene/respawn override and uses normal black
  departure/arrival fades. Native quest progress, age/form, and clock remain
  under their existing systems.
- Pickup callbacks are bound to slot, seed, scene, and mode. Reset/scene teardown
  clears pending pickup events and releases only the Mirror's private prompt.
- The same generated gold/emerald mesh supplies the world reward and get-item
  presentation. The renderer reserves native arena capacity before writing.
  Inventory/name PNGs are identical in both games and use the existing custom
  asset collection. Rebuild recipes and native renderer checks are in
  `tools/magic_mirror/README.md`.

## Verification and limits

Focused runners are registered in `scripts/diagnostics/run_combo_nei_regressions.sh`:

```sh
python3 -B tests/magic_mirror/run_tests.py
python3 -B tests/magic_mirror/run_form_tests.py
python3 -B tests/magic_mirror/run_integration_tests.py
python3 -B tests/magic_mirror_inventory/run_tests.py
python3 -B tests/magic_mirror_inventory/run_action_tests.py
python3 -B tests/magic_mirror_inventory/run_sync_tests.py
python3 -B tests/magic_mirror_inventory/run_save_tests.py
python3 -B tools/magic_mirror/test_visuals.py
MIRROR_TEST_CXXFLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all -no-pie" \
  ASAN_OPTIONS=detect_leaks=0 python3 -B tools/magic_mirror/run_draw_tests.py
```

The tests execute the actual runtime, save/ownership/selector functions, and
native graphics allocator/setup bodies with real engine types. External system
boundaries such as disk saves, launcher calls, and GPU rendering are fixtures.
The recovery archive records exact command results, syntax probes, asset gate,
source trees, and independent patch-restoration evidence.

This checkpoint has no configured in-game proof or user acceptance. A complete
executable/archive build and link was not performed here. The real-header syntax
probes are not a substitute for that build; in particular the whole SoH save
translation unit requires the unavailable libzip development header. Its actual
travel save/load bodies are exercised by the focused save fixture instead.

Before promotion, build the candidate with both custom texture sets and the
usual forest resources, then test the actual selected launcher mode and archive
load order. Runtime acceptance must cover the moved stump's visible placement
and ground pickup, get-item scale and sound/text, C/D-pad prompt and cancellation,
save/reload, both cross-game directions, solo forest visits, and blocked-use
states. Include Time Gate ownership/selection and Farore recall in that session.

## Recovery

Use the published candidate ref or the recovery archive's binary-safe patch.
For the patch route, check out the exact parent above in a clean copy of
`marsh6487/ComboShip`, apply `source.patch` with `git apply --index --binary`, and
compare `git write-tree` with the manifest's candidate tree. Restore the input
archive only to its intended scene-mod position; the supplied copy is unchanged.
Generated previews are software renders of the native mesh, not game captures.
