# Rejected NPC model archive repair checkpoint

All model candidates remain unaccepted. No master promotion or configured game runtime proof is implied.

The game-source rebuild checkpoint is `ee83e2613ebcd0cb029869fc6dce90f7a09fdd75` on `poc/npc-runtime-review-20261020`, tree `f1a5ab6db1c8fcc2dbeff596fc5e37c72e92583f`. That branch is frozen for the user's bug-fix rebuild. It adds optional uniform DrawScale and size-checked V2 placed-Saria ABI while preserving legacy ABI sizes; rebuild both OoT and MM modules. The requested 1.10 values live in the replacement young-Malon and anime-Saria archives. These archive-tool changes do not require further game-source updates.

## Verified archive repairs

- Twins: six 256x128 shoulder bakes did not cover their native 512x512 render tile. Exact 2x4 pixel duplication fixes the repeated bands; every mesh/UV/normal/rig/pose byte stays exact. One active shoulder bake increases from 128 KiB to 1 MiB, a memory tradeoff documented for runtime review.
- Young Fado: the preceding checkpoint contains its connected shoulder-cap refit and existing-contract skin-resource recipe. Original binary resources remain exact; all static clips and laughter were checked offline. Residual underarm intersections remain in some poses. No acceptance claim.
- Adult Malon and anime Saria: bounded common skin normal fields reduce polygon-scale forehead/cheek lighting variation. Only normal XYZ bytes in 17 head/expression buffers per model change. Saria's authored eye cleanup uses a connected aperture mask; rejected POC2 had a cheek seam, so only POC3 is eligible for integration.
- Anime Saria: native-reference blue hue changes iris texels in Diffuse and EyesHalf only. Pupils, catchlights, shapes, alpha, non-iris skin/hair and closed blink remain exact. Traditional Saria's existing blue eyes are audited and preserved.
- Anime Saria hands: bounded topology-local normal smoothing preserves geometry, grip, UVs, weight ownership and wrist-rim normals. Broad shape and baked crease artwork remain; a separate authored art probe is still pending.
- Both Saria ocarinas: rejected packed RGBA16/RAW pixels were interpreted by the raw RGBA32 importer. Repack the same decoded colors as raw RGBA32, horizontal byte scale16 and vertical pixel scale8. The original 32x16 RGBA16 tile then uploads the intended 256x128 image. Geometry, UVs, grip and display lists stay exact. Do not stack an older prop overlay over the repaired full model.
- Lossless rebatching from the prior checkpoint reduces redundant matrix/vertex commands; triangle corners retain matrix ownership and every 16-byte vertex record. No NPCs or triangles are removed. Actual FPS and TP-young-Epona cost are unmeasured.

## Evidence and remaining work

The production raw-upload fixture reproduces the old 128x128 color mismatch and passes the repaired 256x128 decoded colors under ASan/UBSan. Python unit and exported-resource tests cover masks, palette preservation, shoulder sampling, hand normal boundaries and batching. An independent review found file-alias overwrite hazards; shared path/inode guards now reject protected destinations before parsing or writing. Actual synthetic hardlink corruption and report aliases were demonstrated red, then green after correction.

New blouse/hand bitmap probes are isolated, optional and not part of this checkpoint. Root owns candidate assembly, protected-resource readback, literal-reference/load-order checks, durable recovery publication and the final handoff. Full executable build and configured in-game evaluation remain pending. Existing parent archives and native `oot.o2r` remain read-only.
