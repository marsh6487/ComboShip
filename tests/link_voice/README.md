# Link player voice fixtures

These tests cover the human player voice routing and PCM mixer in 2Ship. They
are independent of Din or any other replacement player model.

`mixer_test.cpp` checks renewal, overlap bounds, PCM output and concurrent slot
publication. CI runs it with AddressSanitizer and UndefinedBehaviorSanitizer.

`run_tests.py PATH_TO_OPTIONAL_PACK` compiles the actual Link voice handler,
native player voice dispatch and shared item helpers for solo and Combo builds.
It checks child/adult action mapping, exact selected PCM, stop/renewal, volume,
explicit pitch controls and excluded forms/actors. The native adult fallback is
a controlled clip boundary in this fixture; real OoT decoding is covered by
`tests/oot_adult_voice`. Neither fixture proves audible in-game behavior.

The internal API and filenames now use `LinkVoice` / `link_voice`. The existing
`custom/din_voice/` archive namespace is intentionally retained solely for
compatibility with previously prepared optional packs. It does not select or
require a Din model. The rename changes no action mappings, sample selection,
archive lookup, automatic pitch policy or native fallback behavior.

The optional PCM pack format is a separate input from the original OoT voicepak.
This rename does not claim that an unconverted OoT voicepak is directly loaded
by the MM provider. Actual Marin pack contents and audible parity require their
own configured verification.
