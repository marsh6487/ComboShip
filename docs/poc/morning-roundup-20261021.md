# ComboShip morning roundup — October 21, 2026

## Authorization, baseline and scope

cor requested all morning source implementations and bugfixes committed and pushed to `marsh6487/ComboShip`, and an audit for omitted work. This explicitly supersedes the earlier conditional-Fairy instruction to hold publication while collecting fixes.

Candidate: `integration/morning-roundup-20261021`, isolated from current remote `develop` at `66bde021ccd651a3728db2338251961b1a44c9f7`, tree `5ba3645e9d78d97345529b37728f614c51c73caa`. The remote baseline preserves PR48/49/50 gameplay fixes and PR51/52 parallel CI/fail-fast behavior. Source integration and publication do not promote a runtime-accepted scene master.

## Requested checklist

| Requested implementation | Baseline/checkpoint and verification status |
| --- | --- |
| 2Ship Lost Woods Saria and Termina Field song-giver dialogue freeze | PR48 includes `6d171889` via `f75d048e`; fresh audit pending. |
| Black Market entrance/access/crash | PR48 retains `8ab33dee` and native door/scene-loader work; fresh audit pending. |
| Native 2Ship private Lost Woods framing/textures, authored tree tunnel and scene scrolling | PR48 native forest presentation and split canonical resource source; fresh ownership/asset audit pending. |
| OoT mask transformation skipping SFX | PR48 retains audio input `58e5c4bd` via `6cd9e41b`; fresh DSP/controller audit pending. |
| Lost Woods room 7 Woodfall shortcut | Separate checkpoint `3352869b`, implementation `93d09aa3`; integration pending. |
| Lost Woods Great Fairy conditional return | Separate checkpoint `34d6f1f3`; integration pending. Requires cor's separately authored collision exit/export. |
| Original solo OoT Fairy / Combo private Lost Woods portal regression | Existing native solo forest and paired entrance fixtures; composed verification pending. |
| 2Ship Mirror input, one-press confirmation, use and stump-relative return | PR49 merged through PR48, checkpoint `a7b5244e`; fresh audit pending. |
| Mirror shimmer and acquisition on stump top | PR48 `ec4bc59b`/`039a8f41`/`a1f6c40e`; PR50 pale-blue/white follow-up; fresh audit pending. |
| Saria authored shimmer | PR48 actual NEI twinkle implementation; fresh audit pending. |
| Woods of Mystery and private Lost Woods black-space shimmer | PR48 actual NEI field covers both black faces including centers; fresh audit pending. |
| Narrow segment binding for otherwise unbound vertical scrolling | Existing final-forest scroll work; exact coverage audit pending. |

## Preservation and recovery

Accepted input archives remain unchanged. The native canonical forest source uses its committed manifest and verified numbered parts; it must reconstruct independently without a leftover local ZIP. Original/solo/combined routing, inventory/progression/forms, native/shuffled entrances, explicit return payloads and all earlier source integrations are protected.

The candidate retains individual source histories and checkpoint records. Before publication, restore a fresh tracked-source archive, compare its tree/content and reconstruct the required native resource source. Do not substitute a successful compile or sanitizer fixture for configured game traversal or visual acceptance.

## Execution ledger

- Plan and baseline recorded before integration. Parallel audits own core bugfix verification, forest/scroll verification and morning gap/provenance review; root alone owns candidate integration and publication.
- Existing user authorization permits the isolated worktree, source push and cumulative publication without another permission round.
- Baseline native solo-forest fixtures passed for both hosts. Initial missing JSON/SDL include roots were environment setup only; supplying existing native dependency headers resolved them without source edits.
- Woodfall's published checkpoint `68aec1266580a1e56aaa401806b00ddadfc7c430` has exactly local `3352869b`'s tree. Merge `5797ff8b` retains that published history and all PR51/52 CI behavior. Its sanitizer run passed all five production fixture groups with the actual native MM archive (`f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`, 459 vertices/648 polygons).
- Fairy checkpoint composed cleanly in player/scene code. The workflow conflict was its obsolete serial-gate insertion; retained the current workflow unchanged and placed the Fairy sanitizer command in `scripts/ci/regressions.json` instead. All 21 CI policy/runner tests passed.
- The complete composed entrance runner passed, including six production renderer objects, original portal/paired boot, all forms, save/Farore priority, stump return, camera and conditional Fairy handling. The dedicated Fairy sanitizer also passed all six native fountains in OoT-only/Combo modes, moved room, native/randomized ordinary visits and reload/stale-state cleanup.
- Both real-bundle solo routes passed against committed resource dependencies. Fairy/ Woodfall controllers and OoT Play passed real-header syntax checks; the player overlay passed after its normal mods include root was supplied (334 existing compiler warnings).
- Core read-only audit: 17/17 targeted commands passed. Includes Saria's nine arrow/button cases and full actor object, both native Mirror layouts, both-host inventory/action/sync/save, eight real-DSP audio cases/116 transformation controller scenarios/28 audio lifetime cases, five-form market startup and 18 bundled entry/return/re-entry cycles. Full native-base/keeper coverage is being checked separately.
- Forest read-only audit: 48 resource tests (46 pass, two optional original-input skips), actual particle draw bodies/renderers under sanitizers, both native Mirror adapters, four physical stump-access tests, six-track legacy/two-track native scroll fixtures, MM material registration and both native-bundle solo routes passed.
- Gap audit retained seasonal Song of Storms flags, NPC repair recipes/alias protection, BossSoul POC5 repair tooling/pinned inputs, native transformation camera/Goron light cleanup and prior Moon-tree seasonal targeting. NPC/flame/optional appearance output packs remain separate installations.
- Ruling: the requested extra vertical binding was only a proposal after the earlier UV workaround, not a missing committed checkout. The current explicit source-roundup request authorizes a bounded COMBO-only Lost Woods XLU segment `0x0A`, with two 32x16 layers moving +1 vertically and wrapping at 64. Sheets opt in through native target `0x0A000001`; no authored archive/texture/UV is changed. A separate source branch owns its real-draw regression and implementation.
