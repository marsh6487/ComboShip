#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;
typedef double f64;
#include "native_types.inc"
#define SQ(x) ((x) * (x))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define LERPIMP(a,b,t) F32_LERPIMP(a,b,t)
#define CLAMP(x,low,high) MAX(low,MIN(x,high))
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))
#define osSyncPrintf(...) ((void)0)
#define VT_COL(a,b) ""
#define VT_RST ""
#define CHECK_BTN_ALL(button, mask) (((button) & (mask)) == (mask))
#define BTN_B 0x4000
#define BGCHECKFLAG_GROUND 1
#define PLAYER_STATE1_DEAD (1u << 7)
#define PLAYER_STATE1_IN_CUTSCENE (1u << 29)
#define PLAYER_STATE1_INPUT_DISABLED (1u << 5)
#define PLAYER_STATE1_GETTING_ITEM (1u << 10)
#define PLAYER_STATE1_DAMAGED (1u << 2)
#define PLAYER_STATE1_HOSTILE_LOCK_ON (1u << 4)
#define PLAYER_STATE1_IN_WATER (1u << 27)
#define ACTOR_FLAG_SFX_FOR_PLAYER_BODY_HIT (1u << 24)
#define MMFORM_STATE_ACTIVE 2
#define MM_FORM_LIMB_COUNT 22
#define ANIMMODE_ONCE 2
#define ANIMMODE_LOOP 0
#define PLAYER_LIMB_L_HAND 16
#define PLAYER_LIMB_R_HAND 19
#define PLAYER_LIMB_L_FOREARM 15
#define PLAYER_LIMB_R_FOREARM 18
#define PLAYER_LIMB_R_SHIN 5
#define PLAYER_LIMB_WAIST 2
#define MMFORM_ON_GROUND(p) ((p)->actor.bgCheckFlags & BGCHECKFLAG_GROUND)
#define TOTAL_EFFECT_COUNT 512
#define EFFECT_BLURE2 1
#define GORON_PUNCH_DAMAGE 2
#define ZORA_PUNCH_DAMAGE 1
#define GERUDO_SLASH_DAMAGE 2
#define GERUDO_FINISHER_DAMAGE 4
#define NA_SE_IT_SWORD_SWING_HARD 0
#define NA_SE_IT_HAMMER_HIT 0
#define MM_NA_SE_IT_GORON_PUNCH_SWING 0
#define MM_NA_SE_PL_GORON_PUNCH 0
#define NA_SE_PL_BODY_HIT 0
#define MTXMODE_APPLY 1
enum { PLAYER_HIT_RESPONSE_NONE, PLAYER_HIT_RESPONSE_KNOCKBACK_LARGE, PLAYER_HIT_RESPONSE_KNOCKBACK_SMALL,
       PLAYER_HIT_RESPONSE_FROZEN, PLAYER_HIT_RESPONSE_ELECTRIFIED };

