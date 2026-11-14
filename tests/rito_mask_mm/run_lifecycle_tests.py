#!/usr/bin/env python3
"""Never free actor/effect addresses from a departed MM play arena."""
from pathlib import Path
import sys,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function
f=(ROOT/'mm/mods/forms/rito_flight.inc.c').read_text()
b=(ROOT/'mm/mods/forms/rito_bow.inc.c').read_text()
c=(ROOT/'mm/mods/forms/form_rito.cpp').read_text()
state=f[f.index('#define RITO_CHARGE_MIN'):f.index('static struct {',f.index('#define RITO_CHARGE_MIN'))]
bstate=b[b.index('#define RITO_BOW_VOLLEY'):b.index('static RitoBow sRitoBow;')+len('static RitoBow sRitoBow;')]
code=r'''#include "global.h"
#include "mods/items/objects/object_tornado.h"
#include <cassert>
#include <cstring>
#include <cstdio>
static int kills,stops,restores;
extern "C" void Actor_Kill(Actor*){++kills;}
extern "C" void Tornado_RibbonsStop(PlayState*,TornadoRibbons*){++stops;}
static void MmRitoRestoreLanding(){++restores;}
'''+state+'\n'+bstate+r'''
#define RITO_FLAME_MAX 8
#define RITO_WIND_GROW_MIN 0.35f
static TornadoRibbons sRitoWindRibbons;
static Actor* sRitoFlames[8];
static u8 sRitoWindOn;
static f32 sRitoWindFade,sRitoWindGrow;
static struct {s16 lastHealth;u8 hurt;} sRitoDamage;
static PlayState* sRitoPlay;
static Player* sRitoPlayer;
static u32 sRitoLastFrame;
'''
for source,name in ((f,'MmRitoWindClear'),(f,'MmRitoClearFlightState'),(b,'MmRitoBowClearArrows'),(b,'MmRitoBowReset'),(c,'RitoForm_ForgetContext'),(c,'RitoForm_SyncContext')):
 code+='\n'+function(source,name)
code+=r'''
static void poison(){
 sRito.state=RITO_FLY_OFF;sRitoBow.state=RITO_BOW_OFF;
 sRitoWindOn=1;sRitoWindRibbons.active=1;
 for(int i=0;i<8;++i)sRitoFlames[i]=(Actor*)(uintptr_t)(1+i);
 for(int i=0;i<3;++i){sRitoBow.arrows[i].arrow=(Actor*)1;sRitoBow.arrows[i].target=(Actor*)2;}
}
static void clean(){assert(!sRitoWindOn&&!sRitoWindRibbons.active);for(auto p:sRitoFlames)assert(!p);
 for(auto a:sRitoBow.arrows)assert(!a.arrow&&!a.target);assert(kills==0&&stops==0);}
int main(){
 PlayState a{},b{};Player p{},q{};a.gameplayFrames=100;b.gameplayFrames=1;
 RitoForm_SyncContext(&p,&a);poison();RitoForm_SyncContext(&q,&b);clean();
 poison();RitoForm_ForgetContext();clean(); // actual save-load/scene-destroy callback
 RitoForm_SyncContext(&p,&a);poison();a.gameplayFrames=0;RitoForm_SyncContext(&p,&a);clean();
 poison();RitoForm_SyncContext(&q,&a);clean();
 puts("PASS MM Rito no-kill lifecycle: scene arena, file load, frame rollback, player replacement; lingering OFF effects cleared");
}
'''
with tempfile.TemporaryDirectory(prefix='rito-mm-lifecycle-') as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 inc=['-I'+str(ROOT/v) for v in ('mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h','libultraship/include','combo','combo/menu')]
 subprocess.run(['c++','-std=c++20','-w','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t',*inc,str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
