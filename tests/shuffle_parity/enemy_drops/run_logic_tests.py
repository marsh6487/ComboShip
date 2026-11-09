#!/usr/bin/env python3
"""Exercise first-drop witness lambdas with the complete native combat body.

This verifies the native combat/soul and disabled-trick boundary. It does not
claim a seeded full-graph fill or prove witnesses missing from the source audit.
"""
from pathlib import Path
import os,re,subprocess,tempfile,sys
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
r=ROOT/'soh/soh/Enhancements/randomizer'
native=(r/'logic.cpp').read_text()
combat=''
for name,next_name in [('CanKillEnemy','CanPassEnemy'),('CanGetDekuBabaSticks','CanGetDekuBabaNuts'),('CanGetDekuBabaNuts','CanHitEyeTargets')]:
 start=native.index('bool Logic::'+name+'(');end=native.index('\nbool Logic::'+next_name+'(',start)
 combat+=native[start:end]+'\n'
witness=(r/'EnemyFirstDropsWitnesses.inc').read_text()+'\n'+(r/'EnemyFirstDropsAdditionalWitnesses.inc').read_text()
methods=set(re.findall(r'logic->(\w+)\(',witness))
base={'HasItem','CanUse','CanKillEnemy','Get','CanGetDekuBabaSticks','CanGetDekuBabaNuts','EffectiveHealth'}
methods|={'BlastOrSmash','CanAttack','CanDamage','CanJumpslash','CanJumpslashExceptHammer','CanReflectNuts','CanStandingShield','CanUseSword','HasExplosives','HasFireSource','HookshotOrBoomerang','TakeDamage'}
source=r'''
#include "soh/soh/Enhancements/randomizer/EnemyFirstDrops.h"
#include "combo/rando/CrossEntranceBoss.h"
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <set>
#include <vector>
#include <map>
#define REQUIRE(expr) do{if(!(expr)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#expr);abort();}}while(0)
#define SPDLOG_ERROR(...) ((void)0)
struct Option {bool Get(){return true;}operator bool(){return true;}bool Is(int){return false;}};
struct Context {
 bool GetTrickOption(RandomizerTrick){return false;}
 Option GetOption(RandomizerSettingKey){return {};}
} context;auto*ctx=&context;
namespace Rando {
struct Logic {
 bool IsAdult=true,IsChild=false,AtDay=true,AtNight=false,combat=true,soulsEnabled=true;
 RandomizerGet missing=RG_NONE;
 Context*ctx=&context;
 bool HasItem(RandomizerGet item){
  if(item>=RG_SOUL_ENEMY_STALFOS&&item<=RG_SOUL_ENEMY_THIEVES)return !soulsEnabled||item!=missing;
  return combat;
 }
 bool CanUse(RandomizerGet){return combat;}
 bool Get(LogicVal){return true;}
 int EffectiveHealth(){return combat?99:0;}
 bool CanKillEnemy(RandomizerEnemy,EnemyDistance=ED_CLOSE,bool=true,uint8_t=1,bool=false,bool=false);
 bool CanGetDekuBabaSticks();bool CanGetDekuBabaNuts();
'''
for method in sorted(methods-base):source+=f'template<class...T> bool {method}(T...){{return combat;}}\n'
source+='};\n'+combat+'\n}\nRando::Logic state;auto*logic=&state;\n'
source+='template<class F> bool AnyAgeTime(F f){return f();}\n'
source+='#ifdef COMBO_BUILD\nextern "C" int Combo_SOH_CrossBossLogicalClear(int, int cleared){return cleared;}\n#endif\n'
source+='struct Witness{RandomizerCheck check;RandomizerGet soul;RandomizerRegion region;bool(*condition)();};\nWitness witnesses[]={\n'
source+='#define ENEMY_DROP_WITNESS(key,region,condition) {RC_ENEMY_DROP_##key,RG_SOUL_ENEMY_##key,region,[]{return logic->HasItem(RG_SOUL_ENEMY_##key)&&(condition);}},\n#include "soh/soh/Enhancements/randomizer/EnemyFirstDropsWitnesses.inc"\n#include "soh/soh/Enhancements/randomizer/EnemyFirstDropsAdditionalWitnesses.inc"\n#undef ENEMY_DROP_WITNESS\n};\n'
source+='void CheckWitnessIdentity(){\n'
for line in witness.splitlines():
 if not line.startswith('ENEMY_DROP_WITNESS('):continue
 key=re.match(r'ENEMY_DROP_WITNESS\((\w+)',line)[1]
 enemies=re.findall(r'CanKillEnemy\((RE_\w+)',line)
 if not enemies:continue
 enemy=enemies[0] if key=='GOHMA_LARVA' else enemies[-1]
 source+=f'REQUIRE(Rando::SoulShuffle::EnemyForLogic({enemy})==RG_SOUL_ENEMY_{key});\n'
source+='}\n'
source+=r'''
int main(){
 CheckWitnessIdentity();
 std::set<RandomizerCheck> covered, reachable;
 for(auto& witness:witnesses){
  REQUIRE(witness.region!=RR_ROOT);covered.insert(witness.check);
  state.combat=true;state.soulsEnabled=true;state.missing=witness.soul;
  REQUIRE(!witness.condition()); // own soul cannot be placed behind its type's check
  state.missing=RG_NONE;
  for(bool adult:{false,true})for(bool night:{false,true}){
   state.IsAdult=adult;state.IsChild=!adult;state.AtNight=night;state.AtDay=!night;
   if(witness.condition())reachable.insert(witness.check);
  }
  state.combat=false;REQUIRE(!witness.condition());
  state.combat=true;state.missing=witness.soul;state.soulsEnabled=false;
  // Disabled family restores the original weapon predicate.
  bool off=false;
  for(bool adult:{false,true})for(bool night:{false,true}){state.IsAdult=adult;state.IsChild=!adult;state.AtNight=night;state.AtDay=!night;off|=witness.condition();}
  state.soulsEnabled=true;
 }
 REQUIRE(covered.size()==39);REQUIRE(reachable==covered);
 printf("PASS %zu genuine native combat witness types: soul self-lock, weapons, disabled tricks/family controls\n",covered.size());
}
'''
with tempfile.TemporaryDirectory(prefix='oot-enemy-drop-logic-') as t:
 t=Path(t);cpp=t/'test.cpp';cpp.write_text(source)
 for mode in ('original','combo'):
  exe=t/mode
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/'soh/include'),'-I'+str(ROOT/'combo'),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
