#!/usr/bin/env python3
"""Execute MM quest stone assignment and the complete native hold/tap reader."""
import argparse,os,re,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from run_tests import PREFIX,flags,function,claim_definition
EXTRA=r'''
#include "mods/spiritual_stones/spiritual_stones.h"
static bool hasStone=true;
static int sOotQuestCursorPoint=0x12,sPendingWarpStone=-1;
static s16 sHoldFrames[SPIRITUAL_STONE_COUNT];
static int summoned=-1,prompted=-1;
static bool OotQuest_Has(s16){return hasStone;}
void SpiritualStone_TogglePassive(s32){}
void Interface_Dpad_LoadItemIcon(PlayState* play,u8 btn){Interface_Dpad_LoadItemIconImpl(play,btn);}
static bool StonesEnabled(){return true;}
static auto& State(){return gSaveContext.save.shipSaveInfo.spiritualStones;}


u8 Message_GetState(MessageContext*){return 0;}
bool Message_ShouldAdvance(PlayState*){return false;}
void Message_CloseTextbox(PlayState*){}
static void ExecuteWarp(PlayState*,int){}
static void OpenWarpPrompt(PlayState*,Player*,int stone){prompted=stone;}
static void SummonStatueHere(PlayState*,Player*,int stone){summoned=stone;}
void Actor_PlaySfx(Actor*,u16){}
'''
CHECKS=r'''
int main(int argc,char**argv){
 const u16 masks[]={BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT,BTN_DRIGHT,BTN_DLEFT,BTN_DDOWN,BTN_DUP};
 const u16 items[]={EXT_ITEM_SPIRITUAL_STONE_KOKIRI,EXT_ITEM_SPIRITUAL_STONE_GORON,EXT_ITEM_SPIRITUAL_STONE_ZORA};
 static PlayState play;Player player{};
 for(int stone=0;stone<3;++stone)for(int b=0;b<7;++b){
  Reset();play={};pacci=riding=false;hasStone=true;memset(sHoldFrames,0,sizeof(sHoldFrames));
  sPendingWarpStone=summoned=prompted=-1;sOotQuestCursorPoint=0x12+stone;
  if(!strcmp(argv[1],"assign")){
   // An existing unrelated C mapping and full payload must survive D assignment.
   ExtButton_SetItem(0,1,EXT_ITEM_ROD_OF_SEASONS);
   auto bank=gSaveContext.save.saveInfo.equips;auto ext=gSaveContext.save.shipSaveInfo.extButtons;
   play.state.input[0].press.button=masks[b];StoneSelect(&play,&play.state.input[0]);
   assert((b<3?ExtButton_GetItem(0,b+1):ExtButton_GetDpadItem(0,b-3))==items[stone]&&"all spiritual stones equip directly onto every C/D direction");
   if(b>=3){assert(!memcmp(&bank,&gSaveContext.save.saveInfo.equips,sizeof(bank)));assert(!memcmp(&ext,&gSaveContext.save.shipSaveInfo.extButtons,sizeof(ext)));}
   hasStone=false;Reset();hasStone=false;StoneSelect(&play,&play.state.input[0]);
   assert(cIcons==0&&dIcons==0);
   dpadEnabled=0;hasStone=true;play.state.input[0].press.button=BTN_DUP;StoneSelect(&play,&play.state.input[0]);assert(dIcons==0);
  }else{
   if(b<3)ExtButton_SetItem(0,b+1,items[stone]);else ExtButton_SetDpadItem(0,b-3,items[stone]);
   for(auto& warp:State().warp)warp.entranceId=1;
   play.state.input[0].cur.button=masks[b];
   for(int frame=0;frame<SPIRITUAL_STONE_SUMMON_HOLD_FRAMES;++frame)SpiritualStone_TickHold(&play,&player);
   assert(summoned==stone&&"Dpad stone hold must reach the real summon branch");
   memset(sHoldFrames,0,sizeof(sHoldFrames));summoned=prompted=-1;
   SpiritualStone_TickHold(&play,&player);play.state.input[0].cur.button=0;SpiritualStone_TickHold(&play,&player);
   assert(prompted==stone&&"Dpad short release must reach the existing warp-prompt branch");
   memset(sHoldFrames,0,sizeof(sHoldFrames));cryonis=true;play.state.input[0].cur.button=masks[b];
   SpiritualStone_TickHold(&play,&player);
   assert(sHoldFrames[stone]==((b==5||b==6)?0:1)&&"Cryonis distance controls cannot also summon stones; C and left/right remain active");cryonis=false;
   if(b>=3){
    for(int mode=0;mode<4;++mode){
     memset(sHoldFrames,0,sizeof(sHoldFrames));play.state.input[0].cur.button=masks[b];
     dpadEnabled=mode!=0;pacci=mode==1;riding=mode==2;gSaveContext.shipSaveContext.dpad.status[b-3]=mode==3?BTN_DISABLED:BTN_ENABLED;
     SpiritualStone_TickHold(&play,&player);assert(sHoldFrames[stone]==0);
    }
   }
  }
 }
 printf("PASS MM spiritual stones %s: all 3 identities x seven C/D inputs, ownership and Dpad mode/status guards\n",argv[1]);
}
'''
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--source-ref');ap.add_argument('--sanitize',action='store_true');ap.add_argument('--solo',action='store_true');a=ap.parse_args()
 def read(p):return subprocess.check_output(['git','show',a.source_ref+':'+p],cwd=ROOT,text=True) if a.source_ref else (ROOT/p).read_text()
 buttons=read('mm/mods/ext_buttons/ext_buttons.cpp');collect=read('mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_collect.c');stone=read('mm/mods/spiritual_stones/spiritual_stones.cpp')
 first=collect.index('    if (point >= 0x12 && point <= 0x14 && OotQuest_Has(point))');last=collect.index('    // Stone of Agony',first)
 code=PREFIX+claim_definition(read)+EXTRA+'\n'+buttons[buttons.index('u16 ExtButton_GetItem'):buttons.rindex('} // extern')]
 code+='\nvoid StoneSelect(PlayState* play,Input* input){s16 point=sOotQuestCursorPoint;\n'+collect[first:last]+'}\n'
 code+='\n'+function(stone,'StoneItemId')+'\n'+function(stone,'SpiritualStone_TickHold')+CHECKS
 with tempfile.TemporaryDirectory(prefix='nei-controls-mm-stones-') as td:
  p=Path(td);s=p/'test.cpp';b=p/'test';s.write_text(code)
  san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'] if a.sanitize else []
  subprocess.run(['c++','-std=c++20',*flags(),*([] if a.solo else ['-DCOMBO_BUILD']),*san,str(s),'-o',str(b)],check=True)
  results=[subprocess.run([str(b),case],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')).returncode for case in ('assign','hold')]
  return int(any(results))
if __name__=='__main__':raise SystemExit(main())
