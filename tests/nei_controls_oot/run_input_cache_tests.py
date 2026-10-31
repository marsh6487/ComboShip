#!/usr/bin/env python3
"""Check duplicate native-width NEI bindings against the production input cache."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/oot_pause_inventory"))
from run_tests import function
source=(ROOT/"soh/mods/items/helpers/equip_helper.c").read_text()
prefix=r'''
#include "global.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/extended_inventory.h"
#include <cassert>
#include <cstring>
#include <cstdio>
SaveContext gSaveContext{};
static int dpad=1, claimed;
extern "C" int32_t CVarGetInteger(const char*,int32_t) {return dpad;}
extern "C" u8 ItemInput_ButtonIsClaimed(u16 mask) {return claimed && (mask&(BTN_DUP|BTN_DDOWN|BTN_DLEFT|BTN_DRIGHT));}
extern "C" uint8_t Sw97_IsBowItem(uint16_t) {return 0;}
extern "C" uint8_t Sw97_EffectiveElement(uint8_t) {return 0;}
'''
types=source[source.index("typedef struct {"):source.index("static void EquipCache_Update")]
code=prefix+types+"\n"+function(source,"EquipCache_Update")+"\n"+function(source,"ItemInput_GetEquippedButton")+r'''
int main() {
 PlayState play{};play.gameplayFrames=1;
 memset(gSaveContext.equips.buttonItems,ITEM_NONE,sizeof(gSaveContext.equips.buttonItems));
 gSaveContext.equips.buttonItems[1]=ITEM_WHIP;gSaveContext.equips.cButtonSlots[0]=SLOT_WHIP;
 gSaveContext.equips.buttonItems[4]=ITEM_WHIP;gSaveContext.equips.cButtonSlots[3]=SLOT_WHIP;
 assert(ItemInput_GetEquippedButton(ITEM_WHIP,&play)==(BTN_CLEFT|BTN_DUP) && "D-pad duplicate hides working C binding");
 ++play.gameplayFrames;claimed=1;
 assert(ItemInput_GetEquippedButton(ITEM_WHIP,&play)==BTN_CLEFT && "claimed D-pad must not suppress independent C binding");
 ++play.gameplayFrames;claimed=0;dpad=0;
 assert(ItemInput_GetEquippedButton(ITEM_WHIP,&play)==BTN_CLEFT);
 ++play.gameplayFrames;dpad=1;gSaveContext.buttonStatus[1]=BTN_DISABLED;
 assert(ItemInput_GetEquippedButton(ITEM_WHIP,&play)==BTN_DUP && "disabled C binding hid valid D input");
 ++play.gameplayFrames;gSaveContext.buttonStatus[5]=BTN_DISABLED;
 assert(ItemInput_GetEquippedButton(ITEM_WHIP,&play)==0 && "disabled gameplay buttons leaked NEI input");
 puts("PASS OoT native-width NEI input cache: C/D union, claimed/disabled D-pad and gameplay button statuses");
}
'''
includes=["-I"+str(ROOT/p) for p in ("soh","soh/include","soh/include/PR","soh/src","soh/assets","soh/soh","libultraship/include","combo","combo/menu")]
flags=["-std=c++20","-w","-DF3DEX_GBI_2","-DCONTROLLERBUTTONS_T=uint32_t"]
if "--sanitize" in sys.argv: flags += ["-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie"]
with tempfile.TemporaryDirectory(prefix="oot-nei-input-cache-") as td:
 path=Path(td)/"test.cpp";path.write_text(code);binary=Path(td)/"test"
 subprocess.run([os.environ.get("CXX","c++"),*flags,*includes,str(path),"-o",str(binary)],check=True)
 subprocess.run([str(binary)],check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0","UBSAN_OPTIONS":"halt_on_error=1"})
