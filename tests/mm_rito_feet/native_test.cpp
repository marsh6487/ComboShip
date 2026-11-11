#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <initializer_list>
extern "C" {
#include "global.h"
#include "mods/forms/custom_forms.h"
#include "mods/forms/rito_foot_contact.h"
#include "objects/gameplay_keep/gameplay_keep.h"
extern MtxF* sMatrixStack;
extern MtxF* sCurrentMatrix;
void Player_AdjustSingleLeg(PlayState*, Player*, SkelAnime*, Vec3f*, Vec3s*, s32, s32, s32);
void RitoFeetTest_NativeLeg(PlayState*, Player*, SkelAnime*, Vec3f*, Vec3s*, s32, s32, s32);
f32 qNaN0x10000 = NAN;
f32 Math_SinS(s16 angle) { return sins(angle) * (1.0f / 32767); }
f32 Math_CosS(s16 angle) { return coss(angle) * (1.0f / 32767); }
void FrameInterpolation_RecordMatrixPush() {}
void FrameInterpolation_RecordMatrixPop() {}
void FrameInterpolation_RecordMatrixPut(MtxF*) {}
void FrameInterpolation_RecordMatrixTranslateRotateZYX(Vec3f*, Vec3s*) {}
void FrameInterpolation_RecordMatrixTranslate(f32, f32, f32, u8) {}
void FrameInterpolation_RecordMatrixScale(f32, f32, f32, u8) {}
PlayState* gPlayState;
}

static int sActiveForm = CUSTOM_FORM_RITO;
static int sAdult = 1;
static float sRenderScale = .01f, sRenderYOffset;
static bool sMario, sWolf, sSurf;
static unsigned sRays, sFlames;
static f32 sFloorY;
static bool sNoFloorHit;
static s32 sFloorBg = BGCHECK_SCENE;
static CollisionPoly sFloor;
static struct { u8 state; } sRito, sRitoBow;
#define RITO_FLY_OFF 0
#define RITO_BOW_OFF 0
#define RITO_DRAW_Y_BASE -1059.0f
#define PLAYER_STATE3_PAUSE_ACTION_FUNC PLAYER_STATE3_4
#define FORMS_STATE1_LOADING PLAYER_STATE1_200
#define FORMS_STATE1_CLIMBING_LEDGE PLAYER_STATE1_4
#define FORMS_STATE1_GETTING_ITEM PLAYER_STATE1_400
#define FORMS_STATE1_DAMAGED PLAYER_STATE1_4000000
#define KEATON_ROOT_SCALE_ADULT .335f
#define KEATON_ROOT_SCALE_CHILD .476f
#define GERUDO_ROOT_SCALE_CHILD .71f

extern "C" {
s32 CustomForms_ActiveForm() { return sActiveForm; }
s32 AdultLink_IsActive() { return sAdult; }
f32 CVarGetFloat(const char* name, f32 fallback) {
    if (!strcmp(name, "gAdultLink.Scale")) return sRenderScale;
    if (!strcmp(name, "gAdultLink.YOffset")) return sRenderYOffset;
    return fallback;
}
u8 Sm64Mario_ShouldHideLink() { return sMario; }
u8 WolfLinkForm_IsSelected() { return sWolf; }
u8 KiteSurf_IsActive() { return sSurf; }
f32 BgCheck_EntityRaycastFloor3(CollisionContext*, CollisionPoly** out, s32* bg, Vec3f*) {
    ++sRays; *out = sNoFloorHit ? nullptr : &sFloor; *bg = sFloorBg; return sNoFloorHit ? BGCHECK_Y_MIN : sFloorY;
}
FloorType SurfaceType_GetFloorType(CollisionContext*, CollisionPoly*, s32) { return FLOOR_TYPE_2; }
u32 SurfaceType_IsWallDamage(CollisionContext*, CollisionPoly*, s32) { return 0; }
void EffectSsGFire_Spawn(PlayState*, Vec3f*) { ++sFlames; }
int ResourceMgr_OTRSigCheck(char*);
}
#include "native_query.inc"
#include "fixture.inc"

