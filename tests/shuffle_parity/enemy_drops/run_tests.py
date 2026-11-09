#!/usr/bin/env python3
"""Execute production OoT defeat callbacks using real native engine headers."""
from pathlib import Path
import os,subprocess,tempfile,sys,re
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
f=functions((ROOT/'soh/src/code/z_actor.c').read_text())
goma=functions((ROOT/'soh/src/overlays/actors/ovl_En_Goma/z_en_goma.c').read_text())
gerudo_source=(ROOT/'soh/src/overlays/actors/ovl_En_GeldB/z_en_geldb.c').read_text()
gerudo=functions(gerudo_source)
gerudo_enum=next(m[0] for m in re.finditer(r'typedef enum\s*\{.*?\}\s*\w+;',gerudo_source,re.S) if 'GELDB_DEFEAT' in m[0])
# Read the actual native bitfield getter; receipt is the existing hook's flag setter.
with tempfile.TemporaryDirectory(prefix='oot-enemy-drops-') as temp:
 temp=Path(temp)
 (temp/'native_save_flags.inc').write_text(f['Flags_GetRandomizerInf']+'\n'+f['Flags_SetRandomizerInf'])
 native=temp/'native_deaths.c'
 native.write_text('''#include "global.h"
#include "din_fire_sword.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "overlays/actors/ovl_En_Goma/z_en_goma.h"
#include "overlays/actors/ovl_En_GeldB/z_en_geldb.h"
#include "objects/object_gol/object_gol.h"
#include "objects/object_geldb/object_geldb.h"
extern PlayState* gPlayState;
void EnGoma_Dead(EnGoma*, PlayState*);
void EnGeldB_Defeated(EnGeldB*, PlayState*);
void EnGeldB_SetupAction(EnGeldB*, EnGeldBActionFunc);
'''+gerudo_enum+'\n'+'''// Asset loader/animation are stubbed engine boundaries, retaining native signatures.
#define Animation_Change(s,a,...) Animation_Change(s,(AnimationHeader*)(a),__VA_ARGS__)
#define Animation_GetLastFrame(a) Animation_GetLastFrame((void*)(a))
#define Animation_MorphToPlayOnce(s,a,m) Animation_MorphToPlayOnce(s,(AnimationHeader*)(a),m)
'''+f['Actor_Kill']+'\n'+goma['EnGoma_SetupDead']+'\n'+gerudo['EnGeldB_SetupDefeated'])
 flags=['-std=c++20','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t','-DSOH_BUILD_DLL','-I'+str(ROOT),'-I'+str(temp)]
 for path in ('soh/include','soh/include/PR','soh/src','soh','soh/assets','soh/soh','libultraship/include','libultraship/src','combo','combo/menu'):
  flags+=['-isystem',str(ROOT/path)]
 if '--sanitize' in sys.argv:
  flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 for mode in ('original','combo'):
  exe=temp/mode
  obj=temp/(mode+'.o')
  subprocess.run([os.environ.get('CC','cc'),*['-std=gnu11' if flag=='-std=c++20' else flag for flag in flags],*(['-DCOMBO_BUILD'] if mode=='combo' else []),'-c',str(native),'-o',str(obj)],check=True)
  subprocess.run([os.environ.get('CXX','c++'),*flags,*(['-DCOMBO_BUILD'] if mode=='combo' else []),str(Path(__file__).with_name('runtime_test.cpp')),str(obj),'-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
print('PASS native first enemy-type drops, original and Combo guards')
