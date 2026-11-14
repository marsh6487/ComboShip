# Replaceable Din's Fire audio

cor provisionally accepts the softer sounds and requests replacement through an
o2r mod later. Base: `950eccbd4eb51e47eab971e664808ce8f00fd989`, tree
`595da19f1febda5ad489caf94afa968af111b129`. Branch:
`candidate/dins-fire-audio-mods-20261111`. Publication remains held for NEI work.

## Design

Use optional raw RIFF/WAVE entries at the five paths in
`combo/audio/DinsFireSfxSamples.h`. This follows the existing Midna archive reader
pattern. Each supported mod cue overrides its compiled softer default; missing
or invalid cues fall back independently. No global sound IDs or visual/timing
changes. No external pack is needed for normal use.

The builder's format choice is mono signed-16 PCM, 32 kHz, nonempty, at most ten
seconds per file. Unknown bounded RIFF chunks may be skipped; malformed or
unsupported streams are rejected. Preserve valid sample amplitudes, including
silence. Cue trigger times and actor expiration remain authoritative. A loop
repeats its entire data region; mod authors should provide a seamless loop.

Load/decode outside the audio callback and mixer lock. Cache against the owning
host resource manager; in Combo use the registered `oot`/`mm` manager and never
the other game's active manager. Installing/editing a mod requires a restart;
live filesystem watching is outside this change. A missing manager must not
permanently cache failure. Snapshot immutable owned samples for a cast, preserving
them through pause and archive-manager changes. Reclaim prior banks only on the
game thread, outside the mixer lock; callback cleanup must not free sample data.

## Work and ownership

- [x] Resources worker: own WAV decoder and raw archive/cache implementation,
  native resource test fixture and focused provider/parser tests. Contract is
  `SampleBank` with five optional vectors and `DinsLoadOverrideBank()` returning
  `std::shared_ptr<const CwDinsAudio::SampleBank>` from the shared resource include.
- [x] Mixer worker: own mixer and adapter integration plus existing native audio
  test/runner. `Mixer::Begin` accepts the optional immutable bank. Per-cue views
  select override or compiled fallback. Preserve default output exactly.
- [x] Pack worker: own deterministic starter-o2r builder and modding guide. Use
  the same paths and strict WAV format; permit partial packs and export the
  current defaults as an editable starter. Keep all preview audio out of source.
- [x] Root: shared contract, integration review, CI registration, bounded final
  verification and source handoff preparation. Serialize all staging/commits.
  The final recovery archive is built from the clean verified tree and its save
  result is recorded separately from the source commit.

## Required evidence

Exercise real decoder/provider and mixer paths: partial and all-five overrides,
invalid/missing cues, valid silence, loop wrap, short/long clips, owner selection,
cache behavior, restart contract and sample lifetime across clear/new casts.
Cover original and Combo for both hosts. Prove no-mod output is byte-identical
to the accepted softer audition. Validate the starter archive and its native
read path where available, distinguishing fixture boundaries from an in-game
run. Preserve PCM include/recipe/manifest, renderer/assets and timing exactly.