static PlayState sPlay;
static Player sPlayer;
static PlayerAgeProperties sAge;

static void Reset() {
    std::memset(&sPlay, 0, sizeof(sPlay));
    std::memset(&sPlayer, 0, sizeof(sPlayer));
    sActiveForm = CUSTOM_FORM_RITO; sAdult = 1; sRenderScale = .01f; sRenderYOffset = 0;
    sMario = sWolf = sSurf = false; sRito.state = sRitoBow.state = 0;
    gPlayState = &sPlay; sPlay.actorCtx.actorLists[ACTORCAT_PLAYER].first = &sPlayer.actor;
    sPlayer.transformation = PLAYER_FORM_HUMAN;
    sPlayer.actor.scale = { .01f, .01f, .01f };
    sPlayer.actor.shape.yOffset = RITO_DRAW_Y_BASE;
    sPlayer.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    sAge.unk_08 = 1; sPlayer.ageProperties = &sAge;
    sPlayer.skelAnime.animation = (PlayerAnimationHeader*)gPlayerAnim_link_normal_run_free;
    sFloor = {}; sFloor.normal.y = 32767; sFloorBg = BGCHECK_SCENE; sFloorY = 0; sNoFloorHit = false;
}

static bool Eligible() { return RitoForm_CanAdjustFeet(&sPlay, &sPlayer, &sPlayer.skelAnime); }

static void QueryControls() {
    Reset(); assert(Eligible());
    const char* allowed[] = { gPlayerAnim_link_normal_walk_free, gPlayerAnim_link_normal_walk,
        gPlayerAnim_link_fighter_walk_long, gPlayerAnim_link_normal_run_free, gPlayerAnim_link_normal_run,
        gPlayerAnim_link_fighter_run, gPlayerAnim_link_fighter_run_long };
    for (auto* clip : allowed) { sPlayer.skelAnime.animation = (PlayerAnimationHeader*)clip; assert(Eligible()); }
    for (auto* clip : { gPlayerAnim_link_normal_wait_free, gPlayerAnim_link_normal_jump,
                        gPlayerAnim_link_normal_landing, gPlayerAnim_link_bow_bow_wait }) {
        sPlayer.skelAnime.animation = (PlayerAnimationHeader*)clip; assert(!Eligible());
    }
    PlayerAnimationHeader binary{};
    sPlayer.skelAnime.animation = &binary; assert(!Eligible());
    sPlayer.skelAnime.animation = nullptr; assert(!Eligible());
    sPlayer.skelAnime.animation = (PlayerAnimationHeader*)1; assert(!Eligible());
    Reset(); sActiveForm = CUSTOM_FORM_NONE; assert(!Eligible());
    Reset(); sAdult = 0; assert(!Eligible());
    Reset(); sRenderScale = .005f; assert(!Eligible());
    Reset(); sRenderScale = .02f; assert(!Eligible());
    Reset(); sRenderYOffset = 1; assert(!Eligible());
    Reset(); sPlayer.actor.scale.x = .02f; assert(!Eligible());
    Reset(); sPlayer.actor.scale.y = -.01f; assert(!Eligible());
    Reset(); sPlayer.actor.scale.z = .02f; assert(!Eligible());
    Reset(); sPlayer.actor.shape.yOffset++; assert(!Eligible());
    Reset(); sAge.unk_08 = .64f; assert(!Eligible());
    Reset(); sPlayer.ageProperties = nullptr; assert(!Eligible());
    Reset(); sPlayer.unk_AB8 = 10; assert(!Eligible());
    Reset(); sPlayer.unk_ABC = 10; assert(!Eligible());
    Reset(); sMario = true; assert(!Eligible());
    Reset(); sWolf = true; assert(!Eligible());
    Reset(); sSurf = true; assert(!Eligible());
    Reset(); sPlayer.transformation = PLAYER_FORM_DEKU; assert(!Eligible());
    Reset(); sPlayer.actor.bgCheckFlags = 0; assert(!Eligible());
    Reset(); sPlay.pauseCtx.state = PAUSE_STATE_OPENING_0; assert(!Eligible());
    Reset(); sPlay.msgCtx.msgMode = MSGMODE_TEXT_START; assert(!Eligible());
    Reset(); sPlay.csCtx.state = CS_STATE_RUN; assert(!Eligible());
    for (int state = 1; state < 9; ++state) { Reset(); sRito.state = state; assert(!Eligible()); }
    for (int state = 1; state < 8; ++state) { Reset(); sRitoBow.state = state; assert(!Eligible()); }
    for (u32 flag : { PLAYER_STATE1_DEAD, PLAYER_STATE1_TALKING, PLAYER_STATE1_CARRYING_ACTOR,
        PLAYER_STATE1_IN_CUTSCENE, PLAYER_STATE1_INPUT_DISABLED, PLAYER_STATE1_IN_WATER,
        PLAYER_STATE1_CLIMBING_LADDER, PLAYER_STATE1_FIRST_PERSON, FORMS_STATE1_LOADING,
        FORMS_STATE1_CLIMBING_LEDGE, FORMS_STATE1_GETTING_ITEM, FORMS_STATE1_DAMAGED }) {
        Reset(); sPlayer.stateFlags1 = flag; assert(!Eligible());
    }
    Reset(); sPlayer.stateFlags3 = PLAYER_STATE3_PAUSE_ACTION_FUNC; assert(!Eligible());
    Reset(); Player peer = sPlayer; assert(!RitoForm_CanAdjustFeet(&sPlay, &peer, &peer.skelAnime));
    SkelAnime other{}; assert(!RitoForm_CanAdjustFeet(&sPlay, &sPlayer, &other));
    assert(!RitoForm_CanAdjustFeet(nullptr, &sPlayer, &sPlayer.skelAnime));
}

