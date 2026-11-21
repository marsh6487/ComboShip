#!/usr/bin/env python3
"""Execute native OoT completion branches; physical rewards survive logical world ownership."""
import pathlib,re,subprocess,tempfile
import os, sys
sanitize_flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if '--sanitize' in sys.argv else []
if sanitize_flags:
    os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'
root=pathlib.Path(__file__).resolve().parents[2]
def body(text,marker):
 start=text.index(marker); opening=text.index('{',start); depth,end=1,opening+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[opening+1:end-1]
door=(root/'soh/src/overlays/actors/ovl_Door_Warp1/z_door_warp1.c').read_text()
parts=[]
for name in ['Child','Ruto','Adult']:
 fn=body(door,'void DoorWarp1_'+name+'WarpOut(DoorWarp1* this, PlayState* play) {')
 marker='if (sWarpTimerTarget < this->warpTimer' if name=='Child' else 'if (this->warpTimer > sWarpTimerTarget'
 parts.append('void '+name+'(PlayState* play) {'+body(fn,marker)+'}\n')
code=''.join(parts)
hooks=(root/'soh/soh/Enhancements/randomizer/hook_handlers.cpp').read_text()
recovery='static void QueueUnclaimedBossWarpRewards(const std::vector<RandomizerCheck>& restored) {'
names=sorted(set(re.findall(r'\b(?:SCENE_|EVENTCHKINF_|RAND_INF_|ENTR_|CHAMBER_CS_|VB_|ITEM_|QUEST_|TRANS_)[A-Z0-9_]+',code)))
prefix='''#include <cassert>
#include <algorithm>
#include <queue>
#include <vector>
#define COMBO_BUILD 1
#define osSyncPrintf(...) ((void)0)
#define CHECK_QUEST_ITEM(...) false
#define LINK_IS_ADULT true
'''+ 'enum {'+','.join(names)+'};\n'+r'''
struct PlayState { int sceneNum=0,nextEntranceIndex=0,transitionTrigger=0,transitionType=0; };
struct { int nextCutsceneIndex=0xFFEF,chamberCutsceneNum=0,nextTransitionType=0; } gSaveContext;
static bool flags[1000]; static bool crossBoss=false; static int notified=-1;
static int Flags_GetEventChkInf(int flag) { return flags[flag]; }
static void Flags_SetEventChkInf(int flag) { flags[flag]=true; }
static void Flags_SetRandomizerInf(int flag) { flags[flag]=true; }
static bool GameInteractor_Should(int behavior,bool value,...) { return behavior==VB_GIVE_ITEM_FROM_BLUE_WARP?false:value; }
static void Item_Give(PlayState*,int) {}
static int Combo_SOH_CrossBossWarp(int scene) { if(!crossBoss)return 0; notified=scene; return 1; }
'''
rewardFixture=r'''
enum RandomizerCheck { RC_MORPHA, RC_TWINROVA, RC_BONGO_BONGO, RC_VOLVAGIA };
enum RandomizerGet { RG_NONE, RG_SHADOW_MEDALLION, RG_FOREST_MEDALLION, RG_BLUE_RUPEE, RG_FIRE_MEDALLION };
enum RandomizerInf : int {};
enum { SPOILER_CHK_EVENT_CHK_INF, SPOILER_CHK_RANDOMIZER_INF, ITEMTYPE_DUNGEONREWARD, CAN_OBTAIN, RCSHOW_SCUMMED };
bool IS_RANDO = true;
static bool Combo_IsCombinedGame() { return true; }
static bool Flags_GetRandomizerInf(RandomizerInf flag) { return flags[flag]; }
struct Collection { int type, flag; };
struct Location {
 Collection collection;
 Collection GetCollectionCheck() const { return collection; }
};
struct ItemLocation {
 RandomizerGet reward;
 bool obtained;
 bool HasObtained() const { return obtained; }
 RandomizerGet GetPlacedRandomizerGet() const { return reward; }
 void SetCheckStatus(int status) { assert(status == RCSHOW_SCUMMED); obtained = false; }
};
struct Item {
 int type;
 int GetItemType() const { return type; }
};
namespace Rando {
struct Context {
 ItemLocation items[4]{{RG_SHADOW_MEDALLION, true}, {RG_FOREST_MEDALLION, false},
                       {RG_BLUE_RUPEE, true}, {RG_FIRE_MEDALLION, true}};
 static Context* GetInstance() { static Context context; return &context; }
 ItemLocation* GetItemLocation(RandomizerCheck check) { return &items[check]; }
};
struct StaticData {
 static inline const std::vector<RandomizerCheck> dungeonRewardLocations{RC_MORPHA, RC_TWINROVA, RC_BONGO_BONGO, RC_VOLVAGIA};
 static const Location* GetLocation(RandomizerCheck check) {
  static const Location locations[]{
   {{SPOILER_CHK_EVENT_CHK_INF, EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP}},
   {{SPOILER_CHK_RANDOMIZER_INF, RAND_INF_DUNGEONS_DONE_SPIRIT_TEMPLE}},
   {{SPOILER_CHK_RANDOMIZER_INF, RAND_INF_DUNGEONS_DONE_SHADOW_TEMPLE}},
   {{SPOILER_CHK_EVENT_CHK_INF, EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP}}};
  return &locations[check];
 }
 static Item RetrieveItem(RandomizerGet get) { return {get == RG_BLUE_RUPEE ? -1 : ITEMTYPE_DUNGEONREWARD}; }
};
}
struct Randomizer {
 int GetItemObtainabilityFromRandomizerGet(RandomizerGet get) { return get == RG_FIRE_MEDALLION ? -1 : CAN_OBTAIN; }
} randomizer;
struct OTRGlobals {
 Randomizer* gRandomizer;
 static inline OTRGlobals* Instance;
} globals{&randomizer};
std::queue<RandomizerCheck> randomizerQueuedChecks;
'''
main=r'''
int main() {
 PlayState play{}; crossBoss=true; play.sceneNum=SCENE_DEKU_TREE_BOSS; Child(&play);
 assert(notified==SCENE_DEKU_TREE_BOSS && "warp use must notify source owner before native flags");
 assert(flags[EVENTCHKINF_USED_DEKU_TREE_BLUE_WARP] && "physical reward flag must survive");
 assert(!flags[EVENTCHKINF_OBTAINED_KOKIRI_EMERALD_DEKU_TREE_DEAD] && "physical room cannot unlock native forest");
 notified=-1; gSaveContext.nextCutsceneIndex=0xFFEF; Child(&play);
 assert(notified==SCENE_DEKU_TREE_BOSS && play.nextEntranceIndex==ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP);
 play.sceneNum=SCENE_JABU_JABU_BOSS; notified=-1; gSaveContext.nextCutsceneIndex=0xFFEF; Ruto(&play);
 assert(notified==SCENE_JABU_JABU_BOSS && flags[EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP]);
 const int scenes[]={SCENE_FOREST_TEMPLE_BOSS,SCENE_FIRE_TEMPLE_BOSS,SCENE_WATER_TEMPLE_BOSS,SCENE_SPIRIT_TEMPLE_BOSS,SCENE_SHADOW_TEMPLE_BOSS};
 const int rewards[]={EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP,EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP,EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP,RAND_INF_DUNGEONS_DONE_SPIRIT_TEMPLE,RAND_INF_DUNGEONS_DONE_SHADOW_TEMPLE};
 for(int i=0;i<5;++i) { notified=-1; play.sceneNum=scenes[i];gSaveContext.nextCutsceneIndex=0xFFEF;Adult(&play);assert(notified==scenes[i] && flags[rewards[i]]); }
 crossBoss=false; notified=-1;flags[EVENTCHKINF_USED_DEKU_TREE_BLUE_WARP]=false;gSaveContext.nextCutsceneIndex=0xFFEF;play.sceneNum=SCENE_DEKU_TREE_BOSS;Child(&play);
 assert(notified==-1 && flags[EVENTCHKINF_OBTAINED_KOKIRI_EMERALD_DEKU_TREE_DEAD]);

 // Morpha's physical warp fired before a foreign handoff and an old save promoted the check.
 // Missing Shadow Medallion is recoverable. An already-owned medallion and consumed junk stay put.
 OTRGlobals::Instance = &globals;
 QueueUnclaimedBossWarpRewards({RC_TWINROVA});
 assert(randomizerQueuedChecks.size() == 1 && randomizerQueuedChecks.front() == RC_MORPHA);
 auto* morpha = Rando::Context::GetInstance()->GetItemLocation(RC_MORPHA);
 assert(!morpha->HasObtained());
 randomizerQueuedChecks.pop();
 QueueUnclaimedBossWarpRewards({RC_MORPHA});
 assert(randomizerQueuedChecks.size() == 1 && randomizerQueuedChecks.front() == RC_TWINROVA);
 randomizerQueuedChecks.pop();
 flags[EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP] = false;
 QueueUnclaimedBossWarpRewards({RC_TWINROVA});
 assert(randomizerQueuedChecks.empty());
}
'''
with tempfile.TemporaryDirectory(prefix='oot-cross-boss-') as tmp:
 source=pathlib.Path(tmp)/'test.cpp';binary=pathlib.Path(tmp)/'test';source.write_text(
     prefix + code + rewardFixture + recovery + body(hooks, recovery) + '}\n' + main)
 subprocess.run(['g++',*sanitize_flags,'-std=c++20',str(source),'-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
