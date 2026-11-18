#!/usr/bin/env python3
"""Execute Switch Hook manual input and native MM weapon-aim state bodies.

The rendering/audio/collision services are fixtures. Player layouts, manual input,
weapon upper actions and Action43 exit/retention decisions are production code.
"""
import subprocess
import sys
import tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions
REF = sys.argv[sys.argv.index('--source-ref') + 1] if '--source-ref' in sys.argv else None

def source(path):
    if REF:
        return subprocess.check_output(['git', 'show', REF + ':' + path], cwd=ROOT, text=True)
    return (ROOT / path).read_text()

player = functions(source('mm/src/overlays/actors/ovl_player_actor/z_player.c'))
item = functions(source('mm/mods/items/logic/item_switchhook.c'))
actor_source = source('mm/src/code/z_actor.c')
actor = functions(actor_source[actor_source.index('bool func_800B7118'):actor_source.index('bool func_800B715C')])
equip = functions(source('mm/mods/items/helpers/equip_helper.c'))
# The legacy projectile implementation is unreachable after the active handler's
# unconditional return. Exclude that unreachable tail, without replacing logic.
handler = item['Handle_SwitchHook'].split('\n    ItemInputState in;', 1)[0] + '\n}'
parts = [equip[n] for n in ['ItemInput_IsBlockedEx', 'ItemInput_IsBlocked']]
parts += [actor[n] for n in ['func_800B7118', 'func_800B7128']]
parts += [item[n] for n in ['SwitchHook_IsAimingManual', 'SwitchHook_OnFired']]
parts += [player[n] for n in ['Player_SetUpperAction', 'func_808306F8', 'func_80830E30', 'func_80831010', 'func_80831094', 'func_80831814', 'func_80831194', 'Player_UpperAction_6', 'Player_UpperAction_7', 'Player_UpperAction_8', 'func_8083868C', 'Player_ActionHandler_0', 'Player_Action_43']]
if 'SwitchHook_CleanupManualAim' in item:
    parts += [item['SwitchHook_CleanupManualAim']]
