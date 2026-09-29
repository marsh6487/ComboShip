"""Execute native final hand selectors and actual adult override/fit query bodies."""
from pathlib import Path
import re,sys,tempfile,subprocess
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions
native=(ROOT/'mm/src/code/z_player_lib.c').read_text()
adult=(ROOT/'mm/mods/items/logic/adult_link_render.cpp').read_text()
def extract(source,name):
 mask=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',lambda m:' '*len(m[0]),source,flags=re.S)
 match=re.search(r'^[^\n;]*\b'+name+r'\([^;]*?\)\s*\{',mask,re.M)
 assert match,name
 start=match.start();pos=match.end();depth=1
 while depth:
  depth+=(mask[pos]=='{')-(mask[pos]=='}');pos+=1
 return source[start:pos]
names=['Player_OverrideLimbDrawGameplayDefault','Player_OverrideLimbDrawGameplayFirstPerson','Player_ApplyNeiHeldHand']
nf={n:extract(native,n) for n in names}
af={n:extract(adult.replace('extern "C" ',''),n) for n in ['AdultLink_UsesAdultPresentation','AdultLink_ApplyNeiHeldHand','AdultLink_OverrideLimb']}
for name in ['Player_OverrideLimbDrawGameplayDefault','Player_OverrideLimbDrawGameplayFirstPerson']:
 assert 'Player_ApplyNeiHeldHand(play, player, limbIndex, dList)' in nf[name]
