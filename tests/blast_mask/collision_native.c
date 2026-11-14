/* Native bomb, collision and shield functions are inserted by run_tests.py.
 * Graphics, sound, terrain and animation playback are boundary doubles. The
 * shield eligibility, bomb collider setup/timing, attack-type routing, overlap
 * calculation and damage calculation below execute production code.
 */
#include "global.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "overlays/actors/ovl_En_Bom/z_en_bom.h"
#include "overlays/effects/ovl_Effect_Ss_Dead_Sound/z_eff_ss_dead_sound.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/game-interactor/vanilla-behavior/PlayerAnimOverride.h"
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

extern PlayState* gPlayState;
static Collider* registeredAttack;
static Input* sControlInput;
static LinkAnimationHeader fixtureAnimation;
static LinkAnimationHeader* D_808543A4[] = { &fixtureAnimation, &fixtureAnimation };
static LinkAnimationHeader* D_808543AC[] = { &fixtureAnimation, &fixtureAnimation };
static LinkAnimationHeader gPlayerAnim_clink_normal_defense_ALL;
#define GET_PLAYER_ANIM(group, type) (&fixtureAnimation)
static Gfx* fixtureDLists[2];
static Gfx** sPlayerDListGroups[PLAYER_MODELTYPE_MAX];
static s8 sActionHandlerListIdle[] = { 0 };
static int recoveryActionOffers;

float CVarGetFloat(const char* name, float fallback) { return fallback; }
uint8_t GameInteractor_GetRandomBombFuseTimerActive(void) { return 0; }
bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t result, ...) { return result; }
void GameInteractor_ExecuteOnPlayerHoldUpShield(void) {}
void osSyncPrintfUnused(const char* format, ...) {}
u8 Randomizer_SoulCanDamageActor(PlayState* play, Actor* actor) { return 1; }
u8 DinFireSword_DamageEntry(PlayState* play, Actor* actor, ColliderInfo* attack, u32 flags, u8 value) { return value; }
void DinFireSword_FinishAcceptedHit(PlayState* play, Collider* collider, ColliderInfo* attack, u32 flags, u8 damage) {}
static u8 NEI_PlayerDamageBoostActive(void) { return 0; }

void Actor_ProcessInitChain(Actor* actor, InitChainEntry* entries) {}
void ActorShape_Init(ActorShape* shape, f32 yOffset, ActorShadowFunc shadow, f32 scale) {}
void ActorShadow_DrawCircle(Actor* actor, Lights* lights, PlayState* play) {}
f32 Rand_ZeroOne(void) { return 0; }
void Audio_PlayActorSound2(Actor* actor, u16 sfx) {}
void Actor_MoveXZGravity(Actor* actor) {}
void Actor_UpdateBgCheckInfo(PlayState* play, Actor* actor, f32 wall, f32 floor, f32 ceiling, s32 flags) {}
void Actor_PlaySfx_SurfaceBomb(PlayState* play, Actor* actor) {}
u32 Actor_HasParent(Actor* actor, PlayState* play) { return actor->parent != NULL; }
u32 Actor_HasNoParent(Actor* actor, PlayState* play) { return actor->parent == NULL; }
void Actor_OfferCarry(Actor* actor, PlayState* play) {}
s32 Player_IsBurningStickInRange(PlayState* play, Vec3f* pos, f32 xz, f32 y) { return 0; }
void Actor_Kill(Actor* actor) { actor->update = NULL; }
void Rumble_Request(f32 distance, u8 strength, u8 duration, u8 decay) {}
void EffectSsGSpk_SpawnFuse(PlayState* play, Actor* actor, Vec3f* pos, Vec3f* velocity, Vec3f* acceleration) {}
void func_8002829C(PlayState* play, Vec3f* pos, Vec3f* velocity, Vec3f* acceleration, Color_RGBA8* prim,
                   Color_RGBA8* env, s16 scale, s16 scaleStep) {}
void EffectSsBomb2_SpawnLayered(PlayState* play, Vec3f* pos, Vec3f* velocity, Vec3f* acceleration, s16 scale,
                              s16 scaleStep) {}
void EffectSsBlast_SpawnWhiteShockwave(PlayState* play, Vec3f* pos, Vec3f* velocity, Vec3f* acceleration) {}
s32 Camera_AddQuake(Camera* camera, s32 type, s16 speed, s32 duration) { return 0; }
void EffectSsDeadSound_SpawnStationary(PlayState* play, Vec3f* pos, u16 sfx, s16 priority, s16 repeat,
                                      s32 life) {}
void Actor_SetFocus(Actor* actor, f32 height) {}
void CollisionCheck_HitEffects(PlayState* play, Collider* at, ColliderInfo* atInfo, Collider* ac,
                               ColliderInfo* acInfo, Vec3f* pos) {}
void CollisionCheck_SetHitEffects(PlayState* play, CollisionCheckContext* context) {}
void Champion_NoteIncomingAttacks(PlayState* play) {}

/* The queue boundary records exactly the collider registered by EnBom_Explode.
 * Native CollisionCheck_AC below still owns type/actor/shape eligibility. */
s32 CollisionCheck_SetAT(PlayState* play, CollisionCheckContext* context, Collider* collider) {
    Collider_ResetJntSphAT(play, collider);
    registeredAttack = collider;
    return 0;
}
s32 CollisionCheck_SetAC(PlayState* play, CollisionCheckContext* context, Collider* collider) { return 0; }
s32 CollisionCheck_SetOC(PlayState* play, CollisionCheckContext* context, Collider* collider) { return 0; }

