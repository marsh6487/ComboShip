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

The shared Linux engine has compiled and linked. The combined default build is still in progress; it was safely resumed after an environment interruption and preserves the old build log. Asset collision checks report no new conflicts. Matching port archives are generated only after final source/build validation; a stale untracked interrupted AtlasTex temporary was quarantined outside the asset tree before packaging.

## Verification still required

Complete the final owner/snapshot/thread-save and immutable hash-resource checks, rebuild the frozen matching engine + both games, run the relevant native/CTest/regression gates, generate and verify both port archives, then hand off one identifiable candidate. The Kakariko pickup/bomb/pot/grotto/custom entrance/night/Keaton checks and OoT↔MM inventory/transition checks require configured game runtime. No comparative performance experiment is a prerequisite and no performance gain is claimed.

Known donor custom-dungeon inventory/restriction aliasing and narrow per-actor object-slot assumptions remain documented limitations for new custom content. Native camera/cutscene/path and other unrelated legacy limits were not expanded implicitly. Actual D3D11/Metal driver output, sky appearance and moving-receiver shadow appearance remain runtime checks. Existing custom Prism materials retain their previous shade inputs.

## Source ready for platform build checks

The immutable hash fix is complete: archive offsets remain unchanged, both Fast::Vertex and MM vertex arrays declare their serialized stride through the shared renderer interface, and bounds use actual runtime storage. It passed resource/owner retirement and reload tests plus actual hidden-visibility shared-library tests. The final Hammergeist custom-actor adapter retains float effect positions; its native sanitizer fixture and full configured unity source compile pass.

Standalone CI now injects the candidate's two graphics ABI definitions for all supported compiler entry points, including hardcoded compiler names. Intentional legacy/wide format tests select explicit mode. Native runner CLI tests cover argument/environment preservation and cleanup. This prevents CI from silently testing an older layout. The build is still running; source/native checks do not establish configured gameplay acceptance.