typedef enum {
    MM_PLAYER_FORM_FIERCE_DEITY, MM_PLAYER_FORM_GORON, MM_PLAYER_FORM_ZORA, MM_PLAYER_FORM_DEKU,
    MM_PLAYER_FORM_HUMAN, MM_PLAYER_FORM_PIKACHU, MM_PLAYER_FORM_GARO, MM_PLAYER_FORM_GERUDO,
    MM_PLAYER_FORM_RITO, MM_PLAYER_FORM_KEATON, MM_PLAYER_FORM_KAFEI, MM_PLAYER_FORM_MAX
} MmPlayerTransformation;
typedef struct Actor {
    struct { Vec3f pos; Vec3s rot; } world;
    struct { Vec3s rot; } shape;
    struct { u8 atHitEffect, acHitEffect; } colChkInfo;
    Vec3f scale;
    u32 bgCheckFlags, flags;
} Actor;
typedef struct { struct { u16 button; } cur, press; } Input;
typedef struct CollisionCheckContext { Collider** colAT; s32 colATCount, colATCapacity; } CollisionCheckContext;
typedef struct PlayState {
    CollisionCheckContext colChkCtx;
    struct { Input input[1]; } state;
} PlayState;
typedef struct { struct { s16 frameCount; } common; } LinkAnimationHeader;
typedef struct { LinkAnimationHeader* animation; f32 curFrame, playSpeed, endFrame; u8 mode; } SkelAnime;
typedef struct { Vec3f base, tip; u8 active; } WeaponInfo;
typedef struct Player {
    Actor actor;
    ColliderQuad meleeWeaponQuads[2];
    ColliderCylinder cylinder;
    WeaponInfo meleeWeaponInfo[3];
    f32 linearVelocity;
    s16 yaw, invincibilityTimer;
    s16 unk_A87, csAction;
    u32 stateFlags1;
    struct { s16 fallDamageStunTimer; } av2;
} Player;
typedef struct {
    int calcMode, flags, addAngleChange;
    u8 p1s[4], p2s[4], p1e[4], p2e[4];
    int duration, unk, drawMode, mode4;
    u8 altPrim[4], altEnv[4];
} EffectBlureInit2;
typedef struct { int unused; } EffectBlure;
typedef void Gfx;
static struct {
    MmPlayerTransformation currentForm;
    s32 state, goronAction, actionTimer, comboStep, boomerangState;
    u8 comboBPressed, comboBufferTimer, wallRecoilActive, punchTrailActive, punchTrailActiveR;
    u8 zoraMeleeActive, meleeActive, jumpKickActive, zoraJumpTrailActive;
    f32 zoraPunchCollisionFrame, punchCollisionFrame;
    s32 punchTrailEffectIndex, punchTrailEffectIndexR;
    SkelAnime formSkelAnime;
    LinkAnimationHeader *idleAnim, *cutterAttack, *punchA, *punchB;
} gFormState;
static Player player;
static Actor enemy;
static ColliderCylinder target;
static PlayState play;
static LinkAnimationHeader clip = {{32}}, otherClip = {{32}};
static Vec3f matrixOffset;
static MtxF matrix;
static MtxF* sCurrentMatrix = &matrix;
static EffectBlure effect;
static f32 recordedMorph;
static int damageResponses;

/* Limb parents are explicit fixtures. The actual OoT matrix operations apply
 * the native hand override, so translation/order/double-application are tested. */
#define FrameInterpolation_RecordMatrixTranslateRotateZYX(...) ((void)0)
#define FrameInterpolation_RecordMatrixScale(...) ((void)0)
void SkinMatrix_SetScale(MtxF* m, f32 x, f32 y, f32 z) { assert(!"Only MTXMODE_APPLY is used in this fixture"); }
void Math_Vec3f_Copy(Vec3f*, Vec3f*);
s32 Collider_ResetQuadAT(PlayState*, Collider*);
s32 Collider_ResetCylinderAT(PlayState*, Collider*);
void Collider_SetQuadVertices(ColliderQuad*, Vec3f*, Vec3f*, Vec3f*, Vec3f*);
s32 CollisionCheck_SetAT(PlayState* p, CollisionCheckContext* ctx, Collider* c) {
    if (c->shape == COLSHAPE_QUAD) Collider_ResetQuadAT(p, c);
    else Collider_ResetCylinderAT(p, c);
    assert(ctx->colATCount < ctx->colATCapacity);
    ctx->colAT[ctx->colATCount++] = c;
    return ctx->colATCount - 1;
}
void CollisionCheck_HitEffects(PlayState* p, Collider* a, ColliderInfo* ai, Collider* b, ColliderInfo* bi, Vec3f* v) {}
void Effect_Delete(PlayState* p, s32 index) {}
void Effect_Add(PlayState* p, s32* index, s32 type, u8 a, u8 b, void* init) { *index = 1; }
void* Effect_GetByIndex(s32 index) { return &effect; }
void EffectBlure_AddVertex(EffectBlure* e, Vec3f* a, Vec3f* b) {}
void MmForm_KillTrail(PlayState* p, s32* index, u8* active) { *active = 0; }
static void MmForm_StopRootMotion(void) {}
static void MmForm_ApplyRootMotion(Player* p) {}
static void MmForm_StartRootMotion(u8 step) {}
static LinkAnimationHeader* MmForm_GetPunchEndAnim(u8 step, u8 lock) { return &clip; }
static LinkAnimationHeader* MmForm_GetPunchAttackAnim(u8 step) { return &clip; }
static u8 MmForm_ZoraBoomerangHoldReady(PlayState* p) { return 0; }
static void Player_StartZoraBoomerang(Player* p, PlayState* state) {}
static void MmForm_PlayAttackVoice(Player* p) {}
static void MmForm_PlaySfx(Player* p, u16 mm, u16 oot) {}
void Player_PlaySfx(void* actor, u16 sfx) {}
static s32 Player_SpiritTunicAbsorbHit(PlayState* p, Player* actor) { return 0; }
static void func_80838280(Player* p) {}
static s16 Actor_WorldYawTowardActor(Actor* a, Actor* b) { return 0; }
static void func_80837C0C(PlayState* play, Player* p, s32 type, f32 speed, f32 yVelocity, s16 yaw, s32 invincibility) {
    ++damageResponses;
}
void EffectSsHahen_SpawnBurst(PlayState* state, Vec3f* pos, f32 burst, s16 unused, s16 scale, s16 range,
                            s16 count, s16 object, s16 life, Gfx* dl) {}
