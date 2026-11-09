#!/usr/bin/env python3
"""Run actual MM stored EVENT predicates with native soul/event identifiers."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[3]
s=(ROOT/'mm/2s2h/Rando/Logic/Logic.h').read_text()
a=s.index('#define EVENT(');b=s.index('#define EXIT(',a)
helper=''
if 'inline bool SoulEventRequirement(' in s:
 a1=s.index('inline bool SoulEventRequirement(');b1=s.index('\n}',a1)+2;helper=s[a1:b1]
cpp='''#include "mm/2s2h/Rando/SoulShuffle.h"
#include "soh/tests/test_require.h"
#include <map>
#include <functional>
unsigned options[RO_MAX]{};unsigned short flags[(RANDO_INF_MAX+15)/16]{};
#define RANDO_SAVE_OPTIONS options
bool Flags_GetRandoInf(RandoInf f){return flags[f/16]&(1u<<(f%16));}
bool song=false;
#define CAN_PLAY_SONG(x) song
'''+helper+'\n'+s[a:b]+'\nstd::map<RandoEvent,std::function<bool()>> events={\n'
for file,event in [('Central','RE_TERMINA_FIELD_SCRUB_ENTERED_GROTTO'),('BeneathTheWell','RE_ACCESS_MILK_REFILL')]:
 source=(ROOT/f'mm/2s2h/Rando/Logic/Regions/{file}.cpp').read_text();line=next(l for l in source.splitlines() if 'EVENT('+event+',' in l);cpp+=line+'\n'
cpp+='''};
int main(){
 options[RO_SHUFFLE_MISC_SOULS]=1;options[RO_SHUFFLE_ANIMAL_SOULS]=1;
 song=true;
 REQUIRE(!events.at(RE_TERMINA_FIELD_SCRUB_ENTERED_GROTTO)());
 REQUIRE(!events.at(RE_ACCESS_MILK_REFILL)());
 for(auto item:{RI_SOUL_ANIMAL_COW,RI_SOUL_MISC_BUSINESS_SCRUB}){
   auto*s=Rando::SoulShuffle::Find(item);flags[s->flag/16]|=1u<<(s->flag%16);
 }
 REQUIRE(events.at(RE_TERMINA_FIELD_SCRUB_ENTERED_GROTTO)());
 REQUIRE(events.at(RE_ACCESS_MILK_REFILL)());
 song=false;REQUIRE(!events.at(RE_ACCESS_MILK_REFILL)());
 for(auto &flag:flags)flag=0;
 options[RO_SHUFFLE_MISC_SOULS]=0;options[RO_SHUFFLE_ANIMAL_SOULS]=0;song=true;
 REQUIRE(events.at(RE_TERMINA_FIELD_SCRUB_ENTERED_GROTTO)());
 REQUIRE(events.at(RE_ACCESS_MILK_REFILL)());
 puts("PASS actual MM stored event predicates: missing/owned/off and unrelated song requirement");}
'''
with tempfile.TemporaryDirectory(prefix='soul-events-') as t:
 t=Path(t);src=t/'events.cpp';src.write_text(cpp)
 for mode in ('original','combo'):
  exe=t/mode
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(src),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)

# OoT event rows are source-qualified rather than global event-ID gates: root
# setting shortcuts must never inherit an NPC requirement accidentally.
access=(ROOT/'soh/soh/Enhancements/randomizer/location_access.h').read_text()
a=access.index('#define EVENT_ACCESS(');b=access.index('class EventAccess',a)
source='''#include "soh/soh/Enhancements/randomizer/SoulShuffle.h"
#include "soh/tests/test_require.h"
#include <set>
#include <string>
struct Logic {
 bool IsChild=true,enabled=true,jumpslash=true,bunny=false;
 std::set<RandomizerGet> items;
 bool HasItem(RandomizerGet item){return (Rando::SoulShuffle::Find(item)&&!enabled)||items.count(item);}
 bool CanUse(RandomizerGet item){return items.count(item);}
 bool CanJumpslash(){return jumpslash;}
 bool BunnyHood(){return bunny;}
} state;auto*logic=&state;
int price=200,capacity=200;
int GetCheckPrice(RandomizerCheck){return price;}
int GetWalletCapacity(){return capacity;}
std::string CleanConditionString(const char*s){return s;}
struct EventAccess {LogicVal id;const char*name;bool(*condition)();std::string text;};
'''+access[a:b]+'\nEventAccess events[]={\n'
for file,region,event in [('haunted_wasteland','RR_HAUNTED_WASTELAND','LOGIC_CARPET_MERCHANT'),
                          ('kakariko','RR_KAK_WINDMILL_LOWER','LOGIC_DRAIN_WELL'),
                          ('market','RR_MARKET_BOMBCHU_BOWLING','LOGIC_COULD_PLAY_BOWLING')]:
 text=(ROOT/f'soh/soh/Enhancements/randomizer/location_access/overworld/{file}.cpp').read_text()
 start=text.index('areaTable['+region+']');end=text.find('areaTable[',start+1)
 line=next(l for l in text[start:end].splitlines() if 'EVENT_ACCESS('+event+',' in l)
 source+=line+'\n'
source+='''};
int main(){
 state.items={RG_CHILD_WALLET,RG_SPEAK_HYLIAN};REQUIRE(!events[2].condition());
 state.items.insert(RG_SOUL_NPC_BOMBCHU_BOWLING_LADY);REQUIRE(events[2].condition());
 state.items.erase(RG_CHILD_WALLET);REQUIRE(!events[2].condition());state.items.insert(RG_CHILD_WALLET);
 state.items.erase(RG_SPEAK_HYLIAN);REQUIRE(!events[2].condition());state.items.insert(RG_SPEAK_HYLIAN);
 state.items.erase(RG_SOUL_NPC_BOMBCHU_BOWLING_LADY);state.enabled=false;REQUIRE(events[2].condition());state.enabled=true;
 state.items={RG_ADULT_WALLET};REQUIRE(!events[0].condition());
 state.items.insert(RG_SOUL_NPC_CARPET_MAN);REQUIRE(events[0].condition());
 capacity=199;REQUIRE(!events[0].condition());capacity=200;
 state.items.erase(RG_ADULT_WALLET);REQUIRE(!events[0].condition());state.items.insert(RG_ADULT_WALLET);
 state.jumpslash=false;REQUIRE(!events[0].condition());
 state.items.insert(RG_HOVER_BOOTS);REQUIRE(events[0].condition());state.items.erase(RG_HOVER_BOOTS);
 state.bunny=true;REQUIRE(events[0].condition());state.bunny=false;state.jumpslash=true;
 state.items.erase(RG_SOUL_NPC_CARPET_MAN);state.enabled=false;REQUIRE(events[0].condition());
 state.enabled=true;state.items={RG_SONG_OF_STORMS};REQUIRE(events[1].condition());
 state.IsChild=false;REQUIRE(!events[1].condition());state.IsChild=true;
 state.items.clear();REQUIRE(!events[1].condition());
 puts("PASS actual OoT stored events: Carpet refill soul/price/route controls and independent child well drain");
}
'''
with tempfile.TemporaryDirectory(prefix='soul-oot-events-') as t:
 t=Path(t);cpp=t/'events.cpp';cpp.write_text(source)
 for mode in ('original','combo'):
  exe=t/mode
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),'-I'+str(ROOT/'soh/include'),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(cpp),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
