# MM child/adult voice integration candidate

November 8 follow-up: `mm-adult-voice-followup-20261108.md` records the logged
PR87 source mismatch, real Marin/native-loader checks, removal of the remaining
automatic adult pitch fallback, and new provider-selection diagnostics. The
current source no longer applies the historical `0.85` approximation even if
both adult providers are unavailable.

The optional paired Marin archive now selects child or adult recordings using
`AdultLink_IsActive()`, the same Time Gate state used by adult rendering. Without
that archive, young human Link retains native MM playback and adult human Link
uses recordings from the registered native OoT base archive. The native provider
and its exact mappings are documented in `tests/oot_adult_voice/README.md`.

The shared custom-item helper now delegates to native `Player_AnimSfx_PlayVoice`.
This supplies MM's human/form/scents/giant routing once, including Light Rod
backfire, NEI Roc's Feather and Pegasus collision voices. Existing native and
custom transformation voices remain in their own routing. The unrelated vanilla
feather's nonvoice sound is not changed. Young Epona boost samples are preserved.

The human playback boundary handles ordinary and renewable voice flags, native
stop/reset/query calls, same-frame duplicate requests, and game-frame renewal.
The audio thread mixes immutable 32000 Hz PCM through four atomic publication
slots. An optional pack is validated completely before publication and cached
until restart. It cannot silently install only one age bank. Variants use a
private PRNG, matching OoT's approach without changing gameplay RNG state.

The donors retain their recorded pitch: the automatic adult 0.85 multiplier is
excluded from this PCM path. The explicit audio-editor pitch slider and native
master/SFX volume controls remain effective. Other native voices retain their
existing pitch behavior. Custom-form voice IDs are normalized to their expected
base IDs before their own override is consulted.

## Install and recover

Apply the cumulative source checkpoint to the matching baseline, rebuild the
whole matching application, and replace the previous Din MM voice O2R with
`Din_2Ship_Child_Adult_Marin_Native_Voices_POC3_20261108.o2r`. Restart after any
voice archive change. This pack supplies the human cosmetic voice override as
the prior pack did; it is not a per-character profile picker.

The new archive SHA256 is
`343a067df0edaff08f6da7477e411248b36d2fec8e6cfbe958b30e200f0c1c41`.
All 116 original POC2 entries are byte-identical. Added resources contain 125
source Marin recordings plus two preserved young-boost recordings, converted
to mono PCM16 at 32000 Hz by resampling, without pitch shifts. Its provenance
lists explicit aliases for MM actions without separate donor groups. Original
PAKs and the prior O2R are unchanged. The source checkpoint excludes audio and
game base assets; the optional voice archive is a separate deliverable.

## Verification and remaining proof

Passed ASan/UBSan fixtures for the production loader and playback bodies with
real MM headers, native voice dispatch and item-helper bodies in both solo and
Combo modes: all 32 actions at both ages, missing-pack fallback, source PCM
selection, flags, stop/renewal, routing isolation, volume and manual pitch.
The mixer sanitizer/thread stress checks also pass. Six actual C translation
units compile (SFX, audio thread and item helper, each solo and Combo). The
default OoT provider separately passes actual-asset tests and sample-exact
comparison with the production native ADPCM decoder.

These are source and fixture checks. A full application build is unavailable
here because the native dependency setup is incomplete, including spdlog.
New C++ modules are covered by the existing recursive CMake mod glob, but their
complete native compilation/linking remains pending. No configured gameplay
or listening acceptance is claimed. Test adult/child attacks, damage, falling,
horse calls/boost, Light Rod backfire, NEI feather, stop/renewal, form changes,
volume and pitch both with and without the optional archive. The default OoT
provider preserves donor recordings and order, but does not emulate sequence
rests, envelopes and other native sequence effects; see its README for limits.
