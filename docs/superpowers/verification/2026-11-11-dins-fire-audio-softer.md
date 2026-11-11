# Din's Fire audio: softer second audition

## Requested revision

cor heard the first buildup as a racecar and the ignition as too explosive. This
is a narrow sound-material revision over audio candidate
`a0dc95ad9554fa7c4480bcb56f643505509f62be`, tree
`8a10cd13f3b1305fa0aa38410519365adcb825e3`, on isolated branch
`candidate/dins-fire-audio-softer-20261111`.

The earlier audio and source archive remain intact locally as comparison and
recovery inputs. The new audition is a proposed response to that feedback;
subjective sound quality and acceptance still require listening.

## Changes and preserved scope

- Charge uses stationary filtered-noise layers and a gentle volume swell. The
  rising low oscillator, spectral sweep and accelerating modulation are removed.
- Contact is a quieter filtered flame catch with a slower onset. The low bass
  oscillator and thud are removed.
- Release blooms into a flame wash with less low-frequency content and a gentler
  attack. It keeps its original start time and duration.

Only the first three PCM cues, their recipe/metadata, and documentation change.
All sample counts and the 32 kHz signed-16 format remain fixed. The flame loop
and burnout are held byte-for-byte at their first-pass values:

| Cue | SHA-256 of little-endian PCM |
|---|---|
| Loop | `9df85156713a3a91675b3db7749986cbdf785f3a3a9565e94f0fe2c66add3961` |
| Burnout | `1b1d718d3bcdac02701d560f6bdb8b8ef0e3c9fbe0e146ace9d451c4937a08b7` |

The audio bridge, mixer, native actor/player hooks, camera, collision and damage,
six-second usable fire lifetime, visual textures/renderer, cosmetics, checkbox
and What's New accent have no changes. The source includes the PCM arrays, so
the revision requires no external audio pack. Vanilla spell sounds remain
suppressed only for the latched remake; existing voice and stock routes remain.

## Verification and audition boundary

The evidence files prefixed `dins-audio-soft-` record this revision. Older audio
evidence in the recovery increment documents the prior native integration.
Verification is scoped to deterministic sample generation, unchanged cue counts
and preserved PCM, the source diff, and the existing production audio harness in
the four OoT/MM original/Combo configurations with sanitizers. The prior full
visual/gameplay gate is not repeated for an audio-asset-only edit.

The deterministic recipe check passes. The actual native adapter/buffer harness
passes all four sanitized configurations in 14.00 seconds against asset commit
`d21564ff7a4a554beab0a7f0dfecfccc2432d819`. Only documentation changes follow that
tested source. The signal comparison records reduced charge energy below 250 Hz
and a quieter early contact/release stack; these are measurements of the edit,
not proof of the desired perceived timbre.

The new 8.5-second WAV is exported through the actual production adapter/mixer and
extracted native audio-buffer routine, using the same centered source, 100-unit
distance and 100% Master/SFX gains as the first audition. It contains spell
sounds only, with silent native synth/device boundaries. It is not a game
capture or evidence of mix balance against voices/music. The old and new final
fire tails can be compared directly because all affected one-shots end by 2.35 s.

The exported waveform is `Dins_Fire_Audio_Softer_Audition_20261111.wav`, SHA-256
`4b2fda3db48df59f5583b1f52c9dcff78de1d402c2e57096fd0344aff070575a`.
Its peak is 12,078 out of 32,768, with zero clipped samples. Direct comparison
proves its entire tail from 2.35 s through the end is byte-identical to audition
one. The first 200 ms from contact has peak -12.843 dBFS and RMS -25.331 dBFS,
compared with -2.854 and -14.730 dBFS in the original production-mixer audition.

Full application linking, configured in-game listening, mix acceptance, and
combination/publication with cor's independent NEI changes remain pending.
The final recovery increment still uses the saved six-second visual/gameplay
baseline `36409a9d62a8c58aff6b7df34751f77dff9063d6`; see its RESTORE.txt for exact
archive/hash and tree restoration instructions.
