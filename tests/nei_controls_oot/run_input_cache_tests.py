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
sys.path.insert(0,str(ROOT/"scripts/diagnostics"))
from run_time_pedestal_tests import functions
source=(ROOT/"soh/mods/items/helpers/equip_helper.c").read_text()
buttons=(ROOT/"soh/mods/ext_buttons/ext_buttons.cpp").read_text()
prefix=r'''
#include "global.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/extended_inventory.h"
#include "gameplay/OcarinaItemInput.h"
#include <cassert>
#include <cstring>
#include <cstdio>
SaveContext gSaveContext{};
PlayState* gPlayState = nullptr;
static int dpad=1, claimed;
extern "C" int32_t CVarGetInteger(const char*,int32_t) {return dpad;}
extern "C" u8 Pacci_UltrahandModeActive() {return claimed;}
extern "C" u8 MasterCycle_IsRiding() {return 0;}
extern "C" u8 Cryonis_ModeActive() {return 0;}
extern "C" uint8_t Sw97_IsBowItem(uint16_t) {return 0;}
extern "C" uint8_t Sw97_EffectiveElement(uint8_t) {return 0;}
'''
types=source[source.index("typedef struct {"):source.index("static void EquipCache_Update")]
cache=functions(source)
code=prefix+"\n"+"\n".join(function(buttons,name) for name in (
 "ExtButton_GetItem","ExtButton_SetItem","ExtButton_GetInputMask"))+"\n"+types+"\n"+"\n".join(cache[name] for name in (
 "ItemInput_ButtonIsClaimed","EquipCache_Update","ItemInput_GetEquippedButton",
 "ItemInput_Update","ItemInput_CheckOtherButtons"))+r'''
int main() {
 static PlayState play{}; Player player{}; play.gameplayFrames=1; gPlayState=&play;
 memset(gSaveContext.equips.buttonItems,ITEM_NONE,sizeof(gSaveContext.equips.buttonItems));
 gSaveContext.equips.buttonItems[1]=ITEM_WHIP;gSaveContext.equips.cButtonSlots[0]=SLOT_WHIP;
 gSaveContext.equips.buttonItems[4]=ITEM_WHIP;gSaveContext.equips.cButtonSlots[3]=SLOT_WHIP;
 assert(ItemInput_GetEquippedButton(ITEM_WHIP,&play)==(BTN_CLEFT|BTN_DUP) && "D-pad duplicate hides working C binding");
 play.msgCtx.msgMode=MSGMODE_OCARINA_PLAYING;
 play.state.input[0].cur.button=play.state.input[0].press.button=BTN_DUP;
 const auto note=play.state.input[0];
 ItemInputState input{};
 ItemInput_Update(&input,ITEM_WHIP,&player,&play);
 assert(input.equippedButton==BTN_CLEFT&&!input.isPressed&&!input.isHeld&&!input.otherButtonPressed);
 assert(!memcmp(&note,&play.state.input[0],sizeof(Input)));
 play.msgCtx.msgMode=MSGMODE_NONE;
 ItemInput_Update(&input,ITEM_WHIP,&player,&play);
 assert(input.equippedButton==(BTN_CLEFT|BTN_DUP)&&input.isPressed);
 claimed=1; // A claimant can begin after this frame's cache is populated.
 assert(ItemInput_GetEquippedButton(ITEM_WHIP,&play)==BTN_CLEFT && "claimed D-pad must not suppress independent C binding");
 ++play.gameplayFrames;claimed=0;dpad=0;
 assert(ItemInput_GetEquippedButton(ITEM_WHIP,&play)==BTN_CLEFT);
 ++play.gameplayFrames;dpad=1;gSaveContext.buttonStatus[1]=BTN_DISABLED;
 assert(ItemInput_GetEquippedButton(ITEM_WHIP,&play)==BTN_DUP && "disabled C binding hid valid D input");
 ++play.gameplayFrames;gSaveContext.buttonStatus[5]=BTN_DISABLED;
 assert(ItemInput_GetEquippedButton(ITEM_WHIP,&play)==0 && "disabled gameplay buttons leaked NEI input");
 gSaveContext.buttonStatus[5]=BTN_ENABLED;
 for (int slot=4;slot<8;++slot) {
  const u16 direction=sButtonMasks[slot];
  ExtButton_SetItem(slot,EXT_ITEM_PHANTOM_HOURGLASS);
  assert(ExtButton_GetInputMask(EXT_ITEM_PHANTOM_HOURGLASS)==direction);
  play.state.input[0].press.button=play.state.input[0].cur.button=direction;
  play.msgCtx.msgMode=MSGMODE_OCARINA_PLAYING;
  assert(ExtButton_GetInputMask(EXT_ITEM_PHANTOM_HOURGLASS)==0);
  assert(play.state.input[0].press.button==direction&&play.state.input[0].cur.button==direction);
  play.msgCtx.msgMode=MSGMODE_NONE;
  assert(ExtButton_GetInputMask(EXT_ITEM_PHANTOM_HOURGLASS)==direction);
  gSaveContext.equips.buttonItems[slot]=ITEM_NONE;
 }
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