static RitoFeetPose DrawFoot(const MtxF& parent, Vec3f position, const Vec3s* joints, Vec3s rotation, int thigh) {
    RitoFeetPose result;
    result.joints[0] = rotation; result.joints[1] = joints[thigh + 1]; result.joints[2] = joints[thigh + 2];
    Matrix_Push(); Matrix_Put(const_cast<MtxF*>(&parent));
    Matrix_TranslateRotateZYX(&position, &rotation); Matrix_Get(&result.segments[0]);
    for (int segment = 1; segment < 3; ++segment) {
        Vec3f translation = { (f32)kOffsets[thigh + segment - 1].x,
            (f32)kOffsets[thigh + segment - 1].y, (f32)kOffsets[thigh + segment - 1].z };
        Vec3s rot = joints[thigh + segment]; Matrix_TranslateRotateZYX(&translation, &rot);
        Matrix_Get(&result.segments[segment]);
    }
    Matrix_Pop(); return result;
}

static float RenderedContact(const RitoFeetPose& pose, int side) {
    float low = INFINITY;
    for (auto& vertex : kFootTriangles[side]) {
        Vec3f point; Matrix_MultVec3fExt(const_cast<Vec3f*>(&vertex.point), &point,
                                        const_cast<MtxF*>(&pose.segments[vertex.segment]));
        low = std::fmin(low, point.y);
    }
    return low;
}

