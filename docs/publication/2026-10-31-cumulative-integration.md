# October 31 cumulative integration and restoration

Authorized: public source in `marsh6487/ComboShip`, integration/checkpoint branches.
No protected/default-branch merge or game release. All accepted asset masters and
unrelated scene archives remain unchanged.

## Exact inputs and preserved history

- Historical local checkpoint `f2ca8f249994a69caf5d13d47e4d020ee954888f` was
  published as `32680828cdd96fec1173d9cd95069857db12f149`, exact source tree
  `e0fe8178202834acdcca45db02acd48d38e39b09`. The public commit message records
  both the original local hash and exact source tree. Existing branch retained.
- PR #74: `e1b34337c9d2075dd2663b5983f928439942add2`.
- Morning local `2d27f023` maps to public `c043a85b26b8ce150c075bfb565119861fac7bc3`
  with the identical tree, recorded in the preserved NEI integration plan.
- Latest published morning PR #75: `1514491385626d2ac06b463eede4c4dadf4b299b`.
  Its complete tree equals local `ba37349b8`; both source histories are retained.
- Native partner/enemy candidate: `349a4188f2f9c6166d9d2eb4d6829f7e2513ba90`.
- Command Melody acquisition: `ed7a9e5cc30f67779cb659166823e7d4b8cef666`.
- Local merge `d10ae4a1b` has the acquisition and public morning heads as parents.
  Resolving the sole equipment-header include conflict retained the exact
  acquisition source tree.
- `18d6f46cf`: deterministic ROM-setup fixture repair, retaining all assertions.
- `fea690ac3`: clang-format 14.0.6 and required companion/encounter CI coverage.
- `d7d69cee1`: partner/enemy resources in normal generated archives.
- `9f02a718b`: bounded compressed text preserves all 403 shared resource payloads
  and original module identities; both packagers share the verified decoder.

`checkpoint/oct31-before-consolidation-20261031` preserves public `15144913`.
The new public integration retains that published ancestor. GitHub's commit
metadata may map local recovery commits to different public IDs; exact source
**trees** are checked at publication. Local original checkpoint commits travel
in the separately saved prepublication Git bundle, with required public base.
No earlier branch is force-updated or deleted.

## Result and resource ownership

Retains cumulative Unbound, cel, collision, RenderFlight, indexed backend, actor,
audio, chest, equipment, Market, Chamber and morning recovery/NEI/GI fixes.
Indexed rendering remains default-off with an explicit experimental checkbox.
Aligned native boss-soul dialogue receipt textures retain correct dimensions.

Adds native Malon/Cojiro, Command Melody control and four custom enemies with
optional randomization/natural encounters in both games. Cucui Ganon is not
included. Companion checkboxes and natural encounter placement default off.
Vanilla active saves learn Command Melody by default. Randomizer checkboxes do
not grant it free: generation supplies one normal song, and explicit starting
inventory consumes its pool copy. Black Market may roll it as a nonrenewable
wildcard, with physical ownership preventing repeat purchase. Existing frozen
placements/stock are not regenerated. Menus/What's New describe acquisition.

Normal `GenerateSohOtr`/`Generate2ShipOtr` targets merge verified resources before
normalization and copying: 403 shared authored blobs in SoH, and those plus 202
bounded OoT-native dependencies in MM. Sources reproduce the preserved candidate
packs byte-for-byte. No manual partner pack or user ROM is needed to build these
resources. Unrelated output bytes are preserved; differing collisions fail.
Donor manifests, licenses, exact resource hashes and conversion instructions are
tracked. Runtime authored replacements and scene exports were not edited.

## Verification

Published `15144913` already passed Windows/Linux builds, rendering, items,
equipment, entrances and world checks. Its systems failure was reproduced when
the ROM-setup classifier double treated random parent-directory text `mq` as a
ROM type. Test classification now uses filenames, and quest-named directories
exercise that boundary deterministically. No production extraction changes.
All ROM-setup tests, including MQ-layout seed/save checks, now pass locally.

- Partner/acquisition/title/packaging tests: 19 pass; encounter tests: 4 pass.
- CI routing/failure-handling/workflow tests: 25 pass.
- Stock generation: 4096 deterministic full-catalog inventories pass quotas,
  eligibility, pricing, validation and purchase checks.
- Native OoT shop dispatch/ownership passes address/undefined-behavior sanitizers.
- Actual MM generator passes NEI/quest/Plentiful/starting-song/solo/combined matrix;
  Seasons/computed-start and Grace pool regressions also pass.
- Modified native C++ translation units passed Solo/Combo syntax checks.
- Packaging hashes, preserved pack membership, idempotence, corruption/collision
  rejection and CMake merger/normalizer ordering pass for both games.
- Source preservation/recovery tree comparison precedes publication.

Fresh full platform builds and all six regression groups run on the published
integration. Record their actual outcome after they finish. No configured game
runtime, GPU/FPS performance, visual acceptance or user acceptance is claimed.
Windows build output is a CI artifact, not a game release.

## First CI follow-up

Published candidate `f9aa0b2a` passed source preflight, Windows compile preflight,
entrances and systems. Full builds continued independently. The world, equipment,
items and render groups exposed extracted-function fixtures missing the new
encounter/Cojiro boundaries and an old conditional-song pool expectation.

Repairs preserve the existing assertions and add typed disabled-feature boundary
checks, explicit Command Melody starting-item subtraction, and owned/unowned
Cojiro cell/title/alpha/toggle controls. Exact complete affected runners pass
locally, including camera/spawn, wide collision/enemy randomizer, shared-item
integration, OoT access, MM ownership/selectors and Cape receipt/icon suites.
Local checkpoints: `e76c7f0b6`, `0e9eb5651`, `a84a945a9`, `d96ed1fc9`, `cb4c9945f`.
Published fixture recovery: `1eda1324fb01a207dceb7babca4df0e105013188`.

The audit also found MM's owned Cojiro title requested an unbundled OoT resource,
which could fall back to a boots label in Solo MM. Local `74d525c3a` supplies a
128x16 IA4 title through the existing font pipeline and selects it without an
OoT owner. Tests verify native no-donor selection, exact source hash, IA4 bounds
and byte-identical regeneration. Published candidate recovery:
`0c89ac85d368a909c06224f6751ca55bbcbe6da0`.

Windows's full first build then reached MM archive normalization and failed:
CMake selected Python3.14.9, whose ZIP append writer clears the UTF8 flag in
existing ASCII central records while ZAPD's local headers retain that flag.
Local `c70f96938` preserves the original local region and raw central records,
and adjusts offsets only for separately generated additions. The unchanged
normalizer now passes the reproduced case. Tests retain compressed payloads,
forced ZIP64 local metadata, data descriptors, extras and comments byte-for-byte.
All 19 partner tests pass after repair. This first Windows run did not compile
the game engines; successful compile preflight is recorded separately.

Fresh full CI on the follow-up head remains required. These local checks and
successful earlier CI groups do not imply all groups or platform builds passed.

## Restoration

Restore the public integration by checking out its exact recorded commit, then
run the normal build targets. Pinned tracked source reconstructs packaged assets.
The prepublication recovery ZIP contains the local original-history Git bundle,
source delta from `15144913`, candidate/base tree IDs and provenance. In a clone
containing the public base, fetch the bundle to restore original local commits,
or apply its binary delta with `git apply --index` and compare `git write-tree`
against recovery.json before building. Roll back to the retained pre-consolidation
branch without rewriting other history.
