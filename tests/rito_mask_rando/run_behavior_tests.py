"""Compile the actual native pool/grant branches and the MM portable GI descriptor."""
from pathlib import Path
import os,re,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
CPP= os.environ.get('CXX','c++')
for host,item,opt in [('soh','RG_RITO_MASK','RSK_SHUFFLE_RITO_MASK'),('mm','RI_OOT_NEI_RITO_MASK','RO_SHUFFLE_RITO_MASK')]:
 pool=(ROOT/('soh/soh/Enhancements/randomizer/3drando/item_pool.cpp' if host=='soh' else 'mm/2s2h/Rando/Logic/GeneratePools.cpp')).read_text()
 branch=re.search(r'    if \([^\n]*'+opt+r'[^\n]*\)\n\s*[^\n]*'+item+r'[^\n]*;',pool)[0]
 native=(ROOT/('soh/soh/Enhancements/randomizer/randomizer.cpp' if host=='soh' else 'mm/2s2h/Rando/GiveItem.cpp')).read_text()
 grant=re.search(r'        case '+item+r':\s*RitoMask_Grant\(\);\s*break;',native)[0]
 remove='';roundtrip=''
 if host=='mm':
  source=(ROOT/'mm/2s2h/Rando/RemoveItem.cpp').read_text();arm=re.search(r'        case '+item+r':\s*RitoMask_Remove\(\);\s*break;',source)[0]
  remove='void Remove(int item){switch(item){'+arm+'}}'
  roundtrip='assert(owned);Remove(ITEM);assert(!owned);Give(ITEM);assert(owned);'
 fixture='''#include <vector>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include <array>
#include <cstring>
struct Option { bool on; operator bool() const{return on;} };
struct Context { Option GetOption(int){return {enabled};} bool enabled; } context;
auto* ctx=&context;
struct Save {std::array<int,1> randoSaveOptions;} saveInfo;
std::vector<int> itemPool;
void AddFixedItemToPool(int item,int count){while(count--)itemPool.push_back(item);}
bool owned;int grants;void RitoMask_Grant(){++grants;owned=true;}void RitoMask_Remove(){owned=false;}
constexpr int ITEM=99,OPTION=0,RO_GENERIC_ON=1;
void Pool(){BRANCH}
void Give(int item){switch(item){GRANT}}
REMOVE
int main(){for(int on:{0,1}){context.enabled=on;saveInfo.randoSaveOptions[0]=on;itemPool.clear();Pool();assert(std::count(itemPool.begin(),itemPool.end(),ITEM)==on);}grants=0;Give(ITEM);assert(grants==1);ROUNDTRIP puts("PASS HOST native optional pool OFF/ON and native grant route");}
'''.replace('BRANCH',branch).replace('GRANT',grant).replace('REMOVE',remove).replace('ROUNDTRIP',roundtrip).replace(item,'ITEM').replace(opt,'OPTION').replace('HOST',host)
 with tempfile.TemporaryDirectory() as td:
  p=Path(td);(p/'test.cpp').write_text(fixture);subprocess.run([CPP,'-std=c++20',str(p/'test.cpp'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
route_source=(ROOT/'mm/2s2h/Rando/NeiResourceRouting.cpp').read_text();route_start=route_source.index('extern "C" const char* NeiResource_Route(');route_end=route_source.index('\nextern "C" int NeiResource_IsMod(',route_start);real_route=route_source[route_start:route_end]
source=(ROOT/'mm/2s2h/Rando/NeiGiPresentation.cpp').read_text();start=source.index('    if (item == RI_OOT_NEI_RITO_MASK) {');end=source.index('\n    if (item >=',start);branch=source[start:end]
foreign_source=(ROOT/'combo/menu/ComboForeignDrawOOT.h').read_text();foreign_start=foreign_source.index('    static constexpr char kOtrPrefix[] = "__OTR__";');foreign_end=foreign_source.index('    info.count = n;',foreign_start);foreign_route=foreign_source[foreign_start:foreign_end]
fixture='''#include "ComboItemDrawABI.h"
#include "ComboNeiGiResourceOwner.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>
#include <unordered_set>
namespace Kind {constexpr int Gold=1;}
constexpr int RI_OOT_NEI_RITO_MASK=99;
/* REAL_ROUTE */
bool Describe(int item,CwItemDrawInfo* out){*out={};BRANCH return false;}
namespace ComboForeignResolve {constexpr int Unknown=0,Ok=1;}
struct ForeignInfo {const char* dls[CW_DRAW_MAX_DLISTS]{};};
const char* ComboInternRoutedPath(const std::string& path){static std::unordered_set<std::string> paths;return paths.insert(path).first->c_str();}
int RouteForeign(const CwItemDrawInfo& raw,ForeignInfo& info){FOREIGN_ROUTE return ComboForeignResolve::Ok;}
int main(){CwItemDrawInfo out;assert(Describe(99,&out));assert(out.drawKind==CW_DRAW_KIND_NEI_GI&&out.dlistCount==1&&out.scale==1.f&&out.itemShimmer==1);assert(!strcmp(out.dlists[0],"__OTR__objects/nei_gi_redesign/rito_mask/gi_dl"));assert(out.xluStartIndex==-1);assert(CwNeiGiResourceOwner(out.dlists[0])==nullptr);
// Production routing is donor-OoT by default; Rito must not call it for local MM.
assert(!strcmp(NeiResource_Route(out.dlists[0]),"__OTR__@oot:objects/nei_gi_redesign/rito_mask/gi_dl"));
ForeignInfo foreign;assert(RouteForeign(out,foreign)==ComboForeignResolve::Ok);
assert(!strcmp(foreign.dls[0],"__OTR__@mm:objects/nei_gi_redesign/rito_mask/gi_dl"));
assert(!strcmp(CwNeiGiResourceOwner(foreign.dls[0]),"mm"));assert(!Describe(0,&out));puts("PASS MM portable authored Rito descriptor scale1, shimmer, resource identity");}
'''.replace('BRANCH',branch).replace('/* REAL_ROUTE */',real_route).replace('FOREIGN_ROUTE',foreign_route)
with tempfile.TemporaryDirectory() as td:
 p=Path(td);(p/'test.cpp').write_text(fixture);subprocess.run([CPP,'-std=c++20','-I'+str(ROOT/'combo/menu'),str(p/'test.cpp'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