parts += [handler]
prefix = r'''
#include "mods/items/custom_items.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/extended_player.h"
#include "mods/nei_save.h"
#include "mods/extended_inventory.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"
#include "overlays/actors/ovl_En_Test3/z_en_test3.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define MAGIC_REQ(amount) (amount)
#define IDLE_ANIM_NONE 0
#define VB_PLAYER_ALLOW_MIDAIR_AIM 1
#define VB_EXIT_FIRST_PERSON_MODE_FROM_BUTTON 2
#define VB_FIRST_PERSON_CAMERA 3
#define GI_DPAD_EQUIP 1
u16 GameInteractor_Dpad(s32 key,u16 value){return value;}
static u8 sShAimManual=0;
static Input* sPlayerControlInput;
static bool sPlayerUseHeldItem=false,sPlayerHeldItemButtonIsHeldDown=false;
static bool targeted=false,validCamera=true,animDone=true;
static int exits=0,turns=0,shots=0,upperStarts=0,peekCalls=0,cameraUpdates=0;
static s16 cameraMode=CAM_MODE_NORMAL;
SaveContext gSaveContext={0};
u16 D_8085CFB0[4]={0};u8 sMagicArrowCosts[4]={0};u16 D_8085D5FC[8]={0};
PlayerAnimationHeader* D_8085D5E4[3]={0};PlayerAnimationHeader* D_8085D5F0[3]={0};
void Player_Action_43(Player*,PlayState*);
s32 Player_UpperAction_6(Player*,PlayState*);s32 Player_UpperAction_7(Player*,PlayState*);s32 Player_UpperAction_8(Player*,PlayState*);
s32 Player_UpperAction_9(Player*,PlayState*){return 1;}s32 Player_UpperAction_12(Player*,PlayState*){return 1;}
bool func_80831010(Player*,PlayState*);
u8 Nei_ArmsHookVariant(Player* p){return p->heldItemId==ITEM_SWITCH_HOOK?4:0;}
u8 Player_IsZTargeting(Player*){return targeted;}
bool Player_IsHoldingHookshot(Player* p){return p->heldItemAction==PLAYER_IA_HOOKSHOT;}
Camera* Play_GetCamera(PlayState*,s16){static Camera c;return &c;}
s32 Camera_CheckValidMode(Camera*,s16){return validCamera;}
void func_8082E00C(Player*){++upperStarts;}
void Audio_PlaySfx(u16){}
void Player_PlaySfx(Player*,u16){}
void PlayerAnimation_PlayOnce(PlayState*,SkelAnime* sk,PlayerAnimationHeader* anim){}
void PlayerAnimation_PlayLoop(PlayState*,SkelAnime* sk,PlayerAnimationHeader* anim){}
void PlayerAnimation_PlayOnceSetSpeed(PlayState*,SkelAnime* sk,PlayerAnimationHeader* anim,f32 speed){}
s32 PlayerAnimation_Update(PlayState*,SkelAnime*){return animDone;}
PlayerAnimationHeader* Player_GetIdleAnim(Player* p){return NULL;}
void Player_Anim_PlayLoop(PlayState* play,Player* p,void* anim){}
s32 func_808305BC(PlayState*,Player*,ItemId*,ArrowType*){return 0;}

Actor* Actor_SpawnAsChild(ActorContext*,Actor*,PlayState*,s16,f32,f32,f32,s16,s16,s16,s32){return NULL;}
s32 Magic_Consume(PlayState*,s16,s16){return 1;}
bool func_80830B88(PlayState*,Player*){return false;}
bool func_80830F9C(PlayState*){return false;}
bool func_80830FD4(PlayState*){return false;}
bool func_80831124(PlayState*,Player* p){return p->actor.child!=NULL;}
bool func_80831194(PlayState*,Player*);
u8 SwitchHook_GetCharges(void){return 5;}
NeiSaveData* Nei_Save(void){static NeiSaveData data;return &data;}
void Inventory_ChangeAmmo(s16 item,s16 amount){}
void Player_RequestRumble(PlayState*,Player*,u8,u8,u8,u8){}
void GameInteractor_ExecuteOnPlayerReleaseHeldActor(PlayState*,Player*,Actor*){++shots;}
bool func_801242B4(Player*){return false;}
s32 Player_CheckForIdleAnim(Player*){return 0;}
s32 Math_ScaledStepToS(s16*,s16,s16){return 0;}
void Ship_HandleConsoleCrashAsReset(){assert(false);}
void Player_StopHorizontalMovement(Player*){}
s32 GameInteractor_Should(s32,bool def,...){return def;}
void func_808475B4(Player*){}
void func_8084748C(Player*,f32*,f32,s16){}
void Player_DecelerateToZero(Player*){}
bool Player_IsUsingZoraBoomerang(Player*){return false;}
bool Player_UpdateUpperBody(Player* p,PlayState* play){return p->upperActionFunc(p,play);}
s32 Player_UpdateHostileLockOn(Player*){return targeted;}
s32 Camera_ChangeMode(Camera* camera,s16 mode){++cameraUpdates;cameraMode=mode;return validCamera?mode:CAM_MODE_NORMAL;}
s32 Player_ActionHandler_13(Player*,PlayState*){++peekCalls;return true;}
bool Player_CheckHostileLockOn(Player*){return false;}
PlayerItemAction Player_ItemToItemAction(Player*,s32){return PLAYER_IA_NONE;}
s32 Inventory_GetBtnBItem(PlayState*){return ITEM_NONE;}
bool Player_FriendlyLockOnOrParallel(Player*){return targeted;}
bool func_8082EF20(Player*){return false;}
s32 Player_ActionHandler_Talk(Player*,PlayState*){return false;}
void func_80839ED0(Player* p,PlayState*){++exits;p->unk_AA5=PLAYER_UNKAA5_0;}
bool func_801240DC(Player*){return false;}
s16 func_80847190(PlayState*,Player*,s32){++turns;return 123;}

'''
checks = r'''
Player p={0};PlayState play={0};Actor hook={0};sPlayerControlInput=&play.state.input[0];
p.heldItemId=ITEM_SWITCH_HOOK;p.heldItemAction=PLAYER_IA_HOOKSHOT;p.actor.bgCheckFlags=BGCHECKFLAG_GROUND;
p.transformation=PLAYER_FORM_HUMAN;p.actor.id=ACTOR_PLAYER;p.stateFlags1=PLAYER_STATE1_8;p.actor.child=&hook;p.heldActor=&hook;p.unk_B28=-3;p.upperActionFunc=Player_UpperAction_6;
play.state.input[0].press.button=BTN_CUP;
Handle_SwitchHook(&p,&play);
assert(SwitchHook_IsAimingManual() && p.unk_AA5==PLAYER_UNKAA5_3);
bool toggleConsumed=!(play.state.input[0].press.button&BTN_CUP);
// Dispatch order is custom input, then the native weapon upper action/Action43.
play.state.input[0].press.button=0;
for(int frame=0;frame<30;++frame){
    animDone=frame>=5;
    Handle_SwitchHook(&p,&play);Player_Action_43(&p,&play);
    if(exits || !func_800B7128(&p))fprintf(stderr,"aim frame %d: native exits=%d AA5=%d ACC=%d\n",frame,exits,p.unk_AA5,p.unk_ACC);
    assert(exits==0 && p.unk_AA5==PLAYER_UNKAA5_3 && func_800B7128(&p));
}
assert(toggleConsumed);
assert(turns>0 && shots==0 && cameraUpdates==30 && cameraMode==CAM_MODE_SLINGSHOT);
// Equipped-button press/release must still reach native hook launch.
sPlayerUseHeldItem=sPlayerHeldItemButtonIsHeldDown=true;
Handle_SwitchHook(&p,&play);Player_Action_43(&p,&play);
sPlayerUseHeldItem=sPlayerHeldItemButtonIsHeldDown=false;
Player_Action_43(&p,&play);assert(shots==1 && !p.actor.child && !p.heldActor && !hook.parent);
SwitchHook_OnFired(&p);assert(!SwitchHook_IsAimingManual() && p.unk_AA5==PLAYER_UNKAA5_0);
// The C-Up exit must not become native C-Up peek on the same edge.
p.actor.child=&hook;p.heldActor=&hook;p.unk_B28=-3;p.upperActionFunc=Player_UpperAction_6;
play.state.input[0].press.button=BTN_CUP;Handle_SwitchHook(&p,&play);
play.state.input[0].press.button=BTN_CUP;Handle_SwitchHook(&p,&play);
assert(!SwitchHook_IsAimingManual() && p.unk_AA5==PLAYER_UNKAA5_0);
Player_ActionHandler_0(&p,&play);assert(p.unk_AA5==PLAYER_UNKAA5_0 && peekCalls==0);
// Native admission rejects lock-on, invalid cameras and airborne entry.
for(int scenario=0;scenario<4;++scenario){
    Actor target={0};targeted=scenario==0;p.focusActor=scenario==1?&target:NULL;
    validCamera=scenario!=2;p.actor.bgCheckFlags=scenario==3?0:BGCHECKFLAG_GROUND;
    play.state.input[0].press.button=BTN_CUP;Handle_SwitchHook(&p,&play);
    assert(!SwitchHook_IsAimingManual() && p.unk_AA5==PLAYER_UNKAA5_0);
    Player_ActionHandler_0(&p,&play);assert(p.unk_AA5==PLAYER_UNKAA5_0);
}
targeted=false;p.focusActor=NULL;validCamera=true;p.actor.bgCheckFlags=BGCHECKFLAG_GROUND;
// Every native cancellation ends ownership rather than being reopened next update.
for(int scenario=0;scenario<10;++scenario){
    p.heldItemId=ITEM_SWITCH_HOOK;p.heldItemAction=PLAYER_IA_HOOKSHOT;p.unk_AA5=PLAYER_UNKAA5_0;
    play.state.input[0].press.button=BTN_CUP;Handle_SwitchHook(&p,&play);
    assert(SwitchHook_IsAimingManual());play.state.input[0].press.button=0;
    switch(scenario){
    case 0:p.unk_AA5=PLAYER_UNKAA5_0;break;
    case 1:p.heldItemId=ITEM_BOW;p.heldItemAction=PLAYER_IA_BOW;p.unk_AA5=PLAYER_UNKAA5_3;break;
    case 2:p.stateFlags1|=PLAYER_STATE1_DAMAGED;break;
    case 3:p.csAction=PLAYER_CSACTION_1;break;
    case 4:play.csCtx.state=1;break;
    case 5:play.transitionTrigger=TRANS_TRIGGER_START;break;
    case 6:play.state.input[0].press.button=BTN_A;break;
    case 7:play.state.input[0].press.button=BTN_B;break;
    case 8:play.state.input[0].press.button=BTN_R;break;
    case 9:SwitchHook_OnFired(&p);break;
    }
    Handle_SwitchHook(&p,&play);assert(!SwitchHook_IsAimingManual());
    if(scenario==1)assert(p.unk_AA5==PLAYER_UNKAA5_3);else assert(p.unk_AA5==PLAYER_UNKAA5_0);
    p.stateFlags1&=~PLAYER_STATE1_DAMAGED;p.csAction=PLAYER_CSACTION_NONE;play.csCtx.state=CS_STATE_IDLE;play.transitionTrigger=TRANS_TRIGGER_OFF;
}
// Ordinary hookshot admission remains native, regardless of manual ownership.
p.heldItemId=ITEM_HOOKSHOT;p.heldItemAction=PLAYER_IA_HOOKSHOT;p.unk_AA5=PLAYER_UNKAA5_0;
play.state.input[0].press.button=BTN_CUP;Handle_SwitchHook(&p,&play);
assert(play.state.input[0].press.button&BTN_CUP);
assert(!func_80831010(&p,&play) && p.unk_AA5==PLAYER_UNKAA5_3);
puts("PASS native Switch Hook aim retention, input, launch, cancellation and native Hookshot admission");
'''
with tempfile.TemporaryDirectory(prefix='mm-switchhook-aim-') as tmp:
    path=Path(tmp)/'aim.c'
    path.write_text(prefix+'\n'+'\n'.join(parts)+'\nint main(){\n'+checks+'\n}\n')
    binary=Path(tmp)/'aim'
    sanitize=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g'] if '--sanitize' in sys.argv else []
    subprocess.run(['cc','-std=gnu17',*flags(),*sanitize,'-Wno-incompatible-pointer-types','-Wno-discarded-qualifiers','-Wno-discarded-array-qualifiers','-ffunction-sections','-fdata-sections',str(path),'-Wl,--gc-sections','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
