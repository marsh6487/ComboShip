# PR #88 fixture dependency repair and Link voice naming

## Baseline and scope

Parent: `ebd20b87b50618c0d2801b9bc3a7ec21c496b559`, the existing
`candidate/cumulative-poc6-20261108` head with the Mikau correction.
All cumulative integrations are preserved. This is a local candidate; publication
requires cor's approval. No merge or accepted-master promotion is included.

Observed GitHub run `42502632206` completed with successful Windows and Linux
builds/packages, fast source checks and forest/world regressions. Five regression
groups failed at the fixture boundaries below. Windows compile preflight
`42502632115` passed. These are parent results, not CI results for this correction.

## Root causes and exact repair boundaries

| Group | Actual production dependency | Fixture correction |
| --- | --- | --- |
| Equipment | `Player_AnimSfx_PlayFloorWalk` now calls static `Player_GetStepSfxOffset` in native `z_player.c`. | Extract that actual helper too; include weather/season declarations and supply Native/Off weather and inactive Roc Boots boundaries in the Wolf lifecycle fixture. No footstep assertion is removed. |
| Entrances | Native `GameState_Update` includes `GameState_UpdateUiVisibility` between frame hooks and main. | Supply a no-input UI boundary in the transition frame fixture, retaining the complete production frame ordering. Run the separate actual UI-control predicate suite. |
| Rendering | Production triangle submission reads `Texture::NeutralizeShading`, default false. | Add the same transient field/default to the fixture's substitute Texture type. Production Texture and renderer remain unchanged. |
| Items | `FindMultiItemPlacement` uses inline `MagicHints.h` policy, including foreign identity and progressive conversion. | Include the production header; supply the foreign lookup boundary. Add a fail-fast progressive-conversion boundary because these color cases place concrete items. Real progressive policy remains covered by `tests/magic_hints`. |
| Systems | The real SFX engine calls the player-voice interception, stop and frame hooks. | Supply typed voice-off boundaries so real native Time Gate requests, bank processing and cancellations execute unchanged. Player voice behavior is separately exercised. |

These five failures were missing harness dependencies, not confirmed gameplay
defects. No existing assertion, failure handling or compiler policy is weakened.

## User-requested voice naming

Rename `DinVoice_*` to `LinkVoice_*`, `DinVoiceMixer` to `LinkVoiceMixer`,
`mm/mods/voice_pack/din_voice*` to `link_voice*`, and `tests/din_voice` to
`tests/link_voice`. Update the real MM SFX/audio-thread callers and the registered
CI mixer command. The CI label now says "Link age voice mixer".

This handler concerns the human player's child/adult voice actions, independently
of any replacement model. Keep `custom/din_voice/` resource paths unchanged for
compatibility with existing prepared optional packs. The eight renamed/calling
source and test files compare exactly to the parent after reversing identifiers,
filenames and the explanatory comment. No sample selection, pitch, volume,
action, fallback or resource-lookup logic changes.

## Verification

All six complete selected CI command rows pass locally through the repository's
combined-graphics-ABI compiler adapters:

- Native cross-game entrance sequencing and forest arrival: receipt syntax,
  four sanitized fade/handoff variants, cold blue warp, camera arrival, opening
  forest, portal saves and active mesh checks.
- MM Wolf host: equipment policy and native lifecycle/footstep/voice host fixture.
- Native Time Gate audio: Yes/No/B/timeout/damage/unequip, repeat use and pending
  request cancellation, with ASan/UBSan.
- Triangle submission: normal and ASan/UBSan runs, each with 16,384 state/clip/
  cull/texture variants and identical vertices/backend events.
- Native gossip routing plus original/Combo dynamic-color sanitizer cases.
- Link age voice mixer: sanitizer and concurrent publication stress.

Additional passes:

- Actual UI-control functions: 736 dispatches in four OoT/MM original/Combo
  configurations with input bytes preserved.
- Actual MM magic-hint policy, original and Combo, under sanitizers.
- Actual Link voice handler + native/item dispatch with a synthetic two-age,
  32-action PCM pack: solo and Combo, pack present/absent, selected PCM comparison,
  native-adult fallback boundary, renewal/stop and explicit pitch/volume checks.
- Real production `link_voice.cpp` syntax checks in solo and Combo, and real
  MM audio-thread syntax check.
- Winter terrain: 309 material cohorts and 256 renderer decisions under
  sanitizers, using real production Texture implementations.
- All 25 CI policy tests; whitespace check; naming-only equality audit.

Local environment: GCC/G++ 14.2, upstream nlohmann/json 3.12.0, fmt 10.2.1,
spdlog 1.14.1, SDL 2.30.11, ImGui 1.91.9b-docking and thread-pool 4.1.0 headers.
GCC 14 promotes legacy C pointer conversions to errors where CI's compiler does
not; a local-only CC wrapper restores warning severity for those conversions.
No repository compiler flags changed. Existing header macro/qualifier warnings
remain. LeakSanitizer cannot run under this host's ptrace environment; the
triangle sanitizer rerun used `ASAN_OPTIONS=detect_leaks=0` while keeping address
and undefined-behavior sanitizers enabled. Initial dependency setup failures
were local environment failures, not candidate passes.

## Limits and next gate

No complete local executable link or game run was performed. Full candidate
platform CI must run after approved publication. The selected command rows do
not constitute rerunning every complete regression group.

Synthetic voice checks prove dispatch and rename preservation, not audible
parity with cor's actual child/adult Marin packs. The native adult provider and
its OoT sample data are unchanged. Existing optional PCM packs and original OoT
voicepaks are distinct inputs; this repair does not claim direct MM support for
an unconverted original voicepak. No audio/model/ROM assets are included.

Recovery: check out the exact parent above on an isolated branch, apply the
provided binary-safe patch with `git apply --index`, and compare the candidate
tree/ref in the accompanying recovery manifest. The archive includes changed
source, exact baseline versions, CPU test logs and restoration verification.