u8 TransformMasks_GetShieldMode(void) { return MMFORM_SHIELD_VANILLA; }
u8 GerudoMhr_UsesBladeGuard(Player* player) { return 0; }
u8 GerudoMhr_ForcesFighter(Player* player) { return 0; }
LinkAnimationHeader* GerudoMhr_GetGuardAnim(Player* player, s32 phase) { return NULL; }
void func_808326F0(Player* player) {}
void Player_DetachHeldActor(PlayState* play, Player* player) { player->heldActor = NULL; }
void Player_PlaySfx(Actor* actor, u16 sfx) {}
void func_80832318(Player* player) {}
void func_80833C3C(Player* player) {}
void Player_StartAnimMovement(PlayState* play, Player* player, s32 flags) {}
void Player_SetModelGroup(Player* player, s32 group) {}
s32 Player_ActionToModelGroup(Player* player, s32 action) { return 0; }
void Player_Action_80843188(Player* player, PlayState* play) {}
s32 Player_SetupAction(PlayState* play, Player* player, PlayerActionFunc action, s32 flags) {
    if (player->actionFunc == action) return 0;
    player->actionFunc = action;
    return 1;
}
s32 func_80834C74(Player* player, PlayState* play) { return 0; }
void func_8008EC70(Player* player) {}
void Animation_Reverse(SkelAnime* animation) {}
s16 Animation_GetLastFrame(void* animation) { return 0; }
s32 LinkAnimation_Update(PlayState* play, SkelAnime* animation) { return 0; }
void LinkAnimation_Change(PlayState* play, SkelAnime* animation, LinkAnimationHeader* resource, f32 speed,
                          f32 start, f32 end, u8 mode, f32 morph) { animation->animation = resource; }
s32 Player_DecelerateToZero(Player* player) { return 0; }
void func_80839F90(Player* player, PlayState* play) {}
s32 Player_TryActionHandlerList(PlayState* play, Player* player, s8* actions, s32 upperBody) {
    assert(actions == sActionHandlerListIdle && upperBody);
    ++recoveryActionOffers;
    return 0;
}
s32 Player_GetMovementSpeedAndYaw(Player* player, f32* speed, s16* yaw, f32 mode, PlayState* play) {
    return 0;
}

/* NATIVE_COLLIDER */
/* NATIVE_MATH */
/* NATIVE_COLLISION */
/* NATIVE_BOMB */
/* NATIVE_SHIELD */
/* NATIVE_DAMAGE_RECOVERY */

int BlastTest_DamageRecoveryOffersActions(PlayState* play, Player* player) {
    recoveryActionOffers = 0;
    Player_Action_8084370C(player, play);
    return recoveryActionOffers != 0;
}

void BlastTest_InitBomb(PlayState* play, EnBom* bomb) {
    bomb->actor.id = ACTOR_EN_BOM;
    bomb->actor.category = ACTORCAT_EXPLOSIVE;
    bomb->actor.params = BOMB_BODY;
    bomb->actor.update = EnBom_Update;
    bomb->actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    EnBom_Init(&bomb->actor, play);
}

void BlastTest_InitBody(PlayState* play, Actor* actor, ColliderCylinder* body, int enemy) {
    /* NATIVE_PLAYER_BODY */
    Collider_InitCylinder(play, body);
    ColliderCylinderInit init = D_80854624;
    if (enemy) init.base.acFlags = AC_ON | AC_TYPE_PLAYER;
    Collider_SetCylinder(play, body, actor, &init);
    Collider_UpdateCylinder(actor, body);
}

void BlastTest_ClearHits(PlayState* play, ColliderCylinder* body) {
    body->base.actor->colChkInfo.damage = 0;
    Collider_ResetCylinderAC(play, &body->base);
}

void BlastTest_BombFrame(PlayState* play, EnBom* bomb, ColliderCylinder* body, ColliderCylinder* enemy) {
    Collider* receivers[] = { &body->base, &enemy->base };
    registeredAttack = NULL;
    BlastTest_ClearHits(play, body);
    BlastTest_ClearHits(play, enemy);
    if (bomb->actor.update != NULL) EnBom_Update(&bomb->actor, play);
    if (registeredAttack == NULL) return;
    play->colChkCtx.colAC = receivers;
    play->colChkCtx.colACCount = 2;
    play->colChkCtx.colAT = &registeredAttack;
    play->colChkCtx.colATCount = 1;
    CollisionCheck_AT(play, &play->colChkCtx);
    CollisionCheck_ApplyDamageCyl(play, &play->colChkCtx, &body->base);
    CollisionCheck_ApplyDamageCyl(play, &play->colChkCtx, &enemy->base);
    play->colChkCtx.colAC = NULL;
    play->colChkCtx.colACCount = 0;
    play->colChkCtx.colAT = NULL;
    play->colChkCtx.colATCount = 0;
}

int BlastTest_RaiseShield(PlayState* play, Player* player, Input* input, int targeted) {
    for (int i = 0; i < PLAYER_MODELTYPE_MAX; ++i) sPlayerDListGroups[i] = fixtureDLists;
    sControlInput = input;
    player->stateFlags1 &= ~PLAYER_STATE1_SHIELDING;
    if (targeted) return Player_TryRaiseUpperBodyShield(play, player);
    return Player_ActionHandler_11(player, play);
}

void BlastTest_HoldShield(PlayState* play, Player* player, Input* input) {
    sControlInput = input;
    player->stateFlags1 &= ~PLAYER_STATE1_SHIELDING;
    func_80834B5C(player, play);
}
