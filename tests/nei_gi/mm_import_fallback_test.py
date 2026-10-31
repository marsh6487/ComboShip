"""Audit imported custom catalog rows through the real MM fallback bridge."""
from pathlib import Path
import os,re,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions
source=(ROOT/'combo/menu/ComboItemDrawMM.h').read_text()
catalog=(ROOT/'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
rows=re.findall(r'RI\((RI_OOT_(?:NEI|EXT)_\w+),\s*"[^"]*",\s*"([^"]+)"[^\n]*GID_NONE',catalog)
rows += [('RI_OOT_IRON_KNUCKLE_AXE', "Iron Knuckle's Axe")]
# Cane recipes retain MM's model priority; weather and Mirror have dedicated native exports.
rows=[(id,name) for id,name in rows if '_CANE_' not in id or id.endswith('BYRNA')]
rows=[(id,name) for id,name in rows if '_SEASON_' not in id and not id.endswith(('ROD_OF_SEASONS','MAGIC_MIRROR'))]
code=r'''
#include "combo/menu/ComboItemDrawABI.h"
#include "mm/2s2h/Rando/Types.h"
#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <iostream>
namespace Rando::StaticData {struct Item{const char* name;};std::map<RandoItemId,Item> Items;}
std::string described;
int Describe(const char* name,CwItemDrawInfo* out){described=name;out->drawKind=CW_DRAW_KIND_CUSTOM_GI;
out->dlistCount=1;out->dlists[0]="__OTR__legacy_model";return 1;}
void* Combo_ResolveSym(const char* game,const char* symbol){assert(!strcmp(game,"soh")&&!strcmp(symbol,"OOT_GetItemDrawInfo"));return (void*)Describe;}
const char* NeiResource_Route(const char* p){assert(!strcmp(p,"__OTR__legacy_model"));return "__OTR__@oot:legacy_model";}
'''+functions(source)['MM_FillImportedSwordFallback']+'\nint main(){\n'
for id,name in rows:
    expected='Poké Ball' if id=='RI_OOT_NEI_POKE_BALL' else name
    code+=f'Rando::StaticData::Items[{id}]={{"{name}"}};{{CwItemDrawInfo out{{}};out.resolvedName="award";assert(MM_FillImportedSwordFallback({id},&out)==1&&"imported custom item lost its native fallback");assert(described=="{expected}"&&out.resolvedName==std::string("award")&&!strcmp(out.dlists[0],"__OTR__@oot:legacy_model"));}}\n'
code+='CwItemDrawInfo out{};assert(!MM_FillImportedSwordFallback(RI_COMBO_FOREIGN,&out));std::cout<<"PASS imported custom catalog fallback and canonical owner boundary; foreign sentinel never recurses\\n";}\n'
with tempfile.TemporaryDirectory(prefix='mm-import-fallback-') as td:
    p=Path(td)/'test.cpp';p.write_text(code);binary=Path(td)/'test'
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-I'+str(ROOT)]
    if '--sanitize' in sys.argv:flags+=['-g','-fsanitize=address,undefined','-fno-pie','-no-pie']
    subprocess.run([os.environ.get('CXX','c++'),*flags,str(p),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
