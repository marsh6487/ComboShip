# Missing music sample source repair — 2026-10-29

Baseline: `candidate/cel-unbound-smoke-repair-20261029`, source commit
`3cbb22d1f09e85deb206d2b085fc62105bc09fad`, tree
`711c3cdf8bde43bb283345ac4ff20e1b3749f045`. This is the source of the
runtime-tested Windows merge build `3415d461a25723960095731ea884ecf3059231a8`.
The isolated candidate is `candidate/missing-music-sample-guard-20261029`.

## Evidence and bounded design

The two new startup failures in Fleet of Harkinian(6).log have identical
2ship.dll RVA `0x55FF32`. The accompanying dump contains the XML Path
`custom/samples/Kingdom Hearts - Re Chain of Memories - Graceful AssassinL`.
The raw audio chunks were unmounted while the core metadata remained mounted.
The factory immediately dereferenced the null result of ArchiveManager::LoadFile.
cor confirmed that removing the remaining music definitions restores startup.

Input SHA256: log
`e3c7f6f1351ca84d0426c8449fee7bd39cb1df40e59de7bc5e1ecc66521cf7da`;
dump `15d33f8e06216c3f51802c1c7bc77991dedbd67ff79fdb347e64ae1b29c62876`.

Return a valid, silent native sample when the XML references missing/empty
raw audio. Log both the metadata resource and the requested data path. Use
the existing CODEC_S16_INMEMORY silence branch, MEDIUM_RAM, no preload flag,
zero payload size and a bounded, non-looping loop descriptor. A nullptr
resource is unsuitable: soundfont parsers and note scheduling expect sample
objects to remain valid. Changing every consumer would widen this repair.

Apply the same guard to OoT and MM, including solo and Combo builds. Keep
per-game resource-manager routing, valid sample bytes/metadata, decoders,
soundfonts, native binary samples, collision expansion, scenes and all assets
unchanged. Malformed encoded streams and general XML validation are outside
this missing-data repair. No accepted master or release is changed.

## Plan and verification ledger

- [x] Reproduce the null result with the actual XML factories before editing.
- [x] Add the guard to both games and verify missing file, null/empty buffer,
      missing/empty Path across FLAC, MP3, Ogg, WAV and native ADPCM inputs.
- [x] Exercise real soundfont parsing and both native silence branches;
      preserve valid raw bytes and metadata; repeat with ASan/UBSan.
- [x] Register the regression in CI, review the narrow diff and preserve
      source, tests, verification results and restoration instructions.

Native tests compile the actual factory bodies, production resource types and
destructors, real tinyxml2 and native synthesis branches. Archive lookup and
codec workers are controlled boundaries; these checks do not claim a complete
game build, actual codec decoding, sound-device output or configured runtime.

Both untouched factories failed UBSan with a null File member access; valid
sample controls passed in all four game/mode combinations before the repair.
After repair, all 108 native cases pass normally and all 108 pass with
ASan/UBSan. LeakSanitizer cannot inspect `/proc/task` on this host, so only the
local sanitizer command uses `ASAN_OPTIONS=detect_leaks=0`; the CI command
retains the normal sanitizer defaults.

Existing audio checks also pass: actual native SSE2 mixers (134,833 MM and
132,529 OoT cases, zero mismatches), six MM/OoT PCM parity cases at
32/44.1/48 kHz (zero of 64,000 samples differ in each case), both MM worker
controls and all five sanitized OoT worker controls. All 259 existing CI
entries are retained unchanged; one new group runs the normal and sanitized
missing-data suite. Full platform compilation and configured playback are
pending and are not inferred from these native checks.

Independent read-only review found no Critical or Important issues after
tracing note scheduling, native silence, sample ownership and preload/slow-load
paths in both engines. Complete live ResourceManager eviction/reload is not
exercised by the fixture; compressed decoder execution and configured playback
remain outside its proof. The production diff changes only the two XML sample
loaders; no soundfont, binary-sample, decoder or asset bytes change.

## Restore and runtime test

Apply this candidate's patch on the exact baseline above. Run
`python3 -B tests/audio_missing_sample/run_tests.py` and repeat with
`--sanitize`. All tests use in-tree headers and tinyxml2, with a C++20 compiler.

Build the matching engine/game set normally. Restore music core metadata while
leaving one audio chunk out, launch twice and check the named missing-data
error without a crash. Restore all seven matching audio chunks, restart the
game, then confirm normal music/fanfare/SFX playback. These configured runtime checks remain
pending until run with a matching rebuilt executable.
