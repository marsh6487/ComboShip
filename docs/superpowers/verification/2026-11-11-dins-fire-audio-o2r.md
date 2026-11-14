# Din's Fire audio: optional o2r overrides

## Requested behavior and baseline

cor provisionally accepted the softer second sound pass and requested that its
samples remain replaceable by an o2r mod later. This is an audio resource and
ownership change over commit `950eccbd4eb51e47eab971e664808ce8f00fd989`, tree
`595da19f1febda5ad489caf94afa968af111b129`, on
`candidate/dins-fire-audio-mods-20261111`. Publication and integration with cor's
independent vanilla NEI fixes remain held.

The current five compiled samples remain the defaults. Optional raw WAV files
at `audio/dins_fire_remake/{charge,contact,release,loop,burnout}.wav` replace cues
individually. Missing or invalid files use that cue's compiled default; a valid
nonempty silent WAV is respected. The accepted recipe, PCM include and manifest
have no changes. The same is true of the visual assets, renderer, timing,
gameplay camera, six-second usable fire window, stick/arrow interactions,
damage, cosmetic color hooks, default-off checkbox and What's New accent.

The parser accepts bounded RIFF/WAVE PCM format 1, mono signed-16 little-endian
at 32000 Hz, with at most 320000 samples per file. Cue timing and actor expiry
stay authoritative; a long file does not extend the spell. The loop repeats
its complete data region. No normalization is applied to authored PCM.

## Resource ownership and lifetime

Loading and decoding happen on the game thread for a valid live, unpaused cast,
before taking the mixer mutex. Combo uses only the registered owning host's
resource manager (`oot` or `mm`); standalone uses its native context. Literal
raw archive filenames retain their `.wav` extensions and do not use the typed
resource cache or the Alt Assets toggle.

The cache tracks the manager weakly and is retryable when no manager exists.
Registry teardown clears cached references before module unload. Each cast pins
its selected immutable sample bank. Clear, cancellation, expiry, context loss
and the audio watchdog retain that bank; the next valid game-thread Begin
retires it after unlocking. The callback performs no resource or CVar access,
allocation or sample-bank destruction. Installing or editing a mod requires a
full restart; this implementation does not watch files or promise hot reload.

## Reusable mod

`scripts/assets/build_dins_fire_audio_mod.py` creates a deterministic ZIP-format
o2r with only the supplied WAV entries. `--defaults` verifies the existing
recipe against checked-in PCM/metadata before exporting; `--input-dir` accepts
any nonempty subset of the five cue names. Full instructions and installation
folders are in `tools/dins_fire_audio/MODDING.md`.

The delivered starter contains the accepted softer defaults unchanged:

- File: `Dins_Fire_Audio_Starter_20261111.o2r`
- Bytes: 413754
- SHA-256: `f92e96b1075765029ec294f96232d835f350060029b6f934b80aa1f615abc597`
- Combo folders: `mods/soh/` and `mods/2ship/`; standalone hosts scan `mods/`.

## Verification

The actual native host adapter, production mixer and extracted audio-buffer
routine pass in all four configurations: OoT original, OoT Combo, MM original
and MM Combo. The final run used AddressSanitizer, UndefinedBehaviorSanitizer
and float-cast-overflow checks, completed in 115.872 seconds and exited 0. The
verified mixer/adapter/tests are committed at
`2e116b96280d108a30da29c8b41a8204e7c8ce04`; the provider source was tested from
the same working tree. Evidence: `dins-audio-mod-native-runtime.log`.

Coverage includes literal output from all five overrides, partial and malformed
packs, silence, short and long loop wrap, own-host routing with the other host
active, retry/cache/owner replacement, invalid casts, callback allocation/free
and resource-access guards, and uniquely owned banks with reentrant destructors
and concurrent replacement. Existing generation, pause, gain, spatial, lifecycle
and watchdog assertions remain in the same executed suite.

The strict parser and provider also pass independently under ASan/UBSan in all
four configurations. The generated starter's bytes and hash match the delivered
archive. The native O2R reader passed a narrow sanitized retry in 48.86 seconds
after correcting a trailing `const` in the test fixture's real-header method
declaration; no production loader or parser change was required. The actual
reader opens/indexes the archive, loads all five raw entries by path and CRC
hash, decodes them to exactly the accepted PCM, rejects a missing entry, and
preserves the copied samples after close. The original provider log preserves
the first fixture compile failure; `dins-audio-mod-native-reader-retry.log`
records the corrected reader run. The already-passed provider modes were not
repeated for the fixture-only correction.

The resource/parser/fixture sources are committed at
`a4b1ab46acf29c26255ae499333f0975c0d45c2b`. Reproduction commands, with existing
real dependency include roots supplied when needed, are:

```sh
python3 -B tests/dins_fire_audio/run_tests.py --sanitize --export /absolute/path/fallback.wav
python3 -B tests/dins_fire_audio/run_override_tests.py --sanitize --build-starter
```

The bounded reader retry used the second command with `--archive-only`; it
skips provider modes already verified in the preceding log.

The no-mod native audition export is byte-identical to the accepted softer
audition: 1088044 bytes, 272000 stereo frames, SHA-256
`4b2fda3db48df59f5583b1f52c9dcff78de1d402c2e57096fd0344aff070575a`.
A separate reviewer compared the accepted and current no-mod mixers across
320000 stereo frames, including all cues, pause, changing gain/pan and addition
over existing native output, with exact sample equality. The reviewer found no
actionable correctness or regression issue. The review record includes the
comparison source and hashes. Root also verified the protected source diff is
empty and the earlier source ZIP remains unchanged. The existing audio asset
recipe check passes.

The registered Din CI row now includes normal and sanitized provider/parser/
starter-reader checks. The unchanged visual/gameplay suites retain their prior
evidence; they were not repeated for this bounded resource/mixer change. Real
LUS and engine headers, the actual registry source, and real third-party headers
were used. Dependency provenance is in `dins-audio-mod-native-dependencies.json`.

Evidence is scoped to the production decoder/provider, host adapters, mixer and
extracted native audio-buffer and O2R-reader methods. Resource-manager
construction, mounted archive selection, native synth/device and scheduler
remain controlled fixture boundaries. The native O2R check uses real libzip
headers and `libzip.so.5`, with selected unchanged native method bodies rather
than a full application link. LeakSanitizer is disabled for the managed
environment's ptrace limitation; ASan/UBSan checks remain enabled.

Full application linking and configured in-game playback remain unverified in
this environment. The user has accepted the current audition provisionally;
gameplay mix balance is still subject to listening in the combined build.
The source recovery increment continues to require the exact saved usable-fire
base `36409a9d62a8c58aff6b7df34751f77dff9063d6`; its RESTORE.txt contains the
base archive identity, hash, commit/tree and independent restoration steps.
