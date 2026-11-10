# MM animated-material factory validation

This fixture compiles the actual MM `TextureAnimationFactory.cpp`, resource destructor, LUS binary readers and an unchanged extraction of native `AnimatedMat_DrawColorLerp`. Only logging and the native color submission are stubbed. ASan and UBSan run with nonrecovering errors; LeakSanitizer is disabled because the managed host's tracing supervisor does not support it.

Run synthetic cases:

```sh
python tests/mm_texanim_validation/run_tests.py
```

Optionally pass a private native MM archive; all binary OTAN resources are loaded as valid controls, and every frame of every native type-3 clip runs through the actual native interpolation body:

```sh
python tests/mm_texanim_validation/run_tests.py --native /absolute/private/path/mm.o2r
```

No native resource bytes are stored in this test directory.

The malformed cases cover independent key/color array sizes, optional environment colors, zero periods, duplicate Lagrange keys, unordered linear keys, the native 50-key interpolation bound, texture-cycle indices, material types and table terminators. Every byte truncation of six valid controls and a completed-first/partial-second table are checked through the production reader and cleanup path.

Native linear interpolation can read index `keyFrameCount` when the frame reaches the final key. The importer appends a constant final-color interval only when that terminal frame is reachable and no extra interval was supplied. Tests exercise the last frame with ASan, preserve a supplied valid extra interval, reject incomplete/invalid extras and check the `65535` period/key boundary without overflow.

Factory table segments allow native ±1..8 (GPU segments8..15), and preserve the sole type-6/segment-zero native empty resource. The soul consumer's narrower supported material/segment contract is separately validated by its renderer tests.
