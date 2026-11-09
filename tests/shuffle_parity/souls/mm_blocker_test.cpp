#define main other_mm_runtime_suite
#include "mm_runtime_test.cpp"
#undef main
#include "src/overlays/actors/ovl_En_Dai/z_en_dai.h"
#include "src/overlays/actors/ovl_En_Bombers2/z_en_bombers2.h"
static int ocCalls=0,guardActions=0,windCalls=0,openCalls=0,talkOffers=0,killed=0;
extern "C" {
s32 SkelAnime_Update(SkelAnime*){return 0;}
s32 Animation_OnFrame(SkelAnime*,f32){return 0;}
void Actor_PlaySfx(Actor*,u16){}
void Actor_SetFocus(Actor*,f32){}
void Actor_SetScale(Actor*,f32){}
void Actor_MoveWithGravity(Actor*){}
s16 Math_SmoothStepToS(s16*,s16,s16,s16,s16){return 0;}
f32 Rand_ZeroFloat(f32){return 0;}
void Math_Vec3f_Copy(Vec3f*d,Vec3f*s){*d=*s;}
void Collider_UpdateCylinder(Actor*,ColliderCylinder*){}
s32 CollisionCheck_SetOC(PlayState*,CollisionCheckContext*,Collider*){++ocCalls;return 1;}
void Actor_UpdateBgCheckInfo(PlayState*,Actor*,f32,f32,f32,u32){}
bool GameInteractor_Should(GIVanillaBehavior,u32,...){return true;}
s32 SubS_Offer(Actor*,PlayState*,f32,f32,s32,SubSOfferMode){++talkOffers;return 1;}
void Audio_PlaySfx(u16){}
void GameInteractor_ExecuteOnFlagSet(FlagType,u32){}
void GameInteractor_ExecuteOnFlagUnset(FlagType,u32){}
void ActorShape_Init(ActorShape*,f32,ActorShadowFunc,f32){}
void ActorShadow_DrawCircle(Actor*,Lights*,PlayState*){}
void SkelAnime_InitFlex(PlayState*,SkelAnime*,FlexSkeletonHeader*,AnimationHeader*,Vec3s*,Vec3s*,s32){}
s32 Collider_InitAndSetCylinder(PlayState*,ColliderCylinder*,Actor*,ColliderCylinderInit*){return 1;}
void Actor_Kill(Actor*){++killed;}
void SubS_SetOfferMode(u16*f,u16 mode,u16 mask){*f=(*f&~mask)|mode;}

}
static void GuardAction(EnBombers2*,PlayState*){++guardActions;}
static s32 EnDai_ChangeAnim(EnDai*,s32){return 1;}
static void func_80B3EE8C(EnDai*,PlayState*){++openCalls;}
static s32 func_80B3E96C(EnDai*,PlayState*){++windCalls;return 0;}
static s32 func_80B3E7C8(EnDai*,PlayState*){return 0;}
static s32 func_80B3E69C(EnDai*,PlayState*){return 0;}
static void EnDai_HandleCutscene(EnDai*,PlayState*){}
static void func_80B3E834(EnDai*){}
static s32 func_80B3E460(EnDai*){return 0;}
#include "mm_blockers.inc"
static void func_80B3F00C(EnDai*,PlayState*){}
static void func_80C04B40(EnBombers2*self){self->actionFunc=GuardAction;}
#include "mm_blocker_init.inc"
int main(){
 PlayState play{};Player player{};play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&player.actor;
 gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=1;
 EnBombers2 guard{};guard.actor.id=ACTOR_EN_BOMBERS2;guard.actionFunc=GuardAction;guard.actor.world.pos={10,20,30};
 play.sceneId=SCENE_TOWN;REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_BOMBERS2,0));auto before=gSaveContext;
 EnBombers2_Update(&guard.actor,&play);REQUIRE(ocCalls==1&&guardActions==0);REQUIRE(!memcmp(&before,&gSaveContext,sizeof(before)));
 REQUIRE(guard.actor.world.pos.x==10&&guard.actor.world.pos.z==30);Own(RI_SOUL_NPC_BOMBERS);
 EnBombers2_Update(&guard.actor,&play);REQUIRE(ocCalls==2&&guardActions==1);REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_BOMBERS2,0));
 Own(RI_SOUL_NPC_BOMBERS,false);guard.unk_2AC=true;guard.actor.world.pos.x=60;
 EnBombers2_Update(&guard.actor,&play);REQUIRE(ocCalls==3&&guardActions==1&&guard.actor.world.pos.x==60); // already stepped aside
 EnDai giant{};giant.actor.id=ACTOR_EN_DAI;giant.actionFunc=func_80B3EEDC;play.sceneId=SCENE_12HAKUGINMAE;
 REQUIRE(MM_SoulCanSpawnNonEnemy(&play,ACTOR_EN_DAI,0));before=gSaveContext;
 EnDai_Update(&giant.actor,&play);REQUIRE(windCalls==1&&giant.actionFunc==func_80B3EEDC&&openCalls==0&&talkOffers==0);
 REQUIRE(!memcmp(&before,&gSaveContext,sizeof(before)));Own(RI_SOUL_NPC_BIGGORON);EnDai_Update(&giant.actor,&play);
 REQUIRE(giant.actionFunc==func_80B3EE8C);EnDai_Update(&giant.actor,&play);REQUIRE(openCalls==1);
 Own(RI_SOUL_NPC_BIGGORON,false);RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=0;giant.actionFunc=func_80B3EEDC;
 EnDai_Update(&giant.actor,&play);REQUIRE(giant.actionFunc==func_80B3EE8C);
 // Actual native init honors opened and cleared states through save/scene reload.
 RANDO_SAVE_OPTIONS[RO_SHUFFLE_NPC_SOULS]=1;Own(RI_SOUL_NPC_BIGGORON,false);
 CLEAR_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE);CLEAR_WEEKEVENTREG(WEEKEVENTREG_30_01);
 EnDai_Init(&giant.actor,&play);REQUIRE(giant.actionFunc==func_80B3EEDC&&killed==0);
 SET_WEEKEVENTREG(WEEKEVENTREG_30_01);auto persisted=gSaveContext;memset(&gSaveContext,0,sizeof(gSaveContext));gSaveContext=persisted;
 EnDai_Init(&giant.actor,&play);REQUIRE(killed==1); // native opening persists after save reload, even missing soul
 SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE);EnDai_Init(&giant.actor,&play);
 REQUIRE(killed==1&&giant.actionFunc==func_80B3F00C); // cleared-temple idle role, no new wind
 SET_WEEKEVENTREG(WEEKEVENTREG_73_80);guard.actor.csId=1;guard.actor.world.pos={10,20,30};
 EnBombers2_Init(&guard.actor,&play);REQUIRE(guard.unk_2AC&&guard.actor.world.pos.z==80&&killed==1);
 puts("PASS native Bomber/Giant update bodies: retained OC/wind, missing-to-owned interaction, already-open position, flags and Off controls");
}
