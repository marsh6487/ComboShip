#include "global.h"
#ifdef WHIP_FIXTURE_MM
#include "mods/nei_oot_compat.h"
#include "soh/_nei_compat_core.h"
// Existing MM item fixtures use this compatibility seam: the source's OoT
// field spelling has no corresponding production alias in this checkout.
// This fixture is behavioral evidence, not full MM unity-TU build proof.
#define linearVelocity speedXZ
#endif
#include "mods/extended_player.h"
#ifdef WHIP_FIXTURE_MM
#define FIRST_ACTOR(list) ((list).first)
#else
#define FIRST_ACTOR(list) ((list).head)
#endif
#include "mods/items/custom_items.h"
#include "mods/items/logic/item_whip_constants.h"
#include "mods/items/helpers/camera_helper.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/items/helpers/grappling_helper.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

CustomItemState gCustomItemState{};
f32 gSfxDefaultFreqAndVolScale=1;
s8 gSfxDefaultReverb;
static PlayState play;
static Player player;

static CollisionHeader mesh;
static CollisionPoly polygon;
#ifdef WHIP_FIXTURE_MM
static Vec3s vertices[3];
#else
static Vec3i vertices[3];
#endif
static bool ztarget, hidden, collisionEnabled;
static int triangles, rays, bodySegments;
static Gfx gWhipBodyDL[1];
static float obstacle, rayDistance;
static Vec3f rendered, rayStart, rayEnd;

// Graphics and camera services are observable engine boundaries. All production
// draw gating, ray construction and dispatcher decisions execute unchanged.
#undef OPEN_DISPS
#undef CLOSE_DISPS
#undef OVERLAY_DISP
#undef MATRIX_NEWMTX
#undef gSPDisplayList
#undef gSPMatrix
#undef gSPTexture
#undef gDPSetCombineMode
#undef gDPSetPrimColor
#undef gSPVertex
#undef gSP1Triangle
#undef gDPLoadTextureBlock
#define OPEN_DISPS(ctx) { Gfx* fixtureDl=nullptr;
#define CLOSE_DISPS(ctx) }
#define OVERLAY_DISP fixtureDl
#define MATRIX_NEWMTX(ctx) nullptr
#define gSPDisplayList(...) (++bodySegments)
#define Matrix_RotateY(...) ((void)0)
#define Matrix_RotateX(...) ((void)0)
#define Matrix_NewMtx(...) nullptr
#define gSPMatrix(...) ((void)0)
#define gSPTexture(...) ((void)0)
#define gDPSetCombineMode(...) ((void)0)
#define gDPSetPrimColor(...) ((void)0)
#define gSPVertex(...) ((void)0)
#define gSP1Triangle(...) (++triangles)
#define gDPLoadTextureBlock(...) ((void)0)
#define Gfx_SetupDL(dl,setup) (dl)
#define Matrix_Translate(x,y,z,mode) (rendered=Vec3f{x,y,z})
#define Matrix_ReplaceRotation(...) ((void)0)
#define Matrix_Scale(...) ((void)0)
#define SkinMatrix_Vec3fMtxFMultXYZW(matrix,pos,screen,w) (*(w)=200.0f)
static bool UiVisibility_IsHidden() {return hidden;}
static void Whip_BeginLashPose(Player*, PlayState*) {}
static void Whip_UpdateCollider(PlayState*, Vec3f*) {}
extern "C" {
#ifdef WHIP_FIXTURE_MM
bool Player_IsZTargeting(Player*) {return ztarget;}
bool func_80831010(Player* p, PlayState*) {p->unk_AA5=PLAYER_UNKAA5_3; return true;}
#else
s32 Player_IsZTargeting(Player*) {return ztarget;}
#endif
void Player_ZeroSpeedXZ(Player*) {}
f32 Math_FAtan2F(f32 x,f32 z) {return std::atan2(x,z);}
f32 Math_SinS(s16 a) {return std::sin(a*3.141592653589793/32768.0);}
f32 Math_CosS(s16 a) {return std::cos(a*3.141592653589793/32768.0);}
s16 Math_Atan2S(f32 x,f32 z) {return std::atan2(x,z)*32768.0/3.141592653589793;}
f32 Math_Vec3f_DistXYZ(Vec3f* a, Vec3f* b) {return std::sqrt((a->x-b->x)*(a->x-b->x)+(a->y-b->y)*(a->y-b->y)+(a->z-b->z)*(a->z-b->z));}
void Audio_PlaySoundGeneral(u16,Vec3f*,u8,f32*,f32*,s8*) {}
CollisionHeader* BgCheck_GetCollisionHeader(CollisionContext*,s32) {return &mesh;}
u32 SurfaceType_IsHookshotSurface(CollisionContext*,CollisionPoly*,s32) {return 0;}
void CollisionPoly_GetNormalF(CollisionPoly* poly,f32* x,f32* y,f32* z) {*x=poly->normal.x/32767.0f; *y=poly->normal.y/32767.0f; *z=poly->normal.z/32767.0f;}
s32 BgCheck_EntityLineTest1(CollisionContext*,Vec3f* a,Vec3f* b,Vec3f* hit,CollisionPoly** poly,s32,s32,s32,s32,s32* bg) {
    if (!collisionEnabled || a->z>=obstacle || b->z<obstacle) return 0;
    *hit=*b; hit->z=obstacle; *poly=&polygon; *bg=BGCHECK_SCENE; return 1;
}
s32 BgCheck_AnyLineTest3(CollisionContext* ctx,Vec3f* a,Vec3f* b,Vec3f* hit,CollisionPoly** poly,s32 w,s32 f,s32 c,s32 o,s32* bg) {
    ++rays; rayStart=*a; rayEnd=*b; rayDistance=Math_Vec3f_DistXYZ(a,b);
    return BgCheck_EntityLineTest1(ctx,a,b,hit,poly,w,f,c,o,bg);
}
}

