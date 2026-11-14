# Replace Din's Fire remake sounds with an .o2r

The enabled Din's Fire remake can use your own sounds from a small `.o2r` mod.
Each supplied valid cue replaces its compiled softer default. A missing or
invalid cue independently uses the default, so a pack can replace just one cue.
The normal remake still works without an audio mod. These paths affect this
remake's cues, not global vanilla sound IDs or character voices.

The file is an ordinary ZIP-format `.o2r` containing raw WAV files at these exact
case-sensitive paths. Do not add an OTR resource header, `.meta` wrapper, ROM,
preview, or extra enclosing folder.

| Archive entry | Default duration | Role |
|---|---:|---|
| `audio/dins_fire_remake/charge.wav` | 1.00 s | Gathering charge |
| `audio/dins_fire_remake/contact.wav` | 0.35 s | Quiet hand contact / flame catch |
| `audio/dins_fire_remake/release.wav` | 1.30 s | Opening flame bloom |
| `audio/dins_fire_remake/loop.wav` | 3.20 s | Sustained flame; the entire WAV data region repeats |
| `audio/dins_fire_remake/burnout.wav` | 0.60 s | Soft ending |

## Build or edit a pack

Run the helper from the source checkout. To export the current unchanged softer
samples and build an editable starter:

```sh
python3 scripts/assets/build_dins_fire_audio_mod.py --defaults \
  --export-dir /path/to/dins-wavs \
  --output /path/to/Dins_Fire_Audio_Starter.o2r
```

This default export uses the existing NumPy recipe and first verifies that its
output matches the checked-in accepted PCM/manifest. It does not redesign or
normalize any sound. The five WAVs can also be extracted from the starter `.o2r`
with an ordinary ZIP utility. After extraction, they are under
`audio/dins_fire_remake/`.

Edit the named WAVs in an audio editor, then build a new pack:

```sh
python3 scripts/assets/build_dins_fire_audio_mod.py \
  --input-dir /path/to/dins-wavs \
  --output /path/to/My_Dins_Fire_Audio.o2r
```

For an extracted starter, pass its `audio/dins_fire_remake/` directory as
`--input-dir`. The helper reads only the five recognized filenames directly in
that directory; other files are not packed. A partial input directory containing
only `charge.wav`, for example, produces a charge-only mod. At least one named
cue is required. Input-mode packing uses only the Python standard library.
Optional `--report-json /path/to/report.json` writes an external size/hash report;
the report is not included in the mod.

The helper validates every included WAV before building, preserves its bytes and
amplitude, and emits entries in a fixed order with fixed timestamps and no
compression. Identical inputs produce an identical archive. Input validation
errors stop the build rather than knowingly packaging an invalid cue.

## WAV format and timing

Use **RIFF/WAVE PCM format 1, mono, signed 16-bit little-endian, 32,000 Hz**.
Stereo, floating-point, compressed, extensible-format WAVs, and other sample
rates are unsupported. Block alignment must be 2 and byte rate 64,000. Each
file must contain one format chunk and one nonempty data chunk, with no more
than **320,000 samples (10 seconds)**. Unknown bounded metadata chunks and their
RIFF padding are allowed; the entire file is limited to **705,536 bytes**. The
declared RIFF size must exactly cover the file. An empty PCM format extension
is allowed. Malformed/unsupported files fall back per cue in the game.

Prefer the default durations above. File length does not move startup, contact,
release, control return, or expiration: those remain governed by the existing
spell and actor. A shorter one-shot ends earlier; a longer one-shot remains
subject to cue lifecycle/actor expiration. The entire loop data repeats, so
author a seamless loop without an embedded attack or ending. Burnout still has
the existing 0.60-second ending window. Clips up to ten seconds are a parser
bound, not a promise that every sample will play.

Samples enter the existing mixer at **100% of their authored amplitude**, with
no peak normalization or automatic gain boost. Master/SFX volume, existing cue
envelopes, distance, pan, and mix saturation still apply. To quiet a cue, reduce
its amplitude in the editor, leave headroom, and export in the exact format
above. Keep abrupt cuts out of one-shots and match the loop boundary to avoid
clicks. Audition replacement cues together because their overlap can raise the
combined level.

To mute only one cue, include a valid WAV of zero-valued signed-16 samples.
Keep it nonempty and preferably retain that cue's normal duration. A valid
silent WAV is respected as an override; a zero-byte file or missing WAV selects
the compiled fallback and therefore does not mute it.

## Install and restart

For **ComboShip**, put the `.o2r` directly into the applicable per-game mod
folder under the game's resolved application directory:

| Host | Combo mod folder |
|---|---|
| OoT / SoH | `mods/soh/` |
| MM / 2Ship | `mods/2ship/` |

Install a copy in both folders to replace sounds in both hosts. In standalone
non-Combo builds, the corresponding host scans `mods/`. Folder resolution uses
the host's application-directory lookup, which can differ by platform; these
are the relative scanned folders, not a guarantee of an executable-adjacent
absolute path. Put archives directly in the folder rather than a subdirectory.

These folder names follow `appShortName="soh"` in `soh/soh/OTRGlobals.h` and
`appShortName="2ship"` in `mm/2s2h/BenPort.h`, with the actual archive scans in
`MountOotSupplementalArchives` in `soh/soh/OTRGlobals.cpp` and the supplemental
mount block in `mm/2s2h/BenPort.cpp`. The Combo branches append
`"mods/" + appShortName`; they do not scan a shared root `mods/` folder.

The loader reads the owning host's mounted raw archive directly; it does not
require Alt Assets to be enabled. Samples are cached for that resource manager
on the first cast. **Fully quit and restart after installing or editing a pack**
to mount the archive and refresh cached sounds. Installing during a session or
merely changing scenes is insufficient. Keep a single installed provider for
each cue to avoid ambiguous competing packs; remove a replacement pack and
restart to return to the compiled defaults.

The starter contains the provisionally accepted softer sounds unchanged.
Packaging and parser checks do not substitute for listening to an edited pack
in the configured game.
