# Selected sword blade auras

`scripts/assets/build_elemental_sword_aura.py` generates the private, built-in
GBI/vertices and four logical 64×32 I8 maps in
`combo/render/sword_element_aura_data.h`. The generated file disables formatting
inside its data region so clang-format 14 does not invalidate `--check`.

The Fire maps are LANCZOS reductions of the exact I8 red channel in the accepted
Din progressive archive. Its artwork is not regenerated. Fire keeps the existing
`build_din_fire_weapons.sword_meshes` geometry recipe and two-cycle material,
including its opaque core, translucent flame and 3/5 scroll. Ice and Light use
distinct procedural I8 maps and a tighter, translucent cross-section envelope;
the original sword metal remains visible. No new weapon model, magic cost,
particle actor or room effect is introduced.

## Reproduction and provenance

Normal regeneration needs only this directory and the checked-in authored NEI
sword resources. It does not need an external equipment archive:

```sh
python3 -B scripts/assets/build_elemental_sword_aura.py
python3 -B scripts/assets/build_elemental_sword_aura.py --check
```

The retained `*.i8` maps, `din_layers.json.zlib`, accepted equipment alias
commands, and native clipped triangle records are pinned by SHA-256 in the JSON
source manifests. The manifests also identify every source DL/vertex dependency,
blade clipping plane and authored held matrix. Original input archives remain
read-only:

| Input | SHA-256 |
| --- | --- |
| Accepted Din progressive candidate | `5f7972b03e494aa3c309860dc2e91d85ab3bbe18c61244c06d063f8205e27306` |
| Native OoT archive | `ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b` |
| Native MM archive | `f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59` |

Optional `--import-fire`, `--import-oot` and `--import-mm` arguments recreate the
small retained inputs from those exact archives; each import rejects another
archive hash. Native extraction recursively follows actual hashed DL and vertex
commands, clips out the hand/grip, and rejects an unexpected matrix instead of
inventing a transform. Ordinary source regeneration does not read the archives.

## Final blade identity

The shipped NEI swords have independent adult OoT, child OoT and MM human fits,
including their progressive variants and a clipped broken Knife. Native OoT/MM
fallbacks retain their actual shorter blade geometry. The accepted Din pack's
four fits retain its exact protected core/flame vertex records.

The successful held-sword branch registers its model and exact DL pointer.
Native compositors transfer that identity when copying their staging list.
CustomEquipment identifies the accepted Din aliases from their actual hashed
blade target, including a retained broken blade after a Great Fairy upgrade.
The post-limb hook supplies the final DL, so a later equipment/PAK winner cannot
inherit an earlier fit merely because the pack is present. Explicit selections
suppress the old Din hand replacement and preserve the final weapon model.

The captured hand belongs to the local player and current scene, rig, frame and
selection. It is consumed once after the body. Hidden hands, reflections, foreign
players, excluded weapon owners and transformed forms do not receive an aura.
Pause freezes its existing phase. Selected auras depend on the real selector's
element checkbox and corresponding arrow ownership, with no Din/Alt-assets gate.
Unselected legacy Din visuals keep their existing resources and behavior.

Arbitrary third-party sword replacements keep the logical sword's fallback aura
fit; their blade geometry is not inferred. A configured in-game visual pass is
still required for final appearance, blending, animation and user acceptance.

## Verification

```sh
ASAN_OPTIONS=detect_leaks=0 python3 -B scripts/diagnostics/run_elemental_sword_aura_tests.py --sanitize
python3 -B scripts/diagnostics/preview_elemental_sword_auras.py /tmp/aura-offline-comparison.png
```

The aura runner executes actual GBI and vertices, the real held selector,
CustomEquipment compositor, imported native hand branches and selection core.
It covers all three elements, native/progressive/Din fits, assets off/Alt off,
pack-on hand preservation, arrow/checkbox gates, explicit None, exact hand-list
transfer, later replacement, body colors, captured matrices, pause, scene/frame
changes, ownership and duplicate submission. It also compiles the complete
renderer TU for both engines in solo and Combo modes.

The optional preview projects the exact generated vertices and I8 maps over a
neutral blade reference. It is an offline diagnostic, not an in-game screenshot
or evidence of a configured runtime visual pass.
