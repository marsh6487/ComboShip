#define main other_runtime_suite
#include "runtime_test.cpp"
#undef main
#include "overlays/actors/ovl_En_Md/z_en_md.h"
#include "overlays/actors/ovl_En_Kz/z_en_kz.h"
static int ocCalls=0,talkCalls=0,kingActions=0,midoActions=0;
extern "C" {
void Collider_UpdateCylinder(Actor*,ColliderCylinder*){}
s32 CollisionCheck_SetOC(PlayState*,CollisionCheckContext*,Collider*){++ocCalls;return 1;}
s32 SkelAnime_Update(SkelAnime*){return 0;}
void Actor_MoveXZGravity(Actor*){}
void Actor_UpdateBgCheckInfo(PlayState*,Actor*,f32,f32,f32,s32){}
s32 Flags_GetInfTable(s32){return true;}
void Flags_SetInfTable(s32){}
}
static void EnMd_UpdateEyes(EnMd*){}
static void EnMd_UpdateAlphaByDistance(EnMd*,PlayState*){}
static void EnMd_UpdateTalking(EnMd*,PlayState*){++talkCalls;}
static void EnKz_UpdateEyes(EnKz*){}
static void EnKz_StartTimer(EnKz*,PlayState*){}
static void func_80A9CB18(EnKz*,PlayState*){++talkCalls;}
static void KingAction(EnKz*,PlayState*){++kingActions;}
static void MidoAction(EnMd*,PlayState*){++midoActions;}
#include "blockers.inc"
int main(){
 PlayState play{};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;optionValues[RSK_SHUFFLE_NPC_SOULS]=1;
 EnKz king{};king.actor.id=ACTOR_EN_KZ;king.actor.world.pos={10,20,30};king.actionFunc=KingAction;play.sceneNum=SCENE_ZORAS_DOMAIN;
 REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_KZ,0));
 auto before=gSaveContext;EnKz_Update(&king.actor,&play);
 REQUIRE(ocCalls==1&&talkCalls==0&&kingActions==0);REQUIRE(king.actor.world.pos.x==10&&king.actor.world.pos.z==30);
 REQUIRE(!memcmp(&before,&gSaveContext,sizeof(before)));
 Own(RG_SOUL_NPC_KING_ZORA,true);EnKz_Update(&king.actor,&play);
 REQUIRE(ocCalls==2&&talkCalls==1&&kingActions==1);REQUIRE(king.actor.world.pos.x==10);
 // Spawn eligibility stays constant across the soul transition; native init is separately compiled.
 REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_KZ,0));Own(RG_SOUL_NPC_KING_ZORA,false);
 optionValues[RSK_SHUFFLE_NPC_SOULS]=0;EnKz_Update(&king.actor,&play);REQUIRE(kingActions==2&&talkCalls==2);
 optionValues[RSK_SHUFFLE_NPC_SOULS]=1;EnMd mido{};mido.actor.id=ACTOR_EN_MD;mido.actionFunc=MidoAction;
 for(int scene:{SCENE_KOKIRI_FOREST,SCENE_LOST_WOODS}){
  play.sceneNum=scene;int oc=ocCalls,talk=talkCalls,moves=midoActions;
  REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_MD,0));before=gSaveContext;
  EnMd_Update(&mido.actor,&play);REQUIRE(ocCalls==oc+1&&talkCalls==talk&&midoActions==moves+1);
  REQUIRE(!memcmp(&before,&gSaveContext,sizeof(before)));
  Own(RG_SOUL_NPC_MIDO,true);EnMd_Update(&mido.actor,&play);REQUIRE(ocCalls==oc+2&&talkCalls==talk+1);
  REQUIRE(Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_MD,0));Own(RG_SOUL_NPC_MIDO,false);
 }
 play.sceneNum=SCENE_MIDOS_HOUSE;REQUIRE(!Randomizer_SoulCanSpawnNonEnemy(&play,ACTOR_EN_MD,0));
 gSaveContext.ship.quest.id=QUEST_NORMAL;REQUIRE(Randomizer_SoulCanInteract(&play,ACTOR_EN_MD,0));
 puts("PASS native King/Mido update bodies: persistent OC, missing/owned interaction, spawn eligibility, saved flags and Off/vanilla controls");
}
