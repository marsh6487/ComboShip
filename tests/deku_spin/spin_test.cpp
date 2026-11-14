// Engine service doubles host the unmodified production bodies inserted by
// run_tests.py. They are a logic boundary, not substitutes for engine ABI or
// in-game rendering/collision/camera testing.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using s16 = int16_t;
using s32 = int32_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using f32 = float;
constexpr double PI = 3.14159265358979323846;
#define BINANG_TO_RAD(angle) ((angle) * (PI / 32768.0))
#define ABS(value) std::abs(value)
constexpr u32 PLAYER_STATE2_DISABLE_ROTATION_ALWAYS = 0x40;
constexpr u32 PRESERVED_FLAG = 0x100;
constexpr f32 SPEED_MODE_CURVED = 0.018f;
constexpr s32 ANIMMODE_ONCE = 1, ANIMMODE_LOOP = 0, MTXMODE_APPLY = 1;
constexpr s32 PLAYER_LIMB_HAT = 8, PLAYER_BODYPART_WAIST = 0;
constexpr s32 NA_SE_PL_WALK_ICE = 1, NA_SE_PL_WALK_GRASS = 2, NA_SE_PL_WALK_SAND = 3, SFX_FLAG = 0;
constexpr s32 MM_NA_SE_PL_SLIP_LEVEL = 4, MM_NA_SE_PL_DEKUNUTS_ATTACK = 5, NA_SE_PL_BODY_HIT = 6;
constexpr u32 DMG_SPIN_KOKIRI = 1 << 22;

struct Vec3f { f32 x, y, z; };
struct Vec3s { s16 x, y, z; };
struct Color_RGBA8 { u8 r, g, b, a; };
struct PosRot { Vec3f pos; Vec3s rot; };
struct Actor { PosRot world, focus; struct { Vec3s rot; } shape; Vec3f projectedPos; f32 yDistToWater; };
struct LinkAnimationHeader {};
struct SkelAnime { LinkAnimationHeader* animation; void* skeleton; Vec3s* jointTable; };
struct Collider { int base; };
struct Player {
    Actor actor;
    s16 yaw, parallelYaw;
    f32 linearVelocity;
    u32 stateFlags2;
    Actor* focusActor;
    Collider cylinder;
    Vec3f bodyPartsPos[1];
    s32 floorSfxOffset;
};
struct PlayState { int colChkCtx; struct { struct { s32 unk_4B; } targetCtx; } actorCtx; };

/* PRODUCTION_ENUMS */
struct TestFormState {
    MmFormStateId state;
    MmPlayerTransformation currentForm;
    GoronActionId goronAction;
    SkelAnime formSkelAnime;
    LinkAnimationHeader *dekuSpinAttack, *idleAnim, *runAnim;
    /* PRODUCTION_SPIN_FIELDS */
    s32 actionTimer, rollGroundPoundTimer, formDListCount;
    s32 punchTrailEffectIndex;
    u8 punchTrailActive, dekuHopsRemaining;
};
static TestFormState gFormState;
static Player player;
static PlayState play;
static LinkAnimationHeader spinClip, idleClip, runClip;
static bool stickActive;
static f32 stickSpeed;
static s16 stickYaw, cameraInputYaw;
static bool parallel;
static s32 animationUpdates, attackRegistrations, attackDamage, attackRadius, trailKills;
static u32 attackFlags;
static bool attackActive;
static s32 slipSounds, startSounds, trailVertices, drawCalls;
static Vec3f trailTip, trailBase;

static void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}
static bool Near(double a, double b) { return std::fabs(a - b) < 0.0001; }