# Exercise the complete final adult override, not just an intermediate helper.
prefix=r'''
#include "z64.h"
#include "variables.h"
#include "mods/forms/custom_forms.h"
#include <cassert>
#include <string>
#include <iostream>
SaveContext gSaveContext{};
PlayState* gPlayState=nullptr;
u16 gEquipMasks[4]{};u8 gEquipShifts[4]{};
bool mode=false,lantern=false,hook=true,hasHand=true,claimedHand=false;
int form=CUSTOM_FORM_NONE, sPlayerLod=0,sPlayerLeftHandType=0;
u8 sReady=0,sIsChildRig=0,sIsMod=0;
FlexSkeletonHeader skel{},*sSkel=nullptr;
Gfx nativeLeft{},nativeRight{},adultLeft{},adultRight{},customHand{},replacement{},vanillaHook{};
Gfx* gPlayerLeftHandClosedDLs[10]{};Gfx* gPlayerRightHandClosedDLs[10]{};
std::string resolved;
s32 AdultLink_IsActive(){return mode;}
s32 CustomForms_ActiveForm(){return form;}
f32 CustomForms_RootScale(){return .335f;}
f32 CustomForms_RootDropBefore(){return 0;}
f32 CustomForms_RootDropAfter(){return 0;}
Gfx* CustomForms_HandDL(Player*,s32,u8* claimed){*claimed=claimedHand;return &customHand;}
u8 CustomForms_HidesSheath(Player*){return 0;}
const char* ExtEquip_GetShieldDLOverride(){return nullptr;}
void* DinFireSword_HandDL(PlayState*,Player*,Gfx*){return nullptr;}
void* DinFireShield_HandDL(PlayState*,Player*,Gfx*){return nullptr;}
void Player_ApplyBackEquipmentVisibility(s32,Gfx**){}
bool NeiLantern_UsesGrip(const Player*){return lantern;}
bool NeiArticulated_UsesSwitchHook(const Player*){return hook;}
Gfx* capturedHand=nullptr;
bool NeiArticulated_ApplySwitchHookHand(PlayState*,Player*,Gfx** dl,Gfx* hand){if(!hook||!hand)return false;capturedHand=hand;*dl=&replacement;return true;}
u8 ResourceMgr_FileExists(const char*){return hasHand;}
Gfx* ResourceMgr_LoadGfxByName(const char* p){resolved=p;return resolved=="human_left"?&nativeLeft:&nativeRight;}
s32 Player_OverrideLimbDrawGameplayDefault(PlayState*,s32,Gfx** dl,Vec3f*,Vec3s*,Actor*){*dl=&vanillaHook;return 0;}
'''
prefix+='\n'.join(re.findall(r'^static Gfx\* sDL_\w+;',adult,re.M))+'\n'
checks=r'''
int main(){
 Player p{};p.transformation=PLAYER_FORM_HUMAN;p.actor.scale.y=.01f;PlayState play{};Vec3f pos{};Vec3s rot{};
 play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&p.actor;gPlayState=&play;
 gSaveContext.save.linkAge=0;
 assert(!AdultLink_UsesAdultPresentation(nullptr));
 assert(!AdultLink_UsesAdultPresentation(&p));mode=true;assert(!AdultLink_UsesAdultPresentation(&p));
 sReady=1;sSkel=&skel;assert(AdultLink_UsesAdultPresentation(&p));
 sIsChildRig=1;assert(!AdultLink_UsesAdultPresentation(&p));mode=false;form=CUSTOM_FORM_KEATON;assert(!AdultLink_UsesAdultPresentation(&p));
 sIsChildRig=0;assert(AdultLink_UsesAdultPresentation(&p));
 sReady=0;assert(!AdultLink_UsesAdultPresentation(&p));sReady=1;sSkel=nullptr;assert(!AdultLink_UsesAdultPresentation(&p));sSkel=&skel;
 p.transformation=PLAYER_FORM_DEKU;assert(!AdultLink_UsesAdultPresentation(&p));p.transformation=PLAYER_FORM_HUMAN;
 mode=false;form=CUSTOM_FORM_NONE;
 for(int i=0;i<10;i++){gPlayerLeftHandClosedDLs[i]=(Gfx*)"wrong_form";gPlayerRightHandClosedDLs[i]=(Gfx*)"wrong_form";}
 for(int lod=0;lod<2;lod++){
 sPlayerLod=lod;gPlayerLeftHandClosedDLs[PLAYER_FORM_HUMAN*2+lod]=(Gfx*)"human_left";gPlayerRightHandClosedDLs[PLAYER_FORM_HUMAN*2+lod]=(Gfx*)"human_right";
 Gfx* limb=&vanillaHook;Player_ApplyNeiHeldHand(&play,&p,PLAYER_LIMB_RIGHT_HAND,&limb);assert(limb==&replacement&&capturedHand==&nativeRight&&resolved=="human_right");
 lantern=true;limb=&vanillaHook;Player_ApplyNeiHeldHand(&play,&p,PLAYER_LIMB_LEFT_HAND,&limb);assert(limb==&nativeLeft&&sPlayerLeftHandType==PLAYER_MODELTYPE_LH_CLOSED);lantern=false;
 hasHand=false;limb=&vanillaHook;Player_ApplyNeiHeldHand(&play,&p,PLAYER_LIMB_RIGHT_HAND,&limb);assert(limb==&vanillaHook);hasHand=true;
 }
 for(int f=0;f<PLAYER_FORM_HUMAN;f++){p.transformation=f;Gfx* limb=&vanillaHook;Player_ApplyNeiHeldHand(&play,&p,PLAYER_LIMB_RIGHT_HAND,&limb);assert(limb==&vanillaHook);}p.transformation=PLAYER_FORM_HUMAN;
 // Local overlay readiness must not suppress a native peer's third-person hand.
 Player remote=p;
 for(int custom:{CUSTOM_FORM_NONE,CUSTOM_FORM_KEATON}){
   mode=(custom==CUSTOM_FORM_NONE);form=custom;sReady=1;sSkel=&skel;sIsChildRig=0;
   assert(AdultLink_UsesAdultPresentation(&p));
   assert(!AdultLink_UsesAdultPresentation(&remote));
   Gfx* remoteLimb=&vanillaHook;capturedHand=nullptr;
   Player_ApplyNeiHeldHand(&play,&remote,PLAYER_LIMB_RIGHT_HAND,&remoteLimb);
   assert(remoteLimb==&replacement&&capturedHand==&nativeRight);
   lantern=true;remoteLimb=&vanillaHook;
   Player_ApplyNeiHeldHand(&play,&remote,PLAYER_LIMB_LEFT_HAND,&remoteLimb);
   assert(remoteLimb==&nativeLeft);lantern=false;
   Gfx* localLimb=&vanillaHook;Player_ApplyNeiHeldHand(&play,&p,PLAYER_LIMB_RIGHT_HAND,&localLimb);
   assert(localLimb==&vanillaHook);
 }
 sIsChildRig=1;assert(!AdultLink_UsesAdultPresentation(&p));
 sIsChildRig=0;sReady=0;assert(!AdultLink_UsesAdultPresentation(&p));
 sReady=1;sSkel=nullptr;assert(!AdultLink_UsesAdultPresentation(&p));sSkel=&skel;
 mode=true;sDL_RHClosed=&adultRight;sDL_LHClosed=&adultLeft;sDL_RHHookshot=&vanillaHook;
 p.rightHandType=PLAYER_MODELTYPE_RH_HOOKSHOT;
 for(bool mod:{false,true})for(bool child:{false,true}){
 sIsMod=mod;sIsChildRig=child;Gfx* limb=&customHand;
 AdultLink_OverrideLimb(&play,PLAYER_LIMB_RIGHT_HAND,&limb,&pos,&rot,&p.actor);
 assert(limb==&replacement&&capturedHand==((mod||child)?&customHand:&adultRight));
 lantern=true;limb=&customHand;AdultLink_OverrideLimb(&play,PLAYER_LIMB_LEFT_HAND,&limb,&pos,&rot,&p.actor);assert(limb==((mod||child)?&customHand:&adultLeft));lantern=false;
 }
 claimedHand=true;Gfx* limb=&customHand;AdultLink_OverrideLimb(&play,PLAYER_LIMB_RIGHT_HAND,&limb,&pos,&rot,&p.actor);assert(limb==&replacement&&capturedHand==&customHand);
 assert(gSaveContext.save.linkAge==0);
 std::cout<<"PASS native form/LOD fist resolution, final adult/custom/child-rig selectors and read-only actual-rig age query\n";
}
'''
parts=[af['AdultLink_UsesAdultPresentation'],nf['Player_ApplyNeiHeldHand'],af['AdultLink_ApplyNeiHeldHand'],af['AdultLink_OverrideLimb']]
with tempfile.TemporaryDirectory(prefix='mm-nei-hands-') as td:
 path=Path(td)/'hands.cpp';path.write_text(prefix+'\n'.join(parts)+checks);binary=Path(td)/'hands'
 subprocess.run(['c++','-std=c++20',*flags(),str(path),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
