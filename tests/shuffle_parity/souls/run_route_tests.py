#!/usr/bin/env python3
"""Execute actual timed shop, NPC opening, Moon and independent alternative routes."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[3]
logic=(ROOT/'mm/2s2h/Rando/Logic/Logic.h').read_text()
def body(s,signature):
 a=s.index(signature);e=s.index('{',a)+1;d=1
 while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]
def calls(s,name):
 for m in re.finditer(r'\b'+name+r'\(',s):
  a=m.end();e=a;d=1
  while d:d+=(s[e]=='(')-(s[e]==')');e+=1
  chunk=s[a:e-1];parts=[];last=0;depth=0
  for i,c in enumerate(chunk):
   depth+=(c=='(')-(c==')')
   if c==',' and depth==0:parts.append(chunk[last:i].strip());last=i+1
  parts.append(chunk[last:].strip());yield parts
central=(ROOT/'mm/2s2h/Rando/Logic/Regions/Central.cpp').read_text()
north=(ROOT/'mm/2s2h/Rando/Logic/Regions/North.cpp').read_text()
south=(ROOT/'mm/2s2h/Rando/Logic/Regions/South.cpp').read_text()
moon=(ROOT/'mm/2s2h/Rando/Logic/Regions/Moon.cpp').read_text()
moonRoutes=[args[-1] for args in calls(moon,'EXIT') if 'SoulItemRequirement(RI_SOUL_NPC_MOON_CHILDREN)' in args[-1]]
assert len(moonRoutes)==9
shop=[args[-1] for args in calls(central,'CHECK') if args[0].startswith('RC_TRADING_POST_SHOP_ITEM_')]
assert len(shop)==8
meet=next(x[-1] for x in calls(central,'EVENT') if x[0]=='RE_MEET_KAFEI')
sirloin=next(x[-1] for x in calls(north,'EVENT') if x[0]=='RE_SPAWN_SIRLOIN')
princess=next(x[-1] for x in calls(south,'EVENT') if x[0]=='RE_RETURN_DEKU_PRINCESS')
hideout=next(x[-1] for x in calls(central,'EXIT') if x[0]=='ENTRANCE(CURIOSITY_SHOP, 1)')
bomber=next(x[-1] for x in calls(central,'EXIT') if x[0]=='ENTRANCE(ASTRAL_OBSERVATORY, 0)')
cpp='''#include "mm/2s2h/Rando/SoulShuffle.h"
#include "mm/include/z64item.h"
#include "soh/tests/test_require.h"
#include <set>
#include <functional>
unsigned options[RO_MAX]{};std::set<int> owned;bool events[RE_MAX]{};
#define RANDO_SAVE_OPTIONS options
#define RANDO_EVENTS events
bool Flags_GetRandoInf(RandoInf f){return owned.count(f);}
void Own(RandoItemId i,bool on=true){auto*s=Rando::SoulShuffle::Find(i);REQUIRE(s);if(on)owned.insert(s->flag);else owned.erase(s->flag);}
'''+body(logic,'enum TimeSlice')+''';
int now=TIME_DAY1_AM_08_00;unsigned clocks=63;bool clockSetting=true;
bool RawBefore(TimeSlice s){return now<s;}bool RawAfter(TimeSlice s){return now>=s;}
bool RawBetween(TimeSlice a,TimeSlice b){return now>=a&&now<b;}
bool SettingClocks(){return clockSetting;}bool OwnsHalfDayForMode(int i){return clocks&(1u<<i);}
'''
for line in logic.splitlines():
 if line.startswith(('#define CLOCK_DAY','#define CLOCK_NIGHT','#define IS_DAY1(', '#define IS_DAY2(', '#define IS_DAY3(', '#define IS_NIGHT1(', '#define IS_NIGHT2(', '#define IS_NIGHT3(', '#define BEFORE(', '#define BETWEEN(')):cpp+=line+'\n'
cpp+=body(logic,'inline bool ClockFilter(')+'\n'+body(logic,'inline bool SoulItemRequirement(')+'\n'+body(logic,'inline bool SoulEventRequirement(')+'\n'+body(logic,'inline bool TradingPostSoulRequirement(')+'\n'
save=(ROOT/'mm/include/z64save.h').read_text();b=save.index('} DungeonSceneIndex;')+len('} DungeonSceneIndex;');a=save.rfind('typedef enum {',0,b)
nativeDungeonEnum=save[a:b]
cpp+='''bool capability=true,fire=false,song=true,stick=true,afford=true;
#define CAN_BE_DEKU capability
#define CAN_BE_GORON capability
#define CAN_BE_ZORA capability
#define HAS_MAGIC capability
#define CAN_USE_MAGIC_ARROW(x) fire
#define CAN_PLAY_SONG(x) song
#define HAS_ITEM(x) stick
#define CHECK_QUEST_ITEM(x) capability
#define CAN_AFFORD(x) afford
#define CAN_USE_PROJECTILE capability
#define HAS_BOTTLE capability
#define CAN_ACCESS(x) capability
unsigned maskCount=20,remainsCount=4;
unsigned MoonMaskCount(){return maskCount;}unsigned RemainsCount(){return remainsCount;}
'''+nativeDungeonEnum+'\n'+body(logic,'inline bool CanAccessDungeon(')+'''
std::function<bool()> moonRoutes[]={
'''+',\n'.join('[]{return '+e+';}' for e in moonRoutes)+'};\nstd::function<bool()> shop[]={\n'+',\n'.join('[]{return '+e+';}' for e in shop)+'};\n'
for name,expr in [('Meet',meet),('Sirloin',sirloin),('Princess',princess),('Hideout',hideout),('Bomber',bomber)]:
 if name in ['Meet','Princess']:expr=f'SoulEventRequirement(RE_{"MEET_KAFEI" if name=="Meet" else "RETURN_DEKU_PRINCESS"}) && ({expr})'
 cpp+=f'bool {name}(){{return {expr};}}\n'
cpp+='''int main(){options[RO_SHUFFLE_NPC_SOULS]=1;
// Exhaust every native solver slice against each shift ownership combination.
for(now=0;now<=TIME_NIGHT3_AM_05_00;++now)for(int bits=0;bits<4;++bits){
 owned.clear();if(bits&1)Own(RI_SOUL_NPC_FISHING_POND_OWNER);if(bits&2)Own(RI_SOUL_NPC_ROOFTOP_MAN);
 bool curiosity=(now>=TIME_DAY1_AM_06_00&&now<TIME_NIGHT1_PM_10_00)||(now>=TIME_DAY2_AM_06_00&&now<TIME_NIGHT2_PM_10_00);
 bool worker=(now>=TIME_NIGHT1_PM_09_00&&now<TIME_DAY2_AM_06_00)||(now>=TIME_NIGHT2_PM_09_00&&now<TIME_DAY3_AM_06_00)||(now>=TIME_DAY3_AM_06_00&&now<TIME_NIGHT3_PM_10_00);
 bool expected=(curiosity&&(bits&1))||(worker&&(bits&2));REQUIRE(TradingPostSoulRequirement()==expected);
 for(auto&check:shop){afford=true;REQUIRE(check()==expected);afford=false;REQUIRE(!check());}afford=true;
 clocks=0;REQUIRE(!TradingPostSoulRequirement());clocks=63;
}
options[RO_SHUFFLE_NPC_SOULS]=0;owned.clear();REQUIRE(TradingPostSoulRequirement());options[RO_SHUFFLE_NPC_SOULS]=1;
for(auto&route:moonRoutes){owned.clear();options[RO_ACCESS_TRIALS]=RO_ACCESS_TRIALS_OPEN;capability=fire=true;REQUIRE(!route());Own(RI_SOUL_NPC_MOON_CHILDREN);REQUIRE(route());owned.clear();options[RO_SHUFFLE_NPC_SOULS]=0;REQUIRE(route());options[RO_SHUFFLE_NPC_SOULS]=1;}
owned.clear();fire=false;song=stick=capability=true;REQUIRE(!Sirloin());Own(RI_SOUL_NPC_GORON_CHILD);REQUIRE(Sirloin());
capability=false;REQUIRE(!Sirloin());capability=true;owned.clear();fire=true;REQUIRE(Sirloin());fire=false;
owned.clear();Own(RI_SOUL_NPC_DEKU_PRINCESS);REQUIRE(!Princess());Own(RI_SOUL_NPC_DEKU_KING);REQUIRE(Princess());capability=false;REQUIRE(!Princess());capability=true;
now=TIME_DAY2_PM_04_00;events[RE_MAIL_LETTER_TO_KAFEI]=true;owned.clear();REQUIRE(!Meet()&&!Hideout());Own(RI_SOUL_NPC_KAFEI);REQUIRE(!Meet()&&!Hideout());Own(RI_SOUL_NPC_CITIZEN);REQUIRE(Meet()&&Hideout());
now=TIME_DAY2_AM_08_00;REQUIRE(!Meet()&&!Hideout());now=TIME_DAY2_PM_04_00;clocks=0;REQUIRE(!Meet()&&!Hideout());clocks=63;
now=TIME_DAY3_PM_01_00;owned.clear();events[RE_MEET_KAFEI]=true;REQUIRE(Hideout());
owned.clear();capability=true;REQUIRE(!Bomber());Own(RI_SOUL_NPC_BOMBERS);REQUIRE(Bomber());capability=false;REQUIRE(!Bomber());
for(int mode:{RO_ACCESS_DUNGEONS_FORM_AND_SONG,RO_ACCESS_DUNGEONS_FORM_OR_SONG,RO_ACCESS_DUNGEONS_FORM_ONLY,RO_ACCESS_DUNGEONS_SONG_ONLY}){
 options[RO_ACCESS_DUNGEONS]=mode;owned.clear();song=capability=stick=true;
 REQUIRE(!CanAccessDungeon(DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE));Own(RI_SOUL_NPC_BIGGORON);REQUIRE(CanAccessDungeon(DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE));
 capability=false;song=true;REQUIRE(CanAccessDungeon(DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE)==(mode==RO_ACCESS_DUNGEONS_FORM_OR_SONG||mode==RO_ACCESS_DUNGEONS_SONG_ONLY));
 capability=true;song=false;REQUIRE(CanAccessDungeon(DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE)==(mode==RO_ACCESS_DUNGEONS_FORM_OR_SONG||mode==RO_ACCESS_DUNGEONS_FORM_ONLY));
}
owned.clear();options[RO_ACCESS_DUNGEONS]=RO_ACCESS_DUNGEONS_OPEN;song=capability=stick=false;REQUIRE(CanAccessDungeon(DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE));
puts("PASS actual native route predicates: 45 TradingPost slices, 9 Moon NPC routes, Sirloin alternative, Princess and Postman/Kafei/Bomber controls");}
'''
with tempfile.TemporaryDirectory(prefix='souls-routes-') as t:
 t=Path(t);src=t/'routes.cpp';src.write_text(cpp)
 for mode in ('original','combo'):
  exe=t/mode;subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