// Only the unchanged engine numerical services are doubled. In particular,
// movement target selection and the idle shape->travel-yaw copy are production.
static s32 Math_StepToF(f32* value, f32 target, f32 step) {
    *value += std::clamp(target - *value, -step, step);
    return *value == target;
}
static s16 Math_SmoothStepToS(s16* value, s16 target, s16 scale, s16 maxStep, s16 minStep) {
    s16 difference = static_cast<s16>(target - *value);
    s32 step = difference / scale;
    step = std::clamp(step, -s32(maxStep), s32(maxStep));
    if (step >= 0 && step < minStep) step = std::min<s32>(difference, minStep);
    else if (step < 0 && step > -minStep) step = std::max<s32>(difference, -minStep);
    *value = static_cast<s16>(*value + step);
    return difference;
}
static s32 Math_ScaledStepToS(s16* value, s16 target, s16 step) {
    s16 difference = static_cast<s16>(target - *value);
    *value = static_cast<s16>(*value + std::clamp<s32>(difference, -step, step));
    return *value == target;
}
static f32 Math_SinS(s16 angle) { return std::sin(BINANG_TO_RAD(angle)); }
static f32 Math_CosS(s16 angle) { return std::cos(BINANG_TO_RAD(angle)); }
static s16 Math_Vec3f_Yaw(Vec3f* from, Vec3f* to) {
    return static_cast<s16>(std::atan2(to->x - from->x, to->z - from->z) * (32768.0 / PI));
}
static s32 Player_CalcSpeedAndYawFromControlStick(PlayState*, Player*, f32* speed, s16* yaw, f32) {
    *speed = stickActive ? stickSpeed : 0.0f;
    *yaw = stickYaw;
    return stickActive;
}
static s32 Player_FriendlyLockOnOrParallel(Player*) { return parallel; }
#define GET_ACTIVE_CAM(state) (state)
static s16 Camera_GetInputDirYaw(PlayState*) { return cameraInputYaw; }
static bool TransformMasks_IsTransformedAny() { return true; }
f32 MmForm_GetSpeedMultiplier() { return 1.0f; }
static f32 BossRemains_RunSpeedMul() { return 1.0f; }
static f32 GerudoMhr_RunSpeedMul() { return 1.0f; }
static f32 KafeiForm_RunSpeedMul() { return 1.0f; }
static void Player_SetupTurnInPlace(PlayState*, Player*, s16) { std::abort(); }
/* PRODUCTION_MOVEMENT_INPUT */
static void OotNeutralIdleTail(Player* self, PlayState* play) {
    f32 speedTarget;
    s16 yawTarget, yawDiff;
    Player_GetMovementSpeedAndYaw(self, &speedTarget, &yawTarget, SPEED_MODE_CURVED, play);
    Require(speedTarget == 0.0f, "neutral-idle fixture requires no stick input");
    /* PRODUCTION_IDLE_YAW_TAIL */
}

static s32 LinkAnimation_Update(PlayState*, SkelAnime*) { ++animationUpdates; return 0; }
static void LinkAnimation_PlayOnceSetSpeed(PlayState*, SkelAnime* animation, LinkAnimationHeader* clip, f32) {
    animation->animation = clip;
}
static void MmForm_SetAction(GoronActionId action, PlayState*, LinkAnimationHeader* clip, f32, u8) {
    gFormState.goronAction = action;
    gFormState.formSkelAnime.animation = clip;
    gFormState.actionTimer = 0;
}
static void MmForm_SetRollAttack(Player*, u32 flags, s32 damage, s16 radius) {
    attackFlags = flags; attackDamage = damage; attackRadius = radius; attackActive = true;
}
static void CollisionCheck_SetAT(PlayState*, int*, int*) { ++attackRegistrations; }
static void MmForm_ClearRollAttack(Player*) { attackActive = false; }
static void MmForm_KillTrail(PlayState*, s32* index, u8* active) {
    ++trailKills; *active = 0; *index = -1;
}
static void MmForm_PlaySfx(Player*, s32 sfx, s32) { Require(sfx == MM_NA_SE_PL_DEKUNUTS_ATTACK, "spin start sound"); ++startSounds; }
static void MmSfx_PlayAtPos(s32 sfx, Vec3f*) { Require(sfx == MM_NA_SE_PL_SLIP_LEVEL, "spin floor sound"); ++slipSounds; }
static f32 Rand_ZeroOne() { return 0.25f; }
static f32 Rand_ZeroFloat(f32 range) { return range * Rand_ZeroOne(); }
static void EffectSsKiraKira_SpawnDispersed(PlayState*, Vec3f*, Vec3f*, Vec3f*, Color_RGBA8*, Color_RGBA8*, s16, s32) {}
static void EffectSsDust_Spawn(PlayState*, s32, Vec3f*, Vec3f*, Vec3f*, Color_RGBA8*, Color_RGBA8*, s16, s16, s16, u8) {}
/* PRODUCTION_SPIN_GATE */
/* PRODUCTION_START */
/* PRODUCTION_ACTION */

