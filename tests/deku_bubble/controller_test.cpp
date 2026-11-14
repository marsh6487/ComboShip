#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using u8 = uint8_t; using s8 = int8_t; using u16 = uint16_t; using s16 = int16_t;
using u32 = uint32_t; using s32 = int32_t; using f32 = float;
using GraphicsContext = int;
struct Vec3f { f32 x, y, z; };
struct Vec3s { s16 x, y, z; };
struct MtxF { union { f32 mf[4][4]; struct { f32 xx,yx,zx,wx,xy,yy,zy,wy,xz,yz,zz,wz,xw,yw,zw,ww; }; }; };
struct CollisionPoly { Vec3s normal; };
struct Actor {
    struct { Vec3f pos; Vec3s rot; } world, focus;
    struct { Vec3s rot; } shape;
    Vec3f projectedPos; s32 bgCheckFlags, id; void (*update)();
};
struct Player; struct PlayState;
using UpperActionFunc = s32 (*)(Player*, PlayState*);
struct LinkAnimationHeader { int unused; };
struct SkelAnime { LinkAnimationHeader* animation; };
struct Player {
    Actor actor; u32 stateFlags1, stateFlags2, stateFlags3;
    s32 heldItemAction, itemAction, heldItemId, heldItemButton, currentMask;
    s16 unk_860, unk_836, unk_834, unk_6AD, unk_6AE_rotFlags;
    s16 meleeWeaponState, meleeWeaponAnimation, unk_A73;
    f32 linearVelocity; Vec3s upperLimbRot;
    Actor* heldActor; SkelAnime skelAnime, upperSkelAnime; UpperActionFunc upperActionFunc;
};
struct Input { struct { u16 button; } cur, press, rel; };
struct PlayState {
    struct { Input input[1]; int frames; GraphicsContext* gfxCtx; } state;
    int shootingGalleryStatus, bombchuBowlingStatus, actorCtx, colCtx, colChkCtx, gameplayFrames;
    MtxF viewProjectionMtxF;
};
struct ColliderInfo { int elemType; ColliderInfo* atHitInfo; };
struct Collider { u32 atFlags, ocFlags1; Actor* at; };
struct ColliderCylinder {
    Collider base; ColliderInfo info;
    struct { s16 radius,height; Vec3s pos; } dim;
};
struct ItemCameraState {};
struct SaveContext {
    int magic, magicState;
    struct { int buttonItems[8], cButtonSlots[7]; } equips;
    int buttonStatus[8];
} gSaveContext;
struct Gfx { struct { uintptr_t w0, w1; } words; };
struct GraphicsState { int unused; };
enum {
    MM_PLAYER_FORM_HUMAN, MM_PLAYER_FORM_GORON, MM_PLAYER_FORM_ZORA, MM_PLAYER_FORM_DEKU,
    MM_PLAYER_FORM_GARO = 7, MM_PLAYER_FORM_KEATON = 9,
    ITEM_NONE = 255, ITEM_NONE_FE = 254, ITEM_SLINGSHOT = 6, ITEM_BOOMERANG = 14,
    ITEM_SWORD_KNIFE = 61, ITEM_SWORD_BGS = 62, ITEM_BOMBCHU = 9,
    ITEM_EXT_BUTTON = 200, ITEM_EXT_SWORD_1 = 210, ITEM_EXT_BOOTS_2 = 224, ITEM_EXT_BOOTS_3 = 225,
    PLAYER_IA_NONE, PLAYER_IA_SLINGSHOT, PLAYER_IA_BOOMERANG,
    PLAYER_IA_BOW_FIRE = 110, PLAYER_IA_BOW_0E = 115, PLAYER_IA_FISHING_POLE = 120,
    PLAYER_IA_MASK_KEATON = 130, PLAYER_IA_MM_MASK_BUNNY, PLAYER_IA_MM_MASK_KEATON, PLAYER_IA_MM_MASK_TRUTH,
    PLAYER_MASK_NONE, PLAYER_MASK_BUNNY, PLAYER_MASK_KEATON, PLAYER_MASK_TRUTH,
    MMFORM_ACT_BOOMERANG_THROW = 1000,
    BTN_B = 1, BTN_A = 2, BTN_R = 4, BTN_CUP = 8, BTN_CLEFT = 16, BTN_CDOWN = 32, BTN_CRIGHT = 64,
    BTN_DUP = 128, BTN_DDOWN = 256, BTN_DLEFT = 512, BTN_DRIGHT = 1024,
    PLAYER_STATE1_DAMAGED = 1 << 2, PLAYER_STATE1_DEAD = 1 << 7,
    PLAYER_STATE1_IN_CUTSCENE = 1 << 29, PLAYER_STATE1_ITEM_IN_HAND = 1 << 23,
    PLAYER_STATE1_FIRST_PERSON = 1 << 9, PLAYER_STATE1_READY_TO_FIRE = 1 << 4,
    PLAYER_STATE1_CARRYING_ACTOR = 1 << 11,
    PLAYER_STATE1_USING_BOOMERANG = 1 << 24,
    PLAYER_STATE3_PAUSE_ACTION_FUNC = 1 << 3,
    UNK6AE_ROT_UPPER_Z = 4, BGCHECKFLAG_GROUND = 1,
    MAGIC_STATE_IDLE = 0, MAGIC_CONSUME_NOW = 0,
    ARROW_FIRE = 3, ARROW_NORMAL = 2, ACTOR_EN_ARROW = 10, ACTOR_OBJ_SYOKUDAI = 94,
    NA_SE_SY_ERROR = 0, PLAYER_ANIMTYPE_3 = 3, IDLE_ANIM_NONE = 0,
    AT_HIT = 2, AT_BOUNCED = 4, OC1_HIT = 2, ELEMTYPE_UNK4 = 4, BGCHECK_SCENE = 0x7FFF,
    SFX_FLAG = 0x800, MM_NA_SE_PL_DEKUNUTS_FIRE = 0x8E0,
    MM_NA_SE_PL_DEKUNUTS_BUBLE_BREATH = 0x9A1,
    MM_NA_SE_IT_DEKUNUTS_BUBLE_VANISH = 0x1854, MM_NA_SE_IT_DEKUNUTS_BUBLE_SHOT_LEVEL = 0x185A,
    NA_SE_IT_BOW_FLICK, NA_SE_IT_SLING_FLICK,
    VB_PLAYER_UNEQUIP_MASK_WITHOUT_BUTTON, VB_PUTAWAY_BECAUSE_DISABLED_ITEM_BUTTONS,
    VB_OVERRIDE_BUTTON_ITEM_USED, VB_CHANGE_HELD_ITEM_AND_USE_ITEM, VB_GET_ITEM_ON_BUTTON,
    VB_PLAYER_ARROW_MAGIC_CONSUMPTION,
};
#define ARRAY_COUNT(x) (sizeof(x) / sizeof((x)[0]))
#define CHECK_BTN_ALL(state, combo) (((state) & (combo)) == (combo))
#define CHECK_BTN_ANY(state, combo) (((state) & (combo)) != 0)
#define ABS(x) std::abs(x)
#define SQ(x) ((x) * (x))
#define CLAMP(x,lo,hi) std::clamp((x),(lo),(hi))
#define CLAMP_MIN(x,lo) std::max((x),(lo))
#define B_BTN_ITEM gSaveContext.equips.buttonItems[0]
#define C_BTN_ITEM(x) gSaveContext.equips.buttonItems[(x)+1]
#define DPAD_ITEM(x) gSaveContext.equips.buttonItems[(x)+4]
#define CVAR_ENHANCEMENT(x) x
#define MAGIC_REQ(x) (x)
#define GET_PLAYER(play) (&player)
static Player player;
static PlayState play;
static PlayState* gPlayState = &play;
static Input* sControlInput = &play.state.input[0];
static bool sDekuBubbleActive, sZoraBoomerangActive, sUseHeldItem, sHeldItemButtonIsHeldDown;
static bool blocksSword, riding, transformed = true, muted, worldHit;
static int surfaceBg = BGCHECK_SCENE, stonePops, genericPops, atSubmits, upperStarts, fires;
static CollisionPoly surface = {{0, 32767, 0}};
static Vec3f hitPosition = {30, 30, 40};
struct Sound { u16 id; Vec3f* pos; Vec3f atPlay; };
static std::vector<Sound> sounds;
static std::vector<std::pair<u16, Vec3f*>> stopped;
static bool missingAssets;
static int splatDraws;
static std::vector<std::string> resources;
static int bubbleMagicBefore;
static float gChampionSlowFactor = 1.0f;
struct FormState {
    int currentForm = MM_PLAYER_FORM_DEKU, goronAction = 0;
    f32 dekuCheekScale = 1.0f;
    /* STATE */
} gFormState;
/* RESET_AVAILABLE */
static s32 ResolveItemOnButton(PlayState*, s32, s32);
/* NATIVE_DECLARATIONS */
/* BUBBLE_DECLARATIONS */
static void MmForm_ResetDekuBubble(PlayState*);
static void MmForm_UpdateDekuBubbleEffects(PlayState*);
static void MmForm_DrawDekuBubbleEffects(PlayState*);
static s32 Player_IsDekuBubbleActive() { return sDekuBubbleActive; }
static s32 Player_IsZoraBoomerangActive() { return sZoraBoomerangActive; }
static bool TransformMasks_IsTransformed() { return transformed; }
static int MmForm_GetCurrentForm() { return gFormState.currentForm; }
static bool MasterCycle_IsRiding() { return riding; }
static bool MmMaskWear_BlocksSword() { return blocksSword; }
static bool Pacci_UltrahandModeActive() { return false; }
static bool Cryonis_ModeActive() { return false; }
static s32 CVarGetInteger(const char*, s32 defaultValue) { return defaultValue; }
static bool ExtEquip_IsButtonItem(int) { return false; }
static int ExtButton_GetItem(int) { return 0; }
static int ExtEquip_GetLegacyButtonItem(int) { return 0; }
static bool Player_ItemIsItemAction(int, int) { return false; }
static bool Player_ItemIsInUse(Player*, int) { return false; }
static bool func_8008F128(Player*) { return false; }
static bool TransformMasks_HandleFormItemUse(PlayState*, Player*, int) { return true; }
static void ExtEquip_ToggleFromCButton(int) {}
template<typename... Args> static bool GameInteractor_Should(int, bool value, Args...) { return value; }
static bool GameInteractor_Should(int, int, int index, int* item, PlayState* p) {
    *item = ResolveItemOnButton(p, index, *item); return true;
}
static int Player_ItemToItemAction(int item) { return item == ITEM_SLINGSHOT ? PLAYER_IA_SLINGSHOT : PLAYER_IA_NONE; }
static void Player_UseItem(PlayState*, Player*, int) {}
static bool Player_HoldsHookshot(Player*) { return false; }
static bool Player_TryRaiseUpperBodyShield(PlayState*, Player*) { return false; }
static bool Player_IsZTargeting(Player*) { return false; }
static bool func_80834FBC(Player*) { return false; }
static bool func_80834E44(PlayState*) { return false; }
static bool func_80834E7C(PlayState*) { return false; }
static s32 func_8083485C(Player*, PlayState*) { return 0; }
static s32 func_8083501C(Player*, PlayState*) { return 0; }
static s32 func_80835588(Player*, PlayState*) { return 0; }
static void Player_SetUpperActionFunc(Player* p, UpperActionFunc fn) { p->upperActionFunc = fn; }
static s32 func_80834D2C(Player* p, PlayState* state) { return func_8083442C(p, state); }
static s32 func_80834EB8(Player* p, PlayState*) { p->unk_6AD = 2; return 0; }
static bool func_808350A4(PlayState*, Player*) { return false; }
static int func_80834380(PlayState*, Player*, int*, int*) { return 0; }
static bool Magic_RequestChange(PlayState*, int, int) { return true; }
static Actor* Actor_SpawnAsChild(int*, Actor*, PlayState*, int, float,float,float,int,int,int,int) { return nullptr; }
static void Sfx_PlaySfxCentered(int) {}
static void Player_PlaySfx(Player*, int) {}
static void Player_ZeroSpeedXZ(Player* p) { p->linearVelocity = 0; }
static int Player_CheckForIdleAnim(Player*) { return 0; }
static int LinkAnimation_Update(PlayState*, SkelAnime*) { return 0; }
static void LinkAnimation_PlayOnce(PlayState*, SkelAnime*, LinkAnimationHeader*) {}
static void LinkAnimation_PlayLoop(PlayState*, SkelAnime*, LinkAnimationHeader*) {}
static LinkAnimationHeader animations[8], gPlayerAnim_link_bow_side_walk,
    gPlayerAnim_link_bow_bow_shoot_next, gPlayerAnim_link_bow_bow_shoot_end;
