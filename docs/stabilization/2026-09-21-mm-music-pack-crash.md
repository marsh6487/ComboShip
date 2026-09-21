# MM music-pack stress crash

The September 21 stress-test log contains two identical access violations and a separate C++ archive exception. The logged build is `a77259f6acde6689ca419301e5d0e540c23f481a`, whose tree matches the streamed-ID repair at `cc1346ce`. It predates the combined weather/item candidate, so this evidence does not implicate the added rain or item colors.

## Confirmed repeated failure

The exact Windows artifact from workflow `35647288150` was inspected, rather than relying on the crash handler's nearest exported symbol. At `2ship.dll + 0xCC9363`, `ogg_page_serialno` reads the serial-number bytes through a null Ogg page header. Its caller at `+0x45BBF8` is MM's `GetOggType`, immediately after an unchecked `ogg_sync_pageout` failure. The log does not identify which sample supplied the input.

The classifier previously ignored the page and packet parser results, advertised 4096 input bytes even for short files, and compared signatures without checking packet length. Malformed, empty, truncated and checksum-invalid inputs reproduce the null-header crash in the actual production functions linked with libogg/libvorbis.

## Bounded repair

- Classify only a successfully parsed page and packet, using the actual input length and bounded signature comparisons. Handle zero-size callback reads without dividing by zero.
- Check Vorbis open, dimensions, read results and decoded length. Own temporary allocations until decoding finishes; rejected samples remain unpublished. Log the resource path on importer failure instead of throwing from the detached decoder thread.
- Keep the already-cleared streamed playback buffer silent while a sample is pending or rejected. This also prevents selecting a rejected sample from causing a later null-pointer access.
- Check Opus decoder open and seek results and stop on any nonpositive read. Review found these related unchecked failure paths; they are not claimed to be the logged crash address.

Other codecs, archive locking and decoder thread scheduling are outside this patch.

## Verification

`python3 scripts/diagnostics/run_mm_audio_decoder_tests.py` runs 21 cases. It extracts the production Ogg functions and streamed playback branch, uses real libogg/libvorbis with generated tone data, and injects documented Vorbis/Opus error returns at library boundaries. Opus importer tests cover header classification and encoded-byte preservation; mixer tests cover open/seek/read failures, successful partial reads and EOF, not an end-to-end Opus listening test.

The original implementation failed the malformed-header, Vorbis-error, zero-size-read and null-playback regressions. The original Opus mixer failed open/seek/read-error cases. The repaired implementation passes all 21 cases with AddressSanitizer and UndefinedBehaviorSanitizer. LeakSanitizer was disabled because the local executor's tracing environment prevents it from running; no leak-check result is claimed.

The existing real-header MM audio compilation/runtime checks, weather suite and 62 item-color cases also pass locally. CI installs the decoder test dependencies and runs this suite before full Windows/Linux builds. Full build success and the user's pack-specific runtime stress acceptance remain separate gates.

## Remaining evidence and runtime check

The middle log exception (`0xE06D7363`) reaches `libultraship.dll + 0x1068D2` through `O2rArchive::LoadFile`. Disassembly maps it to a failed pool-mutex lock followed by `_Throw_Cpp_error(5)` (`resource_deadlock_would_occur`). Its root cause is unresolved. It is not evidence that a decoder thread limit was reached, and this patch does not claim to fix it.

Retest the same music packs with the combined candidate. Confirm startup, track changes, scene randomization, fanfares and OoT/MM switching. If an importer rejects a sample, retain the new resource-path diagnostic to identify the supplying pack. Preserve any new crash log, especially if the archive exception repeats. Do not promote the candidate to the sacred baseline without runtime acceptance.
