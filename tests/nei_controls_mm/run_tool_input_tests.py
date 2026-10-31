#!/usr/bin/env python3
"""Execute all MM full-ID tool input-mask readers using real accessor storage."""
import argparse,os,re,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from run_tests import PREFIX,flags,function,claim_definition
EXTRA=r'''



'''
CHECKS=r'''
int main(int argc,char**argv){
 const u16 masks[]={BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT,BTN_DRIGHT,BTN_DLEFT,BTN_DDOWN,BTN_DUP};
 const u16 items[]={EXT_ITEM_SHEIKAH_SLATE,EXT_ITEM_PHANTOM_HOURGLASS,EXT_ITEM_ROD_OF_SEASONS};
 u16(*readers[])()={Slate_EquippedButtonMask,Hourglass_EquippedButtonMask,Seasons_EquippedButtonMask};
 int t=atoi(argv[1]);
 for(int b=0;b<7;++b){
  Reset();pacci=riding=false;
  if(b<3)ExtButton_SetItem(0,b+1,items[t]);else ExtButton_SetDpadItem(0,b-3,items[t]);
  assert(readers[t]()==masks[b]&&"full-ID tool reads directly assigned C and Dpad bindings");
  cryonis=true;assert(readers[t]()==((b==5||b==6)?0:masks[b])&&"Cryonis claims only D up/down, preserving C and D left/right");cryonis=false;
  if(b>=3){
   dpadEnabled=0;assert(readers[t]()==0);dpadEnabled=1;
   pacci=true;assert(readers[t]()==0);pacci=false;
   gSaveContext.shipSaveContext.dpad.status[b-3]=BTN_DISABLED;assert(readers[t]()==0);
  }
 }
 printf("PASS MM full-ID tool %d: all C/D directions, Dpad-off/status/Ultrahand guards\n",t);
}
'''
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--source-ref');ap.add_argument('--sanitize',action='store_true');ap.add_argument('--solo',action='store_true');a=ap.parse_args()
 def read(p):return subprocess.check_output(['git','show',a.source_ref+':'+p],cwd=ROOT,text=True) if a.source_ref else (ROOT/p).read_text()
 buttons=read('mm/mods/ext_buttons/ext_buttons.cpp');code=PREFIX+claim_definition(read)+'\n#include <cstdlib>\n'+EXTRA+'\n'+buttons[buttons.index('u16 ExtButton_GetItem'):buttons.rindex('} // extern')]
 for path,name in [('item_sheikah_slate.c','Slate_EquippedButtonMask'),('item_phantom_hourglass.c','Hourglass_EquippedButtonMask'),('item_rod_of_seasons.c','Seasons_EquippedButtonMask')]:
  src=read('mm/mods/items/logic/'+path)
  if path=='item_phantom_hourglass.c':
   code+='\n'+'\n'.join(re.findall(r'^#define HOURGLASS_(?:BUTTON_ITEM|LAST_BUTTON)[^\n]*',src,re.M))
   m=re.search(r'static const u16 sHourglassButtonMasks\[8\] = \{.*?\};',src,re.S)
   if m:code+='\n'+m[0]
  code+='\n'+function(src,name)
 code+=CHECKS
 with tempfile.TemporaryDirectory(prefix='nei-controls-mm-tools-') as td:
  p=Path(td);s=p/'test.cpp';b=p/'test';s.write_text(code)
  san=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie'] if a.sanitize else []
  subprocess.run(['c++','-std=c++20',*flags(),*([] if a.solo else ['-DCOMBO_BUILD']),*san,str(s),'-o',str(b)],check=True)
  results=[subprocess.run([str(b),str(t)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',UBSAN_OPTIONS='halt_on_error=1')).returncode for t in range(3)]
  return int(any(results))
if __name__=='__main__':raise SystemExit(main())
