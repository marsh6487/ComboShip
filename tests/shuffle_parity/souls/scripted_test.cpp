#define main other_runtime_suite
#include "runtime_test.cpp"
#include "macros.h"
#undef main
#include "soh/Enhancements/game-interactor/vanilla-behavior/GIVanillaBehavior.h"
#include "rando/CrossEntranceBoss.h"
static int gifts=0;static bool forceEligibility=false;
extern "C" {
u32 gBitFlags[32]{};static EntranceInfo entranceStorage[ENTR_MAX]{};EntranceInfo*gEntranceTable=entranceStorage;
s32 Flags_GetEventChkInf(s32 f){return (gSaveContext.eventChkInf[f>>4]>>(f&15))&1;}
void Flags_SetEventChkInf(s32 f){gSaveContext.eventChkInf[f>>4]|=1u<<(f&15);}
u8 Item_Give(PlayState*,u8){++gifts;return 0;}
bool GameInteractor_Should(GIVanillaBehavior flag,u32 result,...){return (forceEligibility&&(flag==VB_BE_ELIGIBLE_FOR_NOCTURNE_OF_SHADOW||flag==VB_BE_ELIGIBLE_FOR_LIGHT_ARROWS))||result;}
#ifdef COMBO_BUILD
int Combo_SOH_CrossBossLogicalClear(int,int native){return native;}
#endif
}
#include "scripted.inc"
int main(){
 PlayState play{};for(int i=0;i<32;++i)gBitFlags[i]=1u<<i;
 gSaveContext.ship.quest.id=QUEST_RANDOMIZER;optionValues[RSK_SHUFFLE_NPC_SOULS]=1;
 gSaveContext.entranceIndex=ENTR_LOST_WOODS_BRIDGE_EAST_EXIT;NativeScripted(&play);
 REQUIRE(!Flags_GetEventChkInf(EVENTCHKINF_SPOKE_TO_SARIA_ON_BRIDGE)&&gifts==0&&gSaveContext.cutsceneIndex==0);
 Own(RG_SOUL_NPC_SARIA,true);NativeScripted(&play);REQUIRE(Flags_GetEventChkInf(EVENTCHKINF_SPOKE_TO_SARIA_ON_BRIDGE)&&gifts==1);
 REQUIRE(gSaveContext.entranceIndex==ENTR_LOST_WOODS_SOUTH_EXIT&&gSaveContext.cutsceneIndex==0xFFF0);Own(RG_SOUL_NPC_SARIA,false);
 gSaveContext.cutsceneIndex=0;gSaveContext.entranceIndex=ENTR_DESERT_COLOSSUS_OUTSIDE_TEMPLE;NativeScripted(&play);
 REQUIRE(!Flags_GetEventChkInf(EVENTCHKINF_LEARNED_REQUIEM_OF_SPIRIT)&&gSaveContext.cutsceneIndex==0);
 Own(RG_SOUL_NPC_SHEIK,true);NativeScripted(&play);REQUIRE(Flags_GetEventChkInf(EVENTCHKINF_LEARNED_REQUIEM_OF_SPIRIT)&&gSaveContext.cutsceneIndex==0xFFF0);Own(RG_SOUL_NPC_SHEIK,false);
 // An enhancement override may force eligibility, but cannot bypass source ownership.
 forceEligibility=true;gSaveContext.cutsceneIndex=0;gSaveContext.entranceIndex=ENTR_KAKARIKO_VILLAGE_FRONT_GATE;
 NativeScripted(&play);REQUIRE(!Flags_GetEventChkInf(EVENTCHKINF_BONGO_BONGO_ESCAPED_FROM_WELL)&&gSaveContext.cutsceneIndex==0);
 Own(RG_SOUL_NPC_SHEIK,true);NativeScripted(&play);REQUIRE(Flags_GetEventChkInf(EVENTCHKINF_BONGO_BONGO_ESCAPED_FROM_WELL));Own(RG_SOUL_NPC_SHEIK,false);
 gSaveContext.cutsceneIndex=0;gSaveContext.entranceIndex=ENTR_TEMPLE_OF_TIME_ENTRANCE;NativeScripted(&play);
 REQUIRE(!Flags_GetEventChkInf(EVENTCHKINF_RETURNED_TO_TEMPLE_OF_TIME_WITH_ALL_MEDALLIONS)&&gSaveContext.cutsceneIndex==0);
 Own(RG_SOUL_NPC_ZELDA,true);NativeScripted(&play);REQUIRE(Flags_GetEventChkInf(EVENTCHKINF_RETURNED_TO_TEMPLE_OF_TIME_WITH_ALL_MEDALLIONS)&&gSaveContext.cutsceneIndex==0xFFF8);
 puts("PASS actual scripted reward branches: Saria/Requiem/Nocturne/LACS defer native flags and transitions until owned, including forced-eligibility overrides");
}
