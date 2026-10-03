# Boss soul model routes — isolated PR33 candidate

Baseline: PR33 commit `69957d96eca1ddf1880c0f7df3c21acac2bc75e8`, tree
`07da4fa13dd39661e5d2d70bc69f26371f40bdd7`, local header-restoration
commit `fb8a166`. The user reports MM's native flame and TP Volvagia head coexist
in this baseline; preserve that result. Morpha showing a generic skull is user
testimony, not a screenshot inspected for this candidate.

The static OoT export previously selected the generic skull for every boss soul
that did not qualify for the skeleton export. Morpha is intentionally excluded
from that export because it has no skeleton, so it lost both of its actual core
display lists. This candidate describes Morpha's membrane and nucleus separately
and replays native `DrawMorpha` scale, X/Z rotations, segment 8/9 scrolls and
colors in MM. MM still draws its own flame. `SimplerBossSoulModels` still selects
the skull when enabled, and its selection stays live after acquisition latching.
No model archive or accepted Volvagia skeleton renderer was changed.

## OoT soul audit

Canonical paths below begin `__OTR__objects/`. When rendered in MM, model paths
are routed through `@oot:` and inner references resolve under OoT's resource
manager bracket. OoT's resource manager checks its own `alt/` path first when
Alt is enabled and falls back to its vanilla path when absent. This preserves
pack selection without inventing a replacement resource name.

| Soul | Native model / clip | MM foreign route |
| --- | --- | --- |
| Gohma | `object_goma/gGohmaSkel`, `gGohmaIdleCrouchedAnim` | Existing skeletal export |
| King Dodongo | `object_kingdodongo/object_kingdodongo_Skel_01B310`, `object_kingdodongo_Anim_00F0D8` | Existing skeletal export |
| Barinade | `object_bv/gBarinadeBodySkel`, `gBarinadeBodyAnim` frozen on its last frame | Generic skull remains; required frame-dependent limb rotations/scales and XLU post-limb surgery exceed the present ABI |
| Phantom Ganon | `object_gnd/gPhantomGanonSkel`, `gPhantomGanonNeutralAnim` | Existing skeletal export |
| Volvagia | `object_fd/gVolvagiaHeadSkel`, `gVolvagiaHeadEmergeAnim` | Existing skeletal export, preserved |
| Morpha | `object_mo/gMorphaCoreMembraneDL` and `gMorphaCoreNucleusDL`; no animation clip | New non-skeletal core route, native X=`frames*0.1`, Z=`frames*0.16` rotation |
| Bongo Bongo | `object_sst/gBongoLeftHandSkel`, `gBongoLeftHandIdleAnim` | Existing skeletal export |
| Twinrova | `object_tw/gTwinrovaKotakeSkel`, `gTwinrovaKotakeKoumeFlyAnim` | Existing skeletal export with head/ice-hair surgery |
| Ganon | `object_ganon2/gGanonSkel`, `gGanonGuardIdleAnim` | Existing skeletal export |

SoH's native drawers already advance these clips once per frame. Native Morpha
rotates its core and scrolls materials; it does not bounce vertically. Barinade
has deliberate clip freezing plus procedural limb movement. A static replacement
display list can inherit whole-model rotation or translation, but individual
limb motion requires a compatible skeleton, animation and weighted geometry.
The loaded-resource-type dispatcher already supports normal/flex replacements;
this candidate does not change it.

## Four native MM boss souls

Native MM dispatch already draws actual boss models and flames. Previously all
four foreign exports aliased their remains. The first three can reuse the
existing skeletal ABI with their exact native parameters and now do so.

| Soul | MM owner path / clip | Candidate foreign route |
| --- | --- | --- |
| Goht | `object_boss_hakugin/gGohtSkel`, `gGohtRunAnim`; segment 8=`gGohtMetalPlateWithCirclePatternTex` | Actual model, scale 0.005, Y=-20, green flame size 30 |
| Gyorg | `object_boss03/gGyorgSkel`, `gGyorgGentleSwimmingAnim` | Actual model, scale 0.05, Y=-20, blue flame size 3 |
| Odolwa | `object_boss01/gOdolwaSkel`, `gOdolwaReadyAnim` | Actual model, scale 0.005, Y=-20, purple flame size 25 |
| Twinmold | `object_boss02/gTwinmoldHeadSkel`, `gTwinmoldHeadFlyAnim`; skin on segment 8; separate 23-matrix allocation on segment 13 | Existing remains alias retained; no unsupported segment-13 equivalence claimed |

Foreign MM resources continue to load through MM's resource manager and its own
Alt selection. Existing skeleton cache keys include owner and live Alt selection.
Native MM behavior is unchanged. Majora is outside the requested four and retains
the existing foreign Twinmold-remains stand-in.

## Verification and limits

- The new executable fixture first failed the real production Morpha recipe's
  `dlistCount == 3` assertion (baseline returned the skull). After that route was
  implemented, the same fixture failed the production MM animated export gate
  for Goht. A separate red test exposed static souls' grant-latched CVar selection.
- `run_boss_soul_model_tests.py` passes normally and with ASan/UBSan: actual MM
  headers/GBI, production recipes and consumer, native MM flame, owner-routed
  core paths, exact scroll/rotation/colors, segment cleanup, simpler-model
  selection, appearance dependency, and three MM boss model exports.
- Existing `run_foreign_soul_skeleton_tests.py --sanitize` passes both hosts:
  native limb traversal, replacement flex/jaw matrices, owner/Alt cache changes,
  missing-owner/resource fallback and scope restoration. Resource I/O and
  animation initialization are fixture boundaries.
- Existing `run_mm_item_presentation_tests.py` and
  `run_mm_mask_bridge_tests.py` pass, including all 24 masks and four remains.
- Asset-collision check passes with no new collisions. `git diff --check` passes.
- LeakSanitizer's `/proc` inspection is unsupported in this environment; leak
  detection is disabled for sanitizer execution. No leak-proof claim is made.
- `clang-format-14` is unavailable here. Full application build and visual
  runtime verification remain pending.
- Two attempts to retrieve the supplied `zTPBosses.otr` failed with HTTP 502.
  Its exact replacement contents, skeletal compatibility and MM mappings remain
  unaudited. Canonical paths and owner Alt fallback are verified from source and
  fixtures; installed-pack appearance is not established by that evidence.

This is an implemented, command-test-verified candidate, not runtime acceptance
or master promotion. Decisive runtime check: under the same accepted TP pack/load
order compare Morpha and Volvagia in MM with Alt off/on, shelf/re-entry and
acquisition; confirm Morpha's selected core model/motion with the native flame,
and that Volvagia's head/jaw/flame remain intact. Compare foreign Goht/Gyorg/Odolwa
souls in SoH with their native MM presentations and owning MM Alt state.
