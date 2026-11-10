#!/usr/bin/env python3
"""Exercise production Rito flight transitions and bow volley decisions with native structs."""
from pathlib import Path
import subprocess,tempfile,sys,os
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_scene_randomization_tests import function
flight=(ROOT/'mm/mods/forms/rito_flight.inc.c').read_text()
bow=(ROOT/'mm/mods/forms/rito_bow.inc.c').read_text()
start=flight.index('#define RITO_CHARGE_MIN');end=flight.index('static struct {',start)
state=flight[start:end]
bs=bow.index('#define RITO_BOW_VOLLEY');be=bow.index('static RitoBow sRitoBow;')+len('static RitoBow sRitoBow;')
# Reuse native constants/structures while excluding unrelated graphical helpers.
bstate=bow[bs:be]
code=r'''#include "global.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "mods/nei_oot_compat.h"
#include <cassert>
#include <cstdio>
#define PLAYER_STATE3_PAUSE_ACTION_FUNC PLAYER_STATE3_4
#define FORMS_STATE1_DAMAGED PLAYER_STATE1_4000000
#define CUSTOM_FORM_RITO 4
SaveContext gSaveContext;
static Input raw;
static int tookOver, errors, active=4;
int CustomForms_ActiveForm(){return active;}
const Input* Forms_RawInput(){return &raw;}
u8 Forms_OnGround(Player* p){return (p->actor.bgCheckFlags&BGCHECKFLAG_GROUND)!=0;}
u8 Forms_IsAdultAge(){return 0;}
#define LINK_IS_ADULT Forms_IsAdultAge()
static int sRitoWindOn;
static void MmRitoWindTick(PlayState*){}
static void MmRitoLoadClips(){}
static u8 MmRitoInputBlocked(){return 0;}
static void MmRitoWindTrailsOn(PlayState*,Player*){}
static void MmRitoPlay(PlayState*,Player*,PlayerAnimationHeader*,u8,f32){}
static int MmRitoAdvance(PlayState*,Player*){return 0;}
static u8 MmRitoOotTookOver(Player*,PlayerActionFunc){return tookOver;}
static void MmRitoEnterAir(PlayState*,Player* p){p->actor.bgCheckFlags&=~BGCHECKFLAG_GROUND;}
extern "C" void Player_PlaySfx(Player*,u16){}
extern "C" void Audio_PlaySfx(u16){++errors;}
extern "C" s32 Camera_ChangeMode(Camera*,s16){return 0;}
extern "C" s16 Camera_GetInputDirYaw(Camera*){return 0;}
extern "C" void Player_SetAction(PlayState*,Player*,PlayerActionFunc,s32){}
extern "C" void Player_Action_Idle(Player*,PlayState*){}
'''+state
# Avoid Forms_IsAdultAge aliases in donor macro expanding global save age in the harness.
code=code.replace('#define RITO_ROCS_VELOCITY (LINK_IS_ADULT ? 7.5f : 7.0f)', '#define RITO_ROCS_VELOCITY 7.0f')
code+=function(flight,'MmRitoRelease')+'\n'+function(flight,'MmRitoSpendLaunchMagic')+'\n'+function(flight,'MmRitoGlideMove')+'\n'+function(flight,'MmRitoFlightUpdate')
code+='\n'+bstate+r'''
static int shots, homing, cleared;
static Actor arrows[3], target;
static void MmRitoBowClearArrows(){shots=homing=0;++cleared;}
static s32 MmRitoBowFindTargets(PlayState*,Actor** out,s32 max,s16){for(int i=0;i<max;++i)out[i]=&target;return max;}
static Actor* MmRitoBowSpawnArrow(PlayState*,Player*,s16,s16,s32){return &arrows[shots++];}
static void MmRitoBowSetTarget(Actor*,Actor*){++homing;}
'''+function(bow,'MmRitoBowSpokeYaw')+'\n'+function(bow,'MmRitoBowLoose')+r'''
int main(){
 PlayState play{};Player p{};PlayerAnimationHeader clip{};
 sRito.charge_=&clip;sRito.fly=&clip;sRito.launch=&clip;
 p.actor.bgCheckFlags=BGCHECKFLAG_GROUND;raw.cur.button=raw.press.button=BTN_A;
 gSaveContext.magicCapacity=48;gSaveContext.save.saveInfo.playerData.magic=30;
 assert(MmRitoFlightUpdate(&p,&play)==1&&sRito.state==RITO_FLY_CHARGE);
 raw.press.button=0;
 for(int i=0;i<8;++i)MmRitoFlightUpdate(&p,&play);
 raw.cur.button=0;MmRitoFlightUpdate(&p,&play);
 assert(sRito.state==RITO_FLY_LAUNCH&&gSaveContext.save.saveInfo.playerData.magic==18);
 assert(p.actor.velocity.y>8&&p.actor.gravity==0);
 raw.cur.button=BTN_A;
 for(int i=0;i<30;++i)MmRitoFlightUpdate(&p,&play);
 assert(sRito.state==RITO_FLY_GLIDE&&p.speedXZ==4.5f&&p.actor.velocity.y==-0.35f);
 assert(gSaveContext.save.saveInfo.playerData.magic==18&&"glide must be free");
 raw.cur.button=0;MmRitoFlightUpdate(&p,&play);
 assert(sRito.state==RITO_FLY_OFF&&p.actor.gravity==-1.2f);
 sRito.state=RITO_FLY_CHARGE;sRito.charge=8;gSaveContext.save.saveInfo.playerData.magic=11;
 MmRitoFlightUpdate(&p,&play);assert(sRito.state==RITO_FLY_OFF&&gSaveContext.save.saveInfo.playerData.magic==11);
 sRito.state=RITO_FLY_GLIDE;tookOver=1;raw.cur.button=BTN_A;
 MmRitoFlightUpdate(&p,&play);assert(sRito.state==RITO_FLY_OFF&&!(p.stateFlags3&PLAYER_STATE3_PAUSE_ACTION_FUNC));
 sRitoBow.charge=19;MmRitoBowLoose(&play,&p);assert(shots==1&&homing==0);
 sRitoBow.charge=20;MmRitoBowLoose(&play,&p);assert(shots==2&&homing==2);
 p.focusActor=&target;sRitoBow.charge=0;MmRitoBowLoose(&play,&p);assert(shots==1&&homing==1);
 puts("PASS MM Rito production controller: launch billing, free glide/release, interruption, one/two homing volley");
}
'''
with tempfile.TemporaryDirectory(prefix='rito-mm-controller-') as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 inc=['-I'+str(ROOT/v) for v in ('mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h','libultraship/include','combo','combo/menu')]
 subprocess.run(['c++','-std=c++20','-w','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t',*inc,str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
