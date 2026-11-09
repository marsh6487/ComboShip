# Adult Marin voice follow-up — November 8, 2026

## Required behavior

Native 2Ship human adult mode must use the optional HW Marin adult recordings
when the paired pack is available, and the native OoT adult recording provider
otherwise. Automatic pitch reduction is not a substitute for either provider.
An explicitly enabled audio-editor pitch setting remains intentional.

`AdultLink_IsActive()` reads the saved `Nei_Save()->timeGateAdultMode` bit or
the diagnostic `gAdultLink.ForceOn` override. It does not require Time Gate
ownership, a cast in progress, or another gate activation. The voice selector
and adult renderer use this same predicate.

## Evidence from the supplied session

The attached log contains six starts. The first four use POC2; the last two
use `Din_2Ship_Child_Adult_Marin_Native_Voices_POC3_20261108.o2r`, with no POC2
entry in those later starts. The latest POC3 mount is at log lines 19202–19204.
Native MM gameplay and modded adult rendering are established at 24087 and
24100–24116. Earlier MM sample/pitch diagnostics are from the isolated MM SFX
engine used while OoT is active, and do not identify the native MM adult voice.

All six startup strings report `fdb90cb`. GitHub resolves this to
`fdb90cbf30198f37f8a5b73e9c2e3a4a3682cc4c`, the PR87 test merge of
`1415eecde372a5bcf3f4b8b9d650d77f96e3afd9` into
`2054eef2f12aeedfb2d2d0f5519cc23272227e4e`. Direct inspection of that revision
shows no paired-provider interception in `mm/src/audio/sfx.c`, no paired PCM
mixer call in `mm/src/audio/lib/thread.c`, and the older hard-coded adult
`0.85` pitch callback. That revision can mount POC3 without consuming its adult
PCM manifest, while still using the child soundfont patch retained from POC2.

This is a concrete source-version explanation consistent with the report,
not conclusive executable identification: the startup hash is generated at
CMake configure time and can be stale in an incremental build. There is no
provider-selection logging in the preceding cumulative implementation.

The current cumulative candidate already contains both new playback hooks and
the native OoT adult provider. However, if both providers return unavailable,
the old callback still deepens the native MM fallback. This follow-up removes
that obsolete automatic reduction and exposes the actual provider boundary.

## Exact audio verification

POC3 remains unchanged, SHA-256
`343a067df0edaff08f6da7477e411248b36d2fec8e6cfbe958b30e200f0c1c41`.
The original adult PAK remains unchanged, SHA-256
`4eca5c3021d4fb15628f913cedb23d3c07e80a941e50973fad6ce87c1afcb249`.

All 82 adult Ogg recordings were independently decoded/resampled from the
original PAK to mono PCM16 at 32000 Hz. Every byte matched POC3's adult PCM:
2,593,637 samples compared. The archive therefore contains the intended adult
recordings, rather than child recordings with relabeled rates or shifted pitch.

A standalone probe linked the already-built native `libultraship.so`, SHA-256
`db9f8e23521a13b12cc3cc1d553f48e0da6a7befb388bba811acdb81b83fb0f2`.
Real `Ship::ArchiveManager` and `Ship::O2rArchive` loaded all 246 entries through
string, hash and layer APIs and matched independent ZIP extraction byte for
byte (13,138,386 uncompressed bytes). The two 32-action banks contain 161
references to 127 unique PCM resources: 45 child and 82 adult. Every reference
resolved. This checks raw archive access, not a configured running game.

The baseline production-handler fixture passed with real POC3 in solo and
Combo configurations. Its former dispatch stubs did not exercise the pitch
callback/native request queue; the follow-up fixture closes that gap.

## Change and verification

The player pitch callback now applies only an explicitly enabled audio-editor
setting. Human age selection remains at the existing `LinkVoice_TryPlay`
boundary. The paired-pack loader reports readiness or the rejected resource;
the native provider reports the unavailable owner/archive/sample or successful
load. The first selected voice per age, and later explicit pitch changes, log
the provider, action, clip and effective pitch. No logging runs in the mixer.

