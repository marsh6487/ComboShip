# Din's Fire native audio boundary tests

Run from the repository root:

```
python3 -B tests/dins_fire_audio/run_tests.py --sanitize --dependency-root /existing/native/includes
python3 -B tests/dins_fire_audio/run_tests.py --sanitize --dependency-root /existing/native/includes --export /absolute/path/dins_fire_mixer_audition.wav
```

The runner builds **both actual host adapters** and extracts each host's actual
`AudioMgr_CreateNextAudioBuffer` body, then executes the real bridge/mixer through
that native seam. It runs original and Combo configurations for both games. The
MM original configuration deliberately does not define `MM_BUILD_DLL`: host
selection must also work in the standalone target. Engine structs and CVar bridge
headers are the production headers. The override path also uses the real LUS
resource types and production `CrossRMRegistry.cpp`; mounted archive bytes are
the controlled resource boundary. The CVar store, native synth/device, scheduler
and other mod mixers remain fixture boundaries.

The override tests send real mono WAV bytes through the production override
loader, decoder, host adapter, mixer and native stereo buffer. They compare
literal PCM amplitudes for all five cues, exact fallback output for missing or
malformed cues, intentional silence, one-sample clips, short loop wrap and a loop
longer than the compiled loop. They also cover partial packs, foreign-only
packs, an unavailable manager becoming available, manager replacement while a
cast remains audible, same-manager caching and invalid/paused cast requests.
Combo fixtures deliberately leave the other game's resource manager active;
the requested host's archive must still provide the samples. Archive and CVar
boundaries reject audio-thread access, and allocation/free guards cover every
native callback, including the concurrent consumer.
Direct mixer lifetime checks run every clear path with a uniquely owned bank;
its destructor re-enters the mixer to prove reclamation happens on the game
thread after unlocking. Another consumer runs while new owned banks are selected.

Checks cover nonzero generation creation only in valid live play, exclusive
actor attachment, pre-release cancellation versus released actor ownership,
stale handles, idempotent contact/release, actual authored PCM output, sustained
looping, natural burnout/expiration, immediate pause silence with exact cursor
preservation after resume, Master times SFX applied once, both mutes, nonfinite
MM volume, source pan/distance, saturating addition over existing output,
transition/scene/death/context loss, a sample-count watchdog and concurrent game
publication/audio consumption. The runner also checks that each actual
`Play_Update` publishes context at entry and again after Kaleido mutates pause,
before its pause/actor gates; actual `Play_Destroy` clears context before actor
teardown. Both missing lifecycle hooks were observed failing their source-order
assertions before the bounded fixes. It does not
substitute an independently written buffer routine for production dispatch.

The production headers require existing `nlohmann/json.hpp`, `spdlog` headers
(including their fmt headers), `BS_thread_pool.hpp`, and `SDL2` headers. Pass one or more
`--dependency-root /existing/include/root` arguments for real dependency headers;
no replacement headers are generated. `NLOHMANN_JSON_INCLUDE_DIR`, standard
include paths, compiler `CPATH`/`CPLUS_INCLUDE_PATH`, repository `deps`, and an
adjacent `dins_audio_native_dependencies` directory are also searched. Set
`DINS_AUDIO_DEPENDENCY_ROOTS` to an OS-path-separated list of additional roots.
Existing sibling workspace `deps` caches are searched for JSON. The runner
preflights all four header dependencies and downloads nothing. `--sanitize` enables
AddressSanitizer, UndefinedBehaviorSanitizer and float-cast-overflow checks.
LeakSanitizer defaults to disabled because the managed environment's ptrace
prevents LSAN from running; callers may override `ASAN_OPTIONS`.

`--baseline` is a RED diagnostic: an explicitly selected weak silent bridge makes
missing audio fail at `live Begin returns generation`, rather than a compile or
link failure. It is never included in production or ordinary verification.

The exported audition is **8.5 seconds, 32000 Hz stereo signed-16 WAV** rendered
through the production adapter and native buffer seam. It uses a centered source
100 world units from a fixed camera, Master=100%, SFX=100%, contact at1.0 seconds,
release at1.05 seconds, burnout at7.35 seconds and termination at7.95 seconds.
It contains the authored spell alone; native synth/device and other mixers are
silent fixture boundaries. It is an offline production-mixer render, not an
in-game recording or evidence of game playback, native room reverb, occlusion,
sound-bank priority, subjective mix balance, or configured audible acceptance.

The bounded source mixer lives in `combo/audio/DinsFireSfxMixer.h`. It owns four
one-shot slots and one full-array loop, all behind one mutex. PCM is immutable;
the audio callback dereferences no game objects and performs no CVar access,
allocation or I/O. The numerical source model uses a game-thread camera snapshot,
full volume to500 world units and zero at6000, camera-relative stereo pan, and
10ms gain ramps. Pause freezes all cursors while context refresh continues. A
half-second sample watchdog stops audio if game-thread publication disappears.
Context loss/transition/death and explicit generation-qualified Stop clear voices
immediately. Asset details/provenance are in `tools/dins_fire_audio`.

Optional files at `audio/dins_fire_remake/{charge,contact,release,loop,burnout}.wav`
replace individual cues. Empty or unsupported resources select the compiled cue;
a valid nonempty WAV containing only zero-valued samples stays silent. Loading
and decoding occur only for a valid live unpaused cast, before the mixer lock is
taken. Each cast pins its selected bank and repeats the override loop's entire
data region. Manager identity controls the game-thread cache: changing files in
the same mounted manager requires a restart. Clearing a cast retains the bank
until a later valid Begin retires it on the game thread, after unlocking. No-pack
audition export always resets the fixture archives before rendering.
