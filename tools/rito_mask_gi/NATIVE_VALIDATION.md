# Rito native resources and packaging validation

The authored Rito mask resources pass actual native libultraship factories and the available native OoT archive packer. This extends the deterministic topology/material/resource checks in `README.md`; it is not configured gameplay or shimmer acceptance.

## Recorded execution — 2026-11-09

The controller compiled `native_resource_probe.cpp` with the current candidate headers against the retained native libultraship shared library. An initial probe loaded all 16 authored GI resources across the two hosts, including both N64 matrices. The packaging worker then ran the actual retained `soh-o2r-packer` on fresh Rito-only inputs for **each** host and passed the resulting extracted archive resources to that same native-factory probe.

| Check | Result |
|---|---|
| Source equivalence | All22 relevant factory/type/loader/packer source files are byte-identical to the retained library/packer build-source trees; full SHA256 records saved |
| OoT native packer | PASS; eight GI resources preserved exactly, plus converted icon/name;11 archive members including `portVersion` |
| MM-owned inputs through available packer | PASS; same10 resource payloads and dependency checks; Torch copy-through/archive proof only |
| Dependency closure | PASS; all86 GI resource path references resolve within each owner's included eight resources |
| Geometry |768 triangles /2,304 serialized vertex records unchanged by packing |
| Icon conversion | Native v0 RGBA32,32×32,4,096 texel bytes; every packed pixel equals original PNG RGBA |
| Name conversion | Native v0 IA4,128×16,1,024 texel bytes; packed nibbles exactly match native packer's3-bit-intensity/1-bit-alpha quantization |
| Actual factory execution on packed outputs | PASS;20 resources loaded across both hosts, including GI, matrices, converted icons and converted names |
| Archive CRC | PASS both archives |

**Full MM ZAPD was not executed.** Running the OoT/Torch packer against MM-owned source proves archive copy-through and supported PNG conversion for these assets. It is not a substitute for a complete native MM asset build. No original accepted archive or previous native-validation directory was modified.

Factory source SHA256 values, identical between candidate and retained build-source tree:

| Source | SHA256 |
|---|---|
| `DisplayListFactory.cpp` |`e84c43d61fbd116fa2ea6437cc2d94883a072a5a8edaa59416a3d3e3b762959a`|
| `VertexFactory.cpp` |`25dee1cf374844a107969346359a127086b7ee0e0520b92e98cc6c223a60d900`|
| `TextureFactory.cpp` |`1caf3266a10456d5dd1e8d1da7cad1c541579c796015e4294d7fe3f1ae6e2c91`|
| `MatrixFactory.cpp` |`9d2b10c5d4984b83076afe0cefab166860bdfb9b87391e0f1a47d128c7ec0663`|

The serialized icon hash after conversion is `e91d270ad2ed3ce3ab95a5b3a0050628c62c2aa8e9a3d1506a1d8527f520f115`; serialized name hash is `1d0580b2937fa8b346a3d4e5795124ac42cf1323c56ff282fbb5d3a96dedc220` in both hosts. Packer ZIP metadata can vary; these payload identities are the comparison criterion.

## Reproduce

The validation recipe accepts external tool/build paths so it does not require a compiler or prebuilt library at hardcoded locations. It writes fresh, isolated Rito-only stage directories, output archives, extracted archive payloads, command/log files, source-equivalence hashes and JSON results inside its designated output directory. It never writes into either retained tool-source tree or accepted archive.

The recorded invocation, from the repository root:

```sh
python tools/rito_mask_gi/validate_native_packaging.py \
  --output /workspace/scratch/8877f7acb2e3/rito-native-resources \
  --packer /workspace/scratch/763b161ed7b8/door_key_native_validation/soh-o2r-packer \
  --runtime-libs /workspace/scratch/763b161ed7b8/door_key_native_validation/runtime-libs \
  --library /workspace/scratch/c690840d9328/native-local-build-r20-full/libultraship/src/libultraship.so \
  --library-source /workspace/scratch/c690840d9328/native_local_build_r20 \
  --packer-source /workspace/scratch/763b161ed7b8/door-key-gi \
  --probe /workspace/scratch/8877f7acb2e3/rito-native-resources/resource-probe
```

`source-equivalence.json` records all22 matched source hashes plus actual native library, packer and probe binary hashes. `packaging_results.json` records per-host payload dimensions/hashes, counts, archive hashes and explicit pipeline limits. `packed-resource-probe.command.json` and `.log` record the actual20-resource native-factory execution. The controller's `build-command.json` and `build.log` preserve the probe compilation.

This proves serialization, archive preservation, PNG conversion and native loader acceptance for these resources. Actual rendering/culling under game lighting, inherited render state, shimmer animation, receipt framing, spin, Alt Assets parity and user acceptance remain separate runtime checks. No GIF, video or animated preview is produced by this recipe.
