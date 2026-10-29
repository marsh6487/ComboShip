# Native MM cutscene actor vertex candidate

Parent: `candidate/cel-unbound-smoke-repair-20261029` at `3cbb22d1f09e85deb206d2b085fc62105bc09fad` in `marsh6487/ComboShip`.

Scope: repair native cutscene actor vertex addressing in the shared strict MM display-list graph. Both Anju umbrella poses use that graph, as do Skull Kid and other strict graph consumers. Other ordinary MM/OoT cutscene actors use the regular hash-vertex handler, which already performs the archive/runtime stride conversion in this parent.

## Confirmed source defect and correction

Legacy MM vertex records occupy 16 bytes in the archive. The shared Unbound runtime vertex occupies 24 bytes. The strict graph added archive byte offsets directly to runtime pointers and used 16-byte divisors against runtime storage bounds. Nonzero offsets therefore selected incorrect elements or the middle of an expanded vertex.

The graph resolver now provides each resource's actual serialized record size and runtime stride. The patcher derives an element index from the archive offset, checks it against the runtime element count, and computes the runtime address. Mixed legacy 16-byte and wide 22-byte resources remain distinct. Malformed metadata, misalignment, overruns, and pointer addition overflow are rejected.

No model, texture, skeleton, animation, actor placement, actor behavior, collision queue, scene, or message-publication resources were changed. No replacement pack is required.

## Evidence

- The production patcher test failed before the correction: the second native vertex resolved to base + 16 instead of base + 24.
- The corrected patcher and expanded legacy/wide/mixed/bounds cases pass with AddressSanitizer and UndefinedBehaviorSanitizer.
- `audit_archive.py` checked the actual native archive `mm.o2r`, SHA-256 `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59`. It selected the exact runtime element for 1,590 vertex packets from 513 display lists across 14 catalogued object families. 1,118 packets had nonzero offsets. The audit examines vertex addressing; it does not execute materials or render the game.
- The native crying and standing Anju clips retain 43 and 32 frames respectively.
- The supplied videos establish the original native Anju rendering failure in the user's regular build. They do not show this candidate running. The confirmed wide-stride defect in this parent does not by itself prove every cause of the older build's visual failure.
- Existing wide vertex/DSO and full platform regressions are registered for CI; the local narrow snapshot lacks the complete engine build dependencies.

## Verification commands

```bash
python3 -B tests/native_mm_actor_vertices/run_tests.py --sanitize
python3 -B tests/native_mm_actor_vertices/audit_archive.py /path/to/owned/mm.o2r
```

## Runtime acceptance still pending

With no Anju replacement pack, inspect native `En_Viewer 0x7E0D` (crying umbrella) and `0x7E1D` (standing umbrella), including camera rotation, scene exit/re-entry, and pause/unpause. Sample the other native MM cutscene actors, especially Skull Kid, plus an ordinary OoT actor. Platform compilation and an archive audit are separate from visual acceptance.

## Recovery

The parent branch remains unchanged. Fetch the exact parent and candidate commits to recover or discard the bounded source diff. No ROM-derived geometry or textures are included in the source candidate. Performance diagnosis is outside this task.
