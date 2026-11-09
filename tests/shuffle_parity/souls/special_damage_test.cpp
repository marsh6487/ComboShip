#define main runtime_suite
#include "runtime_test.cpp"
#undef main
#include "overlays/actors/ovl_En_Dodongo/z_en_dodongo.h"
#include "overlays/actors/ovl_En_Dodojr/z_en_dodojr.h"
static int swallowed=0;
static s32 EnDodongo_AteBomb(EnDodongo*,PlayState*){++swallowed;return true;}
static s32 EnDodojr_TryEatBomb(EnDodojr*){++swallowed;return true;}
#include "bomb.inc"
int main(){
 REQUIRE(runtime_suite()==0);PlayState play{};gSaveContext.ship.quest.id=QUEST_RANDOMIZER;
 optionValues[RSK_SHUFFLE_ENEMY_SOULS]=1;
 EnDodongo dodongo{};dodongo.actor.id=ACTOR_EN_DODONGO;dodongo.actionState=DODONGO_IDLE;
 EnDodojr baby{};baby.actor.id=ACTOR_EN_DODOJR;
 REQUIRE(!DodongoBomb(&dodongo,&play)&&!BabyBomb(&baby,&play)&&swallowed==0);
 Own(RG_SOUL_ENEMY_DODONGO,true);Own(RG_SOUL_ENEMY_BABY_DODONGO,true);
 REQUIRE(DodongoBomb(&dodongo,&play)&&BabyBomb(&baby,&play)&&swallowed==2);
 dodongo.actionState=DODONGO_DEATH;REQUIRE(!DodongoBomb(&dodongo,&play)&&swallowed==2);
 Own(RG_SOUL_ENEMY_DODONGO,false);Own(RG_SOUL_ENEMY_BABY_DODONGO,false);dodongo.actionState=DODONGO_IDLE;
 optionValues[RSK_SHUFFLE_ENEMY_SOULS]=0;REQUIRE(DodongoBomb(&dodongo,&play)&&BabyBomb(&baby,&play));
 optionValues[RSK_SHUFFLE_ENEMY_SOULS]=1;gSaveContext.ship.quest.id=QUEST_NORMAL;
 REQUIRE(DodongoBomb(&dodongo,&play)&&BabyBomb(&baby,&play));
 puts("PASS actual Dodongo swallow predicates: bombs preserved missing soul, owned/off/vanilla and action-state controls");
}