// A CPU matrix boundary records the transform handed to the skeleton and its
// real hat-trail callback. Display lists, asset loading and limb traversal stay
// outside this test; the inherited actor matrix includes translation and scale.
using Matrix = std::array<double, 16>;
static Matrix currentMatrix, skeletonMatrix;
static std::vector<Matrix> matrixStack;
static void Matrix_Push() { matrixStack.push_back(currentMatrix); }
static void Matrix_Pop() {
    Require(!matrixStack.empty(), "matrix pop must balance a push");
    currentMatrix = matrixStack.back(); matrixStack.pop_back();
}
static void Matrix_RotateY(f32 angle, s32 mode) {
    Require(mode == MTXMODE_APPLY, "spin must apply to inherited actor matrix");
    const double sine = std::sin(angle), cosine = std::cos(angle);
    for (s32 row = 0; row < 4; ++row) {
        const double x = currentMatrix[row * 4], z = currentMatrix[row * 4 + 2];
        currentMatrix[row * 4] = cosine * x - sine * z;
        currentMatrix[row * 4 + 2] = sine * x + cosine * z;
    }
}
static void Matrix_MultVec3f(Vec3f* input, Vec3f* output) {
    output->x = currentMatrix[0] * input->x + currentMatrix[1] * input->y + currentMatrix[2] * input->z + currentMatrix[3];
    output->y = currentMatrix[4] * input->x + currentMatrix[5] * input->y + currentMatrix[6] * input->z + currentMatrix[7];
    output->z = currentMatrix[8] * input->x + currentMatrix[9] * input->y + currentMatrix[10] * input->z + currentMatrix[11];
}
struct EffectBlure {};
static EffectBlure trail;
static void* Effect_GetByIndex(s32 index) { Require(index == 7, "hat callback uses active trail"); return &trail; }
static void EffectBlure_AddVertex(EffectBlure*, Vec3f* tip, Vec3f* base) { ++trailVertices; trailTip = *tip; trailBase = *base; }
static s32 MmForm_OverrideLimbDraw(PlayState*, s32, void**, Vec3f*, Vec3s*, void*) { return 0; }
static void MmForm_PostLimbDraw(PlayState*, s32 limbIndex, void**, Vec3s*, void*) {
    /* PRODUCTION_TRAIL_DRAW */
}
using Override = s32 (*)(PlayState*, s32, void**, Vec3f*, Vec3s*, void*);
using Post = void (*)(PlayState*, s32, void**, Vec3s*, void*);
static void SkelAnime_DrawFlexOpa(PlayState* state, void*, Vec3s*, s32, Override override, Post post, void* actor) {
    Require(override == MmForm_OverrideLimbDraw && post == MmForm_PostLimbDraw, "spin retains form limb callbacks");
    Require(actor == &player.actor, "spin draw keeps live actor");
    ++drawCalls; skeletonMatrix = currentMatrix;
    post(state, PLAYER_LIMB_HAT, nullptr, nullptr, actor);
}
static void DrawSkeleton(Player* player, PlayState* play) {
    /* PRODUCTION_SKELETON_DRAW */
}
static void SetActorMatrix() {
    currentMatrix = {0.01, 0, 0, 10, 0, 0.01, 0, 20, 0, 0, 0.01, 30, 0, 0, 0, 1};
    Matrix_RotateY(BINANG_TO_RAD(player.actor.shape.rot.y), MTXMODE_APPLY);
}
static void RequireDrawYaw(s32 expected) {
    const double angle = BINANG_TO_RAD(expected);
    Require(Near(skeletonMatrix[2], 0.01 * std::sin(angle)) && Near(skeletonMatrix[10], 0.01 * std::cos(angle)),
            "skeleton visual yaw must include spin and steering exactly once");
}
static void DrawAndCheckMatrix() {
    SetActorMatrix();
    const Matrix inherited = currentMatrix;
    const s16 actorYaw = player.actor.shape.rot.y, travelYaw = player.yaw;
    DrawSkeleton(&player, &play);
    Require(currentMatrix == inherited && matrixStack.empty(), "spin draw must restore inherited matrix");
    Require(player.actor.shape.rot.y == actorYaw && player.yaw == travelYaw, "draw must not mutate gameplay yaw");
}
static void Reset() {
    gFormState = {}; player = {}; play = {};
    gFormState.state = MMFORM_STATE_ACTIVE; gFormState.currentForm = MM_PLAYER_FORM_DEKU;
    gFormState.dekuSpinAttack = &spinClip; gFormState.idleAnim = &idleClip; gFormState.runAnim = &runClip;
    gFormState.punchTrailEffectIndex = 7; gFormState.punchTrailActive = 1; gFormState.dekuHopsRemaining = 5;
    player.yaw = player.actor.shape.rot.y = player.actor.focus.rot.y = 0x1000;
    player.actor.yDistToWater = -1; player.stateFlags2 = PRESERVED_FLAG;
    stickActive = false; stickSpeed = 4; stickYaw = cameraInputYaw = 0; parallel = false;
    animationUpdates = attackRegistrations = trailKills = slipSounds = startSounds = trailVertices = drawCalls = 0;
    attackActive = false; matrixStack.clear();
    MmForm_StartDekuSpinFromOot(&player, &play);
}
static void RequireActiveAttack() {
    Require(attackActive && attackDamage == 1 && attackRadius == 30 && attackFlags == DMG_SPIN_KOKIRI,
            "spin retains its light attack cylinder");
    Require((player.stateFlags2 & (PRESERVED_FLAG | PLAYER_STATE2_DISABLE_ROTATION_ALWAYS)) ==
                (PRESERVED_FLAG | PLAYER_STATE2_DISABLE_ROTATION_ALWAYS), "spin retains rotation ownership and unrelated flags");
}
static void RequireFinished(GoronActionId action) {
    Require(gFormState.goronAction == action && !gFormState.dekuSpinActive, "spin transitions to correct next action");
    Require(!attackActive && trailKills == 1 && !gFormState.punchTrailActive, "spin releases cylinder and hat trail");
    Require(player.stateFlags2 == PRESERVED_FLAG, "spin releases only its rotation flag");
    Require(player.actor.shape.rot.y == player.yaw, "finished actor faces travel direction");
    DrawAndCheckMatrix(); RequireDrawYaw(player.yaw);
}

