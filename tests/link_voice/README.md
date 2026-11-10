# Link player voice fixtures

These tests cover the human player voice routing and PCM mixer in 2Ship. They
are independent of Din or any other replacement player model.

`mixer_test.cpp` checks renewal, overlap bounds, PCM output and concurrent slot
publication. CI runs it with AddressSanitizer and UndefinedBehaviorSanitizer.

`run_tests.py [PATH_TO_OPTIONAL_PACK]` compiles the actual Link voice handler,
native player voice dispatch, GameInteractor pitch callback, native SFX request
queue, saved adult-mode predicate, audio-buffer mix call and shared item helpers for solo and Combo
builds. Without an archive, it generates deterministic PCM fixtures for CI;
with one, it reads that pack's actual bytes. It checks child/adult action
mapping, selected PCM, stop/renewal, volume, explicit pitch controls, excluded
forms/actors, malformed-manifest fallback and the failure of both adult providers. The saved adult bit and
ForceOn override are independent of Time Gate item ownership or casting.

The native adult fallback remains a controlled clip boundary in this fixture;
real OoT decoding is covered by `tests/oot_adult_voice`. Archive lookup is a
filesystem boundary here; real ArchiveManager/O2rArchive access to POC3 was
separately checked in the November 8 follow-up. None proves audible in-game
behavior. Diagnostic capture is a lightweight logging boundary; production
object compilation and the native failure probe verify real spdlog integration.

The internal API and filenames now use `LinkVoice` / `link_voice`. The existing
`custom/din_voice/` archive namespace is intentionally retained solely for
compatibility with previously prepared optional packs. It does not select or
require a Din model. The rename changes no action mappings, sample selection,
archive lookup, automatic pitch policy or native fallback behavior.

The optional PCM pack format is a separate input from the original OoT voicepak.
An unconverted OoT voicepak is not directly loaded by the MM provider. POC3's
82 adult PCM resources have been independently compared with resampled original
HW Marin recordings, byte for byte. Configured audible parity remains pending.