using ReferenceVector = std::array<double, 3>;
static double ReferenceDot(const ReferenceVector& a, const ReferenceVector& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
static ReferenceVector ReferenceDelta(const MtxF& a, const MtxF& b) {
    return { a.xw - b.xw, a.yw - b.yw, a.zw - b.zw };
}
static void IndependentOrientationAndKnee(const RitoFeetPose& before, const RitoFeetPose& after) {
    const double angleBudget = 9 * (1.0 / 1024 + 17 * (2.0 * (double)M_PIf / 65536) + 2.0 / 32767);
    for (int column = 0; column < 3; ++column) {
        ReferenceVector oldAxis = { before.segments[2].mf[column][0], before.segments[2].mf[column][1],
                                    before.segments[2].mf[column][2] };
        ReferenceVector newAxis = { after.segments[2].mf[column][0], after.segments[2].mf[column][1],
                                    after.segments[2].mf[column][2] };
        double cosine = ReferenceDot(oldAxis, newAxis) /
                        std::sqrt(ReferenceDot(oldAxis, oldAxis) * ReferenceDot(newAxis, newAxis));
        assert(std::acos(std::fmin(1.0, std::fmax(-1.0, cosine))) <= angleBudget);
    }
    ReferenceVector axis = ReferenceDelta(after.segments[2], after.segments[0]);
    ReferenceVector oldKnee = ReferenceDelta(before.segments[1], before.segments[0]);
    ReferenceVector newKnee = ReferenceDelta(after.segments[1], after.segments[0]);
    const double axisLength2 = ReferenceDot(axis, axis);
    double oldProjection = ReferenceDot(oldKnee, axis) / axisLength2;
    double newProjection = ReferenceDot(newKnee, axis) / axisLength2;
    for (int component = 0; component < 3; ++component) {
        oldKnee[component] -= axis[component] * oldProjection;
        newKnee[component] -= axis[component] * newProjection;
    }
    assert(ReferenceDot(oldKnee, newKnee) > 0); // same bend hemisphere, independent of production pole math
}

static void GeneratedDataControls() {
    for (int side = 0; side < 2; ++side) {
        unsigned footVertex = 0, seamVertex = 0;
        for (const auto& source : kFootTriangles[side]) {
            const Vec3f& generated = source.segment == 2 ? sRitoFeetSupport[side][footVertex++] :
                                                          sRitoFeetShinSeam[side][seamVertex++];
            assert(!std::memcmp(&source.point, &generated, sizeof(Vec3f)));
        }
        assert(footVertex == 104 && seamVertex == 6);
        for (int segment = 0; segment < 2; ++segment)
            assert(!std::memcmp(&sRitoFeetBoneOffsets[side][segment], &kOffsets[(side ? 7 : 4) + segment], sizeof(Vec3s)));
    }
}

static void FallbackControls(const MtxF& parent, Vec3f position, const RitoFeetPose& nativePose, int side) {
    RitoFeetPose result, unchanged;
    std::memset(&result, 0xCD, sizeof(result)); unchanged = result;
    const double bound = RenderedContact(nativePose, side) + 1;
    MtxF bad = parent; bad.xx = -bad.xx; bad.yx = -bad.yx; bad.zx = -bad.zx;
    assert(!RitoFeet_Solve(&bad, &position, &nativePose, side, bound, &result));
    assert(!std::memcmp(&result, &unchanged, sizeof(result)));
    bad = parent; bad.xx = bad.yx = bad.zx = 0;
    assert(!RitoFeet_Solve(&bad, &position, &nativePose, side, bound, &result));
    assert(!std::memcmp(&result, &unchanged, sizeof(result)));
    bad = parent; bad.xy += parent.xx * .5f; bad.yy += parent.yx * .5f; bad.zy += parent.zx * .5f;
    assert(!RitoFeet_Solve(&bad, &position, &nativePose, side, bound, &result));
    assert(!std::memcmp(&result, &unchanged, sizeof(result)));
    assert(!RitoFeet_Solve(&parent, &position, &nativePose, side, bound + 100, &result));
    assert(!std::memcmp(&result, &unchanged, sizeof(result)));
    RitoFeetPose singular = nativePose;
    singular.segments[1].xw = singular.segments[0].xw;
    singular.segments[1].yw = singular.segments[0].yw;
    singular.segments[1].zw = singular.segments[0].zw;
    assert(!RitoFeet_Solve(&parent, &position, &singular, side, bound, &result));
    assert(!std::memcmp(&result, &unchanged, sizeof(result)));
    assert(!RitoFeet_Solve(&parent, &position, &nativePose, side, NAN, &result));
    assert(!std::memcmp(&result, &unchanged, sizeof(result)));
}

static void ExcludedSeamControls(const MtxF& parent, Vec3f position, const Vec3s* frame, int thigh) {
    for (int control = 0; control < 19; ++control) {
        Reset();
        PlayerAnimationHeader binary{};
        alignas(2) static const char custom[] = "__OTR__misc/link_animetion/not_a_native_gait";
        switch (control) {
            case 0: sActiveForm = CUSTOM_FORM_NONE; break;
            case 1: sAdult = 0; break;
            case 2: sRenderScale = .005f; break;
            case 3: sRenderYOffset = 1; break;
            case 4: sRito.state = 1; break;
            case 5: sRitoBow.state = 1; break;
            case 6: sPlayer.actor.bgCheckFlags = 0; break;
            case 7: sFloor.normal = { 8192, 31726, 0 }; break;
            case 8: sFloorBg = 0; break;
            case 9: sNoFloorHit = true; break;
            case 10: sMario = true; break;
            case 11: sWolf = true; break;
            case 12: sSurf = true; break;
            case 13: sPlayer.skelAnime.animation = &binary; break;
            case 14: sPlayer.skelAnime.animation = (PlayerAnimationHeader*)custom; break;
            case 15: sPlay.pauseCtx.state = PAUSE_STATE_OPENING_0; break;
            case 16: sPlay.msgCtx.msgMode = MSGMODE_TEXT_START; break;
            case 17: sPlay.csCtx.state = CS_STATE_RUN; break;
            case 18: sPlay.actorCtx.actorLists[ACTORCAT_PLAYER].first = nullptr; break;
        }
        Vec3s nativeJoints[22], actualJoints[22];
        std::memcpy(nativeJoints, frame, sizeof(nativeJoints));
        std::memcpy(actualJoints, frame, sizeof(actualJoints));
        Vec3s nativeRotation = frame[thigh], actualRotation = nativeRotation;
        sPlayer.skelAnime.jointTable = nativeJoints; sRays = sFlames = 0; Matrix_Put(const_cast<MtxF*>(&parent));
        RitoFeetTest_NativeLeg(&sPlay, &sPlayer, &sPlayer.skelAnime, &position, &nativeRotation, thigh, thigh + 1, thigh + 2);
        unsigned nativeRays = sRays, nativeFlames = sFlames;
        sPlayer.skelAnime.jointTable = actualJoints; sRays = sFlames = 0; Matrix_Put(const_cast<MtxF*>(&parent));
        Player_AdjustSingleLeg(&sPlay, &sPlayer, &sPlayer.skelAnime, &position, &actualRotation, thigh, thigh + 1, thigh + 2);
        assert(sRays == nativeRays && sFlames == nativeFlames);
        assert(!std::memcmp(nativeJoints, actualJoints, sizeof(nativeJoints)));
        assert(!std::memcmp(&nativeRotation, &actualRotation, sizeof(Vec3s)));
    }
    Reset();
}

int main() {
    MtxF stack[20], world[21]; sMatrixStack = stack; sCurrentMatrix = stack;
    GeneratedDataControls(); QueryControls();
    unsigned corrected = 0, unchanged = 0, excluded = 0;
    float idleMinimum[2] = { INFINITY, INFINITY };
    for (int variant = 0; variant < 6; ++variant) {
        Reset(); const float floor = variant == 5 ? 17 : 0; sFloorY = floor;
        for (unsigned frameIndex = 0; frameIndex < sizeof(kFrames) / sizeof(kFrames[0]); ++frameIndex) {
            Vec3s frame[22]; std::memcpy(frame, kFrames[frameIndex], sizeof(frame));
            if (kIsGait[frameIndex] && (variant == 3 || variant == 4)) {
                int delta = variant == 3 ? 256 : -256;
                for (int joint : { 4, 7 }) { frame[joint].x += delta; frame[joint].y -= delta; }
                for (int joint : { 6, 9 }) { frame[joint].x -= delta; frame[joint].y += delta; }
            }
            for (int limb = 0; limb < 21; ++limb) {
                if (kParents[limb] < 0) {
                    Matrix_Translate(variant == 5 ? 100 : 0, floor - 10.59f, variant == 5 ? 200 : 0, MTXMODE_NEW);
                    Vec3f zero{}; Vec3s yaw = { 0, (s16)(variant == 1 ? 0x2000 : variant == 2 ? -0x2000 : 0), 0 };
                    Matrix_TranslateRotateZYX(&zero, &yaw); Matrix_Scale(.01f, .01f, .01f, MTXMODE_APPLY);
                } else Matrix_Put(&world[kParents[limb]]);
                Vec3f translation = { (f32)kOffsets[limb].x, (f32)kOffsets[limb].y, (f32)kOffsets[limb].z };
                if (limb == 0) translation = { frame[0].x * .7036f, frame[0].y * .7036f, frame[0].z * .7036f };
                Matrix_TranslateRotateZYX(&translation, &frame[limb + 1]); Matrix_Get(&world[limb]);
            }
            sPlayer.skelAnime.animation = (PlayerAnimationHeader*)kAnimations[frameIndex];
            for (int side = 0; side < 2; ++side) {
                const int thigh = side ? 7 : 4; MtxF parent = world[kParents[thigh - 1]];
                Vec3f position = { (f32)kOffsets[thigh - 1].x, (f32)kOffsets[thigh - 1].y, (f32)kOffsets[thigh - 1].z };
                Vec3s baseline[22], candidate[22]; std::memcpy(baseline, frame, sizeof(frame));
                std::memcpy(candidate, frame, sizeof(frame)); Vec3s nativeRot = frame[thigh], rot = nativeRot;
                sRays = sFlames = 0; sPlayer.skelAnime.jointTable = baseline;
                Matrix_Put(&parent); RitoFeetTest_NativeLeg(&sPlay, &sPlayer, &sPlayer.skelAnime, &position,
                                                          &nativeRot, thigh, thigh + 1, thigh + 2);
                unsigned nativeRays = sRays, nativeFlames = sFlames;
                assert(nativeRays == 1); sRays = sFlames = 0; sPlayer.skelAnime.jointTable = candidate;
                Player playerBytes = sPlayer; Matrix_Put(&parent);
                Player_AdjustSingleLeg(&sPlay, &sPlayer, &sPlayer.skelAnime, &position, &rot, thigh, thigh + 1, thigh + 2);
                assert(sRays == nativeRays && sFlames == nativeFlames);
                assert(std::memcmp(&sPlayer, &playerBytes, sizeof(Player)) == 0);
                MtxF after; Matrix_Get(&after); assert(std::memcmp(&after, &parent, sizeof(parent)) == 0);
                for (int joint = 0; joint < 22; ++joint)
                    if (joint < thigh || joint > thigh + 2) assert(!std::memcmp(&candidate[joint], &baseline[joint], sizeof(Vec3s)));
                RitoFeetPose nativePose = DrawFoot(parent, position, baseline, nativeRot, thigh);
                RitoFeetPose actualPose = DrawFoot(parent, position, candidate, rot, thigh);
                if (!kIsGait[frameIndex]) {
                    assert(!std::memcmp(baseline, candidate, sizeof(frame)) && !std::memcmp(&rot, &nativeRot, sizeof(rot)));
                    if (variant == 0 && frameIndex < 89) {
                        idleMinimum[side] = std::fmin(idleMinimum[side], RenderedContact(nativePose, side));
                        if (frameIndex == 88)
                            assert(std::fabs(idleMinimum[side] - sRitoFeetIdleFloorOffset[side]) <=
                                   8 * FLT_EPSILON * (std::fabs(idleMinimum[side]) + 1));
                    }
                    ++excluded;
                } else {
                    assert(RenderedContact(actualPose, side) >= floor + sRitoFeetIdleFloorOffset[side]);
                    if (frameIndex == 122 && variant == 0) {
                        FallbackControls(parent, position, nativePose, side);
                        ExcludedSeamControls(parent, position, frame, thigh);
                    }
                    if (std::memcmp(baseline, candidate, sizeof(frame))) {
                        assert(!std::memcmp(&rot, &candidate[thigh], sizeof(rot)));
                        IndependentOrientationAndKnee(nativePose, actualPose); ++corrected;
                    } else ++unchanged;
                }
            }
        }
    }
    assert(corrected >= 222 && corrected + unchanged == 6 * kGaitFeet);
    std::printf("PASS Rito C post-native contact: %u corrected, %u unchanged, %u exact idle controls; 110 vertices/foot, one native ray/effect path\n",
                corrected, unchanged, excluded);
}
