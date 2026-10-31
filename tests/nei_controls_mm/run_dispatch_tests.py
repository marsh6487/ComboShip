#!/usr/bin/env python3
"""Run native MM button readers, input reservation and equipment toggle policy.

Player refresh/audio are boundaries; the native save layout, accessor code,
button-read status policy, dispatch condition, reserve-before-tools body,
ownership/age/Trident checks and toggle body are production.
"""
import argparse,os,re,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from run_tests import PREFIX,flags,function,claim_definition
EXTRA=r'''
static u8 current[4];static bool ageAllowed=true,pikachu,enabled;
static u16 seenPress;
static const u16 sPlayerItemButtons[]={BTN_B,BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT};
static const u16 sDpadItemButtons[]={BTN_DRIGHT,BTN_DLEFT,BTN_DDOWN,BTN_DUP};
#define MM_PLAYER_FORM_PIKACHU 9
u8 ExtEquip_GetCurrent(s16 type){return current[type];}
void ExtEquip_SetSlot(s16 type,u8 index){current[type]=index;}
u8 ExtEquip_CheckAgeReq(s16,u8){return ageAllowed;}
u8 ExtEquip_SlotRetired(s16,u8){return 0;}
u8 ExtEquip_IsEnabled(){return enabled;}
extern "C" void CVarSetInteger(const char*,s32 value){enabled=value;}
u8 TransformMasks_IsTransformedAny(){return pikachu;}
int MmForm_GetCurrentForm(){return pikachu?MM_PLAYER_FORM_PIKACHU:0;}


bool GameInteractor_Should(GIVanillaBehavior,uint32_t result,...){return result;}
s32 Inventory_GetBtnBItem(PlayState*){return BUTTON_ITEM_EQUIP(CUR_FORM,EQUIP_SLOT_B);}
Vec3f gSfxDefaultPos{};f32 gSfxDefaultFreqAndVolScale=1;s8 gSfxDefaultReverb;
void Audio_PlaySoundGeneral(u16,Vec3f*,u8,f32*,f32*,s8*){}
void CustomItems_Update(Player*,PlayState* play){seenPress=play->state.input[0].press.button;}
'''
CHECKS=r'''
int main(){
 static PlayState play;Player player{};
 const u16 masks[]={BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT,BTN_DRIGHT,BTN_DLEFT,BTN_DDOWN,BTN_DUP};
 for(int row=0;row<4;++row)for(int col=1;col<=3;++col)for(int b=0;b<7;++b)for(int legacy=0;legacy<2;++legacy){
  Reset();memset(current,0,sizeof(current));ageAllowed=true;pikachu=riding=pacci=enabled=false;play={};
  player={};gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;
  const u16 item=legacy?ITEM_EXT_SWORD_1+row*3+col-1:ExtEquip_GetButtonItemId(row,col);
  if(b<3){if(legacy)BUTTON_ITEM_EQUIP(0,b+1)=item;else ExtButton_SetItem(0,b+1,item);}
  else ExtButton_SetDpadItem(0,b-3,item);
  nei.extEquipOwnedBits=0xFFF0000;
  play.state.input[0].press.button=masks[b];
  player.heldItemAction=PLAYER_IA_HOOKSHOT;
  Player_UpdateCustomItems(&player,&play);
  assert(!(seenPress&masks[b])&&"equipment edge must not stow another held NEI tool");
  assert(play.state.input[0].press.button==masks[b]);
  ItemId actual=b<3?Player_GetItemOnButton(&play,&player,(EquipSlot)(b+1)):
                       Player_Dpad_GetItemOnButton(&play,&player,(DpadEquipSlot)(b-3));
  assert(actual==ITEM_EXT_SWORD_1+row*3+col-1 && "native reader resolves full equipment identity and legacy saves");
  NativeToggle(&play,&player,b<3?b+1:DPAD_TO_HELD_ITEM(b-3),actual);
  assert(current[row]==col&&enabled);
  NativeToggle(&play,&player,b<3?b+1:DPAD_TO_HELD_ITEM(b-3),actual);
  assert(current[row]==0 && player.heldItemAction==PLAYER_IA_HOOKSHOT);
  cryonis=true;
  ItemId cryonisItem=b<3?Player_GetItemOnButton(&play,&player,(EquipSlot)(b+1)):Player_Dpad_GetItemOnButton(&play,&player,(DpadEquipSlot)(b-3));
  assert(cryonisItem==((b==5||b==6)?ITEM_NONE:actual)&&"Cryonis distances must not also toggle native D equipment; C and D left/right remain usable");cryonis=false;
  if(b>=3){pacci=true;assert(Player_Dpad_GetItemOnButton(&play,&player,(DpadEquipSlot)(b-3))==ITEM_NONE&&"Ultrahand Dpad rotation must not toggle independently assigned equipment");pacci=false;}
  // An age change may prevent putting a piece on, never taking it off.
  ageAllowed=false;ExtEquip_ToggleFromCButton(item);assert(current[row]==0);
  current[row]=col;ExtEquip_ToggleFromCButton(item);assert(current[row]==0);
  ageAllowed=true;nei.extEquipOwnedBits=0;ExtEquip_ToggleFromCButton(item);assert(current[row]==0);
 }
 // Dedicated full-ID tools remain markers for native code, never equipment.
 for(int d=0;d<4;++d){
  Reset();play={};pacci=riding=false;ExtButton_SetDpadItem(0,d,EXT_ITEM_ROD_OF_SEASONS);
  assert(Seasons_EquippedButtonMask()==masks[d+3]);
  assert(Player_Dpad_GetItemOnButton(&play,&player,(DpadEquipSlot)d)==ITEM_EXT_BUTTON);
  gSaveContext.shipSaveContext.dpad.status[d]=BTN_DISABLED;assert(Seasons_EquippedButtonMask()==0);
  gSaveContext.shipSaveContext.dpad.status[d]=BTN_ENABLED;
  dpadEnabled=0;assert(Seasons_EquippedButtonMask()==0);dpadEnabled=1;
  pacci=true;assert(Seasons_EquippedButtonMask()==0);pacci=false;
  riding=true;assert(Player_Dpad_GetItemOnButton(&play,&player,(DpadEquipSlot)d)==ITEM_NONE);riding=false;
 }
 puts("PASS MM 12 equipment pieces x seven C/D inputs: full-ID/legacy, toggles, held-item preservation, ownership/age; Rod all D directions/status/mode guards");
}
'''
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--sanitize',action='store_true');ap.add_argument('--solo',action='store_true');a=ap.parse_args()
 def read(p):return (ROOT/p).read_text()
 eq=read('mm/mods/extended_equipment.c');lib=read('mm/src/code/z_player_lib.c');player=read('mm/src/overlays/actors/ovl_player_actor/z_player.c');buttons=read('mm/mods/ext_buttons/ext_buttons.cpp')
 code=PREFIX+claim_definition(read)+EXTRA+'\n'+buttons[buttons.index('u16 ExtButton_GetItem'):buttons.rindex('} // extern')]
 for n in ('ExtEquip_GetButtonItemId','ExtEquip_IsButtonItem','ExtEquip_GetLegacyButtonItem','ExtEquip_GetBit','ExtEquip_HasItem','ExtEquip_TridentAllowsShield','ExtEquip_Equip','ExtEquip_ToggleFromCButton'):code+='\n'+function(eq,n)
 for n in ('Player_GetItemOnButton','Player_Dpad_GetItemOnButton'):code+='\n'+function(lib,n)
 code+='\n'+re.sub(r'\bthis\b','player',function(player,'Player_UpdateCustomItems'))
 # Execute the actual native branch, with readers exercised above. The remainder
 # of Player_UpdateItems contains unrelated native combat and mask use.
 start=player.index('} else if (i != EQUIP_SLOT_B && item >= ITEM_EXT_SWORD_1')
 end=player.index('} else if (item == ITEM_F0)',start)
 branch=player[start:end]
 code+='\nvoid NativeToggle(PlayState* play,Player* player,s32 i,ItemId item){ if(false){'+branch+'}}\n'
 code+='\n'+function(read('mm/mods/items/logic/item_rod_of_seasons.c'),'Seasons_EquippedButtonMask')+CHECKS
 # Preserve C's implicit enum conversions in the C++ test harness.
 code=re.sub(r'(item\s*=\s*)(C_BTN_ITEM|DPAD_BTN_ITEM)(\([^;]+\))',r'\1(ItemId)\2\3',code)
 code=code.replace('ItemId item = Inventory_GetBtnBItem(play);','ItemId item = (ItemId)Inventory_GetBtnBItem(play);')
 code=code.replace('play, player, button);','play, player, (EquipSlot)button);',1)
 code=code.replace('Player_Dpad_GetItemOnButton(play, player, button);','Player_Dpad_GetItemOnButton(play, player, (DpadEquipSlot)button);')
 with tempfile.TemporaryDirectory(prefix='nei-controls-mm-dispatch-') as td:
  p=Path(td);s=p/'test.cpp';b=p/'test';s.write_text(code)
  san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'] if a.sanitize else []
  subprocess.run(['c++','-std=c++20',*flags(),*([] if a.solo else ['-DCOMBO_BUILD']),*san,str(s),'-o',str(b)],check=True)
  subprocess.run([str(b)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1'))
if __name__=='__main__':main()