typedef int MmFormWallHitResult;
enum { MMFORM_WALL_HIT_NONE, MMFORM_WALL_HIT_GORON, MMFORM_WALL_HIT_ZORA };
static MmFormWallHitResult MmForm_CheckWallHit(Player* p, PlayState* s, u8 step, u8 goron, f32 frame, f32 start) { return 0; }
s32 Math_StepToF(f32* value, f32 target, f32 step) { *value = target; return 1; }
f32 Math_SinS(s16 a) { return sinf(a * (M_PI / 32768.0f)); }
f32 Math_CosS(s16 a) { return cosf(a * (M_PI / 32768.0f)); }
void LinkAnimation_Change(PlayState* p, SkelAnime* sa, LinkAnimationHeader* anim,
                          f32 speed, f32 start, f32 end, u8 mode, f32 morph) {
    *sa = (SkelAnime){anim, start, speed, end, mode};
    recordedMorph = morph;
}
s16 Animation_GetLastFrame(void* anim) { return ((LinkAnimationHeader*)anim)->common.frameCount - 1; }
typedef struct { s16 unk_0; Vec3s unk_2; } struct_80124618;
#include "native_scale.inc"
#include "native_collision.inc"
#include "form_melee.inc"

static void reset_matrix(void) {
    memset(&matrix, 0, sizeof(matrix));
    matrix.xx = matrix.yy = matrix.zz = .01f;
    matrix.ww = 1;
    matrix.xw = matrixOffset.x; matrix.yw = matrixOffset.y; matrix.zw = matrixOffset.z;
}

static void draw_limb(int limb) {
    Vec3f pos = {0}; Vec3s rot = {0};
    reset_matrix();
    /* Action damage is latched before the gameplay animation tick; drawing
     * uses its updated pose/frame, just like native MM Action_84. */
    gFormState.formSkelAnime.curFrame += 1;
    production_hand_override(limb, &pos, &rot);
    Matrix_TranslateRotateZYX(&pos, &rot);
    Form_Draw(&play, &player, limb);
}

