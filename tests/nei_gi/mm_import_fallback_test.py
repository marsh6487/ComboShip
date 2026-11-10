"""Audit imported custom catalog rows through the real MM fallback bridge."""
from pathlib import Path
import os,re,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from, functions
from foreign_gi_fixture_dependencies import clock_descriptor
source=(ROOT/'combo/menu/ComboItemDrawMM.h').read_text()
catalog=(ROOT/'mm/2s2h/Rando/StaticData/Items.cpp').read_text()
rows=re.findall(r'RI\((RI_OOT_(?:NEI|EXT)_\w+),\s*"[^"]*",\s*"([^"]+)"[^\n]*GID_NONE',catalog)
rows += [('RI_OOT_IRON_KNUCKLE_AXE', "Iron Knuckle's Axe")]
# Cane recipes retain MM's model priority; weather and Mirror have dedicated native exports.
rows=[(id,name) for id,name in rows if '_CANE_' not in id or id.endswith('BYRNA')]
rows=[(id,name) for id,name in rows if '_SEASON_' not in id and not id.endswith(('ROD_OF_SEASONS','MAGIC_MIRROR'))]
# Rito has a mandatory local MM descriptor, exercised through the real dispatcher below.
assert sum(id == 'RI_OOT_NEI_RITO_MASK' for id,name in rows) == 1
rows=[(id,name) for id,name in rows if id != 'RI_OOT_NEI_RITO_MASK']
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

# Reuse only the existing export harness's service/type boundaries. Both native
# production functions and the descriptor's binding data are extracted intact.
presentation=(ROOT/'mm/2s2h/Rando/NeiGiPresentation.cpp').read_text()
native=clock_descriptor(ROOT)+(ROOT/'tests/ikana_gi/mm_export_test.cpp').read_text().split('/* MM_PRODUCER */')[0]
for name in ('MM_DescribeNeiGi','MM_FillImportedSwordFallback','GetItem_GetDrawTableEntry','CVarGetInteger'):
    stub=functions(native)[name]
    assert native.count(stub)==1,name
    native=native.replace(stub,'',1)
native+=r'''
using NeiGi::Kind;
int donorCalls=0,nativeCalls=0,effects=0;
bool altAssets=false;
void* Combo_ResolveSym(const char*,const char*) {++donorCalls;assert(false&&"Rito must not need an OoT donor");return nullptr;}
const char* NeiResource_Route(const char*) {assert(false&&"Rito must retain its local MM resource");return nullptr;}
bool ResourceMgr_IsAltAssetsEnabled() {return altAssets;}
bool HasMmLegacyGiMod(RandoItemId) {assert(false);return false;}
bool GetSelectedOwnerGi(RandoItemId,CwItemDrawInfo*) {assert(false);return false;}
int CVarGetInteger(const char* key,int) {assert(!strcmp(key,"gEnhancements.SkijerNEI.ItemEffects"));return effects;}
int GetItem_GetDrawTableEntry(int,void**,int,int*,float*,int*,int*) {++nativeCalls;assert(false);return 0;}
int ComboCollectibleShimmer_ProfileForDrawId(s16) {return -1;}
'''
native+=block_from(presentation,presentation.index('struct Binding {'))+';\n'
native+=block_from(presentation,presentation.index('const Binding kBindings[] = {'))+';\n'
native+=functions(presentation)['MM_DescribeNeiGi']+'\n'
owner_functions=functions(source)
for name in ('MM_Op','MM_OpV','MM_IsProgressiveItem','MM_FillElementalSpellDrawInfo',
             'MM_FillImportedSwordFallback','MM_FillItemDrawInfo'):
    native+=owner_functions[name]+'\n'
native+=r'''
void ExpectRito(const CwItemDrawInfo& out) {
    assert(out.drawKind==CW_DRAW_KIND_NEI_GI&&out.dlistCount==1&&out.xluStartIndex==-1);
    assert(!strcmp(out.dlists[0],"__OTR__objects/nei_gi_redesign/rito_mask/gi_dl"));
    assert(out.scale==1.f&&out.neiEffect==static_cast<int>(Kind::Gold));
    assert(out.neiShimmer==static_cast<int>(Kind::Gold)+1&&out.itemShimmer==1&&out.stateDependent==2);
}
int main() {
    Rando::StaticData::Items[RI_OOT_NEI_RITO_MASK]={"Rito Mask",GID_NONE};
    assert(!MM_DescribeNeiGi(RI_OOT_NEI_RITO_MASK,nullptr));
    const char award[]="preserved Rito award";
    for(bool alt:{false,true}) for(int preference:{0,1}) {
        altAssets=alt;effects=preference;
        CwItemDrawInfo descriptor{};
        assert(MM_DescribeNeiGi(RI_OOT_NEI_RITO_MASK,&descriptor));
        ExpectRito(descriptor);
        CwItemDrawInfo out{};out.resolvedName=award;
        assert(MM_FillItemDrawInfo(RI_OOT_NEI_RITO_MASK,&out)==1);
        ExpectRito(out);
        assert(out.resolvedName==award&&donorCalls==0&&nativeCalls==0);
    }
    CwItemDrawInfo legacy{};
    assert(MM_FillImportedSwordFallback(RI_OOT_NEI_RITO_MASK,&legacy)==0);
    assert(donorCalls==0&&nativeCalls==0);
    std::cout<<"PASS actual Rito native descriptor and dispatcher: local MM GI, scale1, mandatory shimmer, preserved award and no donor dependency\n";
}
'''
with tempfile.TemporaryDirectory(prefix='mm-native-rito-') as td:
    p=Path(td)/'test.cpp';p.write_text(native);binary=Path(td)/'test'
    subprocess.run([os.environ.get('CXX','c++'),*flags,'-DCOMBO_BUILD','-I'+str(ROOT/'mm/include'),
                    str(p),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
