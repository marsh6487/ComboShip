# Runtime GI follow-up checkpoint (WIP)

Public baseline: PR #34, `1147a1a893ad2e2c0425049cc9b2ed618d944656`, tree `a13eb980994f4bdf772cf453ce65401fbde13724`. Local baseline `7647b2a6` is byte-identical to that public tree. No gameplay code was reconstructed.

## Implemented inventory section

- Grace and Phantom Hourglass share cell 41, with independent append-only ownership flags, save serialization and cross-host unlock sync. Existing item IDs remain unchanged; earlier Grace cells are preserved instead of erased.
- Canonical grants, current-table editor catalog, Give All and both seed pools include Grace. Finding either sibling preserves the selected item; pause A toggles the wheel, directional input selects the sibling, and equipped u16 items are preserved.
- Slate title resolves the active owned power. Both pause wheels invalidate the cached title when selection changes. Five IA4 name textures per host use the existing Century Gothic font.
- Tests reproduced the original missing Grant All cell and constant Slate title before implementation.

## Verification so far

Passed: `python -B tests/nei_shared_slot/run_tests.py` (both hosts); `python -B tests/mm_editor/run_tests.py` (ASan/UBSan ownership/grant/pool tests and real SaveEditor/GiveItem syntax). Native pause UI, new save fields and complete regression gate remain pending. Game runtime remains untested for this candidate.

Headers use existing cached dependencies through CPATH (json, spdlog, imgui, SDL2, stb, thread-pool). No dependency code was changed.

## New runtime evidence and active work

Eight supplied clips inspected at three frames each. Observed: compass native tutorial then title with no reward line; empty fairy-bottle interior; sparse Bolero particles; Storms weather-only presentation; custom sword models with blue shimmer and insufficient pickup framing; Nocturne; Sun's circle; Razor sword custom model. No Epona/Lullaby/Saria clip was supplied; those observations and approvals are user reports.

Independent isolated probes from `9eff802c`: `work-compass`, `work-songs`, `work-gi`. User requests compass information, fairy inside TP bottle, upright sword fit plus custom-mod particles, song-color shimmer for every warp song, Storms clef with rain/no thunder, Epona orange shimmer and Sun yellow shimmer/no circle. Preserve user-approved Lullaby/Saria visuals, custom model priority and existing inventory work.

This is an explicitly incomplete candidate. No merge, sacred-master promotion or runtime acceptance. Keep recovery documentation on checkpoint branches and exclude it from PR #34.
