# Native form instruments in OoT

## Intent and baseline

cor requested a final bounded ComboShip polish pass returning to the MM forms'
native instruments and soundbanks while playing in OoT. Continue the existing
implementation rather than introducing another instrument system.

Baseline: local `ecdb8128786389c42d709cc32c97a4041e593e8e`, tree
`ab1a683f2c91fd14a039cc58800a112ae92966b9`. PR #88 identifies published
`ced18542387d5ac8879e0f1cb303e790e4e3a1a8` with this same tree. Work is isolated
on `poc/oot-native-instruments-20261108`; the cumulative branch remains intact.
The baseline is source provenance, not configured runtime acceptance.

## Required behavior

- Deku plays the existing MM pipes with the Deku soundfont instrument.
- Goron plays the existing MM drums with native per-button drum gestures.
- Zora plays the existing MM guitar with its native soundfont instrument.
- Fierce Deity and human Link retain the ordinary ocarina path.
- Live notes and song replay retain the correct form voice, pitch, held-note
  behavior, and note-off. Changing pitch bend while holding a note must update
  that note without restarting its sample or envelope.
- Opening, closing, interruption and form changes must not leave a stuck note,
  an incorrect instrument, or a stranded playing pose.
- Preserve current native/Alt resource resolution, model/eye/texture work,
  progression, recent LinkVoice work, existing custom-form voices, and MM's
  native gameplay implementation.

## Implementation approach

Reuse the existing form controller, native MM animation resources, retained
MM soundfont loader and direct audio mixer. Port the native Goron wait and
arm-specific strike behavior through OoT's animation queue. Extend the current
note update boundary only where a reproduced regression requires it. This is
the implementation choice for this pass.

A new synthesizer, global soundbank replacement, or regenerated model/texture
pack would broaden ownership and regression risk without being needed for
these gaps. They are outside this candidate.

## Verification and limits

Use the actual production controller/audio functions with controlled engine
service boundaries. Establish failing regressions before corrections, compare
native MM gesture tables/queue behavior, and cover original and Combo ABI
builds where affected. Reuse the existing MM sample rendering, resource
lifetime and transformation checks. Compile the affected real translation
units if this environment supplies their dependencies.

Configured visual/audible checks still require the matching executable, base
MM resources and active model packs. Source fixtures do not prove GPU
appearance, audio-device output, or user acceptance. Do not promote an
accepted master or imply runtime parity from compilation alone.
