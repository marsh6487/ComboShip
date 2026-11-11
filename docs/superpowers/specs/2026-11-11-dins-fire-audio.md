# Din's Fire remake audio candidate

cor requires the remake to stop using the vanilla Din's Fire SFX. At the start
of this pass, the native candidate still played NA_SE_PL_MAGIC_FIRE and shared
vanilla charge/strike whooshes. Character voice customization, floor sounds and separate
stick-ignition feedback remain relevant and must continue through their native
routes. This is a bounded continuation of the authorized remake implementation;
the new authored sound design is a candidate for listening, not an accepted mix.

Baseline: commit 36409a9d62a8c58aff6b7df34751f77dff9063d6, tree
95980168191d77fa8057b39029aebd9bd9663d20, saved as
ComboShip_Dins_Fire_Native_Usable_Linger_20261111.zip. New isolated branch:
candidate/dins-fire-audio-20261111. Publication and NEI combination remain held.

## Sound and approach

Proposed authored cues: a rising magical charge; a short ground-contact impact;
a broad release rush; a quieter seamless flame/crackle bed; a soft .6-second
burnout. These are original synthesized samples, not a pitch-shifted vanilla
sound or a claimed extraction/match of the reference clip's soundtrack.

Use the existing post-synth stereo PCM mixer seams and immutable 32 kHz mono
signed-16 samples compiled into each host. This is the builder's choice: it
avoids an external sound pack/load-order dependency. A raw-WAV archive loader
was considered but adds unnecessary loading/validation state; modifying global
native sound IDs would affect unrelated users and is outside scope.

## Preservation and lifecycle

- Preserve all visual assets, cast/contact/release/control ticks 12/20/21/27,
  the 138-update actor, its six-second interaction window, damage and cosmetics.
- Remove vanilla MAGIC_FIRE, PL_SKIP and SWORD_SWING_HARD requests only from
  the latched remake path. Keep setting-off/missing-prerequisite legacy audio,
  other spells/medallions, character VOICE hooks, floor sounds and stick ignition.
- Start charge only after successful native magic reservation. Bind the current
  audio generation to the tagged actor on Init; actor stores the nonzero handle.
- Contact and release are separate idempotent cues. Actor updates publish the
  fixed cast position and fire age; burnout starts at126 and stops at138.
- Player action changes cancel charge before release; normal recovery leaves
  released actor audio alive. Kill and direct Destroy stop that generation.
- A stale handle cannot stop another cast. Zero handles are silent no-ops.
- Publish pause/live/volume on each host's actual Play_Update before actor
  gating. Pause freezes cursors; transition/scene/death stops audio. Keep a bounded
  sample watchdog for missing game-thread publication. No world dereference,
  allocation, I/O or CVar access on the audio thread.
- Apply Master times SFX once, finite/clamped distance attenuation and stereo
  panning at the anchored world point, smooth gain changes and saturating mix.
  This does not claim native room reverb, occlusion or sound-bank priorities.

## Evidence and handoff

Use real actor/action callbacks to prove native suppression and preserved stock
dispatch; production mixer/host adapters to verify PCM, loop, pause, cancellation,
volume, position and generation ownership. Run the existing Din gate and affected
native units. Deliver an audition separately from the source recovery increment;
do not include preview movies in source. A linked application build and configured
listening remain separate and unproven until actually performed.
