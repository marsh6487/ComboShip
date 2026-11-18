# Positional music follow-up

Baseline: prepublication candidate `c55377c9042c17533a460badbbb12abd09942bf1`.
The report names Zora Shop and other unidentified locations; no matching
audio clip, current audio pack, sound-mode setting or toggle state was supplied.

Two native defects were reproduced and corrected:

- OoT's `aHiLoGainImpl` processed eight signed 16-bit samples, then subtracted
  only eight bytes from its remaining byte count. Gain therefore continued
  into adjacent synthesis workspace. MM already had the sixteen-byte
  correction. OoT now uses the same span accounting; the gain arithmetic and
  command rounding stay unchanged.
- Both engines' channel pan-weight commands compared the incoming weight
  against `newPan` instead of `panChannelWeight`. A centered source with
  pan 64 could suppress a requested weight 64 and retain a stale weight 127.
  The handlers now compare/update the actual weight, using its unsigned byte
  payload so the native value 128 also behaves correctly. Pan position,
  distance volume, reverb, pitch, sequence ownership and filters are unchanged.

The existing **Clean Streamed Positional Music** option is MM-only and defaults
off. When enabled it bypasses MM's exact shared point-source filter and gain
127 for S16/OPUS sample codecs. It cannot affect OoT Zora Shop. Its native note
initialization/release regression still passes; no filter/gain policy change
or new automatic default was made in this follow-up.

MM ordinary object music uses flags `0x3F`, applying native spatial pan,
surround, filter, gain and distance volume. The policy removes only the
filter/gain pair. A separate coverage observation remains unresolved:
generated stereo streamed sequences use distinct channel pans, while native
point-source commands overwrite all channel pans. This can collapse the
stereo arrangement; changing it would require preserving authored stereo
identity rather than globally disabling positional pan. That was not changed
without evidence connecting it to the reported locations.

Verification:

- The extended full production mixer fixture failed before the OoT byte-span
  correction, including 179,507 mismatched samples outside or within the
  independently expected gain result. After correction, each host passed
  134,833 cases, including every 8-bit gain, rounded spans, adjacent workspace,
  SSE2 mix arithmetic and reverb. ASan/UBSan passed with leak detection disabled
  for this ptrace-hosted environment.
- The production pan-command fixture failed before the handler correction.
  Both full handlers then passed using real native audio layouts, exhaustive
  pan positions 0..127 and previous/requested weights 0..128, including
  unchanged/no-op handling and byte-identical unrelated channel attributes.
- `run_mm_streamed_positional_tests.py --sanitize` passed unchanged, preserving
  authored filters, non-positional/native sample processing, spatial
  attributes and released-note behavior.

This is source/native-fixture verification, not audible proof that the
reported Zora Shop symptom is resolved. The next decisive gameplay check is
the reported shop with the user's installed music pack: compare clarity while
turning/moving, exit/re-enter, then test one ordinary non-positional scene.
For MM, also compare the existing cleanup option off/on. Existing bounded
`gDeveloperTools.MMAudioPCMTrace` diagnostics can distinguish filter/gain
processing from a remaining mix/stream issue if the sound is still washed out.