int main(int argc, char** argv) {
    Require(argc == 2, "name one regression case");
    Reset();
    const std::string name = argv[1];
    if (name == "stationary") {
        for (s32 frame = 0; frame < 3; ++frame) {
            OotNeutralIdleTail(&player, &play);
            MmForm_Action_DekuSpin(&player, &play);
            Require(player.yaw == 0x1000 && player.actor.shape.rot.y == 0x1000,
                    "stationary spin must hold travel and camera-sampled actor yaw");
            RequireActiveAttack(); DrawAndCheckMatrix();
        }
        RequireDrawYaw(59296);
        Require(animationUpdates == 3 && attackRegistrations == 3 && startSounds == 1 && slipSounds == 1,
                "spin retains animation, collision registration, start and floor sounds");
    } else if (name == "steering" || name == "release-stick") {
        stickActive = true; stickYaw = 0x1000; cameraInputYaw = 0x1000;
        MmForm_Action_DekuSpin(&player, &play);
        Require(player.yaw == 0x1320, "spin keeps its existing steering rate and camera-relative input");
        Require(gFormState.dekuSpinRotAccum == 19200, "steering must not be counted again in visual spin offset");
        Require(player.actor.shape.rot.y == 0x1320, "steering changes camera-sampled yaw only by real travel turn");
        DrawAndCheckMatrix(); RequireDrawYaw(24096);
        if (name == "release-stick") {
            stickActive = false; OotNeutralIdleTail(&player, &play);
            Require(player.yaw == 0x1320, "OoT neutral idle must not feed visual spin into travel yaw");
            MmForm_Action_DekuSpin(&player, &play);
            Require(player.yaw == 0x1320, "released stick keeps travel yaw");
            DrawAndCheckMatrix(); RequireDrawYaw(42496);
        } else {
            MmForm_Action_DekuSpin(&player, &play);
            Require(player.yaw == 0x1640 && gFormState.dekuSpinRotAccum == 37600, "continued steering remains independent of spin");
            DrawAndCheckMatrix(); RequireDrawYaw(43296);
        }
        RequireActiveAttack();
    } else if (name == "render-trail") {
        gFormState.dekuSpinRotAccum = 0x4000;
        DrawAndCheckMatrix(); RequireDrawYaw(0x5000);
        Require(trailVertices == 1 && drawCalls == 1, "spin skeleton still feeds its hat trail");
        const double angle = BINANG_TO_RAD(0x5000);
        Require(Near(trailTip.x, 10 + 30 * std::cos(angle)) && Near(trailTip.z, 30 - 30 * std::sin(angle)),
                "hat trail tip follows rendered spin matrix");
        Require(Near(trailBase.x, 10 + 23 * std::cos(angle)) && Near(trailBase.z, 30 - 23 * std::sin(angle)),
                "hat trail base follows rendered spin matrix");
    } else if (name == "draw-gates") {
        gFormState.dekuSpinRotAccum = 0x4000;
        for (s32 form = 0; form < MM_PLAYER_FORM_MAX; ++form) {
            gFormState.currentForm = static_cast<MmPlayerTransformation>(form);
            DrawAndCheckMatrix(); RequireDrawYaw(form == MM_PLAYER_FORM_DEKU ? 0x5000 : 0x1000);
        }
        gFormState.currentForm = MM_PLAYER_FORM_DEKU;
        for (MmFormStateId state : {MMFORM_STATE_INACTIVE, MMFORM_STATE_TRANSFORMING, MMFORM_STATE_DETRANSFORMING}) {
            gFormState.state = state; DrawAndCheckMatrix(); RequireDrawYaw(0x1000);
        }
        gFormState.state = MMFORM_STATE_ACTIVE;
        for (GoronActionId action : {GORON_ACT_IDLE, GORON_ACT_RUN, MMFORM_ACT_DEKU_FLOWER, MMFORM_ACT_DEKU_FLY,
                                   MMFORM_ACT_OOT_ACTION, GORON_ACT_DAMAGE}) {
            gFormState.goronAction = action; DrawAndCheckMatrix(); RequireDrawYaw(0x1000);
        }
    } else if (name == "end-idle" || name == "end-run" || name == "water-void") {
        gFormState.dekuSpinTimer = 1;
        if (name == "end-run") { stickActive = true; stickYaw = 0x1000; player.linearVelocity = 3; }
        if (name == "water-void") { gFormState.dekuHopsRemaining = 0; player.actor.yDistToWater = 1; }
        MmForm_Action_DekuSpin(&player, &play);
        RequireFinished(name == "end-run" ? GORON_ACT_RUN : name == "water-void" ? MMFORM_ACT_WATER_VOID : GORON_ACT_IDLE);
    } else if (name == "anti-reversal") {
        stickActive = true; stickYaw = static_cast<s16>(0x9000); player.linearVelocity = 3;
        MmForm_Action_DekuSpin(&player, &play);
        Require(player.yaw == 0x1000 && Near(player.linearVelocity, 1.5), "spin retains anti-reversal deceleration");
        Require(player.actor.shape.rot.y == 0x1000, "reversal attempt must not expose visual yaw to camera");
        RequireActiveAttack(); DrawAndCheckMatrix(); RequireDrawYaw(23296);
    } else {
        Require(false, "unknown regression case");
    }
    std::printf("PASS %s\n", name.c_str());
}
