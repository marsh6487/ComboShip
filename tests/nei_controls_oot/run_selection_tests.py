#!/usr/bin/env python3
"""Execute production pause selection when a selected cell becomes empty."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/oot_pause_inventory"))
from run_tests import function,PREFIX,CHECKS
PREFIX=PREFIX.replace("unsigned char Picto_IsOwned(void) { return 0; }", "unsigned char Picto_IsOwned(void) { return nei.pictoboxOwned; }")
ui=(ROOT/"soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c").read_text()
inventory=(ROOT/"soh/mods/extended_inventory.c").read_text()
scope=(ROOT/"soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_scope_PAL.c").read_text()
draw=function(ui,"KaleidoScope_DrawItemSelect")
locals_=draw[draw.index("    Input* input"):draw.index("    OPEN_DISPS")]
input_=draw[draw.index("    // Update extended inventory pagination timer"):draw.index("    gDPSetCombineLERP(OVERLAY_DISP")]
production="\n".join(function(inventory,n) for n in ("TravelTools_IsOwned","MagicMirror_IsOwned","ExtInv_Reset","ExtInv_Update","ExtInv_CanSwitchPage","ExtInv_SwitchPage","ExtInv_GetCurrentPage","ExtInv_GetMaxPages","ExtInv_IsCustomItemsEnabled","ExtInv_IsMmMasksEnabled","ExtInv_GetInventorySlot"))
production+="\n"+function(scope,"KaleidoScope_MoveCursorToSpecialPos")
production+="\nstatic void Step(PlayState* play) {\n"+locals_+input_+"\n}\n"
picto=(ROOT/"soh/mods/items/logic/picto_box.c").read_text()
# Keep the real mode predicates and real early name dispatch; unrelated custom
# names remain outside this focused fixture.
def picto_function(name):
    match=re.search(r"^u8 "+name+r"\([^;{}]*\)\s*\{",picto,re.M)
    cursor=match.end();depth=1
    while depth:
        depth+=(picto[cursor]=="{")-(picto[cursor]=="}");cursor+=1
    return picto[match.start():cursor]
production+="\nstatic u8 sPictoOnLens;\n"+picto_function("Picto_IsOnLensActive")+"\n"
production+=function(ui,"Picto_Cycle")+"\n"
start=inventory.index("void* ExtInv_GetCustomItemNameTex(")
production+="\n"+inventory[start:inventory.index("    // 2026-08-06",start)]+"return NULL;\n}\n"
PREFIX+="\nunsigned char Picto_IsOnLensActive(void);\nvoid Picto_SetOnLensActive(unsigned char on);\n"
production+="\nvoid Picto_SetOnLensActive(unsigned char on) {sPictoOnLens=on?1:0;}\n"
setup=CHECKS[:CHECKS.index("int main(")]
checks=r'''
int main(void) {
 for(int page=0;page<3;page++) {
  reset();sExtInvState.currentPage=page;
  play.pauseCtx.cursorItem[PAUSE_ITEM]=ITEM_WHIP;
  play.pauseCtx.cursorPoint[PAUSE_ITEM]=0;play.pauseCtx.cursorX[PAUSE_ITEM]=0;play.pauseCtx.cursorY[PAUSE_ITEM]=0;
  play.pauseCtx.stickRelY=40;play.state.input[0].press.button=BTN_CLEFT;
  Step(&play);
  assert(equipCalls==0 && "empty cell copied stale selected item onto C");
  assert(play.pauseCtx.cursorItem[PAUSE_ITEM]==PAUSE_ITEM_NONE && "empty cell retains stale title");
 }
 puts("PASS OoT selection: empty native/custom/mask cells never equip or name stale item");
 for(int lensOwned=0;lensOwned<2;lensOwned++) {
  reset();nei.pictoboxOwned=1;sPictoOnLens=lensOwned;
  gSaveContext.inventory.items[SLOT_LENS]=lensOwned?ITEM_LENS:ITEM_NONE;
  play.pauseCtx.cursorItem[PAUSE_ITEM]=ExtInv_GetSlotItem(SLOT_LENS);
  play.pauseCtx.cursorPoint[PAUSE_ITEM]=SLOT_LENS;play.pauseCtx.cursorX[PAUSE_ITEM]=SLOT_LENS%6;play.pauseCtx.cursorY[PAUSE_ITEM]=SLOT_LENS/6;
  Step(&play);
  assert(play.pauseCtx.cursorItem[PAUSE_ITEM]==ITEM_LENS);
  const char* name=ExtInv_GetCustomItemNameTex(play.pauseCtx.cursorItem[PAUSE_ITEM],0);
  assert(name && !strcmp(name,"__OTR__item_name_static/gItemNamePictographBoxENGTex"));
  if(lensOwned) {
   play.pauseCtx.namedItem=ITEM_LENS;Picto_Cycle(&play,1);
   assert(play.pauseCtx.namedItem==PAUSE_ITEM_NONE && !Picto_IsOnLensActive());
   assert(ExtInv_GetCustomItemNameTex(ITEM_LENS,0)==NULL);
  }
 }
 puts("PASS OoT Pictograph selection: camera-only synthetic Lens and selected camera mode use Pictograph title; cycling invalidates name");
}
'''
flags=["-std=gnu11","-w","-DF3DEX_GBI_2","-DCONTROLLERBUTTONS_T=uint32_t",'-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"','-DCVAR_PREFIX_SETTING="gSettings"',"-Werror=implicit-function-declaration"]
if "--sanitize" in sys.argv: flags += ["-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie"]
includes=["-I"+str(ROOT/p) for p in ("soh","soh/include","soh/include/PR","soh/src","soh/assets","soh/soh","soh/src/overlays/misc/ovl_kaleido_scope","libultraship/include","combo","combo/menu")]
with tempfile.TemporaryDirectory(prefix="oot-nei-selection-") as td:
 path=Path(td)/"test.c";path.write_text(PREFIX+production+setup+checks)
 for combo in (False,True):
  binary=Path(td)/("combo" if combo else "solo")
  subprocess.run([os.environ.get("CC","cc"),*flags,*( ["-DCOMBO_BUILD"] if combo else []),*includes,str(path),"-o",str(binary)],check=True)
  subprocess.run([str(binary)],check=True,env={**os.environ,"ASAN_OPTIONS":"detect_leaks=0","UBSAN_OPTIONS":"halt_on_error=1"})
