# Combined cutscene/NPC adapter candidate

Parent: repaired Unbound-integrated ComboShip `3cbb22d1f09e85deb206d2b085fc62105bc09fad`.
Candidate branch: `candidate/native-cutscene-vertices-20261029`.

All source fixes below belong to one candidate. The repaired parent remains the user's background smoke-test baseline. No release or protected-branch merge is part of this work.

## Changes

1. Shared strict MM graph: convert per-resource 16/22-byte archive offsets to the actual 16/24-byte runtime vertex stride. This covers native Anju, both umbrella poses, Skull Kid, their supported authored graphs, and every other consumer of this graph loader.
2. Shared skeleton replacement: synchronize cached display-list/matrix count in both successful skeleton-swap paths. A same-joint authored/native switch could otherwise allocate the old number of matrices and traverse the new hierarchy. Joint count and joint/morph buffers are unchanged.
3. Authored Saria, Twins and Young Fado: ordinary and static tracking callbacks use the fitted head joint's full XYZ pivot. Native models keep the exact previous 900/1200 pivots. Existing Young/Adult Malon behavior is preserved.
4. Existing archive diagnostic: update its ctypes resolver to the new resource-view contract; its explicit legacy 16-byte fixture remains supported.
5. Matching optional Twins pack: repair 545 private resource hashes in 164 display lists. This asset correction is independent of the executable; the full model is not bundled into the source or game build.

## Requested adapters

| Adapter | Source/asset audit and candidate coverage |
|---|---|
| Saria | NativeNpcV1 ordinary/static tracking corrected; Demo_Sa has no corresponding additive pivot. Current Anime pack's vertex references, rig pairing and palette bounds pass. |
| Young Malon | Existing authored pivot, eye/mouth, thigh and pose routing retained. Current wrist/singing pack passes archive checks. |
| Twins | Ordinary/static tracking corrected; credits route retained. Accepted POC9 private hashes repaired in matching optional pack. |
| Adult Malon | Existing authored pivot, basket, pose and native routing retained. Current basket pack passes archive checks. |
| Young Fado | Ordinary/static authored tracking corrected. Rigid/seated pack has valid rig and matrix ranges. Older base-only skeleton presence semantics remain unchanged. |
| Skull Kid | Atomic native/private graph uses shared stride correction; native 21 limbs/20 matrices and valid animation bounds. |
| Adult Zelda | Shared count synchronization prevents the reproduced live-Alt matrix overrun; seven facial origins match native En_Zl3. |
| Treasure Shop Gal | Optional MMD head/eye/clip fallback source audited; native 23 limbs/16 matrices and two native clips structurally valid. |
| Anju | Native/private hierarchy and both umbrella clips retained. Native 20 limbs/19 matrices; both supplied R9 authored graphs pass selector/patcher readback. |
| Mikau | Ordinary rig, reserved matrix, prop at limb17 and opaque segment-C binding retained; native 22 limbs/20 allocated matrices, 19 drawable. |
| Lulu | Ordinary rig and explicit Alt blink/singing-mouth routing retained; native 22 limbs/21 matrices and four clips structurally valid. |

## Verification

- Shared graph regression failed before the fix on second-vertex selection; corrected unit tests pass ASan/UBSan, including mixed archive formats, boundaries, malformed metadata and overflow.
- Native archive audit: 1,590 vertex packets, 513 lists, 14 catalogued object families; 1,118 nonzero offsets. Exact runtime elements selected. This is packet validation, not recursive renderer execution.
- Both supplied Anju R9 graphs: 8,012 vertex packets, including 7,860 nonzero offsets, pass exact production selector/validator/patcher readback.
- Zelda/skeleton test: baseline fails five of seven cases, with three ASan overruns. Candidate passes both directions and both swap paths, missing resources and fallback, preserving joint/morph storage.
- Tracking test: exact ordinary/static callback bodies and real presence predicates; baseline fails 52 of 1,920 checks, candidate passes 1,920. Native/Alt-off controls and boy isolation are retained.
- Five current authored NPC packs: 35,493 vertex loads pass alignment/cache/bounds checks; paired rigs retain hierarchy and matrix-slot order. Twins private-hash failures are separately repaired.
- Legacy archive diagnostic half: 151 actual Skull Kid vertex loads across 40 lists pass with the new callback view. This local extraction does not execute the preceding viewer fixture.
- New tests are registered in the existing render regression group. Full platform compilation and in-game visual acceptance remain separate checks.

```bash
python3 -B tests/native_mm_actor_vertices/run_tests.py --sanitize
python3 -B tests/native_mm_actor_vertices/alt_matrix_test.py --sanitize
python3 -B tests/native_mm_actor_vertices/saria_tracking_test.py --sanitize
python3 -B tests/native_mm_actor_vertices/audit_archive.py /path/to/owned/mm.o2r
```

The tests extract production callbacks, traversal and swap methods into bounded native fixtures. They do not substitute for a full engine build or a configured game test. Compatible joint topology remains required for skeleton replacement; this change does not resize animation buffers. Authored pack readback is not visual acceptance, and partial-archive callback controls do not establish a complete partial-pack skeleton contract for Fado.

## Twins asset identity and recovery

Original accepted `Kokiri_Twins_Render_Native_NPC_POC9.o2r`: SHA-256 `b8c8bd1c80332d625fad152f8bbcadf78124c75086cea0cca6ba5da84d91ae02`.

Matching `Kokiri_Twins_Render_Reference_Repair_Candidate_20261029.o2r`: SHA-256 `2723332dce6ee36c5049236d8dc7ddd8613eaf2f31bd97b975623b5c3b495164`.

Reproduce with the existing parent helper:

```bash
python3 -B scripts/npc_models/repair_saria_resource_hashes.py original.o2r candidate.o2r
```

ZIP integrity, resource names/lengths, all geometry, textures, rigs, animations and non-display-list bytes match. Only confirmed hash payloads change; a second repair pass changes zero. The repaired pack has no unresolved private hashes. Original inputs remain unchanged.

## Next runtime checks

Inspect both native Anju poses with no replacement pack, authored Saria/Twins/Fado while looking around, and Adult Zelda through Alt on/off and re-entry. Sample the remaining native/authored adapter routes in the same candidate. Record each result independently; do not promote based only on compilation.

User confirmed Lost Woods loads correctly once the scene mod folder is inside `soh`. No blanket scene incompatibility or conversion requirement is inferred. Performance diagnosis is excluded.
