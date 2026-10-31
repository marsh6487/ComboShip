#!/usr/bin/env python3
"""Exercise production MM per-item input cache and actual dispatch loop once/frame."""
import argparse,os,re,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from run_tests import PREFIX,flags,function,claim_definition
EXTRA=r'''
#include "mods/items/custom_items.h"
#include "mods/items/helpers/equip_helper.h"
CustomItemState gCustomItemState{};
static bool bombMode;
u8 Sw97_IsBowItem(u16 item){return item==ITEM_BOW;}
u8 Sw97_IsSlingItem(u16 item){return item==ITEM_FAIRY_SLINGSHOT;}
u8 Sw97_EffectiveElement(u8){return bombMode?SW97_ELEM_BOMB:SW97_ELEM_NONE;}
u8 Sw97_ItemHasBombs(u16 item){return bombMode&&(Sw97_IsBowItem(item)||Sw97_IsSlingItem(item));}
static int calls[64];
'''
CHECKS=r'''
int main(int argc,char**argv){
 static PlayState play;Player player{};
 Reset();play={};play.gameplayFrames=1;gSaveContext.save.playerForm=PLAYER_FORM_HUMAN;
 if(!strcmp(argv[1],"cache")){
  BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_C_LEFT)=ITEM_ROCS_FEATHER_SKIJER;
  DPAD_BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_D_UP)=ITEM_ROCS_FEATHER_SKIJER;
  assert(ItemInput_GetEquippedButton(ITEM_ROCS_FEATHER_SKIJER,&play)==(BTN_CLEFT|BTN_DUP)&&"duplicate independent bindings must retain the union of input edges");
  ItemInputState statusInput{};
  gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT]=BTN_DISABLED;
  play.state.input[0].press.button=play.state.input[0].cur.button=BTN_CLEFT;
  ItemInput_Update(&statusInput,ITEM_ROCS_FEATHER_SKIJER,&player,&play);
  assert(statusInput.equippedButton==BTN_DUP&&!statusInput.isPressed&&"disabled C binding must not contribute an edge to an enabled D duplicate");
  play.state.input[0].press.button=play.state.input[0].cur.button=BTN_DUP;
  ItemInput_Update(&statusInput,ITEM_ROCS_FEATHER_SKIJER,&player,&play);assert(statusInput.isPressed);
  gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT]=BTN_ENABLED;
  gSaveContext.shipSaveContext.dpad.status[EQUIP_SLOT_D_UP]=BTN_DISABLED;
  ItemInput_Update(&statusInput,ITEM_ROCS_FEATHER_SKIJER,&player,&play);
  assert(statusInput.equippedButton==BTN_CLEFT&&!statusInput.isPressed&&"disabled D binding must not contribute an edge to an enabled C duplicate");
  play.state.input[0].press.button=play.state.input[0].cur.button=BTN_CLEFT;
  ItemInput_Update(&statusInput,ITEM_ROCS_FEATHER_SKIJER,&player,&play);assert(statusInput.isPressed);
  gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT]=BTN_DISABLED;
  assert(ItemInput_GetEquippedButton(ITEM_ROCS_FEATHER_SKIJER,&play)==0);
  gSaveContext.buttonStatus[EQUIP_SLOT_C_LEFT]=BTN_ENABLED;
  gSaveContext.shipSaveContext.dpad.status[EQUIP_SLOT_D_UP]=BTN_ENABLED;
  pacci=true;play.state.input[0].press.button=BTN_CLEFT;play.state.input[0].cur.button=BTN_CLEFT;
  ItemInputState input{};ItemInput_Update(&input,ITEM_ROCS_FEATHER_SKIJER,&player,&play);
  assert(input.equippedButton==BTN_CLEFT&&input.isPressed&&"Ultrahand claims only Dpad; duplicated C binding remains usable");
  pacci=false;cryonis=true;
  assert(ItemInput_GetEquippedButton(ITEM_ROCS_FEATHER_SKIJER,&play)==BTN_CLEFT&&"Cryonis distance input must not activate a cached Dup item; C remains usable");
  cryonis=false;play.gameplayFrames++;bombMode=true;
  BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_C_LEFT)=ITEM_BOW;
  DPAD_BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_D_UP)=ITEM_FAIRY_SLINGSHOT;
  assert(ItemInput_GetEquippedButton(ITEM_BOMB_ARROWS,&play)==(BTN_CLEFT|BTN_DUP)&&"bow and slingshot Bomb Arrows share the union of both inputs");
  const u16 cMasks[]={BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT};
  const u16 dMasks[]={BTN_DRIGHT,BTN_DLEFT,BTN_DDOWN,BTN_DUP};
  for(u16 item:{u16(ITEM_WHIP),u16(ITEM_NET)})for(int c=1;c<=3;++c)for(int d=0;d<4;++d){
   Reset();play.gameplayFrames++;
   BUTTON_ITEM_EQUIP(0,c)=item;DPAD_BUTTON_ITEM_EQUIP(0,d)=item;
   assert(ItemInput_GetEquippedButton(item,&play)==(cMasks[c-1]|dMasks[d]));
   gSaveContext.buttonStatus[c]=BTN_DISABLED;
   play.state.input[0].press.button=play.state.input[0].cur.button=cMasks[c-1];
   ItemInput_Update(&statusInput,item,&player,&play);
   assert(statusInput.equippedButton==dMasks[d]&&!statusInput.isPressed);
   play.state.input[0].press.button=play.state.input[0].cur.button=dMasks[d];
   ItemInput_Update(&statusInput,item,&player,&play);assert(statusInput.isPressed);
   gSaveContext.buttonStatus[c]=BTN_ENABLED;gSaveContext.shipSaveContext.dpad.status[d]=BTN_DISABLED;
   ItemInput_Update(&statusInput,item,&player,&play);
   assert(statusInput.equippedButton==cMasks[c-1]&&!statusInput.isPressed);
   play.state.input[0].press.button=play.state.input[0].cur.button=cMasks[c-1];
   ItemInput_Update(&statusInput,item,&player,&play);assert(statusInput.isPressed);
   gSaveContext.buttonStatus[c]=BTN_DISABLED;assert(ItemInput_GetEquippedButton(item,&play)==0);
  }
  Reset();play.gameplayFrames++;
  BUTTON_ITEM_EQUIP(CUR_FORM,EQUIP_SLOT_B)=ITEM_WHIP;gSaveContext.buttonStatus[EQUIP_SLOT_B]=BTN_DISABLED;
  assert(ItemInput_GetEquippedButton(ITEM_WHIP,&play)==BTN_B&&"B's existing per-form input contract remains unchanged");

 }else{
  for(u16 item:{u16(ITEM_NET),u16(ITEM_WHIP),u16(ITEM_BOW)}){
   Reset();memset(calls,0,sizeof(calls));bombMode=item==ITEM_BOW;
   BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_C_LEFT)=item;DPAD_BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_D_UP)=item;
   Dispatch(&player,&play);
   int total=0;for(int count:calls)total+=count;
   assert(total==1&&"one item handler runs once per frame even with C and Dpad duplicates");
  }
 }
 if(strcmp(argv[1],"cache")){
  Reset();memset(calls,0,sizeof(calls));bombMode=false;cryonis=true;
  DPAD_BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_D_UP)=ITEM_NET;Dispatch(&player,&play);
  int total=0;for(int count:calls)total+=count;
  assert(total==1&&"claimed input must not skip active-item cleanup/state update dispatch");
 }
 printf("PASS MM duplicate %s: usable input union, C/D status mapping and once-per-item dispatch\n",argv[1]);
}
'''
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--source-ref');ap.add_argument('--sanitize',action='store_true');ap.add_argument('--solo',action='store_true');a=ap.parse_args()
 def read(p):return subprocess.check_output(['git','show',a.source_ref+':'+p],cwd=ROOT,text=True) if a.source_ref else (ROOT/p).read_text()
 helper=read('mm/mods/items/helpers/equip_helper.c');common=read('mm/mods/items/custom_items_common.c')
 first=helper.index('typedef struct {');last=helper.index('u8 ItemEquip_GetItemOnSlot',first)
 code=PREFIX+claim_definition(read)+EXTRA+helper[first:last]+'\n'+'\n'.join(function(helper,n) for n in ('ItemEquip_GetItemOnSlot','EquipCache_Update','ItemInput_GetEquippedButton','ItemInput_CheckOtherButtons','ItemInput_Update'))
 first=common.index('    u8 dispatched[8];');last=common.index('    // Ultrahand assemblies',first)
 dispatch=common[first:last]
 for index,name in enumerate(sorted(set(re.findall(r'\bHandle_\w+(?=\()',dispatch)))):code+=f'\nvoid {name}(Player*,PlayState*){{++calls[{index}];}}\n'
 code+='\nvoid Dispatch(Player* p,PlayState* play){\n'+dispatch+'}\n'+CHECKS
 with tempfile.TemporaryDirectory(prefix='nei-controls-mm-duplicate-') as td:
  p=Path(td);s=p/'test.cpp';b=p/'test';s.write_text(code)
  san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'] if a.sanitize else []
  subprocess.run(['c++','-std=c++20',*flags(),*([] if a.solo else ['-DCOMBO_BUILD']),*san,str(s),'-o',str(b)],check=True)
  results=[subprocess.run([str(b),case],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')).returncode for case in ('cache','dispatch')]
  return int(any(results))
if __name__=='__main__':raise SystemExit(main())
