# Din’s Fire native asset contract

This directory owns the accepted luminous atlas, its provenance, the protected
POC recovery source, and the deterministic texture recipe used by both native
hosts. The implementation candidate is based on source commit
`cd12567fe1c0d64a205e1d360165cb000166f159` on
`candidate/dins-fire-usable-linger-20261110`. The verified pre-linger checkpoint
is `28cd69306bfb4a8d13cac8f5d8810ef85758d306`; its source and original derived
resources remain recoverable separately. Publication remains held.
The accepted appearance study is a visual target; the native implementation
still requires configured gameplay evidence.

## Accepted input and protected recovery

`flame_ribbons_luminous_rgba.png` is the exact accepted 1536 × 1024 RGBA image.
Its SHA256 is
`9705407ffa7226f6d4f704a7b20575d66e4aa704759ef3a0670309de78fc186e`.
The encoder rejects any change to those bytes. It does not generate new art,
recolor the image, derive alpha from brightness, or edit the accepted source.
`luminous-art-direction.txt` retains the accepted artwork prompt;
`accepted-source-provenance.json` identifies the source archive and exact
reference shader/burnout recipe files.

The earlier `flame_source.png`, `art-direction.txt`, `flameTex` and `sparkTex`
remain recoverable. The two old resource hashes are enforced before any write:

| Resource | SHA256 |
| --- | --- |
| `flameTex` | `dc2d51de60b2ba9bd55b9f3497a312b44ae99654ecf9730ad140f06f82158ff4` |
| `sparkTex` | `45c863532059d5dd7a3469217ae408b0938b9e9ad26e9b052e163fd40a200d4f` |

## Native resources

Every path below is under `custom/dins_fire_remake/`. Both hosts contain the same
bytes under their `assets/custom/custom/dins_fire_remake/` directory. No original
Din, Fire Medallion, sword, shield, trident or shared native resource is replaced.

| Resource | Physical size | Logical GBI size | Encoding |
| --- | --- | --- | --- |
| `ribbon0Tex` | 768 × 172 | 32 × 32 | RGBA32; atlas rows [0, 343) |
| `ribbon1Tex` | 768 × 163 | 32 × 32 | RGBA32; atlas rows [343, 668) |
| `ribbon2Tex` | 768 × 178 | 32 × 32 | RGBA32; atlas rows [668, 1024) |
| `ribbonBurn{band}_{frame}Tex` | Same as the band’s row | 32 × 32 | RGBA32; 6 bands × 11 native fade samples |
| `haloTex` | 128 × 128 | 32 × 32 | Warm soft halo with independent alpha |
| `ribbon{row}PaletteTex` | Same as the matching row | 32 × 32 | Neutral RGB for the native Primary/Secondary palette |
| `ribbonBurn{band}_{frame}PaletteTex` | Same as the matching burned row | 32 × 32 | 66 neutral RGB burnout companions |
| `haloPaletteTex` | 128 × 128 | 32 × 32 | Neutral RGB warm-halo companion |
| `whiteHaloTex` | 128 × 128 | 32 × 32 | White soft halo with independent alpha |
| `trailTex` | 16 × 128 | 32 × 32 | Mint trail, constant along U, soft across V |
| `flameTex` | 256 × 256 | 64 × 64 | Protected intensity8 resource |
| `sparkTex` | 64 × 64 | 64 × 64 | Protected intensity8 glint |

The exact source row cuts are preserved. Each channel is resized independently
with Lanczos to half width and the next integer half height. This is native
encoding of the accepted input, so the source PNG remains unchanged. RGB retains
the luminous cream/yellow/orange detail and alpha retains its independent
coverage. The halos and trail use the accepted reference renderer’s analytical
formulas and dimensions.

The loader contract was checked against the actual repository:

- `libultraship/include/fast/resource/type/Texture.h` declares RGBA32bpp = 1,
  Grayscale8bpp = 6, RAW = 1 and IMG = 2.
- The native V1 factory reads the physical width/height, flags, two scales and
  payload length from the 28 bytes after the 64-byte resource header.
- `Interpreter::ImportTextureImg` uploads the whole RGBA payload with its
  physical dimensions. These assets use flags 3 and unit scales, preserving
  the existing whole-image convention.

A new native draw loads a 32 × 32 RGBA32 logical tile, which describes 4096
bytes. The accepted geometry’s 64-unit UVs are multiplied by 0.5 only when
submitting that tile; normalized UVs are unchanged. The old intensity resources
keep their existing 64 × 64 load and UV contract. The generated
`native_resources.h` is the sole declaration of aligned resource names and
lookup tables used by the renderer.

## Baked burnout

