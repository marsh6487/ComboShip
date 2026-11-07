#include "global.h"
#include "mods/extended_equipment.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// Affine renderer boundary: column vectors, post-multiplication like the engine.
static float matrix[4][4], saved[4][4];
static int registrations, trails, disableTrail;
static float roomOffset;
PlayState* gPlayState;
ExtendedEquipmentState gExtEquipState;
static void identity(float m[4][4]) { memset(m, 0, 16 * sizeof(float)); for (int i=0;i<4;i++) m[i][i]=1; }
static void multiply(float rhs[4][4]) {
    float result[4][4] = {0};
    for(int i=0;i<4;i++) for(int j=0;j<4;j++) for(int k=0;k<4;k++) result[i][j]+=matrix[i][k]*rhs[k][j];
    memcpy(matrix,result,sizeof(matrix));
}
void Matrix_Push(void) { memcpy(saved,matrix,sizeof(matrix)); }
void Matrix_Pop(void) { memcpy(matrix,saved,sizeof(matrix)); }
void Matrix_Translate(f32 x,f32 y,f32 z,u8 mode) { float m[4][4];identity(m);m[0][3]=x;m[1][3]=y;m[2][3]=z;if(!mode)identity(matrix);multiply(m); }
void Matrix_Scale(f32 x,f32 y,f32 z,u8 mode) { float m[4][4];identity(m);m[0][0]=x;m[1][1]=y;m[2][2]=z;if(!mode)identity(matrix);multiply(m); }
static void rotate(float angle,int axis) { float m[4][4];identity(m);int a=(axis+1)%3,b=(axis+2)%3;m[a][a]=m[b][b]=cosf(angle);m[a][b]=-sinf(angle);m[b][a]=sinf(angle);multiply(m); }
void Matrix_RotateZ(f32 z,u8 mode) { if(!mode)identity(matrix);rotate(z,2); }
void Matrix_RotateZYX(s16 x,s16 y,s16 z,u8 mode) { if(!mode)identity(matrix);rotate(z*M_PI/32768,2);rotate(y*M_PI/32768,1);rotate(x*M_PI/32768,0); }
void Matrix_MultVec3f(Vec3f* src,Vec3f* dst) { float v[3]={src->x,src->y,src->z};float out[3];for(int i=0;i<3;i++){out[i]=matrix[i][3];for(int j=0;j<3;j++)out[i]+=matrix[i][j]*v[j];}dst->x=out[0];dst->y=out[1];dst->z=out[2]; }
u8 ExtEquip_IsEnabled(void) { return 1; }
s32 Player_GetMeleeWeaponHeld(Player* player) { return 1; }
void Math_Vec3f_Copy(Vec3f* dst,Vec3f* src) { *dst=*src; }
s32 Collider_ResetQuadAT(PlayState* play,Collider* collider) { return 0; }
void Collider_SetQuadVertices(ColliderQuad* quad,Vec3f* a,Vec3f* b,Vec3f* c,Vec3f* d) { quad->dim.quad[0]=*a;quad->dim.quad[1]=*b;quad->dim.quad[2]=*c;quad->dim.quad[3]=*d; }
s32 CollisionCheck_SetAT(PlayState* play,CollisionCheckContext* ctx,Collider* collider) { registrations++;return 0; }
void* Effect_GetByIndex(s32 index) { return NULL; }
void EffectBlure_AddVertex(EffectBlure* effect,Vec3f* tip,Vec3f* base) { trails++; }
s32 CVarGetInteger(const char* name,s32 fallback) { return disableTrail; }
static void Net_CaptureAtBlade(PlayState* play,Player* player) { assert(0); }
static u8 sNetCaughtThisSwing;
#include "production.inc"
#include "mesh.inc"

