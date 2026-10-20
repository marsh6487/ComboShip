#!/usr/bin/env python3
"""Execute the OoT scene binding with native Gfx generators and real headers."""
from pathlib import Path
import os,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
source=(ROOT/'soh/soh/Enhancements/Gameplay/ComboSoloForest.cpp').read_text()
signature='extern "C" void ComboSoloForestOot_DrawMaterials('
body=block_from(source,source.index(signature)) if signature in source else 'void ComboSoloForestOot_DrawMaterials(PlayState*) {}'
rcp=(ROOT/'soh/src/code/z_rcp.c').read_text()
wrapper=(ROOT/'soh/soh/GbiWrap.cpp').read_text()
bridge='\n'.join(block_from(wrapper,wrapper.index(sig)) for sig in ['extern "C" void gDPSetTileSizeLerp(', 'extern "C" void gSPSegment('])
generators=bridge+'\n'+'\n'.join(block_from(rcp,rcp.index(s)) for s in ['Gfx* Gfx_TexScrollEx(', 'Gfx* Gfx_TwoTexScrollEx('])
fixture=r'''
#include "global.h"
#include "ComboGameMode.h"
#include "entrances/SoloForest.h"
#include "entrances/SoloForestScroll.h"
#include <cassert>
#include <array>
#include <cstdio>
#undef OPEN_DISPS
#undef CLOSE_DISPS
#define OPEN_DISPS(ctx) { GraphicsContext* __gfxCtx=(ctx)
#define CLOSE_DISPS(ctx) }
extern "C" int ResourceMgr_OTRSigCheck(char*) { return 0; }
extern "C" char* ResourceMgr_LoadIfDListByName(char*) { return nullptr; }
extern "C" { int gComboGameMode=COMBO_GAME_OOT; }
SaveContext gSaveContext{};
static bool available=true;
extern "C" u8 ResourceMgr_FileExists(const char*) { return available; }
static std::array<Gfx,512> arena;
static size_t used=0;
extern "C" void* Graph_Alloc(GraphicsContext*,size_t bytes) {
    assert(bytes%sizeof(Gfx)==0 && used+bytes/sizeof(Gfx)<=arena.size());
    auto* p=&arena[used];used+=bytes/sizeof(Gfx);return p;
}
/* VISIT */
/* GENERATORS */
/* PRODUCTION */
int main() {
 GraphicsContext gfx{}; PlayState play{}; play.state.gfxCtx=&gfx;
 gSaveContext.gameMode=GAMEMODE_NORMAL;gSaveContext.fileNum=0;gSaveContext.sceneLayer=0;
 play.sceneNum=SCENE_SKIEJER_FOREST;
 for(unsigned frame: {0U,1U,2047U,2048U,65535U,0xFFFFFFFFU}) {
  for(int room: {0,1}) for(int layer: {0,1,2,3}) {
   gSaveContext.sceneLayer=layer;
   play.roomCtx.curRoom.num=room;play.gameplayFrames=frame;
   Gfx opa[16]{},xlu[16]{};gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;used=0;
   ComboSoloForestOot_DrawMaterials(&play);
   assert(gfx.polyOpa.p-opa==6 && gfx.polyXlu.p-xlu==6);
   for(int i=0;i<6;++i) {
    const auto& binding=SoloForestScroll::kBindings[i];
    assert((opa[i].words.w0&0xFFFF)==binding.segment*4);
    assert(opa[i].words.w0==xlu[i].words.w0 && opa[i].words.w1==xlu[i].words.w1);
    const auto& a=binding.layers[0]; const auto& b=binding.layers[1];
    Gfx* expected=binding.count==1 ? Gfx_TexScrollEx(&gfx,frame*a.x,frame*a.y,a.width,a.height,a.x,a.y):
       Gfx_TwoTexScrollEx(&gfx,0,frame*a.x,frame*a.y,a.width,a.height,1,
                         frame*b.x,frame*b.y,b.width,b.height,a.x,a.y,b.x,b.y);
    const auto* actual=reinterpret_cast<const Gfx*>(opa[i].words.w1);
    for(int c=0;c<(binding.count==1?7:12);++c)
      assert(actual[c].words.w0==expected[c].words.w0 && actual[c].words.w1==expected[c].words.w1);
   }
  }
 }
 for(int guard=0;guard<7;++guard) {
  Gfx opa[16]{},xlu[16]{};gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;used=0;
  play.sceneNum=SCENE_SKIEJER_FOREST;play.roomCtx.curRoom.num=0;
  gComboGameMode=COMBO_GAME_OOT;gSaveContext.sceneLayer=0;gSaveContext.fileNum=0;available=true;
  if(guard==0)gComboGameMode=COMBO_GAME_BOTH;
  if(guard==1)gComboGameMode=COMBO_GAME_MM;
  if(guard==2)play.sceneNum=SCENE_LOST_WOODS;
  if(guard==3)play.roomCtx.curRoom.num=2;
  if(guard==4)gSaveContext.sceneLayer=4;
  if(guard==5)gSaveContext.fileNum=255;
  if(guard==6)available=false;
  ComboSoloForestOot_DrawMaterials(&play);
  assert(gfx.polyOpa.p==opa && gfx.polyXlu.p==xlu && used==0);
 }
 puts("PASS OoT final scroll: six OPA/XLU native interpolated lists, both rooms, exact rates/sizes, wrap and mode/scene/pack guards");
}
'''
visit=block_from(source,source.index('bool ForestVisit('))
fixture=fixture.replace('/* VISIT */',visit).replace('/* GENERATORS */',generators).replace('/* PRODUCTION */',body)
# Before the converter emits the real header, derive the expected existing FINAL
# data solely for the fixture; never make production a test-only dependency.
header=ROOT/'combo/entrances/SoloForestScroll.h'
assert header.exists(),'generate FINAL resource contracts before this native scroll probe'
with tempfile.TemporaryDirectory(prefix='final-scroll-native-') as temp:
 path=Path(temp)/'test.cpp';path.write_text(fixture);binary=Path(temp)/'test'
 includes=['soh','soh/include','soh/include/PR','soh/src','soh/assets','soh/soh','combo','libultraship/include']
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-fpermissive','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD','-g',
  '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
  *['-I'+str(ROOT/p) for p in includes],str(path),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