The native lifetime now comes from the pure shared contract
`combo/render/DinsFireTiming.h`. The encoder reads only its exact named literal
integer bases; it does not evaluate C expressions. The manifest records the
header's SHA256, those base values, and the resulting timing. A timing-header
change therefore makes `--check` require regenerated metadata and resources.

At 20 Hz, fire release remains at cast update 21 / 1.05 seconds, and control
returns at update 27 / 1.35 seconds. The user has **six unpaused seconds after
control returns before burnout begins**. Fire spans 138 native updates:
8 growth, 118 hold, and 12 fade. The fade begins at fire frame 126 / cast time
7.35 seconds; the visible fire ends at fire frame 138 / 7.95 seconds. The
unchanged damage hold remains a separate shared gameplay value; the extra
interaction hold does not extend the initial strong damage ramp.

Frame 126 uses the unburned row. Frames **127–137** use the 66 baked colored
resources and their 66 neutral companions. Band rows remain
`[0, 1, 2, 0, 1, 2]`; burnout begins at
`[7.43, 7.41, 7.39, 7.37, 7.35, 7.40]` seconds for those bands. These are the
accepted band offsets shifted by 4.30 seconds. Their envelopes and the
0.6-second fade duration are preserved.

Horizontal motion keeps the **actual elapsed fire age**. For example, the first
new baked sample uses fire age 127 / 20 = 6.35 seconds, rather than resetting UV
scroll to the previous first sample's age. Only the fade schedule is shifted;
the longer hold therefore flows continuously into the later baked textures.

For each native frame the encoder retains the reference burn envelope:

```text
burn = smoothstep(clamp((time - bandStart) / (endSeconds - bandStart), 0, 1)) ** 1.35
u = localU - fireFrame / 20 * (0.27 + band * 0.025)
v = (rowStart + localV * rowHeight) / 1024
detail = 0.025 * burn * (sin(u*41.9 + v*123.4 + band*1.1)
                       + 0.5*sin(u*103.7 - v*271.1))
alphaOut = alpha * smoothstep((alpha + detail - (burn**0.80*0.68 - 0.08)) / 0.20)
```

The erosion is skipped when burn is zero, matching the reference branch.
The fine detail depends on **unwrapped** U. Baking a repeating generic mask
would change that detail each time the source crosses U = 0; therefore each
fade image bakes both the original horizontal scroll and erosion across the
whole ribbon. The fade mesh samples local U = 0..1 while the accepted scroll
coefficient is retained in the texture. No extra whole-sheet `(1 - burn)`
multiplier is baked. Native geometry still supplies its accepted opacity,
view-dependent grazing fade, tint and shrinking/peeling shape.

The finite native representation uses a half-resolution row, 8-bit channels
and bilinear sampling. Those encoding and rasterization differences are
explicit; these files do not establish pixel-identical gameplay output against
the offline reference renderer.

## Native Primary/Secondary cosmetic colors

The 12 timing-independent resources retain their exact pre-linger bytes. The
66 default colored burnout resources are intentionally rebaked for the later
phase, then their 66 neutral companions are derived from those new bytes.
Default native Din colors continue to use the accepted full-color artwork. Editing either existing
`Magic.DinsPrimary` or `Magic.DinsSecondary` color selects 70 neutral companions:
three ribbon bases, 66 burned ribbon samples and the warm halo.

Each companion is derived **from the already encoded colored resource**. Its
RGB channels all contain the nearest-byte luminance
`0.299 R + 0.587 G + 0.114 B`, computed with exact integer coefficients and
half-up rounding:

```text
gray = (299*R + 587*G + 114*B + 500) // 1000
RGB = (gray, gray, gray)
alpha = original encoded alpha byte
```

The entire 92-byte header, physical dimensions and every alpha byte are copied
unchanged. These are RGBA32 resources with independent alpha; they do not turn
brightness into a replacement transparency mask. The accepted resampling,
horizontal scroll and burnout coverage are already present in each source and
are preserved in its companion.

The native custom-color material maps the neutral RGB between the two existing
editor colors: `Secondary + (Primary - Secondary) * gray`, retaining texture
alpha × vertex alpha. This lets either color recolor the flame without retaining
orange from the accepted artwork. The default material still uses the original
RGBA. If a selected neutral burnout variant is missing, the renderer uses the
neutral moving base instead of introducing colored-source pixels.

The timing-independent set is enforced by canonical SHA256
`67e95a75d182d584c4102a2c4563dff360c68efd3b56be18d9782c0d129b4d3d`.
It contains the three colored bases, three neutral bases, both colored/white
halos plus the neutral halo, the trail, and the two protected recovery textures.
`pre_linger_resource_provenance.json` preserves all 144 earlier resource hashes,
the earlier canonical sets and checkpoint identity. Its own hash is enforced
by the encoder. Every new burnout resource records the exact earlier-phase
resource name and hash that it replaces, so the intentional derived changes
are distinct from protected inputs.