#include "production.inc"

static void require(bool condition,const char* message) {if(!condition){std::fprintf(stderr,"FAIL: %s\n",message); abort();}}
static void reset(bool wall=false) {
    player={}; play={}; gCustomItemState={}; mesh={}; polygon={};
    whipCollider={}; ztarget=hidden=false; triangles=rays=0;
    collisionEnabled=true; obstacle=900; rayDistance=0;
    player.heldItemAction=PLAYER_IA_WHIP;
    whipActive=1; whipState=WHIP_STATE_EQUIP; whipFirstPerson=1;
    polygon.flags_vIA=0; polygon.flags_vIB=1; polygon.vIC=2; polygon.normal.x=32767;
    vertices[0]={0,0,900}; vertices[1]={0,(s16)(wall?400:20),900}; vertices[2]={0,0,(s16)(wall?1300:1200)};
    mesh.vtxList=vertices; mesh.numVertices=3; mesh.polyList=&polygon; mesh.numPolygons=1;
#ifdef WHIP_FIXTURE_MM
    sFpAimTimer=0;
#else
    FirstPerson_Init(&player,&play);
#endif
}
static void draw() {triangles=rays=0; CustomItems_OverrideDraw(&player,&play);}
static void actorUpdate(Actor*,PlayState*) {}
static void flight() {ItemInputState input{}; input.isPressed=1; WhipStateEquip(&player,&play,&input); while(whipState==WHIP_STATE_EXTENDING) WhipStateExtending(&player,&play);}
int main(int argc, char**) {
    if (argc==1) {
    reset(); ItemInputState idle{}; WhipStateEquip(&player,&play,&idle); draw();
    require(triangles==1,"native whip equip aim must draw reticle with legacy timer zero");
    require(std::fabs(rayDistance-1040)<0.1f,"whip marker must stop at actual1040 reach");
    hidden=true; draw(); require(triangles==0 && rays==0,"hidden HUD suppresses marker"); hidden=false;
    whipFirstPerson=0; draw(); require(triangles==0,"released whip suppresses marker");
#ifdef WHIP_FIXTURE_MM
    whipFirstPerson=1; player.unk_AA5=PLAYER_UNKAA5_0; draw(); require(triangles==0,"inactive native aim suppresses marker");
    player.unk_AA5=PLAYER_UNKAA5_3; player.unk_ACC=0; draw(); require(triangles==0,"native aim readiness required");
    player.unk_ACC=10; whipActive=0; draw(); require(triangles==0,"inactive whip cannot borrow native readiness");
    whipActive=1; whipState=WHIP_STATE_EXTENDING; draw(); require(triangles==0,"in-flight whip cannot borrow native readiness");
#endif
    reset(); obstacle=1041; WhipStateEquip(&player,&play,&idle); draw(); require(triangles==0,"beyond-reach geometry must not get whip marker");
    reset(); whipFirstPerson=0; player.heldItemAction=PLAYER_IA_NONE;
    FirstPerson_Init(&player,&play); FirstPerson_DrawReticle(&player,&play,0,255,0,0);
    require(triangles==1,"legacy helper still draws for legacy aim readiness");
    FirstPerson_Exit(&player,&play); triangles=0; FirstPerson_DrawReticle(&player,&play,0,255,0,0);
    require(triangles==0,"legacy helper still suppresses inactive aim");
    }
    reset(); flight(); require(whipState==WHIP_STATE_ATTACHED && whipAttachPos.z==900,"eligible beam beyond520 must attach");
    reset(true); flight(); require(whipState==WHIP_STATE_RETRACTING && whipTipPos.z==900,"flat wall must reject at increased reach");
    reset(); obstacle=1041; flight(); require(whipState==WHIP_STATE_RETRACTING && whipTipPos.z==1040,"flight must retract at1040 before beyond-range target");
    reset(); obstacle=1040; flight(); require(whipState==WHIP_STATE_ATTACHED,"eligible beam at exact1040 boundary must attach");
    reset(); collisionEnabled=false; player.actor.focus.rot.y=0x4000;
    player.actor.focus.pos={11,55,7}; player.bodyPartsPos[PLAYER_BODYPART_R_HAND]={4,20,7};
    ItemInputState aimedIdle{}; WhipStateEquip(&player,&play,&aimedIdle); draw();
    require(rayStart.x==11 && rayStart.y==55 && std::fabs(rayEnd.x-1051)<0.1f,
            "marker keeps focus-origin ray and rotated aim direction");
    flight(); require(std::fabs(whipTipPos.x-1044)<0.1f && whipTipPos.y==20,
                      "lash keeps hand-origin launch with same distance budget");
    reset(); collisionEnabled=false;
    Actor torch{}; torch.id=ACTOR_OBJ_SYOKUDAI; torch.update=actorUpdate; torch.world.pos.z=900;
    FIRST_ACTOR(play.actorCtx.actorLists[ACTORCAT_PROP])=&torch;
    flight(); require(whipState==WHIP_STATE_ATTACHED && whipAttachPos.z==900,"existing torch actor grip survives longer flight");
    reset(); collisionEnabled=false;
    Actor enemy{}; enemy.id=ACTOR_EN_FIREFLY; enemy.update=actorUpdate; enemy.world.pos.z=900;
    FIRST_ACTOR(play.actorCtx.actorLists[ACTORCAT_ENEMY])=&enemy;
    flight(); require(whipState==WHIP_STATE_HIT_ENEMY && whipPullTarget==&enemy && enemy.colorFilterTimer==60,
                      "extended flight preserves existing Keese stun/pull response");
    reset(); collisionEnabled=false; enemy={}; enemy.id=ACTOR_EN_IK; enemy.update=actorUpdate; enemy.world.pos.z=900;
    FIRST_ACTOR(play.actorCtx.actorLists[ACTORCAT_ENEMY])=&enemy;
    flight(); require(whipState==WHIP_STATE_RETRACTING && (enemy.home.rot.z & 0x4000),"extended flight preserves disarm response");
    reset(); Vec3f start{},end{0,0,1040}; bodySegments=0;
    Whip_DrawSnakeBody(&play,&start,&end,0);
    require(bodySegments>=69 && rendered.z==1040,"fallback rope retains full-reach segment density and endpoint");
    puts("PASS: aim/draw dispatch/reticle,1040 boundary,beam/torch grip,wall rejection,Keese/disarm,rope density,legacy gates,focus/hand offsets");
}