static float distance(Vec3f a,Vec3f b) { return sqrtf((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z)); }
static void hand(float scale,float angle) { identity(matrix);Matrix_Translate(100+roomOffset,80,-70,MTXMODE_APPLY);Matrix_RotateZ(angle,MTXMODE_APPLY);Matrix_Scale(scale,scale,scale,MTXMODE_APPLY); }
static void draw(Player* player,float scale,float angle) { hand(scale,angle);Invoke(gPlayState,player); }
static Vec3f inverseFrame(float frame[4][4],Vec3f world) {
    float result[3]={0};float v[3]={world.x-frame[0][3],world.y-frame[1][3],world.z-frame[2][3]};
    for(int axis=0;axis<3;axis++){float norm=0;for(int row=0;row<3;row++){result[axis]+=v[row]*frame[row][axis];norm+=frame[row][axis]*frame[row][axis];}result[axis]/=norm;}
    return (Vec3f){result[0],result[1],result[2]};
}
int main(void) {
    static PlayState play;
    // GET_PLAYER reads the actor list; use a real Player fixture at that boundary.
    static Player fixture;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head=(Actor*)&fixture;
    Player* player=&fixture;gPlayState=&play;gExtEquipState.currentExtSword=1;
    for(int age=0;age<2;age++) for(int combo=0;combo<9;combo++) for(int spin=0;spin<2;spin++) {
        memset(player,0,sizeof(*player));player->meleeWeaponState=1;player->unk_845=combo;
        player->meleeWeaponAnimation=spin?0x18:0;player->stateFlags2=spin?PLAYER_STATE2_SPIN_ATTACKING:0;
        float scale=age?0.01f:0.008f;
        registrations=trails=0;draw(player,scale,0);draw(player,scale,0.3f);
        assert(registrations==2 && trails==2);
        assert(player->unk_845==(combo>=3?combo+2:combo));
        // Shaft is 66.8 world units at adult scale. Collision is only 2 units
        // longer and <20 units from its axis; crossed diagonals remain <76.
        // A combo cannot multiply that reach.
        float reach=distance(player->meleeWeaponInfo[1].tip,player->meleeWeaponInfo[1].base);
        if(reach>76.0f) { fprintf(stderr,"oversized cane reach %.2f age=%d combo=%d spin=%d\n",reach,age,combo,spin);return 1; }
        assert(reach>50.0f);
        Vec3f tip,base;hand(scale,0.3f);ExtEquip_ApplySwordDLMatrix();
        Vec3f meshBase={0,-416,0},meshTip={0,223,0};Matrix_MultVec3f(&meshBase,&base);Matrix_MultVec3f(&meshTip,&tip);
        assert(distance(player->meleeWeaponInfo[0].base,base)<0.01f);
        assert(distance(player->meleeWeaponInfo[0].tip,tip)<0.01f);
        for(int q=0;q<2;q++) for(int v=0;v<4;v++) {
            Vec3f point=player->meleeWeaponQuads[q].dim.quad[v];
            assert(isfinite(point.x)&&isfinite(point.y)&&isfinite(point.z));
        }
        assert(distance(player->meleeWeaponInfo[1].tip,tip)<10.1f);
        assert(distance(player->meleeWeaponInfo[1].base,base)<19.5f);
        // Actual authored crown and legacy model extrema lie within the
        // production endpoint envelope. In particular, negative mesh X maps to
        // positive collision Y after the +90 degree axis rotation.
        float frame[4][4];hand(scale,0.3f);assert(ExtEquip_ByrnaTrailBegin());memcpy(frame,matrix,sizeof(frame));Matrix_Pop();
        Vec3f low=inverseFrame(frame,player->meleeWeaponInfo[1].tip);
        Vec3f high=inverseFrame(frame,player->meleeWeaponInfo[2].tip);
        hand(scale,0.3f);ExtEquip_ApplySwordDLMatrix();
        for(unsigned n=0;n<sizeof(meshPoints)/sizeof(meshPoints[0]);n++) {
            Vec3f world;Matrix_MultVec3f(&meshPoints[n],&world);Vec3f local=inverseFrame(frame,world);
            assert(local.x>=-2.0f&&local.x<=low.x);
            assert(local.y>=low.y&&local.y<=high.y);
            assert(fabsf(local.z)<=fabsf(low.z)+0.1f);
        }
        draw(player,scale,0.3f);assert(registrations==2); // stationary sweep resets AT
    }
    // Attack gates and sword trail controls stay native, including spins.
    memset(player,0,sizeof(*player));player->meleeWeaponState=1;player->meleeWeaponAnimation=0x18;
    registrations=trails=0;draw(player,0.01f,0);draw(player,0.01f,0.1f);
    assert(registrations==0&&trails==2);
    player->stateFlags1=PLAYER_STATE1_SHIELDING;draw(player,0.01f,0.2f);assert(trails==2);
    player->stateFlags1=0;disableTrail=1;draw(player,0.01f,0.3f);assert(trails==2);disableTrail=0;
    // Production attack reset ends the old sweep. An inactive draw followed by
    // a far-away room hand transform cannot register a cross-room damage quad.
    func_80832318(player);assert(player->meleeWeaponState==0);
    roomOffset=50000;registrations=0;draw(player,0.01f,0.8f);assert(registrations==0);
    assert(!player->meleeWeaponInfo[1].active&&!player->meleeWeaponInfo[2].active);
    player->meleeWeaponState=1;player->meleeWeaponAnimation=0;
    draw(player,0.01f,0.9f);assert(registrations==0);
    draw(player,0.01f,1.0f);assert(registrations==2);
    for(int q=0;q<2;q++)for(int v=0;v<4;v++)assert(player->meleeWeaponQuads[q].dim.quad[v].x>49000);
    roomOffset=0;
    // Native swords retain their original combo/padding and transverse geometry.
    identity(matrix);memset(player,0,sizeof(*player));Vec3f tips[3];D_80126080.x=4000;func_80090A28(player,tips);
    assert(tips[0].x==4000&&tips[0].y==400&&tips[1].x==5200&&tips[1].z==1000&&tips[2].y==1400);
    player->unk_845=3;func_80090A28(player,tips);assert(player->unk_845==4&&tips[1].x==7200);
    puts("PASS: child/adult cane swing/combo/spin swept quads, mesh alignment, vanilla endpoints");
}
