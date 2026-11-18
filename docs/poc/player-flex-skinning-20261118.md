# Optional weighted player Flex meshes

This candidate adds engine support in both hosts. It does not convert Din, any
seasonal outfit, existing PAK/ZOBJ model, equipment or animation archive.
Gameplay appearance and model-specific acceptance remain untested.

The ordinary player skeleton, joint table, animation, collision and limb
callbacks remain native. A model can opt in with an archive-owned palette and
weighted body-mesh companions. Missing or invalid metadata retains its ordinary
rigid meshes. Equipment, hidden limbs and first-person substitutions retain
their existing overrides.

## Resource contract

For a resource skeleton `objects/seasonal_outfits/example/adult/summer/gLinkAdultSkel`, provide:

| Resource | Meaning |
|---|---|
| `objects/seasonal_outfits/example/adult/summer/gLinkAdultSkel/Skin/Palette` | NS/v1 U16 array described below |
| `objects/seasonal_outfits/example/adult/summer/gLinkAdultSkel/Skin/Limbs/Limb00` | Optional weighted replacement for skeleton limb 0 |
| `objects/seasonal_outfits/example/adult/summer/gLinkAdultSkel/Skin/Limbs/LimbNN` | Other optional weighted body limbs |

Keep valid rigid fallback display lists in the original skeleton. The renderer
selects weighted companions only with valid metadata, sufficient matrix space,
and an unchanged native body-mesh selection. A missing individual companion
uses that limb's rigid list. This prevents missing/malformed palette data from
enabling a display list that addresses unallocated matrix slots.

The format matches the existing OoT NPC skin exporter
`scripts/npc_models/native_npc_skin.py`: scalar U16 words `[0x4e53, 1,
nativeMatrixCount, extraMatrixCount]`, followed by 12 little-endian IEEE float
words per native matrix (each float as low/high U16), then extra entries
`[firstSlot, secondSlot, secondWeight]`. Inverse binds are stored row-major 3×4;
the last row is `[0,0,0,1]`. Slots refer to the skeleton's drawable Flex matrix
order, not its limb index. Counts must match its `dListCount`, native count is
1–32, combined count is at most 256, `firstSlot < secondSlot < nativeCount`, and
`secondWeight` is 1–15 out of 16. Inverses must be finite and nonsingular.

Extra matrix `nativeMatrixCount+i` is
`(1-w) * current[first] * inverseBind[first] +
w * current[second] * inverseBind[second]`. Weighted vertices use model bind
space and address their extra matrix through segment 0x0D. Rigid fallback
vertices retain ordinary limb-local positions. A future model export must
retain the source weights, inverse binds, normals/materials and drawable slot
mapping. The format supports two influences per blend, quantized to sixteenths;
arbitrary FBX weights are not automatically imported by the runtime.

For private PAK-selected `.o2r` bodies, the loader supplies the actual selected
skeleton source path. The resolver validates the loaded skeleton's identity,
LOD limb type and matrix count against the pointer being drawn. Raw `.pak` and
`.zobj` bodies do not supply this opt-in metadata. Ordinary player aliases may
also supply companions; lookup follows the owning host's live mounted archives
and Alt state. Cache identity includes source path, rig, palette and Alt state.
This avoids borrowing another model's palette after a draw-time swap.

## Safety and preservation

Both hosts use `SkelAnime_DrawFlexLodWithXlu` for player rendering. Segment-D
bindings are emitted to both streams. Original limb traversal, post/override
callbacks and matrix state are retained. Native matrices are initialized before
optional blending so a skipped override cannot expose uninitialized memory.
Nested draw scopes restore their active plan and queued-command records.

Additional matrices are bounded before allocation. Before native packing, each
blended matrix must be finite and within the legacy 16.16 range. If packing
would be unsafe, deferred weighted-body command slots are rewritten to the
original rigid companions for that frame. Custom equipment/hidden overrides are
never included in that rollback. Existing OoT NPC weighted palettes retain
their original resolver and draw behavior, and cannot overlap player palettes.

## Verification

`python3 tests/player_flex_skin/run_tests.py --sanitize` compiles both real host
bridge translation units, executes both native FlexLOD implementations and the
production resolver/blend policy with actual host resource/GBI types, and
checks missing/invalid sidecars, inverse binds, a known two-bone bend, source
identity, live Alt, selected private/raw PAK queries, matrix/command bounds,
equipment/hidden preservation, native pose/callback preservation, packing
rollback and nested scopes. External resource-manager/graphics services use
fixtures. `--fast-math` exercises the bitwise invalid-float guards under engine
optimization assumptions. Native PAK layout tests remain separate.

This establishes an opt-in engine capability. A weighted Din archive, full game
build, configured gameplay, pause/first-person/equipment combinations and
season/model switching still need runtime tests before promotion.