static LinkAnimationHeader* D_808543CC[2] = {&animations[0],&animations[1]};
static LinkAnimationHeader* D_808543D4[2] = {&animations[2],&animations[3]};
static u16 D_80854398[2] = {}, D_808543DC[2] = {};
static int sMagicArrowCosts[6] = {};
static u16 sItemButtons[8] = {BTN_B, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT, BTN_DUP, BTN_DDOWN, BTN_DLEFT, BTN_DRIGHT};
static s32 Math_ScaledStepToS(s16* value, s16 target, s16) { *value = target; return 1; }
static f32 Math_SinS(s16 value) { return std::sin(value * (3.14159265358979323846f / 32768.0f)); }
static f32 Math_CosS(s16 value) { return std::cos(value * (3.14159265358979323846f / 32768.0f)); }
static s16 Math_Atan2S(f32 x, f32 y) { return (s16)(std::atan2(y,x) * (32768.0f / 3.14159265358979323846f)); }
static f32 Rand_ZeroFloat(f32) { return 0; }
static f32 Rand_CenteredFloat(f32) { return 0; }
static f32 Math_SmoothStepToF(f32* value, f32 target, f32 fraction, f32 maxStep, f32 minStep) {
    float step = std::clamp((target - *value) * fraction, -maxStep, maxStep);
    if (std::abs(step) < minStep) step = std::copysign(minStep, step);
    if (std::abs(target-*value) < std::abs(step)) *value = target; else *value += step;
    return std::abs(target-*value);
}
static s32 Math_StepToF(f32* value, f32 target, f32 step) {
    if (std::abs(*value-target) <= step) { *value = target; return 1; }
    *value += *value < target ? step : -step; return 0;
}
static void Math_Vec3f_Copy(Vec3f* dest, Vec3f* from) { *dest = *from; }
static void SkinMatrix_Vec3fMtxFMultXYZW(MtxF* matrix, Vec3f* src, Vec3f* dest, f32* w) {
    *dest = {src->x+matrix->xw,src->y+matrix->yw,src->z+matrix->zw}; *w = 1;
}
static void SkinMatrix_Vec3fMtxFMultXYZ(MtxF* matrix, Vec3f* src, Vec3f* dest) {
    f32 w; SkinMatrix_Vec3fMtxFMultXYZW(matrix,src,dest,&w);
}
static s32 MmSfx_IsAvailable() { return !muted; }
static s32 MmSfx_PlayAtPos(u16 id, Vec3f* pos) { sounds.push_back({id,pos,pos ? *pos : Vec3f{}}); return 1; }
static void MmSfx_Stop(u16 id) { stopped.emplace_back(id,nullptr); }
static void MmSfx_StopAtPos(u16 id, Vec3f* pos) { stopped.emplace_back(id,pos); }
static void MmForm_PlaySfx(Player* p, u16 id, u16) { if (!muted) MmSfx_PlayAtPos(id,&p->actor.projectedPos); }
static void Collider_ResetCylinderAT(PlayState* state, Collider* collider) {
    assert(state!=nullptr); collider->atFlags = 0; collider->at = nullptr;
}
static void MmForm_InitBubbleCollider(Player*, PlayState*) { gFormState.bubbleColliderInit = 1; }
static void CollisionCheck_SetAT(PlayState* p, int*, Collider* collider) {
    ++atSubmits; Collider_ResetCylinderAT(p,collider); // actual SoH SetAT contract
    gFormState.bubbleCollider.info.atHitInfo = nullptr;
}
static void CollisionCheck_SetOC(PlayState*, int*, Collider* collider) { collider->ocFlags1 = 0; }
static f32 BgCheck_EntityRaycastFloor3(int*, CollisionPoly** poly, s32*, Vec3f*) { *poly=nullptr; return -10000; }
static bool BgCheck_EntityLineTest1(int*, Vec3f*, Vec3f*, Vec3f* hit, CollisionPoly** poly,
                                  bool,bool,bool,bool,s32* bgId) {
    if (!worldHit) return false;
    *hit=hitPosition; *poly=&surface; *bgId=surfaceBg; return true;
}
static bool BgCheck_ProjectileLineTest(int* ctx, Vec3f* prev, Vec3f* next, Vec3f* hit, CollisionPoly** poly,
                                     bool a,bool b,bool c,bool d,s32* bgId) {
    return BgCheck_EntityLineTest1(ctx,prev,next,hit,poly,a,b,c,d,bgId);
}
static void EffectSsBubble_Spawn(PlayState*, Vec3f*, float,float,float,float) { ++genericPops; }
static void EffectSsStone1_Spawn(PlayState*, Vec3f*, int) { ++stonePops; }
static void MmForm_TrackBoomerangsInFlight(Player*, PlayState*) {}
static void KeatonForm_TickClimb(Player*, PlayState*) {}
static void MmForm_TickZoraBoomerangHold(PlayState*) {}
static void CollisionPoly_GetNormalF(CollisionPoly* poly, f32* x, f32* y, f32* z) {
    *x=poly->normal.x*(1.0f/32767); *y=poly->normal.y*(1.0f/32767); *z=poly->normal.z*(1.0f/32767);
}
#define IS_ZERO(value) (std::abs(value)<0.008f)
// Exact host surface helper, retained to detect accidental substitution for
// MM's different +/-X-wall basis. No identity-matrix fixture hides the defect.
void func_80038A28(CollisionPoly* poly, f32 tx, f32 ty, f32 tz, MtxF* dest) {
    f32 nx;
    f32 ny;
    f32 nz;
    s32 pad;
    f32 phi_f2;
    f32 phi_f14;
    f32 phi_f12;
    f32 inv_phi_f2;
    f32 inv_phi_f14;

    if (poly == NULL) {
        return;
    }
    CollisionPoly_GetNormalF(poly, &nx, &ny, &nz);

    phi_f2 = sqrtf(1.0f - SQ(nx));
    if (!IS_ZERO(phi_f2)) {
        inv_phi_f2 = 1.0f / phi_f2;
        phi_f14 = ny * inv_phi_f2;
        phi_f12 = -(nz * inv_phi_f2);
    } else {
        phi_f14 = sqrtf(1.0f - SQ(ny));
        if (!IS_ZERO(phi_f14)) {
            inv_phi_f14 = (1.0f / phi_f14);
            phi_f12 = nx * inv_phi_f14;
            phi_f2 = -(nz * inv_phi_f14);
        } else {
            phi_f12 = 0.0f;
            phi_f2 = 0.0f;
        }
    }
    dest->xx = phi_f2;
    dest->yx = (-nx) * phi_f14;
    dest->zx = nx * phi_f12;
    dest->xy = nx;
    dest->yy = ny;
    dest->zy = nz;
    dest->yz = phi_f12;
    dest->zz = phi_f14;
    dest->wx = 0.0f;
    dest->wy = 0.0f;
    dest->xz = 0.0f;
    dest->wz = 0.0f;
    dest->xw = tx;
    dest->yw = ty;
    dest->zw = tz;
    dest->ww = 1.0f;
}
static s32 Math3D_CosOut(Vec3f* a, Vec3f* b, f32* out) {
    *out=(a->x*b->x+a->y*b->y+a->z*b->z)/
        std::sqrt((SQ(a->x)+SQ(a->y)+SQ(a->z))*(SQ(b->x)+SQ(b->y)+SQ(b->z)));
    return 1;
}
static Gfx display[4096], nativeFlat[2], nativeSliding[2], gEmptyDL[1];
static Gfx* displayPtr=display;
static Gfx* MmAssets_LoadDisplayListGraphStrict(const char* path) {
    resources.emplace_back(path);
    if (missingAssets) return nullptr;
    return std::string(path).find("Sliding")!=std::string::npos ? nativeSliding : nativeFlat;
}
static void* MmAssets_LoadResourceStrict(const char* path) {
    resources.emplace_back(path); return missingAssets ? nullptr : &display[0];
}
static void Matrix_Push() {}
static void Matrix_Pop() {}
static void Matrix_Mult(MtxF*, int) {}
static void Matrix_RotateY(f32, int) {}
static void Matrix_Scale(f32,f32,f32,int) {}
static void Gfx_SetupDL_25Xlu(GraphicsContext*) {}
static Gfx* Gfx_TwoTexScrollEx(GraphicsContext*, int,int,int,int,int,int,int,int,int,int,int,int,int,int) { return display; }
static void DrawList(Gfx*,Gfx* dl) { if (dl==nativeFlat || dl==nativeSliding) ++splatDraws; }
#define OPEN_DISPS(ctx) ((void)0)
#define CLOSE_DISPS(ctx) ((void)0)
#define POLY_XLU_DISP displayPtr
#define MTXMODE_NEW 0
#define MTXMODE_APPLY 1
#define BINANG_TO_RAD(value) ((value)*(3.14159265358979323846f / 32768.0f))
#define MATRIX_NEWMTX(ctx) nullptr
#define G_MTX_NOPUSH 0
#define G_MTX_LOAD 0
#define G_MTX_MODELVIEW 0
#define G_RM_FOG_SHADE_A 0
#define G_RM_ZB_XLU_DECAL2 0
#define gSPMatrix(packet,matrix,flags) ((void)(packet))
#define gDPSetRenderMode(packet,a,b) ((void)(packet))
#define gDPSetPrimColor(packet,a,b,c,d,e,f) ((void)(packet))
#define gSPSegment(packet,segment,address) ((void)(packet))
#define gSPDisplayList(packet,dl) DrawList(packet,dl)
#define MMFORM_LOG(...) ((void)0)
/* EFFECT */
/* RESOLVER */
/* BUBBLE_FUNCTIONS */
/* NATIVE_PLAYER */
static void TickCharge(Player* player=&::player, PlayState* play=&::play) { /* CHARGE */ }
static void TickFrame(Player* player=&::player, PlayState* play=&::play) { /* FRAME_PREFIX */ }
static void nativeFrame(bool held, u16 pressed=0) {
    play.state.input[0] = {}; if (held) play.state.input[0].cur.button = BTN_B;
    play.state.input[0].press.button=pressed;
    sUseHeldItem = sHeldItemButtonIsHeldDown = false;
    Player_ProcessItemButtons(&player,&play);
    player.upperActionFunc(&player,&play);
    TickCharge(); TickFrame(); ++play.gameplayFrames; ++play.state.frames;
}
static void fire(float charge = 16.0f) {
    gFormState.bubbleCharge = charge;
    MmForm_FireDekuBubble(&player,&play);
}
static int count(u16 id) { return std::count_if(sounds.begin(),sounds.end(),[&](const auto& s){return s.id==id;}); }
static void reset() {
    gFormState = {}; gFormState.currentForm=MM_PLAYER_FORM_DEKU;
    player = {}; play = {}; gSaveContext = {}; gSaveContext.magic = 30;
    player.actor.world.pos={0,0,0}; player.actor.projectedPos={1,2,3};
    player.actor.bgCheckFlags=BGCHECKFLAG_GROUND; player.currentMask=PLAYER_MASK_NONE;
    gSaveContext.equips.buttonItems[0]=ITEM_NONE;
    for (int i=1;i<8;++i) gSaveContext.equips.buttonItems[i]=ITEM_NONE;
    sDekuBubbleActive=sZoraBoomerangActive=sUseHeldItem=sHeldItemButtonIsHeldDown=false;
    blocksSword=riding=muted=worldHit=false; transformed=true; surfaceBg=BGCHECK_SCENE;
    sounds.clear(); stopped.clear(); stonePops=genericPops=atSubmits=0;
    missingAssets=false; splatDraws=0; resources.clear(); displayPtr=display;
    surface.normal={0,32767,0};
}
int main(int argc, char** argv) {
    assert(argc==2); reset(); const std::string test=argv[1];
    if (test=="button-alias") {
        assert(ResolveItemOnButton(&play,0,ITEM_SWORD_BGS)==ITEM_NONE);
        sDekuBubbleActive=true;
        assert(ResolveItemOnButton(&play,0,ITEM_NONE)==ITEM_SLINGSHOT);
        assert(ResolveItemOnButton(&play,1,ITEM_BOOMERANG)==ITEM_BOOMERANG);
        riding=true; assert(ResolveItemOnButton(&play,0,ITEM_NONE)==ITEM_NONE);
        riding=false; blocksSword=true; assert(ResolveItemOnButton(&play,0,ITEM_NONE)==ITEM_NONE);
    } else if (test=="hold-release" || test=="rapid-fire" || test=="quick-release") {
        Player_StartDekuBubble(&player,&play); gFormState.bubbleCharging=1;
        for(int i=0;i<(test=="quick-release" ? 0 : 12);++i) nativeFrame(true);
        assert(!gFormState.bubble.active); assert(gSaveContext.magic==30);
        nativeFrame(false);
        assert(gFormState.bubble.active); assert(gSaveContext.magic==28);
        assert(gFormState.bubbleCharge==0); assert(count(MM_NA_SE_PL_DEKUNUTS_FIRE)==1);
        for(int i=0;i<3;++i) nativeFrame(false);
        assert(gSaveContext.magic==28); assert(count(MM_NA_SE_PL_DEKUNUTS_FIRE)==1);
        if(test=="rapid-fire") {
            for(int i=0;i<3;++i) nativeFrame(true);
            nativeFrame(false);
            assert(gSaveContext.magic==26); assert(count(MM_NA_SE_PL_DEKUNUTS_FIRE)==2);
        }
    } else if(test=="charge-sound" || test=="mute-charge") {
        muted=test=="mute-charge";
        Player_StartDekuBubble(&player,&play); gFormState.bubbleCharging=1;
        for(int i=0;i<5;++i) { play.state.input[0].cur.button=BTN_B; TickCharge(); }
        assert(count(MM_NA_SE_PL_DEKUNUTS_BUBLE_BREATH-SFX_FLAG)==(muted ? 0 : 5));
        assert(count(MM_NA_SE_PL_DEKUNUTS_BUBLE_BREATH)==0);
    } else if(test=="close-wall") {
        fire(); worldHit=true; MmForm_UpdateBubbleProjectile(&player,&play);
        assert(count(MM_NA_SE_PL_DEKUNUTS_FIRE)==1);
        assert(count(MM_NA_SE_IT_DEKUNUTS_BUBLE_VANISH)==1);
        assert(std::none_of(stopped.begin(),stopped.end(),[](const auto& s){return s.first==MM_NA_SE_PL_DEKUNUTS_FIRE;}));
    } else if(test=="flight" || test=="frame-owner") {
        fire(); float previous=0;
        for(int i=0;i<12;++i) {
            Vec3f before=gFormState.bubble.pos;
            if(test=="frame-owner") TickFrame(); else MmForm_UpdateBubbleProjectile(&player,&play);
            const auto& b=gFormState.bubble;
            float speed=std::sqrt(SQ(b.hSpeed)+SQ(b.velY));
            assert(b.pos.x!=before.x || b.pos.y!=before.y || b.pos.z!=before.z);
            assert(std::abs(speed-std::clamp(16.0f-b.scale,1.0f,80.0f))<0.0001f);
            assert(speed>=previous); previous=speed;
        }
        assert(gFormState.bubble.active); assert(gFormState.bubble.timer==87);
    } else if(test=="actor-hit" || test=="actor-bounce" || test=="actor-unk4" || test=="torch-hit") {
        fire(); Actor target={}; ColliderInfo targetInfo={};
        gFormState.bubbleCollider.base.at=&target;
        gFormState.bubbleCollider.base.atFlags=AT_HIT;
        gFormState.bubbleCollider.info.atHitInfo=&targetInfo;
        if(test!="actor-hit") gFormState.bubbleCollider.base.atFlags|=AT_BOUNCED;
        if(test=="actor-unk4") targetInfo.elemType=ELEMTYPE_UNK4;
        if(test=="torch-hit") target.id=ACTOR_OBJ_SYOKUDAI;
        MmForm_UpdateBubbleProjectile(&player,&play);
        if(test=="actor-bounce") {
            assert(gFormState.bubble.active); assert(gFormState.bubble.state==-1);
            MmForm_UpdateBubbleProjectile(&player,&play); assert(atSubmits==0);
        } else {
            assert(!gFormState.bubble.active); assert(stonePops==1);
            assert(count(MM_NA_SE_IT_DEKUNUTS_BUBLE_VANISH)==1); assert(atSubmits==0);
        }
    } else if(test=="world-hit" || test=="dynapoly-hit") {
        if(test=="dynapoly-hit") surfaceBg=0;
        fire(); worldHit=true; MmForm_UpdateBubbleProjectile(&player,&play);
        assert(!gFormState.bubble.active); assert(stonePops==1); assert(genericPops==0);
        assert(count(MM_NA_SE_IT_DEKUNUTS_BUBLE_VANISH)==1);
#ifdef HAS_BUBBLE_EFFECT
        if(test=="dynapoly-hit") assert(std::none_of(std::begin(sDekuBubbleSplats),std::end(sDekuBubbleSplats),[](const auto& s){return s.life>0;}));
#endif
    } else if(test=="expiry" || test=="no-magic") {
        if(test=="no-magic") gSaveContext.magic=1;
        fire(test=="no-magic" ? 0.0f : 16.0f);
        int ticks=0; while(gFormState.bubble.active && ticks++<100) MmForm_UpdateBubbleProjectile(&player,&play);
        assert(!gFormState.bubble.active); assert(ticks<100);
        assert(stonePops==0); assert(genericPops==0);
        assert(count(MM_NA_SE_IT_DEKUNUTS_BUBLE_VANISH)==1);
        if(test=="no-magic") assert(gSaveContext.magic==1);
    } else if(test=="projected-audio") {
        play.viewProjectionMtxF.xw=100; play.viewProjectionMtxF.yw=200; play.viewProjectionMtxF.zw=300;
        fire(); MmForm_UpdateBubbleProjectile(&player,&play);
        auto match=std::find_if(sounds.begin(),sounds.end(),[](const auto& s){return s.id==MM_NA_SE_IT_DEKUNUTS_BUBLE_SHOT_LEVEL-SFX_FLAG;});
        assert(match!=sounds.end()); assert(match->atPlay.x==gFormState.bubble.pos.x+100);
        assert(match->atPlay.z==gFormState.bubble.pos.z+300);
    } else if(test=="cancel") {
        Player_StartDekuBubble(&player,&play); gFormState.bubbleCharging=1;
        nativeFrame(true); nativeFrame(true,BTN_A); nativeFrame(false);
        assert(!sDekuBubbleActive); assert(!gFormState.bubbleCharging); assert(!gFormState.bubble.active);
        assert(gSaveContext.magic==30); assert(count(MM_NA_SE_PL_DEKUNUTS_FIRE)==0);
    } else if(test=="impact-sound-lifetime") {
#ifdef HAS_BUBBLE_EFFECT
        fire(); worldHit=true; MmForm_UpdateBubbleProjectile(&player,&play);
        auto found=std::find_if(sounds.begin(),sounds.end(),[](const auto& s){return s.id==MM_NA_SE_IT_DEKUNUTS_BUBLE_VANISH;});
        assert(found!=sounds.end()); Vec3f* emitter=found->pos; Vec3f original=*emitter;
        worldHit=false; player.actor.world.pos={1000,2000,3000}; fire();
        assert(emitter->x==original.x && emitter->z==original.z);
        play.viewProjectionMtxF.xw=100; MmForm_UpdateDekuBubbleEffects(&play);
        assert(emitter->x==original.x+100);
        for(int i=0;i<19;++i) MmForm_UpdateDekuBubbleEffects(&play);
        assert(std::any_of(stopped.begin(),stopped.end(),[&](const auto& s){return s.first==MM_NA_SE_IT_DEKUNUTS_BUBLE_VANISH && s.second==emitter;}));
#else
        assert(false && "Stable native impact emitter is missing");
#endif
    } else if(test=="surface-dimensions" || test=="surface-downhill") {
#ifdef HAS_BUBBLE_EFFECT
        Vec3s normals[]={{32767,0,0},{-32767,0,0},{0,0,32767},{0,32767,0}};
        for(const auto& normal:normals) {
            MmForm_ResetDekuBubble(nullptr); surface.normal=normal;
            MmForm_AddDekuBubbleSplat(&hitPosition,&surface,8.0f);
            const auto& splat=sDekuBubbleSplats[0]; const auto& m=splat.surface;
            if(test=="surface-dimensions") {
                assert(std::abs(SQ(m.xx)+SQ(m.yx)+SQ(m.zx)-1.0f)<0.0001f);
                assert(std::abs(SQ(m.xz)+SQ(m.yz)+SQ(m.zz)-1.0f)<0.0001f);
                assert(std::abs(m.xx*m.xz+m.yx*m.yz+m.zx*m.zz)<0.0001f);
                assert(m.xw==hitPosition.x && m.yw==hitPosition.y && m.zw==hitPosition.z);
            } else if(splat.sliding) {
                // Native texture -Z, rotated about the surface normal, points
                // downhill. Check actual production orientation on all walls.
                const f32 sine=Math_SinS(splat.rotation), cosine=Math_CosS(splat.rotation);
                Vec3f down={-sine*m.xx-cosine*m.xz,-sine*m.yx-cosine*m.yz,-sine*m.zx-cosine*m.zz};
                assert(std::abs(down.x)<0.0002f && std::abs(down.y+1.0f)<0.0002f && std::abs(down.z)<0.0002f);
            } else assert(splat.rotation==0);
        }
#else
        assert(false && "Native surface splat port is missing");
#endif
    } else if(test=="splat-lifetime" || test=="cleanup" || test=="null-cleanup" ||
              test=="splat-draw" || test=="sliding-splat" || test=="paused-draw" || test=="missing-assets") {
#ifdef HAS_BUBBLE_EFFECT
        if(test=="sliding-splat") surface.normal={0,0,32767};
        fire(); worldHit=true; MmForm_UpdateBubbleProjectile(&player,&play);
        assert(std::count_if(std::begin(sDekuBubbleSplats),std::end(sDekuBubbleSplats),[](const auto& s){return s.life>0;})==1);
        if(test=="splat-draw" || test=="sliding-splat" || test=="paused-draw" || test=="missing-assets") {
            missingAssets=test=="missing-assets";
            const int draws=test=="paused-draw" ? 20 : 1;
            for(int i=0;i<draws;++i) MmForm_DrawDekuBubbleEffects(&play);
            assert(splatDraws==(missingAssets ? 0 : draws));
            assert(sDekuBubbleSplats[0].life==93); // Drawing never advances world time.
            assert(resources[0]==(test=="sliding-splat" ? "objects/gameplay_keep/gEffPoppedDekuBubbleSlidingDL" : "objects/gameplay_keep/gEffPoppedDekuBubbleDL"));
            assert(resources[1]==(test=="sliding-splat" ? "objects/gameplay_keep/gEffPoppedDekuBubbleSliding1Tex" : "objects/gameplay_keep/gEffPoppedDekuBubble1Tex"));
            std::printf("PASS %s\n",argv[1]);
            return 0;
        }
        if(test=="cleanup" || test=="null-cleanup") MmForm_ResetDekuBubble(test=="null-cleanup" ? nullptr : &play);
        else for(int i=0;i<93;++i) { MmForm_UpdateDekuBubbleEffects(&play); ++play.state.frames; }
        assert(std::none_of(std::begin(sDekuBubbleSplats),std::end(sDekuBubbleSplats),[](const auto& s){return s.life>0;}));
        assert(std::none_of(std::begin(sDekuBubblePopSounds),std::end(sDekuBubblePopSounds),[](const auto& s){return s.life>0;}));
#else
        assert(false && "Native SBN effect/lifetime port is missing");
#endif
    } else assert(false);
    std::printf("PASS %s\n",argv[1]);
}
