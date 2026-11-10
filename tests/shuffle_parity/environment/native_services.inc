#include "src/overlays/actors/ovl_En_Ishi/z_en_ishi.h"
#include "src/overlays/actors/ovl_Obj_Bombiwa/z_obj_bombiwa.h"
#include "src/overlays/actors/ovl_Obj_Hamishi/z_obj_hamishi.h"
void func_80C25360(ObjSwprize*,Vec3f* v){*v={0,1,0};}
s32 func_809393B0(Actor*);s32 func_80939470(Actor*);
void func_8093A418(Actor*,PlayState*){}void func_8093A608(Actor*,PlayState*){}
void func_80939794(ObjBombiwa*,PlayState*){}void func_80939994(PlayState*,Vec3f*){}void func_8093A080(ObjBombiwa*){}
void func_809A0F78(ObjHamishi*){}void func_809A10F4(ObjHamishi*,PlayState*){}
void func_8093833C(ObjBean*){}void func_80937FB0(ObjBean*){}
extern "C" {
void func_80C25654(ObjSwprize*,PlayState*){}
void ActorShadow_DrawCircle(Actor*,Lights*,PlayState*){}
f32 Math3D_Vec3fDistSq(Vec3f* a,Vec3f* b){return SQ(a->x-b->x)+SQ(a->y-b->y)+SQ(a->z-b->z);}
s16 Math_Atan2S_XY(f32,f32){return 0;}
void Audio_PlaySfx(u16){}s16 CutsceneManager_Stop(s16){return 0;}
void Flags_SetSwitch(PlayState*,s32){}
s32 CollisionCheck_SetAC(PlayState*,CollisionCheckContext*,Collider*){return 0;}
s32 CollisionCheck_SetOC(PlayState*,CollisionCheckContext*,Collider*){return 0;}
#ifdef COMBO_BUILD
int Combo_MM_CrossBossLogicalClear(int,int native){return native;}
#endif
}
#include "environment_native_sources.inc"
