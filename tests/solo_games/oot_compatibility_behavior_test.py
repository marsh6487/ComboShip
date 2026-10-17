#!/usr/bin/env python3
"""Run native Farore input, Triforce receipt and optional MM-model fallback bodies."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def extract(path,marker):
 s=(ROOT/path).read_text();a=s.index(marker);b=s.index('{',a);n=1;i=b+1
 while n:n+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]+'\n'
player=(ROOT/'soh/src/overlays/actors/ovl_player_actor/z_player.c').read_text();st=player.rfind('#ifdef COMBO_BUILD',0,player.index('// DOWN is the parent entrance here;'));en=player.index('#endif',st)+len('#endif')
cpp=r'''
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <string>
#include <initializer_list>
#include <iostream>
#include "ComboGameMode.h"
extern "C"{int gComboGameMode=COMBO_GAME_OOT;}
struct PlayState{int sceneNum=1;};struct GetItemEntry{};
constexpr int SCENE_GROTTOS=1,SCENE_FAIRYS_FOUNTAIN=2,PLAYER_IA_FARORES_WIND=1,RESPAWN_MODE_TOP=0,NA_SE_SY_ERROR=1;
int errors=0,nativeAllowed=0,nativeDraws=0,peerDraws=0,peerCounts=0;
struct {struct{int data=0;}respawn[1];struct{struct{struct{int triforcePiecesCollected=1;}randomizer;}data;}quest; }unused;
struct {struct{int data=0;}respawn[1];struct{struct{struct{struct{int triforcePiecesCollected=1;}randomizer;}data;}quest;}ship;}gSaveContext;
int ComboFw_HasPoint(int){return 0;}void Sfx_PlaySfxCentered(int){++errors;}
bool mmOwner=false;
namespace Ship{struct CrossRMRegistry{static bool Get(const char*){return mmOwner;}};}
struct CwItemDrawInfo { const char* dlists[2]={"__OTR__a","__OTR__b"};float scale=1;uint8_t primColorXlu[4]{};};
int OOT_DescribeMmSpinAttackDraw(CwItemDrawInfo*){return 1;}
const char* ComboInternRoutedPath(const std::string&){return "routed";}
void ComboDrawSpinAttackGi(PlayState*,const char*,const char*,float,const uint8_t*,const char*){++peerDraws;}
constexpr int GID_SWORD_KOKIRI=1;void GetItem_Draw(PlayState*,int){++nativeDraws;}
enum {RSK_RAINBOW_BRIDGE,RSK_RAINBOW_BRIDGE_TRIFORCE_COUNT,RSK_WINCON,RSK_WINCON_TRIFORCE_COUNT,RSK_GANONS_BOSS_KEY,RSK_GBK_TRIFORCE_COUNT,RSK_GANONS_SOUL,RSK_GANONS_SOUL_TRIFORCE_COUNT};
constexpr int RO_BRIDGE_TRIFORCE_PIECES=7,RO_WINCON_TRIFORCE_PIECES=8,RO_GANON_BOSS_KEY_TRIFORCE_PIECES=9,RO_GANONS_SOUL_TRIFORCE_PIECES=10,ITEM_CUSTOM=1;
struct Randomizer{uint8_t GetRandoSettingValue(int key){return key==RSK_WINCON?RO_WINCON_TRIFORCE_PIECES:(key==RSK_WINCON_TRIFORCE_COUNT?2:0);}};
struct OTRGlobals{static OTRGlobals* Instance;Randomizer* gRandomizer;};Randomizer rando;OTRGlobals globals{&rando};OTRGlobals* OTRGlobals::Instance=&globals;
int gComboGoalRequired=8;int CountPeer(){++peerCounts;return 10;}int(*gComboOtherTriforceCount)()=CountPeer;
#define TODO_TRANSLATE ""
struct CustomMessage{
 std::string text;
 CustomMessage()=default;CustomMessage(std::initializer_list<const char*> parts):text(*parts.begin()){}
 void InsertNumber(int){}void AutoFormat(int){}
 void Replace(const std::string&,const std::string&){}void Replace(const std::string&,const CustomMessage&){}
};
'''
draw=(ROOT/'soh/soh/Enhancements/randomizer/draw.cpp').read_text();shield=draw[draw.index('    Gfx* mod = NeiGi_ModOverrideDL("objects/object_link_child/gLinkHumanMirrorShieldDL", true);',draw.index('void Randomizer_DrawExtShieldOfIkana')):draw.index('    if (selected == NULL)',draw.index('void Randomizer_DrawExtShieldOfIkana'))]
cpp+=r'''
using u8=uint8_t;struct Gfx{int value=0;};Gfx nativeModel;int nativeLoads=0;
Gfx* NeiGi_ModOverrideDL(const char*,bool){return nullptr;}
bool ResourceMgr_IsGiModelAvailableForGame(const char*,const char*){return mmOwner;}
void* TransformMasks_LoadMmDL(const char*){++nativeLoads;return &nativeModel;}
#define G_DL_OTR_FILEPATH 1
#define G_DL_PUSH 1
#define gDma1p(...) ((void)0)
#define gSPEndDisplayList(...) ((void)0)
'''+'Gfx* SelectShield(){\n'+shield+'return selected;\n}\n'
cpp+='void Spell(PlayState* play){int itemAction=PLAYER_IA_FARORES_WIND;bool isMedallionSpell=false;\n'+player[st:en]+'\n ++nativeAllowed;\n}\n'
cpp+=extract('soh/soh/Enhancements/randomizer/draw.cpp','extern "C" void Randomizer_DrawMmGreatSpinAttack(')
cpp+=extract('soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp','void BuildTriforcePieceMessage(')
cpp+=r'''
int main(){
 int failed=0;auto check=[&](bool v,const char* s){if(!v){++failed;std::cerr<<"FAIL "<<s<<"\n";}};
 PlayState p;check(SelectShield()==&nativeModel && nativeLoads==1,"solo Shield of Ikana retains optional native MM archive loader");Spell(&p);check(nativeAllowed==1 && errors==0,"solo Farore follows native restrictions");
 Randomizer_DrawMmGreatSpinAttack(&p,nullptr);check(nativeDraws==1 && peerDraws==0,"missing MM owner draws native fallback");
 CustomMessage msg;BuildTriforcePieceMessage(msg);check(msg.text.find("Courage")!=std::string::npos && peerCounts==0,"solo receipt ignores combined goal and peer count");
 gComboGameMode=COMBO_GAME_BOTH;check(SelectShield()==nullptr && nativeLoads==1,"combined shield never substitutes a native raw pointer");Spell(&p);check(errors==1,"combined Farore safety restriction retained");
 mmOwner=true;Randomizer_DrawMmGreatSpinAttack(&p,nullptr);check(peerDraws==1,"available MM owner retains native effect");
 BuildTriforcePieceMessage(msg);check(peerCounts==1 && msg.text.find("Courage")==std::string::npos,"combined receipt retained");
 return failed?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='oot-compatibility-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(cpp)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT/'combo'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS native Farore, Triforce receipt isolation and optional MM draw fallback')
