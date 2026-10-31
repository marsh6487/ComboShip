"""Exercise the real MM exporter for every concrete cane without optional GI assets."""
from pathlib import Path
import os, re, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from, functions
owner=(ROOT/'combo/menu/ComboItemDrawMM.h').read_text()
fns=functions(owner)
fixture=(ROOT/'tests/ikana_gi/mm_export_test.cpp').read_text().split('/* MM_PRODUCER */')[0]
# Remove the borrowed fixture's complete stubs, independent of C++ formatting.
for name in ('MM_FillImportedSwordFallback', 'MM_FillCaneDrawInfo'):
    stub=functions(fixture)[name]
    assert fixture.count(stub)==1, name
    fixture=fixture.replace(stub,'',1)
a=fixture.index('int GetItem_GetDrawTableEntry(')
fixture=fixture[:a]+r'''
bool mmMod=false,ootMod=false,mmPresent=true,ootPresent=true,handPresent=true;
int nativeCalls=0;
int GetItem_GetDrawTableEntry(int,void**,int,int*,float*,int*,int*) { ++nativeCalls;return 0; }
void GetItem_GetDrawSetupDLs(int,void**,void**){}
int GetItem_GetShimmerColor(s16,u8*){return 0;}
namespace NeiAssetPriority {int IsModAsset(const char* native,const char* game,const char*) {assert(!strcmp(native,"mm"));return !strcmp(game,"mm")?mmMod:ootMod;}}
int ResourceMgr_IsGiModelAvailableForGame(const char* game,const char* path) {return (!strcmp(game,"mm")?mmPresent:ootPresent)&&(handPresent||!strstr(path,"ultrahand"));}
const char* NeiResource_Route(const char* p) {static std::map<std::string,std::string> paths;auto& out=paths[p];out=std::string("__OTR__@oot:")+(p+7);return out.c_str();}
'''
# Preserve the real consumer's declaration-before-definition boundary. Defining
# this seam before MM_FillCaneDrawInfo would mask a missing production prototype.
benport=(ROOT/'mm/2s2h/BenPort.cpp').read_text()
mod_query=block_from(benport,benport.index('extern "C" int ResourceMgr_IsModAssetForGame('))
mod_declaration=re.search(r'^extern "C" int ResourceMgr_IsModAssetForGame\([^;{}]*\);',owner,re.M)
fixture+=(mod_declaration.group(0) if mod_declaration else '')+'\n'
fixture+='int ComboCollectibleShimmer_ProfileForDrawId(s16){return -1;}\n'+fns['MM_Op']+'\n'+fns['MM_OpV']+'\n'
fixture+='int DescribeTools(const char* name,CwItemDrawInfo* out){assert(!strcmp(name,\"Dominion Rod\")||!strcmp(name,\"Elemental Wand\"));out->drawKind=CW_DRAW_KIND_CUSTOM_GI;out->dlistCount=1;out->scale=2.5f;out->dlists[0]=\"__OTR__objects/nei_legacy_gi/dominion_rod/gi_dl\";return 1;}\nvoid* Combo_ResolveSym(const char*,const char*){return (void*)DescribeTools;}\n'
fixture+=fns['MM_FillImportedSwordFallback']+'\n'
fixture+=fns['MM_FillCaneDrawInfo']+'\n'+mod_query+'\n'+fns['MM_IsProgressiveItem']+'\n'+fns['MM_FillElementalSpellDrawInfo']+'\n'+fns['MM_FillItemDrawInfo']
fixture+=r'''
int main() {
 const RandoItemId ids[]={RI_OOT_NEI_CANE_OF_SOMARIA,RI_OOT_NEI_CANE_PACCI_FLIP,RI_OOT_NEI_CANE_SOMARIA_BLOCK,
 RI_OOT_NEI_CANE_PACCI_STONE,RI_OOT_NEI_CANE_SOMARIA_PLATFORM,RI_OOT_NEI_CANE_PACCI_ULTRAHAND};
 for(auto id:ids)Rando::StaticData::Items[id]={"concrete skill",GID_NONE};
 for(int selection=0;selection<3;++selection) {
  mmMod=selection==1;ootMod=selection==2;
  for(int i=0;i<6;++i) {
   CwItemDrawInfo out{};
   assert(MM_FillItemDrawInfo(ids[i],&out)==1 && "valid MM cane must export a complete GI, never GID_NONE");
   assert(out.drawKind==CW_DRAW_KIND_CUSTOM_GI && out.stateDependent==2);
   assert(out.dlistCount==(i==5?2:1) && out.xluStartIndex==(i==5?1:-1));
   assert(out.scale==(i==5?.17f:.25f));
   assert(!strncmp(out.dlists[0],selection==2?"__OTR__@oot:":"__OTR__@mm:",selection==2?12:11));
   if(i==5) {assert(!out.primColorOpa[3]&&!out.primColorXlu[3]);}
   else {
    assert(bool(out.primColorOpa[3])==bool(i%2));
    if(i%2)assert(out.primColorOpa[0]==255&&out.primColorOpa[1]==215&&out.primColorOpa[2]==70);
    assert(bool(out.primColorXlu[3])==(i>=2));
    if(i>=2)assert(out.primColorXlu[0]==255&&out.primColorXlu[1]==(i%2?215:60));
   }
  }
 }
 handPresent=false;mmMod=ootMod=false;
 {CwItemDrawInfo fallback{};assert(MM_FillItemDrawInfo(RI_OOT_NEI_CANE_PACCI_ULTRAHAND,&fallback)==1);
  assert(fallback.dlistCount==1&&fallback.scale==.25f&&fallback.primColorOpa[1]==215&&fallback.primColorXlu[1]==215&&"missing legacy Ultrahand must retain the native upgraded gold cane fallback");}
 handPresent=true;
 for(auto id:{RI_OOT_NEI_DOMINION_ROD,RI_OOT_NEI_ELEMENTAL_WAND}) {
  Rando::StaticData::Items[id]={id==RI_OOT_NEI_DOMINION_ROD?"Dominion Rod":"Elemental Wand",GID_NONE};
  authored=false;CwItemDrawInfo info{};assert(MM_FillItemDrawInfo(id,&info)==1&&info.drawKind==CW_DRAW_KIND_CUSTOM_GI&&info.scale==2.5f);
  assert(!strcmp(info.dlists[0],"__OTR__@oot:objects/nei_legacy_gi/dominion_rod/gi_dl"));
  authored=true;info={};assert(MM_FillItemDrawInfo(id,&info)==1&&info.drawKind==CW_DRAW_KIND_NEI_GI);
 }
 assert(nativeCalls==0);
 authored=true;CwItemDrawInfo out{};assert(MM_FillItemDrawInfo(ids[0],&out)==1&&out.drawKind==CW_DRAW_KIND_NEI_GI);
 std::cout<<"PASS real MM cane exporter: six concrete awards, optional authored present/absent, stock/local/donor roots, tint/flame and split Ultrahand\n";
}
'''
with tempfile.TemporaryDirectory(prefix='mm-cane-export-') as td:
 p=Path(td)/'test.cpp';p.write_text(fixture);out=Path(td)/'test'
 flags=['-std=c++20','-DCOMBO_BUILD','-Wall','-Wextra','-Werror','-I'+str(ROOT),'-I'+str(ROOT/'mm/include')]
 if '--sanitize' in sys.argv:flags+=['-g','-fsanitize=address,undefined','-fno-pie','-no-pie']
 subprocess.run([os.environ.get('CXX','c++'),*flags,str(p),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
