# Din's Fire Audio Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans with the already active parallel workflow. Keep explicit file ownership and serialize staging/commits through root.

**Goal:** Give the enabled remake original spell audio and preserve stock and voice behavior.

**Architecture:** Immutable authored PCM feeds one synchronized per-host mixer through
the existing post-synth seams. A generation handle transfers charge ownership from
the player action to the actual persistent actor, with context publication outside
pause-gated actor updates.

**Tech Stack:** Native C/C++20, Python/NumPy offline synthesis, 32 kHz mono s16 PCM.

**Spec:** docs/superpowers/specs/2026-11-11-dins-fire-audio.md.

## Global constraints and review focus

Use combo/audio/DinsFireSfx.h exactly for the cross-worker C bridge. Preserve
the existing timing header and visual assets byte-for-byte. Scope audio to the
latched remake; no global SFX ID replacements, new settings or unrelated audio work.
Review pause with frozen actor updates, stale destruction after a new generation,
failed reservation/spawn, live setting changes and host/scene transitions. Those
conditions are covered by the mixer and existing production-action fixtures below.

## Task 1: Authored samples (audio asset worker)

Create scripts/assets/build_dins_fire_audio.py, combo/assets/dins_fire_sfx_pcm.inc,
and tools/dins_fire_audio/{README.md,manifest.json}. Define five immutable arrays:
kCwDinsChargePcm, kCwDinsContactPcm, kCwDinsReleasePcm, kCwDinsLoopPcm,
kCwDinsBurnoutPcm. Header provides CW_DINS_SFX_SAMPLE_RATE=32000. Every array is
mono int16_t; the entire loop array is the seamless repeating region.

- [x] Generate original procedural cues and a separately saved listening audition.
- [x] Verify determinism, finite samples, bounded peaks/DC, expected duration,
  loop-boundary discontinuity relative to ordinary adjacent samples, and metadata.
- [x] Preserve original waveform recipe/provenance and commit only owned assets.

## Task 2: Mixer and adapters (audio engine worker)

Create combo/audio/DinsFireSfxMixer.h, shared implementation as needed, host adapters
soh/soh/Enhancements/audio/DinsFireSfx.cpp and
mm/2s2h/Enhancements/Audio/DinsFireSfx.cpp. Modify only the two native audio-buffer
seams and two z_play.c context-publication sites. Own tests/dins_fire_audio/ and its
runner. The bridge header's scalar-position/handle functions are the contract.

- [x] Add failing native behavior tests: generation ownership, idempotent contact/
  release, actual PCM/loop output, gain/pan/mute, pause cursor preservation, context
  loss, watchdog, natural burnout, and cancellation before versus after release.
- [x] Implement bounded synchronized state, immutable data, no audio-thread world
  access; use real host pause/volume/scene/death predicates on the game thread.
- [x] Verify both actual adapters and mixer, original/combo guards, sanitizers,
  and actual Play_Update/audio-buffer call sites. Export an audition scenario
  through the production mixer when practical; distinguish any offline mix.
- [x] Notify root before owned staging/commit.

## Task 3: Player cues (gameplay worker)

Own both player cast paths and combo/tests/dins_fire_native_gameplay_test.c with
its runner. Add a static uint32_t charge handle per host. Begin only after a
successful native reservation; cancel the handle on action departure/failure.
CancelCast must preserve a released actor's audio. Use separate remake animation
SFX lists retaining native VOICE and floor callbacks while omitting PL_SKIP and
SWORD_SWING_HARD. Stock lists stay intact.

- [x] First expose the current forbidden dispatch with a failing test using
  production animation cue processing/frame crossing rather than empty stubs.
- [x] Implement the bridge calls and remake-only lists.
- [x] Verify both hosts, native and disabled/fallback casts, voice/floor preservation,
  failed reservation/spawn and pre-release cancellation, original gameplay timing.
- [x] Notify root before owned staging/commit.

## Task 4: Actor ownership, integration and recovery (root)

Own combo/render/DinsFireNativeActor.h, both actual actor definitions/Destroy,
combo/tests/dins_fire_native_actor_test.c and its runner, shared bridge, docs and
the narrowly scoped Din CI manifest row. Add uint32_t nativeAudioHandle to each
actor. Init attaches only tagged native actors; contact/release dispatch cues;
UpdateActor publishes position and age; Kill and direct Destroy stop that handle.

- [x] Add failing actor assertions for no vanilla burst, exactly one contact/
  release, anchored updates through138, pause/cancel ownership and direct destroy.
- [x] Implement the actor bridge without changing collision, light or render paths.
- [x] Integrate worker outputs, review cross-thread and lifecycle risks, register
  bounded audio coverage and run the existing Din gate plus affected native units.
- [x] Commit bounded owned milestones, save a verified recovery increment from
  36409a9d62a8c58aff6b7df34751f77dff9063d6 with assets/evidence/restoration instructions,
  and deliver the separate audition with honest runtime-listening limits.

## Completion boundary

Source and native verification are complete at code/gate commit
`f8972018e94fcfe5bab6ea4e4fa69a5b5d71a6d1`. The final registered 15-command
Din gate passed; review findings for same-tick pause and actual Play teardown
were corrected and verified. Delivery freezes documentation next, then creates
and independently restores the final source increment and saves it with the
separate production-mixer audition. Exact final archive identity is recorded in
its own manifest and recovery evidence. Full application linking, configured
listening, subjective mix acceptance and NEI combination/publication remain
pending; see the verification handoff.
