"""Run the rod through native use/finish/init, with animation scheduling at the engine boundary."""
import ast
import re
import subprocess
import sys
import tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_mm_weather_tests import production_function

def body(path,name): return production_function((ROOT/path).read_text(),name)
def native_decl(text,start):
 begin=text.index(start);end=begin+re.search(r'\n}[^;\n]*;',text[begin:]).end()
 return text[begin:end]
# Reuse the existing real-native-call fixture's unrelated engine boundaries without running its tests.
fixture=ast.parse((ROOT/'tests/mm_nei/run_use_tests.py').read_text())
prefix=next(ast.literal_eval(n.value) for n in fixture.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='caller_prefix' for t in n.targets))
prefix='#include "mods/extended_inventory.h"\n'+prefix
prefix=prefix.replace('void CustomItems_PutAwayHeldItems(Player* p,PlayState* play){}','static void Seasons_Stow(PlayState*,Player*);\nvoid CustomItems_PutAwayHeldItems(Player* p,PlayState* play){Seasons_Stow(play,p);}')
prefix=prefix.replace('void Player_InitItemActionWithAnim(PlayState* play,Player* p,PlayerItemAction action){++initializations;p->heldItemAction=p->itemAction=action;p->modelGroup=p->nextModelGroup;}','void Player_InitItemAction(PlayState*,Player*,PlayerItemAction);\nvoid Player_InitItemActionWithAnim(PlayState* play,Player* p,PlayerItemAction action){++initializations;Player_InitItemAction(play,p,action);}')
prefix=prefix.replace('void Player_StartChangingHeldItem(Player* p,PlayState* play){++transitions;animationDestination=p->nextModelGroup;p->stateFlags3&=~PLAYER_STATE3_START_CHANGING_HELD_ITEM;}','s32 Player_UpperAction_ChangeHeldItem(Player*,PlayState*);\nvoid Player_StartChangingHeldItem(Player* p,PlayState* play){++transitions;animationDestination=p->nextModelGroup;p->stateFlags3&=~PLAYER_STATE3_START_CHANGING_HELD_ITEM;p->upperActionFunc=Player_UpperAction_ChangeHeldItem;}')
prefix+=r'''
PlayState* gPlayState;
int rodEquips,rodStows,menus,menuOpen,sceneNumber;
u8 MasterCycle_IsRiding(void){return 0;}
u8 Pacci_UltrahandModeActive(void){return 0;}
int MMWeather_Season(void){return -1;}
void Seasons_UpdateWeather(PlayState* play){}
NeiSaveData* Nei_Save(void){return &gSaveContext.save.shipSaveInfo.nei;}
void Nei_SetOwnedItem(u8 slot,u16 item){Nei_Save()->ownedItems[slot-24]=item;}
u16 ExtButton_GetItem(s32 form,s32 btn){return BUTTON_ITEM_EQUIP(form,btn)==ITEM_EXT_BUTTON?EXT_BUTTON_ITEM(form,btn):BUTTON_ITEM_EQUIP(form,btn);}
u16 ExtButton_GetDpadItem(s32 form,s32 btn){return gSaveContext.save.shipSaveInfo.dpadEquips.extItems[form][btn];}
void ItemEquip_PlayEquipSFX(PlayState* play,Player* p){++rodEquips;}
void ItemEquip_PlayUnequipSFX(PlayState* play,Player* p){++rodStows;}
void ItemEquip_ResetUnequipSound(PlayState* play,Player* p,s32 action){}
u8 ItemEquip_ClaimUnequipSound(PlayState* play,Player* p,s32 action){return 0;}
PlayerSword Player_SwordFromIA(Player* p,PlayerItemAction a){return a==PLAYER_IA_SWORD_KOKIRI?PLAYER_SWORD_KOKIRI:PLAYER_SWORD_NONE;}
void NativeInitNone(PlayState* play,Player* p){}
ItemActionInitFunc ExtPlayer_GetItemActionInitFunc(s32 action){return NativeInitNone;}
void Player_SetModelGroup(Player* p,PlayerModelGroup group){p->modelGroup=group;p->rightHandType=PLAYER_MODELTYPE_RH_OPEN;}
s32 Player_UpperAction_ChangeHeldItem(Player* p,PlayState* play){return 0;}
typedef struct {const char* iconPath;u8 iconSize;u8 enabled;} BoxMenuEntry;
typedef void (*BoxMenuConfirmFn)(s32);
u8 BoxMenu_IsOpen(void){return menuOpen;}
u8 BoxMenu_Open(PlayState* play,const BoxMenuEntry* entries,s32 count,s32 chosen,u16 hold,BoxMenuConfirmFn fn){++menus;return 1;}
void ExtInv_RefreshButtonIconsForItem(PlayState* play,u16 item){}
'''
player='mm/src/overlays/actors/ovl_player_actor/z_player.c'
lib='mm/src/code/z_player_lib.c'
ps=(ROOT/player).read_text();ls=(ROOT/lib).read_text()
parts=[native_decl(ps,'typedef enum ItemChangeType {'),native_decl(ps,'s8 sPlayerItemChangeTypes['),native_decl(ls,'PlayerModelIndices gPlayerModelTypes[')]
parts+=[body(player,'Player_ItemToItemAction'),body(lib,'Player_ActionToModelGroup')]
parts+=[body(player,n) for n in ['Player_InitItemAction','Player_UseItem','Player_FinishItemChange']]
parts+=[body('mm/mods/items/helpers/equip_helper.c',n) for n in ['ItemInput_IsBlockedEx','ItemInput_IsBlocked']]
parts+=[body('mm/mods/extended_inventory.c',n) for n in ['Seasons_SeasonOwned','Seasons_GrantSeason','Seasons_SeasonCount','Seasons_SeasonAt','Seasons_GetSeason','Seasons_SetSeason']]
parts+=[(ROOT/'mm/mods/items/logic/item_rod_of_seasons.c').read_text().split('// Seasonal particles')[0]]
checks=r'''
void ResetRod(Player* p,PlayState* play,ItemId old){
 ResetCaller(p,play,old,0);play->sceneId=++sceneNumber;gPlayState=play;
 play->actorCtx.actorLists[ACTORCAT_PLAYER].first=&p->actor;
 p->heldItemAction=p->itemAction=Player_ItemToItemAction(p,old);
 p->modelGroup=Player_ActionToModelGroup(p,p->heldItemAction);
 p->rightHandType=old==ITEM_NONE?PLAYER_MODELTYPE_RH_OPEN:PLAYER_MODELTYPE_RH_BOW;
 dpadEnabled=1;menuOpen=0;rodEquips=rodStows=menus=0;
 memset(Nei_Save(),0,sizeof(*Nei_Save()));Seasons_GrantSeason(SEASON_WINTER);
 memset(&gSaveContext.save.shipSaveInfo.dpadEquips,0,sizeof(DpadSaveInfo));
 gSaveContext.save.shipSaveInfo.dpadEquips.extItems[0][EQUIP_SLOT_D_UP]=EXT_ITEM_ROD_OF_SEASONS;
}
void TickRod(Player* p,PlayState* play,u16 press){
 ++play->gameplayFrames;play->state.input[0].press.button=press;Seasons_TickInput(play,p);
}
void FinishNative(Player* p,PlayState* play){
 Player_StartChangingHeldItem(p,play);
 Player_FinishItemChange(play,p);
 assert(p->heldItemAction==PLAYER_IA_NONE&&p->itemAction==PLAYER_IA_NONE);
 // The animation owner releases the upper action only after the native item change completes.
 p->upperActionFunc=NULL;
}
int main(void){
 Player p;PlayState play;
 ItemId items[]={ITEM_SWORD_KOKIRI,ITEM_BOW};
 for(int i=0;i<2;i++){
  ResetRod(&p,&play,items[i]);TickRod(&p,&play,BTN_DUP);
  assert(!Seasons_IsDrawn()&&!rodEquips);
  assert(p.stateFlags3&PLAYER_STATE3_START_CHANGING_HELD_ITEM);
  TickRod(&p,&play,0);assert(!Seasons_IsDrawn()); // pending request survives the native start flag
  Player_StartChangingHeldItem(&p,&play);TickRod(&p,&play,0);assert(!Seasons_IsDrawn());
  Player_FinishItemChange(&play,&p);assert(p.heldItemAction==PLAYER_IA_NONE);
  TickRod(&p,&play,0);assert(!Seasons_IsDrawn()); // final native model setup is still owned by the animation
  p.upperActionFunc=NULL;TickRod(&p,&play,0);
  assert(Seasons_IsDrawn()&&rodEquips==1&&p.rightHandType==PLAYER_MODELTYPE_RH_CLOSED);
  TickRod(&p,&play,0);assert(Seasons_IsDrawn()&&rodEquips==1);
  TickRod(&p,&play,BTN_DUP);assert(menus==1&&Seasons_IsDrawn());
  TickRod(&p,&play,BTN_B);assert(!Seasons_IsDrawn()&&p.rightHandType==PLAYER_MODELTYPE_RH_OPEN);
  assert(play.state.input[0].press.button&BTN_B);
 }
 // All cancellation paths must discard the delayed draw, even if native unequip later finishes.
 for(int cancel=0;cancel<8;cancel++){
  ResetRod(&p,&play,ITEM_BOW);TickRod(&p,&play,BTN_DUP);
  switch(cancel){
   case 0:p.stateFlags1|=PLAYER_STATE1_DAMAGED;break;
   case 1:menuOpen=1;break;
   case 2:p.transformation=PLAYER_FORM_DEKU;break;
   case 3:gSaveContext.save.shipSaveInfo.dpadEquips.extItems[0][EQUIP_SLOT_D_UP]=ITEM_NONE;break;
   case 4:play.pauseCtx.state=PAUSE_STATE_MAIN;break;
   case 5:play.csCtx.state=1;break;
   case 6:p.stateFlags1|=PLAYER_STATE1_SHIELDING;break;
  }
  TickRod(&p,&play,cancel==7?BTN_B:0);
  p.stateFlags1=0;p.transformation=PLAYER_FORM_HUMAN;menuOpen=0;play.pauseCtx.state=PAUSE_STATE_OFF;play.csCtx.state=CS_STATE_IDLE;
  gSaveContext.save.shipSaveInfo.dpadEquips.extItems[0][EQUIP_SLOT_D_UP]=EXT_ITEM_ROD_OF_SEASONS;
  FinishNative(&p,&play);TickRod(&p,&play,0);assert(!Seasons_IsDrawn()&&!rodEquips);
 }
 // A rejected native request must not arm a rod that appears later.
 ResetRod(&p,&play,ITEM_BOW);p.itemAction=PLAYER_IA_SWORD_KOKIRI;TickRod(&p,&play,BTN_DUP);
 assert(!Seasons_IsDrawn()&&p.heldItemId==ITEM_BOW&&!(p.stateFlags3&PLAYER_STATE3_START_CHANGING_HELD_ITEM));
 p.itemAction=p.heldItemAction=PLAYER_IA_NONE;TickRod(&p,&play,0);assert(!Seasons_IsDrawn()&&!rodEquips);
 // A script/native replacement, without a new button edge, also supersedes the pending rod.
 ResetRod(&p,&play,ITEM_BOW);TickRod(&p,&play,BTN_DUP);
 Player_UseItem(&play,&p,ITEM_SWORD_KOKIRI);TickRod(&p,&play,0);
 Player_StartChangingHeldItem(&p,&play);Player_FinishItemChange(&play,&p);p.upperActionFunc=NULL;
 TickRod(&p,&play,0);assert(!Seasons_IsDrawn()&&!rodEquips&&p.heldItemAction==PLAYER_IA_SWORD_KOKIRI);
 ResetRod(&p,&play,ITEM_NONE);TickRod(&p,&play,BTN_DUP);assert(Seasons_IsDrawn()&&rodEquips==1);
 puts("PASS rod native use/finish/init lifecycle, final hand capture and cancellation");
}
'''
with tempfile.TemporaryDirectory(prefix='rod-lifecycle-') as td:
 path=Path(td)/'test.c';path.write_text(prefix+'\n'.join(parts)+checks);binary=Path(td)/'test'
 # The real grant always uses the fixed Rod cell; Release-style inlining keeps
 # the unrelated Grace shared-slot implementation outside this rod fixture.
 subprocess.run(['cc','-std=gnu17','-O2','-w','-Werror=implicit-function-declaration',*flags(),'-ffunction-sections','-fdata-sections',str(path),'-Wl,--gc-sections','-lm','-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
