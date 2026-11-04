# OoT boss soul flame versus TP bottle Alt override

Baseline: `2e28b239cb4aa0f7f31da683cd15b1bcc8874ac1`.
Candidate: `candidate/graveyard-soul-repair-20261104`.

The user reports the OoT Gohma soul flame visible with vanilla assets and
absent with Alt Assets. The supplied clip corroborates an absent flame, but
the supplied log does not enumerate the winning flame resources. This is a
confirmed incompatible override in the saved TP pack; its priority in the
user's complete stack is still unrecorded.

## Concrete resource evidence

Recovered unchanged `zTPMiscObjects(1).otr` contains
`alt/objects/object_gi_fire/gGiBlueFireFlameDL`. This root calls
`objects/object_gi_fire/mat_gGiBlueFireFlameDL_ItemInvis`, whose primitive
color alpha is explicitly zero. Its first combiner cycle alpha is 1; the
second multiplies combined alpha by primitive alpha, producing exactly zero.
Its quad is horizontal. The TP chamberstick root already draws its own flame
alongside the bottle shell. The separately called native blue-fire flame
root is therefore an intentionally invisible placeholder for this bottle.

Before this change, `Randomizer_DrawBossSoul`, the simpler soul recipe and
the animated soul recipe reused this shared bottle flame root. OoT's resource
manager selects `alt/` first when enabled. The bottle override can therefore
make every OoT boss soul flame invisible independently of the boss model.
Changing only the two POC5 flame textures cannot repair that root/material.
The exact TP root and zero-alpha material are retained as regression fixtures.
No saved TP archive was changed.

| Source | SHA-256 |
| --- | --- |
| `zTPMiscObjects(1).otr` | `31eed67c4255fe389fb5dc39904af3a0b2773f59ad9e8690518a552bb282955f` |
| `zTPMiscObjectsTextures(1).otr` (no object_gi_fire entries) | `d9e325eb7f01823cb0cf1e8f4f141c5d9a1a204239bef249a735390f7ec1507c` |
| Native `oot.o2r` reference | `ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b` |
| Retained TP flame-root fixture | `a8784294c24cf4df1f0fb36dd18cdd1ee622e841b40049641a709f6791afbe4a` |
| Retained TP zero-alpha material fixture | `3d6f4b5e1884f2d734991646c1222892795f8d09a950844c4506821e142ad773` |

## Bounded repair and reproducibility

Ship `objects/object_boss_soul/gBossSoulFlameDL` and
`objects/object_boss_soul/gBossSoulFlameVtx` inside `soh.o2r`. Native OoT soul
drawing and both exported OoT soul recipes select this private root. MM-host
foreign soul rendering must retain its established MM-native flame dispatch
when receiving either the legacy or new OoT flame identity.

The private display list is a copy of
`tools/boss_soul_flames/native/OoT-flame-DL`, with only its vertex-resource
CRC changed from `objects/object_gi_fire/object_gi_fireVtx_000C20` to
`objects/object_boss_soul/gBossSoulFlameVtx`. Both CRC words are serialized
little endian (high word followed by low word), using the repository's
`tools/temple_sages/resource_geometry.py::crc64` implementation. The original
native reference display list exactly matches the retained POC5 native list.

The private vertex resource copies that native reference's
`objects/object_gi_fire/object_gi_fireVtx_000C20` unchanged. Its header is
64 bytes, binary type `OARR`, version 0; payload starts with array type 25,
count 4, followed by four 16-byte little-endian vertices. The regression
check records every vertex field, allowing recovery without the full archive.

| Shipped resource | SHA-256 |
| --- | --- |
| `gBossSoulFlameDL` | `f44397831c1fa71acefacfac35368932bc641459e9518816d01e8dd1fcf6ee18` |
| `gBossSoulFlameVtx` | `bc29c47727dccf7436e4f5eeb5f59ee762892083a1b403629113ded994e5b4fc` |

Native geometry, material commands, primitive alpha 170, LOD 128, segment-8
scroll call, transforms, tint and standard OoT flame texture identities remain
unchanged. Standard texture identities intentionally remain shared: installed
POC5 Alt artwork still applies. The TP bottle shell and its original shared
flame roots remain unchanged. No boss model, dialogue, progression or MM
texture is changed by this repair.

The current `soh-o2r-packer` copies binary custom assets unchanged; the two
new resource paths sit beneath its already-recursive input directory.
Deploy the rebuilt `soh.o2r` together with the matching executable/DLLs.

## Verification and boundary

- `python3 -B tests/boss_soul_flame/check_dedicated_root.py`: RED before the
  resources/routing change; GREEN afterward. Checks exact material/scroll/
  texture/quad preservation, TP zero-alpha negative control, and all three
  native/exported routing changes.
- `python3 -B scripts/diagnostics/run_boss_soul_model_tests.py`: PASS on the
  shared candidate tree (production Morpha/export/MM-native flame fixture).
- Regenerated POC5 archives reproduce their documented hashes. The existing
  headless OpenGL checker passes on llvmpipe at 256x512 for all three filter
  modes: 1,024 OoT phases and 256 MM phases, zero escaped visible pixels.
  This validates retained texture behavior, not a rendered in-game pickup.
- Owned source paths pass `git diff --check`.

Configured gameplay, Windows GPU execution, complete build and acceptance
remain separate checks. The decisive runtime comparison uses the user's
unchanged pack stack: Gohma pickup with Alt off/on, the same shelf/simple
preview, MM-host OoT soul flame, and a TP blue-fire/fairy bottle to confirm
its shell remains intact. Master promotion is not implied.
