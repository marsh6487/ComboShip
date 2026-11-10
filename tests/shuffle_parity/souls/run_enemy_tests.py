#!/usr/bin/env python3
"""Exercise full native OoT CanKillEnemy plus the parameter-aware actor classifier."""
from pathlib import Path
import os, subprocess, tempfile, sys
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
header=ROOT/'soh/soh/Enhancements/randomizer/SoulShuffleEnemy.h'
assert header.exists(), 'missing native OoT enemy soul classifier'
source=(ROOT/'soh/soh/Enhancements/randomizer/logic.cpp').read_text()
start=source.index('bool Logic::CanKillEnemy(')
end=source.index('\nbool Logic::CanPassEnemy(',start)
kill=source[start:end]
for name,next_name in [('CanGetDekuBabaSticks','CanGetDekuBabaNuts'),('CanGetDekuBabaNuts','CanHitEyeTargets')]:
    a=source.index('bool Logic::'+name+'(');b=source.index('\nbool Logic::'+next_name+'(',a)
    kill+='\n'+source[a:b]
assert 'SoulShuffle::EnemyForLogic' in kill, 'native combat path bypasses enemy ownership'
with tempfile.TemporaryDirectory(prefix='soul-enemy-') as tmp:
    tmp=Path(tmp)
    fixture=r'''
#include <cassert>
#include <cstdint>
#include <set>
#include "soh/soh/Enhancements/randomizer/SoulShuffleEnemy.h"
#define SPDLOG_ERROR(...) ((void)0)
namespace Rando {
struct FakeContext { bool GetTrickOption(RandomizerTrick) { return false; } };
struct Logic {
    bool combat=true, IsAdult=true;
    bool enabled=false; RandomizerGet owned=RG_NONE;
    FakeContext storage; FakeContext* ctx=&storage;
    bool HasItem(RandomizerGet id) {
        if (id >= RG_SOUL_ENEMY_STALFOS && id <= RG_SOUL_MISC_BUSINESS_SCRUB)
            return !enabled || id==owned;
        return combat;
    }
    bool CanUse(RandomizerGet){return combat;}
    bool BlastOrSmash(){return combat;} bool CanAttack(){return combat;}
    bool CanDamage(){return combat;} bool CanJumpslash(){return combat;}
    bool CanJumpslashExceptHammer(){return combat;} bool CanReflectNuts(){return combat;}
    bool CanStandingShield(){return combat;} bool CanUseSword(){return combat;}
    bool HasExplosives(){return combat;} bool HasFireSource(){return combat;}
    bool HookshotOrBoomerang(){return combat;} bool TakeDamage(){return combat;}
    int EffectiveHealth(){return combat?99:0;}
    bool CanKillEnemy(RandomizerEnemy,EnemyDistance,bool,uint8_t,bool,bool);
    bool CanGetDekuBabaSticks(); bool CanGetDekuBabaNuts();
};
'''+kill+r'''
}
int main(){
    using namespace Rando;
    Logic logic;
    for(int i=RE_GOLD_SKULLTULA;i<=RE_WALLTULA;++i){
        auto e=(RandomizerEnemy)i;
        auto soul=SoulShuffle::EnemyForLogic(e);
        logic.enabled=false; logic.owned=RG_NONE; logic.combat=true;
        bool baseline=logic.CanKillEnemy(e,ED_CLOSE,true,1,false,false);
        logic.enabled=true;
        if(soul!=RG_NONE){
            assert(!logic.CanKillEnemy(e,ED_CLOSE,true,1,false,false));
            logic.owned=soul;
            assert(logic.CanKillEnemy(e,ED_CLOSE,true,1,false,false)==baseline);
            logic.combat=false;
            assert(!logic.CanKillEnemy(e,ED_CLOSE,true,1,false,false));
        } else assert(logic.CanKillEnemy(e,ED_CLOSE,true,1,false,false)==baseline);
    }
    logic.enabled=true;logic.combat=true;logic.owned=RG_NONE;
    assert(!logic.CanGetDekuBabaSticks() && !logic.CanGetDekuBabaNuts());
    logic.owned=RG_SOUL_ENEMY_DEKU_BABA;
    assert(logic.CanGetDekuBabaSticks() && logic.CanGetDekuBabaNuts());
    logic.combat=false;
    assert(!logic.CanGetDekuBabaSticks() && !logic.CanGetDekuBabaNuts());
    using SoulShuffle::EnemyForActor;
    assert(EnemyForActor(ACTOR_EN_TEST,0,false,false)==RG_SOUL_ENEMY_STALFOS);
    assert(EnemyForActor(ACTOR_EN_AM,0,false,false)==RG_NONE); // inert statue
    assert(EnemyForActor(ACTOR_EN_AM,1,false,false)==RG_SOUL_ENEMY_ARMOS);
    assert(EnemyForActor(ACTOR_EN_POH,2,false,false)==RG_NONE); // composer
    assert(EnemyForActor(ACTOR_EN_POH,3,false,false)==RG_NONE);
    assert(EnemyForActor(ACTOR_EN_POH,0,false,false)==RG_SOUL_ENEMY_POE);
    assert(EnemyForActor(ACTOR_EN_SW,0,false,false)==RG_SOUL_ENEMY_SKULLWALLTULA);
    assert(EnemyForActor(ACTOR_EN_SW,0x2000,false,false)==RG_SOUL_MISC_GS);
    assert(EnemyForActor(ACTOR_EN_SKJ,0,false,false)==RG_NONE); // child minigame
    assert(EnemyForActor(ACTOR_EN_SKJ,0,true,false)==RG_SOUL_ENEMY_SKULL_KID);
    assert(EnemyForActor(ACTOR_EN_GOMA,0,false,true)==RG_NONE); // Gohma's children
    assert(EnemyForActor(ACTOR_EN_GOMA,0,false,false)==RG_SOUL_ENEMY_GOHMA_LARVA);
    assert(EnemyForActor(ACTOR_EN_OSSAN,0,false,false)==RG_NONE); // custom shop/viewer
    assert(EnemyForActor(ACTOR_BOSS_GOMA,0,false,false)==RG_NONE); // existing boss path
    assert(EnemyForActor(-1,0,false,false)==RG_NONE);
    std::set<RandomizerGet> actorSouls;
    for(int actor=0;actor<ACTOR_ID_MAX;++actor) for(int params:{0,1,2,3,0x2000}) {
        auto soul=EnemyForActor(actor,params,true,false);
        if(soul>=RG_SOUL_ENEMY_STALFOS && soul<=RG_SOUL_ENEMY_THIEVES) actorSouls.insert(soul);
    }
    assert(actorSouls.size()==43); // every added native enemy soul has a real actor role

}
'''
    cpp=tmp/'test.cpp';cpp.write_text(fixture)
    for mode in ('original','combo'):
        exe=tmp/mode
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/'soh/include'),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(cpp),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
print('PASS full native OoT combat body: soul/weapon/disabled-family and role/parameter controls, both guards')
