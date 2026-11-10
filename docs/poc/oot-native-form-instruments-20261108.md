# Native form instruments in OoT — November 8 candidate

## Status and provenance

This candidate implements the bounded native-instrument polish pass requested
by cor. It is verified at the source, regression-fixture and native object
compilation levels. Configured gameplay appearance, device audio and user
acceptance remain pending.

- Local baseline: `ecdb8128786389c42d709cc32c97a4041e593e8e`.
- Published PR #88 baseline: `ced18542387d5ac8879e0f1cb303e790e4e3a1a8`.
- Shared baseline tree: `ab1a683f2c91fd14a039cc58800a112ae92966b9`.
- Isolated branch: `poc/oot-native-instruments-20261108`.
- Engine boundary milestone: `1903a772acb321e692d3370ac9e1cbbb28e1e1e2`.
- Audio milestone: `ff91587e99ddb60ddfd5ae3280e1970db65e9565`.
- Verified implementation: `4bfb3489b5ae61246d6802a3907151ada8c7b2d6`, tree
  `b1dc61d084f55e17a64845cf051da4ba5cfa6c0a`.

The final commit and tree are recorded in the source-recovery archive manifest;
this document travels inside that commit. Publication of this new feature is
separate from the earlier exact-commit approvals for the cumulative candidate.
No accepted-master promotion is part of this handoff.

## Implemented behavior

| Form | Instrument and existing bank | Changes in this candidate |
|---|---|---|
| Goron | MM drums, Soundfont 0 instrument 92 | Native wait pose; five button-specific strikes; independent left/right arm overlays and individual drum deformation. |
| Zora | MM guitar, Soundfont 0 instrument 93 | Correct dedicated grip hand plus guitar display lists; ordinary hand suppression follows the native opening threshold. |
| Deku | MM pipes, Soundfont 0 instrument 94 | The selected pipe inflates and relaxes independently; note gestures follow the physical button. |
| Fierce Deity and human | Ordinary ocarina | FD preserves the host-selected ocarina right hand for both ages and native/Alt resource routing. Human remains on the ordinary path. |

These bank selections were already correct. The candidate retains the existing
MM soundfont loader and direct sampler. Native instrument onset now rejects an
unretained sample left behind by a failed donor repatch, preventing a native
drum/guitar/pipe from silently selecting an OoT sample. This guard is limited to
slots 92–94, runs only on onset and does not rewrite the shared font.

Held live notes update their pitch/bend from the original sample-rate basis,
without resetting the sample cursor or envelope. Replay callbacks represent
attacks, so consecutive identical replay pitches now retrigger correctly. The
physical button is read before staff publication and remains separate from
sharp/flat pitch modifiers.

Live/replay notes and song jingles now have separate direct-mixer owners.
`0x5800` remains the live/replay owner; private tag `0xFFFE` owns the jingle and
never enters the native SFX-bank dispatcher. Ordinary putaway and damage stop
the live voice while allowing the jingle to finish. Whole-form and scene
teardown separately cancel both. Both voices retain explicit note-off lifetime
control; no speculative replay refresh timer was added.

Early notes wait for the opening animation before changing its pose. Native
ocarina-bank muting suppresses direct instrument audio during the fanfare while
retaining visual events. Instrument OFF explicitly cancels direct playback,
including a repeated OFF request. Cleanup runs before Gakki state is cleared on
damage, restore, reload, initialization, reset, death and play destruction.
Custom native voices restore DEFAULT only while they still own the active host
instrument, avoiding a late cleanup reopening an already-off ocarina.

## Verification

The new manifest row runs through the existing `equipment` regression runner.
Its exact registered command passed after clang-format 14.0.6, with source
hashes unchanged throughout that run.

| Check | Result |
|---|---|
| Engine note/button/mute/instrument boundaries | 1,411 cases per original/Combo mode; ASan/UBSan; missing-OFF-cancel negative control rejected. |
| Native instrument controller | 29 scenarios per mode, 58 runs; ASan/UBSan; actual host animation queue and LinkAnimation implementations. |
| Sampler, ownership and jingle behavior | 10 scenarios per mode, 20 runs; ASan/UBSan; baseline reproduced 12 behavioral failures. |
| Existing native transformations | 116 scenario runs passed. |
| Existing transformation audio lifecycle | 30 scenarios passed. |
| Existing Goron audio | 28 runs passed across sequence/direct backends. |
| Existing Zora presentation/audio | Passed on the final formatted candidate. |
| Existing MM sample render and resource lifetime | Render/clock checks passed; six loader cases and seven native-parser cases passed. |
| CI policy tests | 25 passed after registration. |
| Complete production translation units | `mm_player_form.cpp`, `mm_asset_loader.cpp`, `code_800EC960.c`: all six original/Combo object compilations passed. |

An independent whole-candidate source review found no actionable critical,
important or minor findings. It compared the gesture tables, timing, queue
buffers, voice ownership and teardown against the native source and reviewed
the fixture boundaries separately from runtime acceptance.

The object checks use the real project headers and dependencies, the current
float-matrix/wide-vertex ABI, GNU C++20/C23 and the Linux non-`_DEBUG` path with
assertions enabled. They do not link a complete game. The existing `_DEBUG`
header path separately fails because `Context::GetLogThreadPool` references a
member hidden by that same macro; the baseline has the identical issue.

Only typed service/state boundaries were added to three older fixtures; their
existing assertions were not weakened. Native `mm/` production source and all
asset payloads are unchanged. The final source/result summary is in
`oot-native-form-instruments-20261108-results.json` beside this document.

### Limits of the evidence

Controller fixtures execute real animation-queue copies and display-list
emission decisions with controlled resource/matrix boundaries. They do not
prove final GPU geometry. Lifecycle fixtures execute the extracted cleanup
blocks, so they establish those effects rather than every gameplay route into
them. Audio fixtures use synthetic PCM and controlled archive, decoder and lock
services; they prove ownership and held-state continuity rather than donor
envelope fidelity or runtime thread scheduling.

The sampler's existing envelope/vibrato defaults remain in effect. Full MM
sequence-engine parity, secondary Zora stick/body/face/string gestures, and
recorded audible parity are outside this candidate. This pass does not close
the earlier Black Market slowdown/camera issues listed in the cumulative
publication handoff.

## Required configured check

Use a rebuilt matching executable with the accepted model packs and the
existing MM base resources. The Goron wait and five strike animation IDs are
already declared by the source; the candidate loads them without regenerating
an archive. The three instruments need MM Soundfont 0 and its retained donor
samples. No base archive payload or exact archive hash was available in this
workspace, so archive identity and actual pack appearance must be recorded
during that configured check.

1. Play all five buttons in Goron, Zora and Deku, including rapid opposite-arm
   Goron notes and notes pressed before the opening animation finishes.
2. Hold notes while changing stick bend and sharp/flat modifiers; check both
   the sounding pitch and the selected drum/pipe.
3. Replay a melody containing consecutive identical pitches and a recognized
   song; listen for distinct attacks and a single correctly voiced fanfare.
4. Put away the instrument, take damage, transform, change scenes and reset;
   verify that no instrument voice persists. Check ordinary jingle completion.
5. Check FD's ordinary ocarina for both ages and the active native/Alt pack
   combinations, plus ordinary human and existing custom-form instruments.

## Recovery

The source archive contains the exact changed files, a binary Git patch, hashes,
baseline/target refs and trees, and restoration instructions. Restore against a
clean checkout whose tree is the baseline tree above; either baseline commit
identity is valid when that tree matches. The archive's independent restoration
receipt verifies the resulting target tree. It contains no donor archives,
saves, credentials or build objects.
