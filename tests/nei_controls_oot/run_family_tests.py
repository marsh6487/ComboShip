#!/usr/bin/env python3
"""Run real custom-item dispatch, in-water masks and spiritual-stone holds."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/mm_equipment_pause"))
from run_tests import function
common=(ROOT/"soh/mods/items/custom_items_common.c").read_text()
stones=(ROOT/"soh/mods/spiritual_stones/spiritual_stones.cpp").read_text()
remains=(ROOT/"soh/mods/boss_remains/boss_remains.cpp").read_text()
reader=function(remains,"BossRemains_TickInput")
reader=reader[reader.index("    u16 press ="):]
buttons=(ROOT/"soh/mods/ext_buttons/ext_buttons.cpp").read_text()
sw97=(ROOT/"soh/expansions/sw97/player/sw97_player_behavior.inc.c").read_text()
pikachu=(ROOT/"soh/mods/transformation_masks/pikachu_form.cpp").read_text()
start=pikachu.index("        static const u16 cBtns[]",pikachu.index("// ── SYSTEM 1 (pokeball)"))
pika=pikachu[start:pikachu.index("    }\n\n    // ── Smash flick",start)]
water=common[common.index("    if (p->stateFlags1 & PLAYER_STATE1_IN_WATER) {",common.index("// Zora Mask: allow use in water")):common.index("    // Walk every equip slot",common.index("// Zora Mask: allow use in water"))]
start=common.index("    // Walk every equip slot")
dispatch=common[start:common.index("    // Ultrahand assemblies",start)]
prefix=r'''
#include "global.h"
#include "mods/items/custom_items.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/extended_inventory.h"
#include "mods/spiritual_stones/spiritual_stones.h"
#include <cassert>
#include <cstdio>
#include <cstring>
SaveContext gSaveContext{};
CustomItemState gCustomItemState{};
static int dpad=1,claimed,dispatchCalls,maskCalls,summons,prompts;
static int formUses;
static s16 sWornRemains=ITEM_NONE;
bool DpadEquipsEnabled() {return dpad;}
void BossRemains_StopOdolwaSfx() {}
extern "C" void Player_PlaySfx(Actor*,u16) {}
extern "C" int32_t CVarGetInteger(const char* key,int32_t fallback) {return !strcmp(key,"gEnhancements.DpadEquips")?dpad:fallback;}
extern "C" u8 ItemInput_ButtonIsClaimed(u16 mask) {return claimed && (mask&(BTN_DUP|BTN_DDOWN|BTN_DLEFT|BTN_DRIGHT));}
extern "C" u8 Sw97_IsBowItem(u16) {return 0;}
extern "C" u8 Sw97_EffectiveElement(u8) {return 0;}
extern "C" void TransformMasks_HandleMaskUse(PlayState*,Player*,s32 item) {assert(item==ITEM_MM_MASK_ZORA);++maskCalls;}
extern "C" u8 TransformMasks_HandleFormItemUse(PlayState*,Player*,s32 item) {assert(item==ITEM_WHIP);++formUses;return 1;}
static int sPendingWarpStone=-1;
static s32 sHoldFrames[SPIRITUAL_STONE_COUNT]{};
struct {struct {s32 entranceId=-1;} warp[SPIRITUAL_STONE_COUNT];} gState;
bool StonesEnabled() {return true;}
void ExecuteWarp(int) {}
void SummonStatueHere(PlayState*,int) {++summons;}
void OpenWarpPrompt(PlayState*,int) {++prompts;}
extern "C" void Audio_PlayActorSound2(Actor*,u16) {}
#define SW97_MEDALLIONS_ENABLED() 1
'''
handlers=list(dict.fromkeys(re.findall(r"\b(Handle_\w+)\(p, play\)",dispatch)))
dispatch_items=list(dict.fromkeys(re.findall(r"case (ITEM_\w+):",dispatch)))+["ITEM_NET"]
production="\n".join('extern "C" void '+name+'(Player*,PlayState*) {++dispatchCalls;}' for name in handlers)
production+="\nstatic void Dispatch(Player* p,PlayState* play) {\n"+dispatch+"\n}\n"
production+="\nstatic void Water(Player* p,PlayState* play) {\n"+water+"\n}\n"
production+="\n"+"\n".join(function(buttons,name) for name in ("ExtButton_GetItem","ExtButton_SetItem","ExtButton_ClearItem","ExtButton_GetInputMask"))
production+="\n"+function(stones,"StoneItemId")
for name in ("StoneBoundCButtonSlot","ButtonMaskForSlot","StoneBoundButtonMask"):
    if re.search(r"\b"+name+r"\([^;{}]*\)\s*\{",stones): production+="\n"+function(stones,name)
production+="\n"+function(stones,"SpiritualStone_TickHold")
production+="\n"+"\n".join(re.findall(r"^#define SHADOW_EXCHANGE_[^\n]*",sw97,re.M))+"\n"+function(sw97,"Sw97_TickShadowExchange")
production+="\nstatic void PikaItems(PlayState* play,Player* player) {\n"+pika+"\nadvance_anim: return;\n}\n"
production+="\n"+function(remains,"BossRemains_ItemIndex")+"\nstatic void RemainsInput(PlayState* play,Player* player) {\n"+reader
checks=r'''
static const u16 masks[8]={BTN_B,BTN_CLEFT,BTN_CDOWN,BTN_CRIGHT,BTN_DUP,BTN_DDOWN,BTN_DLEFT,BTN_DRIGHT};
static void reset() {
 gSaveContext={};memset(gSaveContext.equips.buttonItems,ITEM_NONE,sizeof(gSaveContext.equips.buttonItems));
 memset(gSaveContext.equips.cButtonSlots,SLOT_NONE,sizeof(gSaveContext.equips.cButtonSlots));
 claimed=dispatchCalls=maskCalls=summons=prompts=0;dpad=1;memset(sHoldFrames,0,sizeof(sHoldFrames));
}
int main(int argc,char** argv) {
 PlayState play{};Player player{};
 if(argc>1 && !strcmp(argv[1],"remains")) {
  for(u16 item: {u16(ITEM_MM_REMAINS_ODOLWA),u16(ITEM_MM_REMAINS_GOHT),u16(ITEM_MM_REMAINS_GYORG),u16(ITEM_MM_REMAINS_TWINMOLD)})for(int button=1;button<8;button++) {
   reset();sWornRemains=ITEM_NONE;gSaveContext.equips.buttonItems[button]=item;play.state.input[0].press.button=masks[button];
   RemainsInput(&play,&player);assert(sWornRemains==item);
   RemainsInput(&play,&player);assert(sWornRemains==ITEM_NONE);
   if(button>=4) {claimed=1;RemainsInput(&play,&player);assert(sWornRemains==ITEM_NONE && "claimed D-pad toggles boss remains");}
   claimed=0;gSaveContext.buttonStatus[button<4?button:button+1]=BTN_DISABLED;
   RemainsInput(&play,&player);assert(sWornRemains==ITEM_NONE && "disabled gameplay button toggles boss remains");
  }
  puts("PASS OoT boss remains: four wear toggles across seven buttons, claimed D-pad and gameplay statuses");return 0;
 }
 if(argc>1 && !strcmp(argv[1],"pika")) {
  for(int button=1;button<8;button++) {
   reset();formUses=0;gSaveContext.equips.buttonItems[button]=ITEM_WHIP;play.state.input[0].press.button=masks[button];
   PikaItems(&play,&player);assert(formUses==1 && "Pokeball Pikachu form dispatch ignores D-pad binding");
   if(button>=4) {claimed=1;PikaItems(&play,&player);assert(formUses==1);}
  }
  puts("PASS OoT Pokeball Pikachu native-item fallback: C/D form dispatch and claimed D-pad");return 0;
 }
 if(argc>1 && !strcmp(argv[1],"shadow")) {
  for(int button=4;button<8;button++) {
   reset();play.state.input[0].cur.button=0;Sw97_TickShadowExchange(&play,&player);
   gSaveContext.equips.buttonItems[1]=gSaveContext.equips.buttonItems[button]=ITEM_MEDALLION_SHADOW;
   gSaveContext.health=10*0x10;gSaveContext.magic=0;gSaveContext.magicCapacity=48;
   play.state.input[0].cur.button=masks[button];
   for(int frame=0;frame<SHADOW_EXCHANGE_HOLD_FRAMES;frame++)Sw97_TickShadowExchange(&play,&player);
   assert(gSaveContext.magic==SHADOW_EXCHANGE_MAGIC_GAIN && "Shadow medallion ignores independent D-pad hold");
  }
  puts("PASS OoT Shadow medallion: heart/magic exchange on all four independent D-pad binds");return 0;
 }
 if(argc>1 && !strcmp(argv[1],"dispatch")) {
  for(u16 item: {/* DISPATCH_ITEMS */}) {
   reset();gSaveContext.equips.buttonItems[1]=gSaveContext.equips.buttonItems[4]=item;
   Dispatch(&player,&play);assert(dispatchCalls==1 && "independent duplicate bindings tick custom handler twice");
  }
  puts("PASS OoT custom-item dispatcher: all native-width NEI handlers execute once for duplicate C/D bindings");return 0;
 }
 if(argc>1 && !strcmp(argv[1],"water")) {
  for(int button=1;button<8;button++) {
   reset();player.stateFlags1=PLAYER_STATE1_IN_WATER;
   gSaveContext.equips.buttonItems[button]=ITEM_MM_MASK_ZORA;play.state.input[0].press.button=masks[button];
   Water(&player,&play);assert(maskCalls==1 && "Zora water fallback reads wrong D-pad slot");
   if(button>=4) {claimed=1;Water(&player,&play);assert(maskCalls==1);}
  }
  puts("PASS OoT in-water Zora mask: all seven slots, D-pad claim, no out-of-bounds");return 0;
 }
 for(int stone=0;stone<3;stone++) for(int button=4;button<8;button++) {
  reset();gSaveContext.equips.buttonItems[1]=gSaveContext.equips.buttonItems[button]=StoneItemId(stone);
  play.state.input[0].cur.button=masks[button];
  for(int frame=0;frame<SPIRITUAL_STONE_SUMMON_HOLD_FRAMES;frame++) SpiritualStone_TickHold(&play,&player);
  assert(summons==1 && "C-bound stone hides independently bound D-pad hold");
  reset();gSaveContext.equips.buttonItems[button]=StoneItemId(stone);claimed=1;
  for(int frame=0;frame<SPIRITUAL_STONE_SUMMON_HOLD_FRAMES;frame++) SpiritualStone_TickHold(&play,&player);
  assert(summons==0 && "claimed D-pad summons spiritual stone");
 }
 puts("PASS OoT spiritual stones: three duplicate C/D holds, claimed D-pad suppression");
}
'''
checks=checks.replace("/* DISPATCH_ITEMS */", ",".join("u16("+item+")" for item in dispatch_items))
flags=["-std=c++20","-w","-DF3DEX_GBI_2","-DCONTROLLERBUTTONS_T=uint32_t",'-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"']
if "--sanitize" in sys.argv: flags += ["-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie"]
includes=["-I"+str(ROOT/p) for p in ("soh","soh/include","soh/include/PR","soh/src","soh/assets","soh/soh","libultraship/include","combo","combo/menu")]
failed=False
with tempfile.TemporaryDirectory(prefix="oot-nei-family-") as td:
 path=Path(td)/"test.cpp";path.write_text(prefix+production+checks)
 for combo in (False,True):
  binary=Path(td)/("combo" if combo else "solo")
  subprocess.run([os.environ.get("CXX","c++"),*flags,*( ["-DCOMBO_BUILD"] if combo else []),*includes,str(path),"-o",str(binary)],check=True)
  for mode in ("dispatch","water","stones","shadow","pika","remains"):
   result=subprocess.run([str(binary),mode],env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0","UBSAN_OPTIONS":"halt_on_error=1"})
   failed|=result.returncode!=0
sys.exit(int(failed))
