# Blast Mask and form combat POC

Baseline: marsh6487/ComboShip PR90 at 267fc64d7ac6b0f0d6c14a3f108b1a2e7299a02e. This checkout is a partial source snapshot used to prepare an applyable patch; it must never replace the full repository tree.

## Requested outcome

cor explicitly requires B-only Blast Mask detonation with no A activation, MM-style shield+B protection from the mask's own explosion, and investigation/repair of Zora and Goron melee requiring enemy overlap and causing contact damage. cor confirmed Master Sword equipped during the adult-Link MM failure. The child test's sword is unknown.

## Design and scope

1. MM: preserve the native B-action allowlist and extend only human Explode/March/Dance requests to exact supported imported and NEI sword identities. Keep native disabled statuses, cooldown, temporary B-item restrictions, and transformed-form behavior.
2. OoT: remove Blast ownership of A, route detonation and its prompt/lesson consistently to B, and protect only the mask explosion that is cast with an actually raised shield. Preserve unshielded self-damage, enemy damage, ordinary bombs, cooldown, interaction/cutscene safety and existing collider initialization. Match native MM observable activation rather than granting general player invulnerability.
3. Forms: compare native MM attack geometry, active frames, collision registration and damage handling with the OoT port before editing. Preserve animation resources, camera, movement, audio, damage values and unrelated form behavior. Correct the native Goron attack timing and authored fist transforms where they drive collision. Record any uncertainty about the user's exact game/configuration; do not claim runtime proof from source fixtures.

## Approaches

Selected implementation approach is a targeted compatibility and collision repair. A global B override would bypass native restrictions. A blanket player invulnerability timer would mask unrelated damage. Arbitrary hitbox inflation would not diagnose attack geometry or registration bugs. Those alternatives are excluded.

## Preservation and evidence

No asset archives, save formats, published branches, or accepted masters are changed by this POC. Original and COMBO_BUILD source paths must retain behavior. Each fix requires a failing behavioral fixture before code changes and a passing focused fixture afterward; preserve original/negative controls. Compile real production functions and collision helpers where practical. A full linked build and configured gameplay remain distinct gates. Deliver the exact source patch, tested baseline/candidate identities, recovery instructions, and a short runtime acceptance list.

## Implemented source candidate

### Blast Mask

MM's original B-action allowlist excluded the actual Master/Biggoron/extended sword item identities. The additional rule recognizes those exact sword identities only for human Explode, March and Dance commands. Native mask dispatch, item ownership hooks, button-disable checks, equipment payloads and cooldown behavior are retained. Knife and tools that borrow sword item actions are not added.

OoT captures B from the player input copy and leaves A available to native actions. The old A Explode producer is removed. The B drawing branch selects the existing Explode texture without writing A's transition segments. All three receipt languages and the pause description teach B and shielding.

The B request is consumed once and checked again after native actions have run, so entering talk/item/cutscene state drops the pending request. Native disabled B remains disabled. During a damage action, eligibility follows the normal native recovery action's final sixteen-frame interrupt window; remaining invulnerability after recovery alone does not block the mask.

Self-blast protection is confined to a newly spawned mask bomb: remove its `AT_TYPE_ENEMY` category only when a usable native shield is being raised or held. Native item-action compatibility, two-handed restrictions, the existing shield option's Deku Stick exception, child Hylian back guard and first-frame targeted guarding are respected. Enemy damage, ordinary bombs, player invincibility, bomb placement/radius and existing cooldowns are unchanged.

### Goron and Zora in OoT

Normal body radii change from Goron 24 and Zora 20 to native MM's 12. Special roll/pound dimensions remain separate. Goron now shares the native paired, bone-driven sweep mechanism already present for Zora: hands for the first two punches, waist for the butt attack. Authored native coordinates and attacking-fist scale curves are applied before the mesh and collision points are transformed.

Goron uses its native damage windows (6–8, 12–18, 8–14), one authored frame per gameplay update and no old eight-frame idle morph. The pre-update collision frame and post-update render/hand-scale frame remain distinct. Zora's already-correct authored reach is retained. A new form melee action keeps its bones during leftover invulnerability; actual damage still yields to the host damage action.

Existing successful-melee hit priority is retained. A missed attack still permits body-contact damage. No general melee invulnerability or arbitrary world-space range boost is introduced. No analogous native-MM melee source defect was confirmed in this pass.

## Verification and independent review

- MM mask-action source-function fixture: 1,373 checks in standalone and 1,373 in COMBO_BUILD, zero failures. Original source failed the imported-sword cases. Equipment-rewrite, hook-bypass and wrong-form mutation controls were detected.
- OoT Blast suite: native receipt formatter/decoder/font, A action selection, actual B graphics commands, input/context/status ownership, native shield/recovery functions and native bomb/collision/damage paths pass. Both ages and physical shield capability are covered. Each repaired behavior had an observed failing case before the fix.
- New form collision/matrix suite: original pinned source passes only the three existing Zora sweep controls (3/15); candidate passes 15/15.
- Existing OoT Zora suite: 10/10. Existing Goron field-collision suite: 9/9. Both contact-priority mutations are detected.
- ASan and UBSan run on the OoT/form native fixtures. Only the existing native C collision `shift-base` case is excluded because damage flags use signed `(1 << 31)`; production definitions are unchanged.
- Independent review found and closed missing native Goron fist scaling, same-frame talk/detonation, early hitstun activation, disabled-B activation and two-handed guarding without a usable shield. No remaining new production blocker was identified.

The recovery package records exact commits, file hashes, logs, pinned native dependencies, patch restoration and a replay of the five focused regression entries. Its source bundle is a partial inspection snapshot; apply the delta to a complete repository rather than replacing a full source tree.

## Remaining evidence gates

This is a source candidate, not a full application build or gameplay acceptance. Native functions and real native headers execute in CPU fixtures; external actor allocation, animation playback, rendered graphics and selected engine services are controlled boundaries. No runtime asset configuration was selected or modified. No ROM/archive is needed for these CPU checks; runtime must use the existing intended game assets.

The existing wall probe remains actor-relative. Host player update resets quad hit/bounce flags before later form wall checks consume them; that preexisting ordering issue and its DynaPoly grace remain. The timing gate now follows `hitStart + 1`, but fixture success does not prove native wall-recoil or breakable-object parity.

Runtime acceptance should cover:

1. OoT adult and child: A alone never detonates; B does; the prompt stays on B; ordinary A interactions still work.
2. Both shield entry styles and held R+B: usable shield prevents self-blast damage; no shield or an unusable two-handed guard does not grant protection; enemies still take blast damage.
3. MM adult with the reported Master Sword, then child with its equipped sword explicitly recorded: Blast shows and dispatches native B; normal cooldown and shield+B behavior remain native.
4. OoT Zora/Goron: each combo step at visible reach, while approaching enemies, after recovery from a hit, and on a deliberate miss. Check walls, hammer rocks, punchable DynaPoly and pound/grotto cases separately.

No remote branch publication, merge, asset rebuild or runtime promotion is part of this source-candidate handoff.
