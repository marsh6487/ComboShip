# Default OoT adult Link recordings in MM (candidate, 2026-11-08)

Owned files: `mm/mods/voice_pack/oot_adult_voice.{h,cpp}` and this test directory.
This provider loads recorded native OoT donors directly from the native
`oot.o2r` (or `oot-mq.o2r`) archive handle, decodes native ADPCM, and gives the MM PCM mixer immutable mono 32000 Hz
clips. It embeds no game audio and does not change any archive. Combined builds select the registered `oot` manager; solo MM selects its own
manager, whose mounted OoT companion is already supported by the existing
asset loader. Missing owners/native archives return unavailable and retry
later. Normal OoT takes precedence over MQ. Exact native archive selection
bypasses other mods and Alt shadowing.

Native OoT `audio/sequences/000_Sound_Effects` sends voice actions through the
adult dynamic table at sequence offset `0x5FF2` (child table starts `0x6032`).
The channel selects instrument `0x7E`: note + (layer transposition << 6) selects
a sound-effect entry from font 0, `audio/fonts/00_Sound_Effects_1`. The recipe
sample IDs below come from those native scripts and their self-modified common
layer at `0x612D`, not from inferring actions from sample names. All referenced
font entries have tuning 0.625, corresponding to decoded samples played at
20000 Hz in a 32000 Hz mixer. PCM resampling preserves that native speed.

| Action offset (hex) | Font-0 sample IDs (hex), slash separates variants |
|---|---|
| 00, 1F | 00 / 01 / 02 / 03 |
| 01 | 04 / 05 |
| 02 | 15 / 16 |
| 03 | 06 / 19 |
| 04 | 07 / 08 |
| 05, 0D | 09 / 0A / 0B |
| 06 | 0C / 0D / 0E |
| 07 | 11 / 12 |
| 08, 1D | 0F / 10 |
| 09 | 13 / 17 |
| 0A | 38 |
| 0B | 4D, 4E, 4F |
| 0C | 0F / 10 |
| 0E | 50, 51, 52 |
| 0F | 53, 54, 55 |
| 10 | 37 |
| 11 | 56, 57, 58 |
| 12 | 00 |
| 13 | 43 |
| 14 | 1A / 1B |
| 15 | 05 |
| 16 | 86 |
| 17 | 04 |
| 18 | 07 |
| 19 | 06 |
| 1A | 18 |
| 1B | 3C |
| 1C | 3D |
| 1E | 0D |

Sample 43 is **Child Link - Shivers** in native OoT's adult table too. This
provider preserves that actual native definition instead of inventing an adult
shiver recording. The adult table also deliberately aliases several actions.

Native MM bank 0 is not an equivalent substitute: exact META payload and raw
ADPCM hashes match only 11 of 44 adult-labelled OoT resources (attack 1–4,
strong attack 1–2, falling 1–2, gasp 1, hup, glug). The remaining adult OoT
recordings have no byte-identical MM counterpart in the supplied base archives.

## Verification

Run:

```
python tests/oot_adult_voice/run_tests.py /path/to/native/oot.o2r
```

Input tested: OoT archive SHA256
`ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b`.

The fixture compiles the production provider with resource-boundary stubs, then
loads actual native donor bytes. It checks every recipe and variant, nonzero
PCM, variant wrapping, process-lifetime pointers after registry teardown,
invalid input, retry after a missing owner/archive, rejection of a similarly named mod
archive, and solo-MM companion availability. Every
resampled donor is compared sample for sample with the actual scalar native
`aADPCMdecImpl` function extracted from `soh/soh/mixer.c`. Native sequence table
pointers and all used soundfont tunings are checked against the actual archive.

Implementation details follow the native V2 parser in
`soh/soh/resource/importer/AudioSampleFactory.cpp`. A final partial ADPCM frame
is zero-padded like the native DMA boundary in libultraship; no metadata bytes
may enter the decoder. Invalid codec, book, predictor, bounds, and duration
return unavailable rather than exposing a partial cache.

## Limits and handoff

**Implemented and fixture verified; not native-build or in-game proven.**
The fixture substitutes only archive ownership/access, not PCM decoding. The
full native environment currently lacks dependencies including `spdlog/async.h`.

This is a recording provider, not an OoT sequence interpreter. Multipart
choking/sneeze/sigh/stretch actions concatenate their donor recordings in
native order; native sequence note durations, rests, velocity/envelope, channel
frequency effects, and infinite glug/pant repetitions are not synthesized.
The calling MM mixer supplies its existing per-action renewal/stop behavior.
Variants are available in native donor order; the caller decides selection.
No automatic adult 0.85 frequency change belongs on these PCM recordings.

The root integration must call the provider only for adult human Time Gate
Link after the optional Marin provider declines, leaving native child and
transformation routing untouched. It must keep returned buffers immutable and
call loading only from the game thread. `Get("oot")` intentionally does not fall
back to active MM ownership in combined builds. ComboShip initializes OoT
unconditionally before either game enters (`combo/ComboShip.cpp` initialization
step 4), and the OoT constructor registers its manager (`OTRGlobals.cpp`).
Solo MM requires its ordinary mounted native OoT companion archive.

The next decisive test is a configured ComboShip run without the optional
Marin archive: adult attack, horse call, damage, fall, breathing and item
voices must use the mapped OoT recordings, with stop/renewal and pitch-slider
behavior checked. Child/transformation playback and the Marin override must
remain as previously routed. No master promotion or runtime acceptance claimed.

Recovery: restore the commit containing only the owned files, retain the exact
OoT base asset above, and rerun the command. Audio assets are external inputs
and are deliberately absent from this source checkpoint.
