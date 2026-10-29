#!/usr/bin/env python3
"""Execute native reverse-boss spawn and room repair bodies with native/cross shuffles independently."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
text=(ROOT/'soh/soh/Enhancements/randomizer/randomizer_entrance.c').read_text()
functions='\n'.join(block_from(text,text.index(marker)) for marker in [
    'void Entrance_OverrideSpawnScene(s32 sceneNum, s32 spawn)',
    's32 Entrance_OverrideSpawnSceneRoom(s32 sceneNum, s32 spawn, s32 roomNum)'])
types=(ROOT/'soh/include/z64.h').read_text()
entry_end=types.index('} ActorEntry;')+len('} ActorEntry;')
actor_entry=types[types.rfind('typedef struct {',0,entry_end):entry_end]
source=r'''
#include <cassert>
#include <cstdint>
#define COMBO_BUILD 1
using s32=int32_t;
using s16=int16_t;
struct Vec3s { s16 x,y,z; };
struct Vec3f { float x,y,z; };
/* NATIVE_ACTOR_ENTRY */
static ActorEntry modifiedLinkActorEntry;
static struct { ActorEntry* linkActorEntry=nullptr; } play;
static auto* gPlayState=&play;
enum { RSK_SHUFFLE_BOSS_ENTRANCES, RSK_SHUFFLE_DUNGEON_ENTRANCES, RSK_SHUFFLE_GANONS_TOWER_ENTRANCE, RSK_SHUFFLE_OVERWORLD_ENTRANCES };
enum { RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF=0, RO_GENERIC_ON=1, RO_DUNGEON_ENTRANCE_SHUFFLE_ON_PLUS_GANON=2 };
enum { SCENE_JABU_JABU=10, SCENE_WATER_TEMPLE, SCENE_OUTSIDE_GANONS_CASTLE, SCENE_HYRULE_FIELD, SCENE_FOREST_TEMPLE };
#define LINK_IS_CHILD false
#define IS_NIGHT false
static int nativeBoss=0; static bool crossBoss=false;
static int Randomizer_GetSettingValue(int setting) { return setting==RSK_SHUFFLE_BOSS_ENTRANCES?nativeBoss:0; }
[[maybe_unused]] static int Combo_SOH_CrossBossEnabled() { return crossBoss; }
'''+functions+r'''
int main() {
 for(int native:{0,1}) for(bool cross:{false,true}) for(int scene:{SCENE_JABU_JABU,SCENE_WATER_TEMPLE,SCENE_FOREST_TEMPLE}) for(int spawn:{0,1}) {
  nativeBoss=native;crossBoss=cross;
  ActorEntry original{1,{101,202,303},{404,505,606},707};play.linkActorEntry=&original;
  const bool repair=(native||cross)&&spawn==1&&scene!=SCENE_FOREST_TEMPLE;
  Entrance_OverrideSpawnScene(scene,spawn);
  auto* actual=play.linkActorEntry;
  assert(actual->id==1 && actual->params==707 && actual->rot.x==404 && actual->rot.z==606);
  assert(original.pos.x==101 && original.pos.y==202 && original.pos.z==303 && original.rot.y==505);
  if(repair&&scene==SCENE_JABU_JABU) {
   assert(actual->pos.x==101 && actual->pos.y==202 && actual->pos.z==static_cast<int16_t>(0xF7F4) && actual->rot.y==505);
  } else if(repair&&scene==SCENE_WATER_TEMPLE) {
   assert(actual->pos.x==static_cast<int16_t>(0xFF4C) && actual->pos.y==0x0406 && actual->pos.z==static_cast<int16_t>(0xF828) && actual->rot.y==0);
  } else {
   assert(actual==&original && actual->pos.x==101 && actual->pos.y==202 && actual->pos.z==303 && actual->rot.y==505);
  }
  const int room=Entrance_OverrideSpawnSceneRoom(scene,spawn,19);
  assert(room==(repair?(scene==SCENE_JABU_JABU?5:11):19) && "cross-only boss reverse must repair the physical dungeon room");
  assert(nativeBoss==native && crossBoss==cross); // Safety does not rewrite settings.
 }
 nativeBoss=0;crossBoss=false;
 ActorEntry wide{1,{100000.25f,-45000.5f,70000.75f},{404,505,606},707};
 play.linkActorEntry=&wide;
 Entrance_OverrideSpawnScene(SCENE_FOREST_TEMPLE,0);
 assert(play.linkActorEntry==&wide && wide.pos.x==100000.25f && wide.pos.y==-45000.5f && wide.pos.z==70000.75f);
 crossBoss=true;
 Entrance_OverrideSpawnScene(SCENE_JABU_JABU,1);
 assert(play.linkActorEntry->pos.x==100000.25f && play.linkActorEntry->pos.y==-45000.5f);
 assert(play.linkActorEntry->pos.z==static_cast<s16>(0xF7F4) && wide.pos.z==70000.75f);
}
'''
source=source.replace('/* NATIVE_ACTOR_ENTRY */',actor_entry)
# Range-for initializer lists are a fixture-only library dependency.
source='#include <initializer_list>\n'+source
sanitize=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g'] if '--sanitize' in sys.argv else []
if sanitize: os.environ['ASAN_OPTIONS']='detect_leaks=0'
with tempfile.TemporaryDirectory(prefix='oot-boss-spawn-') as temp:
    path=Path(temp);(path/'test.cpp').write_text(source)
    subprocess.run(['c++',*sanitize,'-std=c++20','-Wall','-Wextra','-Werror',str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
print('OoT boss spawn repair: native/cross combinations, signed negative coordinates, preserved wide fractional positions and room repair PASS')
