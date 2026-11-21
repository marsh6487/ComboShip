#!/usr/bin/env python3
"""Execute the launcher's real slot-persistence/notification function at native boundaries."""
import argparse
from pathlib import Path
import subprocess
import tempfile

p=argparse.ArgumentParser()
p.add_argument('--sanitize',action='store_true')
a=p.parse_args()
root=Path(__file__).resolve().parents[2]
source=(root/'combo/ComboShip.cpp').read_text()
begin=source.index('static void Combo_OnCrossBossDefeated(')
end=source.index('// Generation pushes',begin)
rewardBegin=source.index('static bool Combo_GrantBossWarpReward(')
rewardEnd=source.index('// Per-frame drain seam',rewardBegin)
fixture=r'''
#include "combo/rando/CrossDungeonCoordinator.h"
#include "combo/rando/BossWarpRewardSave.h"
#include <cassert>
#include <iostream>
#include <mutex>
using namespace ComboRando::CrossDungeon;
static Coordinator g_CrossDungeons;
static int g_comboCompletionSlot=0,g_MmSaveInMemorySlot=0;
static uint16_t g_crossBossClears=0,hostMasks[2]={};
static std::mutex g_containerMutex;
static std::map<int,nlohmann::json> containers;
static std::map<int,ComboRando::BossWarpRewardSave> g_bossWarpRewardWrites;
static std::map<int,int> g_startupCompletions;
static int waits=0, toasts=0, gives=0;
static bool foregroundReady=false, grantAllowed=false, sourceSaveAvailable=true;
static int grantsTo=-1;
static bool pendingReward[2]={true,true};
static int Ready(int slot){assert(slot==g_comboCompletionSlot);return foregroundReady;}
static void Wait(){++waits;}
static void Combo_OnSharedChanged(int game,int slot){assert(game>=0&&game<2&&slot==g_comboCompletionSlot);}
static void Show(const char* name){assert(std::string_view(name)=="Dungeon reward");++toasts;}
static bool Grant(int slot,const char* name){
    assert(std::string_view(name)=="Dungeon reward");
    if (!grantAllowed) return false;
    ++gives;
    auto& staged=g_bossWarpRewardWrites.at(slot);
    staged.native[grantsTo]={{"award",gives}};
    return true;
}
static const char* Drain(int game,int slot,bool (*grant)(int,const char*)){
    assert(slot==g_comboCompletionSlot);
    if (!pendingReward[game]) return nullptr;
    if (!grant) return "";
    assert(g_containerMutex.try_lock());g_containerMutex.unlock();
    grantsTo=1-game;
    if (!grant(grantsTo,"Dungeon reward")) return nullptr;
    if (sourceSaveAvailable)
        g_bossWarpRewardWrites.at(slot).native[game]={{"check",true}};
    pendingReward[game]=false;
    return "Dungeon reward";
}
static const char* (*g_DrainBossWarpReward[2])(int,bool (*)(int,const char*))={
    [](int slot,bool (*grant)(int,const char*)){return Drain(0,slot,grant);},
    [](int slot,bool (*grant)(int,const char*)){return Drain(1,slot,grant);}
};
static int (*g_BossWarpRewardReady[2])(int)={Ready,Ready};
static bool (*g_GrantBossWarpReward[2])(int,const char*)={Grant,Grant};
static void (*g_ShowBossWarpReward[2])(const char*)={Show,Show};
static void (*g_CaptureBossWarpRewardSave[2])(int)={
    [](int slot){if (sourceSaveAvailable) g_bossWarpRewardWrites.at(slot).native[0]={{"check",true}};},
    [](int slot){if (sourceSaveAvailable) g_bossWarpRewardWrites.at(slot).native[1]={{"check",true}};}
};
static void (*g_WaitBossWarpRewardSaves)()=Wait;
static std::vector<int> effects;
static int flushes=0,loads=0;
static bool loadFails=false;
static bool Combo_PreserveStartupMmState(){return true;}
static bool ComboIsValidSlot(int slot){return slot>=0&&slot<3;}
static nlohmann::json& LoadOrCreateContainer(int slot){return containers[slot];}
static void FlushContainer(int){++flushes;}
static void SetOot(uint16_t mask){hostMasks[0]=mask;}
static void SetMm(uint16_t mask){hostMasks[1]=mask;}
static void Effect(int source,int dormant){
    assert(g_containerMutex.try_lock());g_containerMutex.unlock();
    assert(dormant); // This fixture exchanges both bosses across games.
    effects.push_back(source);
}
static int LoadMm(int slot){++loads;assert(slot==g_comboCompletionSlot);return loadFails?1:0;}
static int (*MM_LoadSaveForCombo)(int)=LoadMm;
static void (*g_SetCrossBossMask[2])(uint16_t)={SetOot,SetMm};
static void (*g_ApplyCrossBossClear[2])(int,int)={Effect,Effect};
'''+source[begin:end]+source[rewardBegin:rewardEnd]+r'''
int main(){
    Dungeon oot{"oot:b",{0,110},{0,111},{0,112},{{0,120}}};
    Dungeon mm{"mm:b",{1,210},{1,211},{1,212},{{1,220}}};
    oot.pool=mm.pool=Pool::Boss;oot.boss=0;mm.boss=8;
    oot.owner="oot:d";mm.owner="mm:d";
    oot.warpReturn=Endpoint{0,130};mm.warpReturn=Endpoint{1,230};
    const auto seed=Encode({oot,mm},{1,0});
    for(auto& host:g_CrossDungeons.hosts)host.SetRuntimeSeed=[](const char*){return 1;};
    g_CrossDungeons.BindRuntime({{"crossDungeonEntrances",seed}});
    containers[0]={{"oot",{{"inventory",1}}},{"mm",{{"inventory",2}}}};
    Combo_OnCrossBossDefeated(0);
    assert(effects==std::vector<int>{8});
    assert(containers[0]["combo"]["crossBossClears"]==(1u<<8));
    assert(hostMasks[0]==(1u<<8)&&hostMasks[1]==(1u<<8));
    assert(containers[0]["oot"]["inventory"]==1&&containers[0]["mm"]["inventory"]==2);
    Combo_OnCrossBossDefeated(0); // Repeat after MM reset reapplies source effect.
    assert(effects.size()==2&&effects.back()==8&&g_crossBossClears==(1u<<8));
    Combo_OnCrossBossDefeated(8);
    assert(effects.back()==0&&g_crossBossClears==((1u<<8)|1));
    const auto previous=containers;
    Combo_OnCrossBossDefeated(-1);Combo_OnCrossBossDefeated(12);
    g_comboCompletionSlot=-1;Combo_OnCrossBossDefeated(0);
    assert(containers==previous);
    // Loading the dormant owner occurs before either persistence or a native effect.
    g_comboCompletionSlot=1;g_crossBossClears=0;g_MmSaveInMemorySlot=0;
    loadFails=true;const auto priorEffects=effects.size();
    Combo_OnCrossBossDefeated(0);
    assert(g_crossBossClears==0&&effects.size()==priorEffects&&!containers.contains(1));
    loadFails=false;Combo_OnCrossBossDefeated(0);
    assert(loads==2&&g_MmSaveInMemorySlot==1&&containers[1]["combo"]["crossBossClears"]==(1u<<8));
    assert(containers[0]==previous.at(0));
    g_CrossDungeons.BindRuntime(nullptr);
    const int before=flushes;Combo_OnCrossBossDefeated(0);
    assert(flushes==before);
    // Native transfer: hold while the arrival is blocked, retry an unavailable
    // grant, and flush each durable source check alongside its native award.
    g_comboCompletionSlot=0;
    Combo_DrainBossWarpReward(0,1);
    assert(!gives && !toasts && !waits);
    foregroundReady=true;
    Combo_DrainBossWarpReward(0,1);
    assert(!gives && !toasts && waits==2 && pendingReward[0]);
    grantAllowed=true;
    const int savesBefore=flushes;
    Combo_DrainBossWarpReward(0,1);
    assert(gives==1 && toasts==1 && flushes==savesBefore+1);
    assert(containers[0]["oot"]["check"]==true && containers[0]["mm"]["award"]==1);
    Combo_DrainBossWarpReward(0,1);
    assert(gives==1 && toasts==1 && flushes==savesBefore+1);
    sourceSaveAvailable=false;
    Combo_DrainBossWarpReward(0,0);
    assert(gives==2 && toasts==1 && flushes==savesBefore+1 && g_bossWarpRewardWrites.contains(0));
    assert(!containers[0]["oot"].contains("award"));
    Combo_DrainBossWarpReward(0,0);
    assert(gives==2 && toasts==1 && flushes==savesBefore+1);
    sourceSaveAvailable=true;
    Combo_DrainBossWarpReward(0,0);
    assert(gives==2 && toasts==2 && flushes==savesBefore+2 && !g_bossWarpRewardWrites.contains(0));
    assert(containers[0]["mm"]["check"]==true && containers[0]["oot"]["award"]==2);
    assert(containers[0]["combo"]["crossBossClears"]==((1u<<8)|1));
    std::cout<<"launcher bosses: slot persistence, repeat-cycle effects, dormant ownership and disabled isolation PASS\n";
}
'''
with tempfile.TemporaryDirectory(prefix='boss-launcher-') as directory:
    cpp=Path(directory)/'test.cpp';binary=Path(directory)/'test'
    cpp.write_text(fixture)
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-I',str(root),'-I',str(root.parent/'build-deps/usr/include')]
    if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(['g++',*flags,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
