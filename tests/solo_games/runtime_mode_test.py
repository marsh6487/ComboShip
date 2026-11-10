#!/usr/bin/env python3
"""Exercise production host-enable/arrival functions with stale active route state."""
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
def block(text, marker):
    start=text.index(marker);pos=text.index('{',start);depth=1;end=pos+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]

source=r'''
#include <cassert>
#include <cstdint>
#include <optional>
#define COMBO_BUILD
#define COMBO_EXPORT
#define SPDLOG_INFO(...) ((void)0)
#include "combo/ComboGameMode.h"
extern "C" {int gComboGameMode=COMBO_GAME_BOTH;}
struct {int gameMode=0,fileNum=1;} gSaveContext;
constexpr int GAMEMODE_NORMAL=0;
constexpr bool IS_RANDO=true;
namespace ComboRando::CrossDungeon {enum class Pool {Boss};}
struct State {
    int queued=0;
    bool enabled=true;
    bool Enabled(){return enabled;}
    bool HasPool(ComboRando::CrossDungeon::Pool){return true;}
    bool QueueArrival(int){++queued;return true;}
};
namespace ComboCrossOotRuntime {State state;std::optional<int> pendingBossRecovery;}
namespace ComboCrossMmRuntime {State state;std::optional<int> pendingBossRecovery;bool bossDeathRecoveryRequested;}
'''
solo=(ROOT/'combo/rando/SoloEntranceMmRuntime.inc.h').read_text()
# The combined map remains active in this fixture; no local map was selected.
# Execute the actual local enable predicates rather than replacing the new
# dispatch boundary with unconditional false results.
source+='\nnamespace ComboSoloMmRuntime { State state{.enabled=false};\n'+block(solo,'static bool Playable(')+'\n}\n'
for marker in ['int Combo_MM_SoloEntranceRuntimeEnabled(', 'int Combo_MM_SoloEntranceBossEnabled(']:
    source+='\n'+block(solo,marker)+'\n'
for prefix,name in [('SOH','Oot'),('MM','Mm')]:
    text=(ROOT/f'combo/rando/CrossDungeon{name}Runtime.inc.h').read_text()
    for marker in [f'int Combo_{prefix}_CrossDungeonRuntimeEnabled(',f'int Combo_{prefix}_CrossBossEnabled(',
                   f'int {prefix}_QueueCrossDungeonArrival(']:
        body=block(text,marker)
        source+='\n'+body+'\n'
source+=r'''
int main(){
    assert(Combo_SOH_CrossDungeonRuntimeEnabled() && Combo_MM_CrossDungeonRuntimeEnabled());
    assert(Combo_SOH_CrossBossEnabled() && Combo_MM_CrossBossEnabled());
    assert(SOH_QueueCrossDungeonArrival(10) && MM_QueueCrossDungeonArrival(20));
    for(int mode:{COMBO_GAME_OOT,COMBO_GAME_MM}){
        gComboGameMode=mode;
        assert(!Combo_SOH_CrossDungeonRuntimeEnabled() && !Combo_MM_CrossDungeonRuntimeEnabled());
        assert(!Combo_SOH_CrossBossEnabled() && !Combo_MM_CrossBossEnabled());
        assert(!SOH_QueueCrossDungeonArrival(10) && !MM_QueueCrossDungeonArrival(20));
    }
    assert(ComboCrossOotRuntime::state.queued==1 && ComboCrossMmRuntime::state.queued==1);
}
'''
with tempfile.TemporaryDirectory(prefix='solo-runtime-') as temp:
    path=Path(temp);(path/'test.cpp').write_text(source)
    subprocess.run(['c++','-std=c++20','-Wall','-Wextra','-Werror','-I'+str(ROOT),str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('Cross-dungeon and boss host gates: stale route state cannot activate either solo game PASS')
