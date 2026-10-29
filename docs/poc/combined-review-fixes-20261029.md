# Combined candidate — review and recovery checkpoint

The candidate remains one cumulative integration. Accepted masters and source archives are unchanged. The preceding published milestone is `f443dc0def72d9a1edf1be539997d9f12f56f3c5`, tree `699d0b4bc5f6a848ab1161ba3b6bacf7ed42cb69` (local `a77da3d248da059ebc2b944eb15e53ca01c74149`). Restore the candidate branch and continue from its checked-in sources and this record.

An independent, read-only whole-branch review identified seven Important compatibility defects. No Critical issue was reported. This checkpoint retains the fixes and ongoing validation rather than implying runtime acceptance.

| Boundary | State at this checkpoint |
|---|---|
| XML shader and persistent shader-cache strings | Owned copies, safe invalid commands, native and ASan/UBSan teardown fixtures pass |
| Save states after external collision buffers grow | Authoritative external queue/geometry owners, transactional snapshots/rebind, room/scene epochs and extended flag snapshots implemented; production Save/Load lifecycle/OOM fixture passes; native lifecycle/OOM checks pass |
| Hash vertex archive offsets | Large v0/v1 offset fixtures pass; immutable commands, MM legacy-array provenance, real shared-library dispatch and owner/Alt replacement tests pass; shared model bounds now use archive 16/22-byte records versus runtime 24-byte storage and decode float matrices correctly |
| Legal collisionless JSON scenes | Owned valid empty context lifecycle implemented with snapshot ownership; repeated empty/normal setup-query-teardown fixture passes |
| Windows crash evidence | CrashHandler restored byte-for-byte to protected host (x64 frames, symbols, module/RVA, minidumps and AV details) |
| Threaded regular saves | Immutable SceneDB payload captured with game-thread SaveContext; delayed worker/sync payload and captured-quest metadata fixtures pass |
| Enemy Randomizer coordinates | Float spawn/adjustment/collider coordinates retained, seed quantization kept separate; native sanitizer and configured syntax checks pass |

Other verified deltas include a 4096-resource/4096-texture renderer capture budget with all retained rows emitted (1000-identity real logger test), explicit overflow/unknown information, complete actor/resource-stage tables, native narrow/wide geometry adapters and deterministic normalization of newly generated candidate port archives. GPU timing and unique decoded/GPU residency remain explicitly unavailable, not inferred from payload bytes.

The full matching Linux build and both Linux/Windows CI builds passed at earlier candidate checkpoints. Both generated port archives passed CRC, payload, shader/cloud and deterministic metadata validation. The final source adds tested custom-light and message fallback repairs, so its matching binary refresh and platform CI are recorded separately in the recovery record. Asset collision checks report no new conflicts.

## Verification still required

The owner/snapshot/thread-save and immutable hash-resource checks passed. All 259 manifest groups have passing retained CI/native coverage, with exact scope in `combined-native-coverage-20261029.json`. Refresh the final matching engine + both games, CTest/smokes, package and platform CI before handoff. Existing verified port archives can be reused when their payload identities are unchanged. Kakariko pickup/bomb/pot/grotto/custom entrance/night/Keaton checks and OoT↔MM inventory/transition checks require configured game runtime. No comparative performance experiment is a prerequisite and no performance gain is claimed.

Known donor custom-dungeon inventory/restriction aliasing and narrow per-actor object-slot assumptions remain documented limitations for new custom content. Native camera/cutscene/path and other unrelated legacy limits were not expanded implicitly. Actual D3D11/Metal driver output, sky appearance and moving-receiver shadow appearance remain runtime checks. Existing custom Prism materials retain their previous shade inputs.

## Source ready for platform build checks

The immutable hash fix is complete: archive offsets remain unchanged, both Fast::Vertex and MM vertex arrays declare their serialized stride through the shared renderer interface, and bounds use actual runtime storage. It passed resource/owner retirement and reload tests plus actual hidden-visibility shared-library tests. The final Hammergeist custom-actor adapter retains float effect positions; its native sanitizer fixture and full configured unity source compile pass.

Standalone CI injects the candidate's two graphics ABI definitions for all supported compiler entry points, including hardcoded compiler names. Intentional legacy/wide format tests select explicit mode. Native runner CLI tests cover argument/environment preservation and cleanup. This prevents CI from silently testing an older layout. Source/native checks do not establish configured gameplay acceptance.

## Final custom light boundary audit

The complete OoT custom light-producer sweep found ten calls still narrowing world XYZ to s16 before the imported f32 API. Only those 30 position casts were removed across seven source files; colors, radii, authored offsets, control flow and MM's native s16 interface are unchanged. Parameterized actual production call expressions fail 30/30 old-coordinate controls and pass all 30 corrected controls with ASan/UBSan. Full Zora insertion/orbit functions and all six original presentation/audio cases also pass; the corrected C/f32 mock prevents a stale C++ overload from hiding interface drift. The matching binary/package refresh is recorded separately from this native proof.

## Legacy message fallback repair

The native world tests reproduced a null-segment copy when an unregistered legacy text table missed the new Unbound hash lookup. Normal, Japanese and credits readers now retain owned-table hash hits and restore sentinel scanning/raw pointer and message-size semantics for legacy tables. Invalid or empty lookups select stable native end streams; JPN uses aligned native u16 `MESSAGE_END_JPN`, not the byte terminator. Real production Find/OpenText bodies and actual MessageTable lookup pass legacy/owned hit, miss, null, sentinel, empty-table, PAL and story-precedence controls in solo and Combo under ASan/UBSan. The complete C translation unit passes syntax verification. Independent narrow review found no defects or material coverage gaps; Japanese glyph drawing is explicitly outside the native fixture's proof.
