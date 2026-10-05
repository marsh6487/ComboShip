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

Song worker full suite: 41 of 43 commands green; season grant and rod lifecycle fixtures linked to the newly introduced Grace grant helper without projecting it. Root repaired those fixture boundaries using the Release optimization that folds the known Rod slot; both narrow fixtures passed.

## Combined candidate, still WIP

Root integrated the completed song palette/fire changes, native save tests, fairy scale/TP recognition, selected sword graph fitting and owner-scoped Din core/flame layers. Local `a6fdf5f6` includes GI follow-up `5545b875`: selected geometry and retained award particles now use the same fitted pose. Private archive tests cover fourteen selected models over 360 rotation frames in pickup/shop bounds; those checks do not constitute in-game visual acceptance.

The first combined gate stopped at real native MM `DrawItem.cpp` syntax: `ComboDinSwordGi.h` used OoT's `Matrix_NewMtx` without an MM API choice. The GI worker reproduced this with bare MM headers and is correcting it; its earlier compatibility-alias fixture did not establish that boundary. A separate native MM dedicated-GI model priority regression is also being corrected before acceptance. The full combined gate is not green.

Root added whole-save, selected-model and actual foreign fallback regressions to the CI NEI command list. A fresh read-only review of combined GI/song source is active. Compass remains open: user-generated On seed shows generic text, while traced generation and new-slot settings hydration have not yet reproduced the loss. The recovered original OoTR reference also requires randomized dungeon entrance information on maps, reward/boss information on compasses and map screens, disabled pedestal reward hints, and Well/Ice reward exclusions. Formatter-only tests are insufficient.

Next: integrate the current MM syntax/model priority corrections and actual compass root cause, finish review, pass the complete combined gate, format with clang-format 14, then publish a source-only candidate to draft PR #34. Existing sacred baselines remain unchanged; no in-game acceptance or merge.

## Later verified corrections and explicit receipt layout

Remote combined checkpoint `88ddc2524e3d02fcca6c152edc72bcd3ef23df60`, branch `checkpoint/grace-slate-runtime-20261005`, tree `0ca5989b3e0e971339e26fc93b471cff248a8214`, matches local `6c29dc53`. It includes the native MM API/model-priority fixes, bounded selected True Master flame and native Din cosmetic handling. Independent re-review found no unresolved Critical/Important GI/song issue: actual MM translation units, production flame arena ASan/UBSan, native priority and foreign layer/cosmetic tests pass. Game runtime remains untested.

The combined CI command list exposed three fixture boundaries omitted by the new selected-sword path. Root retained the real mask dispatcher with fail-on-wrong-sword-branch seams, supplied explicit no-third-party-resource seams to the complete native MM NEI renderer, and included the actual selected-owner helper in the editor pool fixture. All three repaired runners passed. Shared/grant/dungeon reward/audit, reward delivery, Din render/damage/hand and MM item-visual checks passed; reward sanitizer required `ASAN_OPTIONS=detect_leaks=0` because this environment cannot inspect `/proc` for leaks.

Local `4a118cd0` integrates the real foreground/file-select generation gate and native setting propagation probes from worker `879a034c`. Both generation and Plandomizer Save previously allowed mutation during active MM gameplay with a dormant OoT FileChoose. Complete-panel negative controls reproduced four failures; corrected normal/ASan/UBSan controls pass. This is a separate demonstrated defect, not proof of the user's compass omission. Registered CVar, SetAllToContext, FinalizeSettings, settings snapshots and real save array tests rule out a simple dropped setting at those tested boundaries.

User supplied the desired receipt text and a 717×162 layout image: dungeon title, actual shuffled boss, actual boss-check reward name plus sprite beside the reward line in one box. On compass receipts must replace the generic tutorial. Bottom of the Well, Ice Cavern and Gerudo Training Ground are excluded from reward hints. Enabled maps use direct vanilla/MQ information where supported and inverse entrance mapping: if the Deku Tree entrance leads to Forest Temple, the Forest Temple Map points to the Deku Tree entrance. No vanilla reward/boss hardcoding.

Read-only final-message review found no later native MM message-table overwrite: the production custom hook preserves staged bytes. The current donor intentionally prefixes the generic tutorial and uses separate box breaks for title/boss/reward; this demonstrably contradicts the requested direct single-box layout. Message/icon changes are active in the isolated compass worktree and are not yet in this checkpoint.
