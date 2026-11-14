# Din's Fire audio candidate: verification and continuation

This records the first audio pass and the native integration. The subsequent
softer sound revision is documented in `2026-11-11-dins-fire-audio-softer.md`;
the first audition's waveform/hash below is retained as historical evidence.

The native remake now has original authored spell audio in both hosts. This is
an unpublished source candidate and listening audition. A linked application,
configured in-game listening, mix acceptance, and combination with cor's separate
vanilla NEI bugfixes remain pending.

## Exact source identities

The protected six-second interaction baseline is commit
`36409a9d62a8c58aff6b7df34751f77dff9063d6`, tree
`95980168191d77fa8057b39029aebd9bd9663d20`. It remains saved as
`ComboShip_Dins_Fire_Native_Usable_Linger_20261111.zip`, Library identity
`libfile_6b088c705f6c81918edfc9e222eea12e`, SHA-256
`0b47442f3daf273204877e382d47693b2ca3b341f83116bd155afbee2ceb46b6`.

Audio implementation and registered gate are frozen at commit
`f8972018e94fcfe5bab6ea4e4fa69a5b5d71a6d1`, tree
`c13aa2ab2d5470257164dfcf81b015ad93629253`, on
`candidate/dins-fire-audio-20261111`. Later changes for delivery are documentation
only. Bounded milestones: bridge/spec `a91a9730d`, actor ownership `ac61eadd1`,
player dispatch `08302a549`, authored assets `ed41d1873`, mixer/adapters
`f2f25cace`, and registered coverage `f8972018e`.

The independent actor/player/assets recovery checkpoint was durably saved before
engine completion as `ComboShip_Dins_Fire_Audio_Checkpoint_20261111.zip`, Library
identity `libfile_6a10527a34b88191b4e75141ccbdf8d3`. It is explicitly WIP; use the
final audio candidate increment for the complete implementation.

## Resulting behavior

The latched remake uses its own charge, contact, release, flame loop, and burnout.
It no longer requests the stock `NA_SE_PL_MAGIC_FIRE`, `NA_SE_PL_SKIP`, or
`NA_SE_IT_SWORD_SWING_HARD` cues. Native VOICE callbacks, surface/floor callbacks,
stick ignition, legacy casts, and the MM medallion paths retain their existing
routes. This uses the existing Din's Fire Remake checkbox; no new audio option or
global sound-ID replacement is introduced.

| Event | Time from charge start | Audio behavior |
|---|---:|---|
| Charge | 0.00 s | Original rising charge sample |
| Hand contact | 1.00 s | Short contact impact |
| Fire release | 1.05 s | Broad release plus quieter looping fire |
| Player control | 1.35 s | Released audio stays with the planted fire actor |
| Burnout begins | 7.35 s | Fire loop fades and burnout plays |
| Actor expires | 7.95 s | Actor's generation stops |

These use the unchanged 20 Hz native timing constants. The six usable seconds,
damage falloff, stick-tip and swept-arrow interactions, normal gameplay camera,
native cosmetics, graphics checkbox placement and What's New accent remain.
The protected timing, renderer, visual recipe and texture paths have no diff.

Each cast has a generation handle. Pending player cancellation stops charge;
recovery after release preserves actor ownership. Kill and actual actor Destroy
stop that generation. Old handles cannot stop a later cast. Actual Play teardown
also clears context before actor destruction, including a charge canceled before
its actor has spawned.

Both hosts publish scalar context at Play_Update entry and refresh it after
KaleidoSetup_Update before the actor pause gate. Pausing freezes sample cursors.
Scene/transition/death/context loss stops playback. A half-second sample watchdog
bounds audio if game-thread publication disappears. The callback consumes
immutable PCM under a mutex without CVar reads, engine-pointer dereferences,
resource I/O or allocation. Master times SFX gain, anchored pan/distance, short
gain smoothing and saturating addition are applied. Native room reverb, occlusion
and sound-bank priority behavior are not implemented by this numerical mixer.

## Verification

The final registered fifteen-command Din gate passed in 90.56 seconds, recorded in
`evidence/dins-audio-final-ci-verification.json` and its adjacent log in the
delivery increment. It covers the existing geometry, native actor and real player
action fixtures, texture factories and rendering checks, plus deterministic audio
generation and actual native audio tests in normal and sanitized runs.

Independent engine tests compile and execute both real host adapters and the
extracted production audio-buffer routine in OoT/MM original/Combo configurations.
They verify emitted PCM, idempotent cues, cancellation/released ownership, stale
handles, loop persistence, burnout, exact paused cursor preservation, gain/mute,
pan/distance, saturation, transition/scene/death/context stop, watchdog and
concurrent publication. ASan, UBSan and float-cast-overflow checks pass;
LeakSanitizer is disabled because the environment's ptrace configuration prevents
it from operating. This is not a ThreadSanitizer claim.

Fourteen full native C translation-unit checks pass with real headers and both
original/Combo definitions. Four targeted z_play checks were repeated after the
review corrections. The complete Play_Update/Play_Destroy bodies were compiled
and their hook placement inspected/asserted; their full gameplay loops were not
executed by this harness. Synth/device, CVar storage, scheduler and other mixers
remain test boundaries.

Read-only review found and resolved two real integration edges: same-tick pause
publication and charge-before-spawn cleanup during actual Play teardown. The
reviewer independently reran all four sanitized audio configurations and sample
checks, then matched all reviewed production/test/CI hashes to the frozen commit.
There are no unresolved review findings. See `evidence/dins-audio-review.json`.

## Listening audition and assets

`Dins_Fire_Audio_Audition_20261111.wav` is a separate 8.5-second stereo 32 kHz,
signed-16 WAV exported through the production OoT adapter and native buffer seam.
The source is centered 100 world units from a fixed camera, with Master and SFX
at 100%. Native synth/device boundaries are silent. Thus it contains spell cues
only, without Link's voice, music, or other gameplay sounds; it is not a game
capture. SHA-256:
`0822505fbb2b73bdc15bd12d5519edae4782465886eef95f9bc4b8f9178f5249`.
Its 272,000 stereo frames have peak 23,592, zero clipped samples and a silent tail.

The five source cues are original procedural synthesis, not extracted game or
reference-clip recordings. The exact recipe, manifest and compiled PCM are in
source. Decoded data is 412,800 bytes per host, so no separate sound pack or
runtime decoding is required. Sample integrity and a zero-step loop boundary are
verified numerically; those checks do not establish perceived sound quality or
in-game balance.

## Recovery and remaining proof

The final source ZIP is a small increment over the exact saved usable-linger
baseline above. It contains changed source, generated PCM, binary patch, Git
bundle, asset identities, evidence and restoration instructions. Restore the
base first; the increment alone is not a complete checkout. The delivered patch
is applied to a fresh independent index and must produce the exact candidate
tree. Bundle verification uses its declared base prerequisite. Listening WAVs
and preview movies are kept outside the source increment.

CMake and Ninja are unavailable here; no full application link or configured
native game session was performed. Before promotion, build the combined sources
with the intended configuration and listen alongside the user's voices/music and
asset packs. User approval of the visuals does not imply acceptance of this new
sound mix. Publication and combination with the separate NEI fixes remain held.