The canonical representation is sorted UTF-8 records of
`resource name + NUL + resource SHA256 + LF`. Current hashes are:

| Canonical resource set | Count | SHA256 |
| --- | --- | --- |
| Timing-independent, unchanged | 12 | `67e95a75d182d584c4102a2c4563dff360c68efd3b56be18d9782c0d129b4d3d` |
| Default colored and recovery | 74 | `b137c178d9adb2552c46cce0bfb365c7984fc2413f758b9ba4abc88166e2dc2a` |
| Neutral palette companions | 70 | `8029f0252523085257e5b00d138dbbd61b50d60d3d4036528f948d4873ba10fe` |
| Complete current set | 144 | `41363e07fd630c7ca2f19ad2a770b21b0ae04106a79b07948eea0ea93dd2e6ba` |

## Memory, regeneration and archive

There are **144 resources per host**: 74 default colored/recovery resources, including the 2
protected recovery textures, and 70 neutral companions. All sets together have
**72,976,384 decoded bytes (69.60 MiB)** per host; complete resource files total
72,989,632 bytes. The default set remains 36,664,320 decoded bytes (34.97 MiB),
and the neutral companions add 36,312,064 bytes (34.63 MiB). Default colors load
the default colored material set; neutral companions are selected only for customized
colors. Burnout resources are resolved for the current frame, without eagerly
loading all variants.

`manifest.json` records exact payload/resource totals, hashes, alpha statistics,
physical/logical dimensions and frame/band provenance. The encoder enforces a
72 MiB maximum decoded payload for all sets together and a maximum physical
dimension of 768. These are available-resource payload totals, not a measured
running-game cache or peak-memory claim.

The verified deterministic later-phase private-asset archive is
**31,904,120 bytes (30.43 MiB)**, SHA256
`3f0c4a6f83520733bfab0e3de82fe0ec6b75c34dcfbf9079935e84add951b13c`.

The public source candidate includes the generated native resources, their
manifest and the encoder. The two source PNGs are intentionally omitted:
`flame_ribbons_luminous_rgba.png` and `flame_source.png`. Neither the game nor
normal application/resource packaging needs them.

Required CI validates the committed outputs without source artwork:

```sh
python3 -B scripts/assets/build_dins_fire_remake.py --check-native
python3 -B tests/dins_fire_assets/test_resources.py
```

This mode pins the reviewed manifest and resource hashes, checks native texture
format/metadata, memory budgets, complete resource sets, timing, declarations,
pre-linger provenance and both host copies. It does not claim to regenerate the
source recipe. Negative controls reject missing/extra resources, matching
corruption in both hosts, and changed metadata or provenance.

To regenerate artwork, supply the original two PNGs separately at the paths
above in `tools/dins_fire_remake/`. Their required hashes remain recorded in
`manifest.json` and enforced by the encoder. Then, with Python and Pillow available:

```sh
python3 -B scripts/assets/build_dins_fire_remake.py --check
python3 -B scripts/assets/build_dins_fire_remake.py --check --archive /tmp/dins-native-private-assets.o2r
```

Without `--check`, the encoder writes both host copies, the manifest and the
header. After writing the new equivalents, it retires only the explicit
pre-linger burnout filenames for frames 41–51. Unknown files are never removed.
`--check` regenerates the expected bytes in memory and compares every output,
then confirms actual host byte parity and exact 144-resource directory closure.
The accepted source hash, protected recovery hashes, pre-linger provenance and
all 12 timing-independent resource hashes are checked in either mode. Every neutral companion is checked for equal RGB channels and
byte-identical alpha/header data against its colored source. The encoder uses no NumPy or
image-generation dependency.

`--archive` writes a deterministic ZIP/O2R containing all 144 privately named
resources: sorted paths, fixed timestamp, permissions and compression, no
preview media. It reopens the archive, verifies every payload and all metadata,
and prints exact archive bytes/SHA256. It requires the new source build;
installing this archive into an older executable does not implement the spell.
The optional archive is an asset package, separate from the source recovery
checkpoint.

## Verification boundary

Deterministic generation, archive checks, preserved input hashes, host parity,
real native resource-factory parsing, native draw tests and a full linked
application build are separate checks. Neither successful asset parsing nor an
accepted appearance study proves the running game’s final transparency,
resource lifetime, gameplay interaction, camera or animation behavior. Native
runtime acceptance and master promotion remain separate from this source
checkpoint.