static void reset(MmPlayerTransformation form, int step) {
    static Collider* attacks[16];
    memset(&player, 0, sizeof(player)); memset(&enemy, 0, sizeof(enemy)); memset(&target, 0, sizeof(target));
    memset(&play, 0, sizeof(play)); memset(&gFormState, 0, sizeof(gFormState));
    play.colChkCtx.colAT = attacks; play.colChkCtx.colATCapacity = ARRAY_COUNT(attacks);
    gFormState.state = MMFORM_STATE_ACTIVE; gFormState.currentForm = form;
    gFormState.comboStep = step; gFormState.goronAction = GORON_ACT_PUNCH_A + step;
    gFormState.formSkelAnime.animation = gFormState.idleAnim = &clip;
    gFormState.punchA = gFormState.punchB = &clip;
    damageResponses = 0;
    for (int i = 0; i < 2; ++i) {
        player.meleeWeaponQuads[i].base.actor = &player.actor;
        player.meleeWeaponQuads[i].base.shape = COLSHAPE_QUAD;
    }
    player.cylinder.base.actor = &player.actor; player.cylinder.base.shape = COLSHAPE_CYLINDER;
    player.cylinder.base.acFlags = AC_ON | AC_TYPE_ENEMY;
    player.cylinder.info.bumperFlags = BUMP_ON; player.cylinder.info.bumper.dmgFlags = ~0u;
    player.cylinder.dim = (Cylinder16){sFormProps[form].cylinderRadius, 60, 0, {0,0,0}};
    target.base.actor = &enemy; target.base.shape = COLSHAPE_CYLINDER;
    target.base.acFlags = AC_ON | AC_TYPE_PLAYER;
    target.info.bumperFlags = BUMP_ON; target.info.bumper.dmgFlags = ~0u;
    target.info.toucherFlags = TOUCH_ON; target.info.toucher.dmgFlags = DMG_SLASH_MASTER;
    target.dim = (Cylinder16){4, 12, 0, {0,35,65}};
    matrixOffset = (Vec3f){0,30,50};
}

static void update(float frame) {
    play.colChkCtx.colATCount = 0;
    gFormState.formSkelAnime.curFrame = frame;
    MmForm_Action_Punch(&player, &play);
    Form_Update(&player, &play);
}