| Check | Result and boundary |
| --- | --- |
| Regression before the fix | Actual player/pitch-hook/native-queue fixture failed: `native voice 0x6825: expected pitch 1.00, got 0.85`. |
| Final routing and mixer fixtures | Passed under ASan/UBSan in solo and Combo, both with deterministic CI PCM and with unchanged POC3. All 32 actions for each age reach the actual `AudioMgr_CreateNextAudioBuffer` mix call with exact selected PCM. |
| Saved adult selection | Actual `Nei_Save` and `AdultLink_IsActive` bodies confirm the saved adult bit works without Time Gate ownership, ownership alone stays child, and the explicit ForceOn override works. |
| Fallback and controls | Passed missing pack, malformed JSON, native donor availability/unavailability, default 1.0 pitch, explicit 1.5 pitch, retained SFX flags, custom-form precedence, excluded actors/forms, muting, stop, renewal and volume. Donor availability and unrelated synthesis remain fixture boundaries. |
| Native compilation | All three changed production C++ files compiled with the configured Combo production flags. They also compiled with the Combo define/PCH removed as original-game guard probes; this is not a complete separately configured solo application build. |
| Real native failure boundary | Actual compiled native provider, linked to real libultraship/spdlog/fmt, returned null/zero for a missing resource owner twice and emitted exactly one warning, in both guard modes. |
| Existing donor fixture compatibility | Both solo and Combo provider fixtures compiled and linked without warnings. Asset-dependent decoding was not rerun because the base OoT archive was unavailable here. |
| CI and review | Mixer sanitizer/publication stress passed; all 25 CI-policy tests passed. The new asset-free routing command is registered in the existing systems gate. Independent review found no blocking defect; its malformed-pack coverage request is included in the final passing fixtures. |

Reproduce the main fixture with `python3 -B tests/link_voice/run_tests.py`, or
append the unchanged POC3 path to compare actual pack bytes. Both modes run
automatically. The test README identifies controlled resource/logging/native
donor boundaries so this evidence cannot be confused with configured gameplay.

## Source recovery and cumulative integration

The isolated correction starts at
`e2c4fc737bf087b2f35c879e92875729dc95d9fc` on
`fix/mm-adult-voice-trace-20261108`. Implementation commit
`7bf9980911920c4e5202ddb44fd4921a71943896` was cherry-picked into the active
`candidate/ootmm-shuffle-parity-20261108` cumulative source branch as
`b0cc40098ab28ab5f64e366b14dfc08ea6e9d2a7`, after parent
`14ed2e523507b1b3cae1762dafa9ff9eba0e596b`. All 17 unrelated modified source
files were byte-identical immediately before and after that integration. The
bounded commit contains only the voice implementation, fixtures/CI registration
and trace notes. Original recordings are excluded.

The local recovery bundle uses ref
`checkpoint/mm-adult-voice-followup-20261108`. It applies this bounded correction
to the published PR88 parent `ced18542387d5ac8879e0f1cb303e790e4e3a1a8`; all
affected baseline files match the local parent except unrelated CI gate
additions, which are excluded from that checkpoint. Restore into a repository
containing that PR88 parent by fetching the named ref from the bundle, checking
it out, and initializing the repository's pinned submodules. Retain the
unchanged POC3 hash above for the optional pack. The checkpoint commit can be
cherry-picked onto a cumulative source that does not already contain this fix.

Remote publication remains blocked: automatic approval review rejected the
GitHub tree upload because source/diagnostic upload requires explicit user
authorization. No remote checkpoint ref was created. The local bundle is a
recoverable handoff, but is not a substitute for the required durable external
checkpoint. Its exact commit/tree/hash and independent restore verification
accompany the handoff. The active cumulative branch already contains the
implementation; only remote checkpoint publication needs that approval.

## Scope and limits

Keep the existing `custom/din_voice/` resource namespace, original audio
archives, native donor recipes/decoder, action mapping, child voices, forms,
manual controls, and unrelated cumulative integrations intact. Recordings and
private gameplay logs are not committed to the source repository.

Source and native boundary tests are not audible runtime acceptance. If neither
adult provider can load, the final native MM soundfont remains a last-resort
fallback and is now identified in the log; it must not be described as a
successful OoT adult or Marin selection. Native demo IDs beyond the existing
32-action human bank remain outside that bank's coverage. The decisive runtime
check is ordinary adult and child attacks/damage/jumps in the matching rebuilt
cumulative application, with the first provider-selection records identifying
age, source and effective pitch.
