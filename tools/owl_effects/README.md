# Owl POC4 asset recipe

This portable recipe builds the accepted translucent socket flames and the
page-glow vertex header. It requires Python 3 and NumPy; it does not import the
preview renderer or depend on the current working directory.

Keep the POC8 baseline archive outside the Git checkout. Its required SHA256 is
`c8922c8397cdd5de61196cbbc2268ecc75b7dfbee99dedb2a3fcbd727cbc9d04`.

```sh
python tools/owl_effects/build_runtime_assets.py \
  --baseline /path/to/MM_3DS_Owl_POC8_Rectangular_Base_Upright_Book.o2r \
  --output /path/to/zzzzzzzzzz_MM_Owl_SocketFlames_POC4.o2r \
  --header /path/to/ComboOwlPageGlow.h
```

The output archive SHA256 must be
`dbdfafa43a210ee149ff03ed6bb9f0ae64e87d47015e513ded7db5bc57a14f73`.
The generated header is byte-identical to the source copy in
`combo/actors/ComboOwlPageGlow.h`. `Asset_Validation.json` is written beside the
archive. The archive uses sorted resource names, fixed timestamps, and stored
ZIP entries for reproducibility.

Only the native and Alt opened display lists change from POC8. The recipe
removes 1,650 original eye-glow triangles and 384 page-glow triangles so the
runtime can draw their replacements. All 119 other original resources remain
byte-identical, including closed pose, pedestal, upright book, writing, and
960 static page-border triangles. Unreferenced original vertex resources are
retained for recovery.

New resources comprise a marker and 60 RGBA32 32×32 frames per eye, with native
and Alt aliases. Frame indices sample accepted preview frames 0,3,…177. Texture
top row is mathematical v=1, so runtime bottom UV t=1 and top UV t=0. Alpha peaks
at 166/255. The page header retains exact raw actor positions and colors; the
shared renderer normalizes positions before fixed-point conversion.

This verifies deterministic asset construction and references. It does not
establish in-game rendering, donor ownership, or transition behavior.
