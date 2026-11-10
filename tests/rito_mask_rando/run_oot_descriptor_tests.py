"""Execute OoT's real authored NEI recipe exporter with Rito's production catalog row."""
from pathlib import Path
import re,os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
s=(ROOT/'soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp').read_text()
start=s.index('static bool NeiGi_FillCrossGameInfo(');end=s.index('\nextern "C" int32_t NeiGi_DescribeEntry(',start)
function=s[start:end]
row=re.search(r'\{ Randomizer_DrawRitoMask,[^\n]+',s)[0]
fixture=r'''
#include "ComboItemDrawABI.h"
#include <cassert>
#include <cstdio>
#include <cstring>
namespace NeiGi {enum class Kind {Neutral,Gold};bool IsSword(Kind){return false;}}
using NeiGi::Kind;
using CustomDrawFunc=void(*)();
void Randomizer_DrawRitoMask(){} void Randomizer_DrawCaneSomariaUpgrade(){}
struct Point{float x,y,z;};struct Fit{float scale=1,lift=0;};
struct Presentation{CustomDrawFunc draw;const char* opaque;const char* translucent;float scale;Kind effect;Point effectCenter;Fit shop;int identity=0;bool alwaysShimmer=false;int nativeGid=-1;Kind shimmer=Kind::Neutral;};
Kind PresentationShimmer(const Presentation& i){return i.shimmer==Kind::Neutral?i.effect:i.shimmer;}
#define GI_PATH(slug) "__OTR__objects/nei_gi_redesign/" slug "/gi_dl"
#define CVAR_NEI_GI_EFFECTS "fx"
int CVarGetInteger(const char*,int){return 0;}
bool HasLegacyGiMod(const Presentation&,bool){return false;}
template<class F>bool HasSelectedSword(const Presentation&,bool,F){return false;}
int32_t OOT_NeiAltAssetsEnabled(){return 0;}
int OOT_NeiResourceExists(const char*){return 1;}
const char* NeiGi_BaseSwordPath(const char* p){return p;}
bool HasRedesignGiMod(const Presentation&){return false;}
/* FUNCTION */
int main(){Presentation item=/* ROW */;
 for(bool alt:{false,true}){CwItemDrawInfo out{};assert(NeiGi_FillCrossGameInfo(item,&out,alt));assert(out.drawKind==CW_DRAW_KIND_NEI_GI);assert(out.scale==1.f&&out.dlistCount==1&&out.opCount==0&&out.itemShimmer==1);assert(out.neiEffect==int(Kind::Gold));assert(!strcmp(out.dlists[0],"__OTR__objects/nei_gi_redesign/rito_mask/gi_dl"));}
 puts("PASS production OoT authored Rito descriptor: native/foreign identity, scale1, mandatory gold shimmer, vanilla/Alt");}
'''.replace('/* FUNCTION */',function).replace('/* ROW */',row.rstrip(', '))
with tempfile.TemporaryDirectory() as td:
 p=Path(td);(p/'test.cpp').write_text(fixture)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-include','initializer_list','-I'+str(ROOT/'combo/menu'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
