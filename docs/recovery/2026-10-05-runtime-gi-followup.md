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

## Durable checkpoints and later findings

User explicitly approved any GitHub checkpoints on Oct 4, 2026 (America/Chicago). Publication is unblocked. Verified source trees and branch heads:

- Initial restoration: `5a52c70525d24e4fa480a80bb70cc378b8eeffa2`, `checkpoint/grace-slate-runtime-20261005`, tree `7b5d8d0dfc782acf1a784d79ec0b7d86f8a5ebe5` (byte-identical local `754df220`).
- Song corrections: `6774acde1ea5029f50226929bede89e69a462a0e`, `checkpoint/songs-runtime-20261005`, tree `ca48960e3bd6a1ffe95407ec43b299a209a44a24`, local implementation `a76ad235` plus initial restoration/status.
- Fairy/sword corrections: `a91c9f6a2a454d9ee90ad8a16224600d09c18501`, `checkpoint/fairy-runtime-20261005`, tree `d37b05413281234f2c727ffd9ee14fc99c72a52e`, local implementation `0423ab7d` plus initial restoration/status. Final focused GI verification continues.
- Compass/save tests: `50f5062d5bf217b45a52adf41bfbb92cd45f524c`, `checkpoint/compass-runtime-20261005`, tree `c308b6dc5a898af804f83d8cf867af4718a897fa`, local `23a10094`.

On Oct 5 at 00:20 Chicago, user confirmed generating a NEW seed with the Maps/Compasses information checkbox ON still produces generic dialogue. The prior formatter fixture manually hydrated the donor Context and did not prove generation/load propagation or the real CustomMessage formatter. This is an active defect investigation, not resolved by explaining the checkbox. Preserve the user's supplied OoTR reference requirements.

Inventory review exposed missing MM button prototypes, stale equipped slot metadata, wheel state surviving pause/tab resets, incoming sync overriding the selected hourglass, and title cache staying stale after a programmatic rune grant. These have narrow fixes and production-derived tests in this checkpoint. Native UI behavior tests passed both hosts; full pause C syntax passed before the programmatic-title follow-up. The source-derived sync fixture is green in both hosts; baseline `9eff802c` is a failing negative control. New whole MM JSON and OoT NEI section serialization tests passed both hosts and ASan/UBSan; published-baseline serializers fail the lost-flags assertion in both hosts.

Song worker full suite: 41 of 43 commands green; season grant and rod lifecycle fixtures link to the newly introduced Grace grant helper without projecting it. Root must repair those fixture boundaries, integrate isolated worker hunks, run the complete gate and publish a source-only candidate to PR #34. Existing sacred baselines remain unchanged; no in-game acceptance or merge.
