# Native GI HD icons

This candidate replaces the requested 23 inventory icons with 512×512 RGBA
art while retaining their 32×32 logical UI size. The accepted baseline commit
is `122dd5f68cb37fcf515c3726f8a77043db5dcdf4`; no game runtime is claimed here.

Six Slate variants, Elemental Wand and its six rods, Demise Destruction, Mario
Mask and Climbing Boots render the actual `tools/nei_gi/CHECKPOINTS` geometry and materials. The
six non-Magic RPG icons parse their native XML display lists, including nested
calls, vertex loads, triangle indices, signed normals and primitive/environment
colors. The Power model lies in the YZ plane, so its camera faces that plane.
No original 32px icon contributes pixels to these images.

Magic's native ROM-only jar geometry was unavailable. Its separate transparent
image was authored with the built-in image generator; the full prompt and
1280px source are retained in `SOURCE/`. It has a private `gStatMagicTex` path,
so other Magic Jar icons and their 24px UI layouts are unaffected.

Lighting is an offline approximation. Missing ROM-only reflection/decal maps
on three RPG models use neutral illumination with the source colors; these
limitations and every source hash are recorded in `manifest.json`. Model
silhouettes and native mesh topology remain unchanged. The renderer uses
2× supersampling and premultiplied alpha filtering for clean transparent edges.
The source Demise core is exactly black; its native pale translucent shell
still overlays that core.

Rebuild from the checked-in sources with Python, NumPy and Pillow:

```sh
python tools/nei_icons/build.py --render --install --pack /tmp/nei-icons-hd.o2r
python tools/nei_icons/build.py --verify
python tools/nei_icons/verify_routes.py
```

The install step writes byte-identical resources to both host custom trees at
the existing icon paths. It removes the former `*.rgba32.png` input for each
replaced path to avoid duplicate entries during ZAPD packing. Authoring PNGs
remain under `PNGS/` and the review sheet is `contact_sheet.png`.

The serialized format matches `libultraship/src/fast/resource/factory/TextureFactory.cpp`:
64-byte resource header, OTEX version 1, RGBA32 type 1, raw flag 1, dimensions
512×512, HByteScale and VPixelScale 16.0, then 1,048,576 RGBA bytes. These are
base resources only; normal external mod and Alt Assets precedence is preserved.
The optional deterministic `.o2r` contains the same 23 resource paths without
an `alt/` namespace. Production builds include the assets in `soh.o2r` and
`2ship.o2r` through the existing custom-asset targets.

The Wand's already-defined per-rod paths now appear in both hosts' receipt
mapping and `Wand_ModeIcon`. The in-game wheel selects these paths with 32px
logical slots; mode ownership, order, grants and confirmation stay unchanged.
MM's wheel confirmation also reloads its cached HUD pointers, as pause selection
already does; OOT resolves its active icon during each HUD draw.
The standalone Elemental Wand receipt retains its own base icon. The route
probe requires a C99 compiler and executes the actual selectors/wheel builders
with all 256 selector values plus full, sparse and empty owned-mode lists.

Verification checks all 23 source hashes, dimensions, alpha, framing, duplicate
input absence, OTEX version/raw flag/scales, exact decoded pixel equality and
host parity; packaging also verifies every serialized archive entry. In-game
receipt, inventory, tracker and foreign-host presentation with Alt Assets off
and on remain the decisive runtime checks.
