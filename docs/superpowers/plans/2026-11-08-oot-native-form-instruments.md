# Native Form Instruments in OoT Implementation Plan

> **For agentic workers:** Use the executing-plans or subagent-driven-development
> workflow for the bounded tasks below. Keep file ownership explicit.

**Goal:** Finish native form instrument presentation and held-note soundbank
behavior in the existing OoT integration.

**Architecture:** Preserve the current Gakki controller and MM soundfont loader.
Use the donor MM animation selection and arm masks for Goron strikes. Update
active note parameters within the existing audio lock without recreating sound
slots or replacing global soundbanks.

**Tech stack:** C/C++, native OoT/MM animation and audio types, Python regression
runners, ASan/UBSan.

**Spec:** `docs/superpowers/specs/2026-11-08-oot-native-form-instruments.md`

## Global constraints

- Base tree: `ab1a683f2c91fd14a039cc58800a112ae92966b9`.
- Preserve native MM source, human/Fierce Deity ocarina, custom-form voices,
  LinkVoice, models/eyes/textures, progression and existing asset namespaces.
- No whole audio-engine rewrite, external asset repack or master promotion.
- Report source verification separately from configured runtime acceptance.

## Review focus

- Rapid alternating notes must retain independent left/right Goron gestures.
- A held note with changing stick bend must update without an envelope restart.
- Entry/exit and interruption must not keep playing or freeze the controller.
- Missing optional animation/audio resources must retain safe existing behavior.
- Both direct notes and replay must retain native form voice ownership.

## Task 1: Native form presentation

**Owned paths:** `soh/mods/transformation_masks/mm_player_form.cpp`, a focused
instrument helper if required, and `tests/form_instruments/` controller tests.

**Consumes:** existing native animation loader and OoT animation queue.
**Produces:** native Goron wait/individual arm strikes within the existing
Gakki entry/update/exit contract; existing Deku/Zora presentation preserved or
corrected only where donor comparison demonstrates a concrete gap.

- [x] Record the observed donor differences and failing behavioral regressions.
- [x] Load the five existing native drum strike clips and wait clip.
- [x] Use independent native arm masks and per-arm strike timing.
- [x] Cover note mapping, rapid alternating notes, missing resources and exit.
- [x] Run controller tests in original/Combo modes and inspect the scoped diff.

## Task 2: Held native note audio

**Owned paths:** `soh/mods/transformation_masks/assets/mm_asset_loader.cpp`,
`mm_asset_loader.h`, and `tests/form_instruments/` audio tests. Coordinate the
single caller change in `mm_player_form.cpp` with Task 1.

**Consumes:** existing MM soundfont 0 mappings and retained sample ownership.
**Produces:** an active-note update API preserving sample position/envelope
while accepting current pitch/bend; legacy refresh consumers remain valid.

- [x] Reproduce the frozen held-note bend through production note functions.
- [x] Update only the instrument's active slot parameters under the audio lock.
- [x] Pass current note parameters from the existing live/replay driver.
- [x] Verify form font selection, held-note continuity, note-off, replay and
  protected ordinary/custom instrument behavior.
- [x] Run focused audio and existing sample/resource-lifetime regressions.

## Task 3: Integration and handoff

**Owned paths:** CI registration and this candidate's documentation.

- [x] Register the new focused regression runner in the existing CI manifest.
- [x] Run the registered command and affected existing transformation checks.
- [x] Compile affected real translation units where dependencies are available.
- [x] Obtain an independent source review and resolve concrete findings.
- [x] Commit the bounded candidate and verify exact changed-path preservation.
- [x] Record source results, runtime limitations, recovery and next gameplay check.

## Progress

- Baseline repository and remote PR identity inspected; isolated worktree created.
- Five affected baseline suites passed before implementation. Separate workers
  implemented controller and sampler changes with disjoint file ownership.
- Verified code is committed as `4bfb3489b5ae61246d6802a3907151ada8c7b2d6`.
- Final registered gate passes: 1,411 engine cases per ABI, 58 controller runs,
  and 20 sampler/jingle scenarios, all under ASan/UBSan.
- All six complete production objects pass in original/Combo configurations;
  this is the Linux non-`_DEBUG` path with real headers and assertions enabled.
- Independent whole-candidate review found no actionable findings. Changed
  production files pass clang-format 14.0.6 and whitespace validation.
- See `docs/poc/oot-native-form-instruments-20261108.md` and its adjacent results
  JSON for behavior, precise boundaries, recovery and configured gameplay checks.
- Configured gameplay, audible parity and user acceptance remain pending. This
  candidate does not authorize accepted-master promotion or new publication.