static void hit_target(void) {
    for (int i = 0; i < play.colChkCtx.colATCount; ++i) {
        Collider* at = play.colChkCtx.colAT[i];
        if (at->atFlags & AT_ON) CollisionCheck_AC_QuadVsCyl(&play, &play.colChkCtx, at, &target.base);
    }
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const char* name = argv[1];
    if (!strcmp(name, "goron-body") || !strcmp(name, "zora-body")) {
        reset(name[0] == 'g' ? MM_PLAYER_FORM_GORON : MM_PLAYER_FORM_ZORA, 0);
        target.dim = (Cylinder16){10, 60, 0, {0,0,28}};
        target.base.atFlags = AT_ON | AT_TYPE_ENEMY;
        CollisionCheck_AC_CylVsCyl(&play, &play.colChkCtx, &target.base, &player.cylinder.base);
        assert(!(player.cylinder.base.acFlags & AC_HIT) && "Native MM's body does not overlap this enemy at distance 28");
        target.dim.pos.z = 21;
        CollisionCheck_AC_CylVsCyl(&play, &play.colChkCtx, &target.base, &player.cylinder.base);
        assert(player.cylinder.base.acFlags & AC_HIT);
    } else if (!strncmp(name, "goron-", 6) || !strncmp(name, "zora-", 5)) {
        int goron = name[0] == 'g';
        int step = strstr(name, "right") ? 1 : (strstr(name, "butt") || strstr(name, "kick")) ? 2 : 0;
        reset(goron ? MM_PLAYER_FORM_GORON : MM_PLAYER_FORM_ZORA, step);
        int limb = goron ? (step == 0 ? PLAYER_LIMB_L_HAND : step == 1 ? PLAYER_LIMB_R_HAND : PLAYER_LIMB_WAIST) :
                          (step == 0 ? PLAYER_LIMB_L_FOREARM : step == 1 ? PLAYER_LIMB_R_FOREARM : PLAYER_LIMB_R_SHIN);
        int start = goron ? (step == 0 ? 6 : step == 1 ? 12 : 8) : (step == 0 ? 2 : 3);
        if (goron && step == 2) {
            matrixOffset = (Vec3f){0,40,20}; target.dim.pos = (Vec3f){25,90,45};
        } else if (!goron && step < 2) {
            matrixOffset = (Vec3f){30,30,40}; target.dim.pos = (Vec3f){5,35,62};
        } else if (!goron) {
            matrixOffset = (Vec3f){0,30,50}; target.dim.pos = (Vec3f){15,24,55};
        } else {
            /* This cylinder spans the native hand's unscaled and enlarged
             * heights; its forward edge is beyond the old fixed plane. */
            target.dim.pos = (Vec3f){12,35,61};
            target.dim.height = 30;
        }
        update(start); draw_limb(limb);
        update(start + 1); matrixOffset.z += 10;
        draw_limb(limb);
        hit_target();
        assert(target.base.acFlags & AC_HIT && "The enemy inside the animated native limb sweep must receive the melee hit");
        assert(play.colChkCtx.colATCount == 2 && "Native MM submits both swept quads");
        assert((player.meleeWeaponQuads[0].base.atFlags | player.meleeWeaponQuads[1].base.atFlags) & AT_HIT);
        assert(player.cylinder.base.acFlags & AC_ON && "Attacking must not grant blanket body invulnerability");
        assert(!(player.cylinder.base.atFlags & AT_ON));
    } else if (!strcmp(name, "hand-scales")) {
        static const float frames[2][6] = {{0,5,5.5f,6,10.5f,11}, {0,14,14.5f,15,17,18}};
        for (int hand = 0; hand < 2; ++hand) for (int key = 0; key < 6; ++key) {
            reset(MM_PLAYER_FORM_GORON, hand);
            int limb = hand ? PLAYER_LIMB_R_HAND : PLAYER_LIMB_L_HAND;
            gFormState.formSkelAnime.curFrame = frames[hand][key];
            /* Keep the collision frame different to catch use of the stale latch. */
            gFormState.punchCollisionFrame = gFormState.formSkelAnime.curFrame - 1;
            Vec3f pos = {100,200,300}, point = {1500,1500,1500}, actual, expected, nativeScale;
            Vec3s rot = {0x1000,0x4000,0x2000};
            reset_matrix();
            func_80124618(hand ? D_801C0784 : D_801C0750, frames[hand][key], &nativeScale);
            Matrix_TranslateRotateZYX(&pos, &rot);
            Matrix_Scale(nativeScale.x, nativeScale.y, nativeScale.z, MTXMODE_APPLY);
            Matrix_MultVec3f(&point, &expected);
            reset_matrix();
            production_hand_override(limb, &pos, &rot);
            Matrix_TranslateRotateZYX(&pos, &rot); /* Skeleton's normal continuation. */
            Matrix_MultVec3f(&point, &actual);
            assert(fabsf(actual.x-expected.x)<.001f && fabsf(actual.y-expected.y)<.001f &&
                   fabsf(actual.z-expected.z)<.001f && "Goron's rendered hand matrix must match the native MM scale curve");
        }
        for (int control = 0; control < 4; ++control) {
            reset(MM_PLAYER_FORM_GORON, 0);
            gFormState.formSkelAnime.curFrame = 6;
            int limb = PLAYER_LIMB_L_HAND;
            if (control == 0) gFormState.currentForm = MM_PLAYER_FORM_ZORA;
            if (control == 1) limb = PLAYER_LIMB_R_HAND;
            if (control == 2) gFormState.goronAction = GORON_ACT_IDLE;
            if (control == 3) gFormState.formSkelAnime.animation = &otherClip;
            Vec3f pos = {100,200,300}; Vec3s rot = {1,2,3};
            reset_matrix(); MtxF before = matrix;
            production_hand_override(limb, &pos, &rot);
            assert(!memcmp(&matrix,&before,sizeof(matrix)) && pos.x==100 && rot.x==1 &&
                   "Only the native Goron attack clip may scale its attacking hand");
        }
    } else if (!strcmp(name, "hand-reach")) {
        for (int hand = 0; hand < 2; ++hand) {
            reset(MM_PLAYER_FORM_GORON, hand);
            int limb = hand ? PLAYER_LIMB_R_HAND : PLAYER_LIMB_L_HAND;
            int frame = hand ? 14 : 6;
            matrixOffset = (Vec3f){0,30,50};
            target.dim = (Cylinder16){2,20,0,{22,50,72}};
            update(frame); draw_limb(limb);
            matrixOffset.z += 10;
            update(frame + 1); draw_limb(limb);
            hit_target();
            assert(target.base.acFlags & AC_HIT && "The native enlarged fist must reach this enemy beyond the unscaled hand sweep");
        }
    } else if (!strcmp(name, "contact-priority")) {
        for (int form = MM_PLAYER_FORM_GORON; form <= MM_PLAYER_FORM_ZORA; ++form) for (int miss = 0; miss < 2; ++miss) {
            reset(form, 0);
            int frame = form == MM_PLAYER_FORM_GORON ? 6 : 2;
            int limb = form == MM_PLAYER_FORM_GORON ? PLAYER_LIMB_L_HAND : PLAYER_LIMB_L_FOREARM;
            target.dim = (Cylinder16){10,60,0,{5,0,21}};
            target.base.atFlags = AT_ON | AT_TYPE_ENEMY;
            matrixOffset = (Vec3f){miss ? 1000 : (form == MM_PLAYER_FORM_ZORA ? 15 : 0),0,10};
            update(frame); draw_limb(limb);
            matrixOffset.z += 10;
            update(frame + 1); draw_limb(limb);
            hit_target();
            u8 hit = (player.meleeWeaponQuads[0].base.atFlags | player.meleeWeaponQuads[1].base.atFlags) & AT_HIT;
            assert(!!hit == !miss);
            CollisionCheck_AC_CylVsCyl(&play, &play.colChkCtx, &target.base, &player.cylinder.base);
            assert(player.cylinder.base.acFlags & AC_HIT && "The enemy must also make real body contact");
            production_contact_response(&player, &play);
            assert(damageResponses == miss && "Successful melee keeps native AT_HIT priority; a missed active punch must still take the body hit");
        }
    } else if (!strcmp(name, "windows")) {
        static const int starts[2][3] = {{6,12,8},{2,3,3}}, ends[2][3] = {{8,18,14},{5,8,10}};
        for (int form = 0; form < 2; ++form) for (int step = 0; step < 3; ++step) {
            reset(form ? MM_PLAYER_FORM_ZORA : MM_PLAYER_FORM_GORON, step);
            update(starts[form][step] - 1);
            assert(play.colChkCtx.colATCount == 0 && "Native wind-up/negative melee state must not register damage");
            assert(!(player.meleeWeaponQuads[0].base.atFlags & AT_ON));
            update(starts[form][step]);
            assert(player.meleeWeaponQuads[0].base.atFlags & AT_ON);
            update(ends[form][step] + 1);
            assert(!(player.meleeWeaponQuads[0].base.atFlags & AT_ON));
            assert(!(player.meleeWeaponQuads[1].base.atFlags & AT_ON));
        }
    } else if (!strcmp(name, "pose")) {
        for (int form = MM_PLAYER_FORM_GORON; form <= MM_PLAYER_FORM_ZORA; ++form) {
            reset(form, 0); player.invincibilityTimer = 10;
            assert(!production_force_pose(&player) && "A new form melee attack owns its animated bones during remaining i-frames");
            gFormState.goronAction = GORON_ACT_IDLE;
            assert(production_force_pose(&player) && "The existing idle damage-recovery fallback remains available");
            gFormState.goronAction = MMFORM_ACT_OOT_ACTION; player.stateFlags1 = PLAYER_STATE1_DAMAGED;
            assert(production_force_pose(&player));
        }
    } else if (!strcmp(name, "timing")) {
        for (int form = MM_PLAYER_FORM_GORON; form <= MM_PLAYER_FORM_ZORA; ++form) {
            reset(form, 0);
            MmForm_SetAction(GORON_ACT_PUNCH_A, &play, &clip, 1, ANIMMODE_ONCE);
            assert(fabsf(gFormState.formSkelAnime.playSpeed * 1.5f - 1.0f) < .00001f &&
                   "Both native MM punch clips advance one frame per update");
            assert(recordedMorph == 0 && "Native punches begin without an eight-frame blend from idle");
        }
    } else if (!strcmp(name, "lifecycle")) {
        for (int form = MM_PLAYER_FORM_GORON; form <= MM_PLAYER_FORM_ZORA; ++form) {
            reset(form, 0); update(6);
            gFormState.goronAction = GORON_ACT_IDLE; Form_Update(&player, &play);
            assert(!(player.meleeWeaponQuads[0].base.atFlags & AT_ON));
            assert(!(player.meleeWeaponQuads[1].base.atFlags & AT_ON));
            assert(!player.meleeWeaponInfo[1].active && !player.meleeWeaponInfo[2].active);
        }
    } else assert(!"Unknown test case");
    printf("PASS form melee %s\n", name);
    return 0;
}
