#!/usr/bin/env python3
"""Run real departure/arrival equip functions against native saves and JSON."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function

PREFIX = r'''
#include "global.h"
#include "mods/nei_save.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "FleetComboIds.h"
#include <nlohmann/json.hpp>
#include <cassert>
#include <cstdio>
#include <cstring>
SaveContext gSaveContext{};
static NeiSaveData nei{};
extern "C" NeiSaveData* Nei_Save() { return &nei; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return slot>=24 && slot<72 ? nei.ownedItems[slot-24] : ITEM_NONE; }
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { nei.ownedItems[slot-24]=item; }
'''

COMMON_CHECKS = r'''
static void own() {
    nei.ownedItems[3]=0xD0; // elemental-wand cell 27 in both games
    nei.ownedItems[23]=0x0223; // full Rod of Seasons ID, cell 47
}
int main(int argc, char** argv) {
    const char* mode=argc<2?"apply":argv[1];
    reset(); own();
    if (!strcmp(mode,"extract")) {
        setC(1,0xD0,WAND_SLOT); setC(2,0x0223,0xFF);
        setD(0,0x0223,ROD_D_SLOT);
        nlohmann::json sh; ExtractEquips(sh);
        assert(sh["cEquips"][0]==0xD0);
        assert(sh["cEquips"][1]==0x0223 && "departure must carry the full Rod of Seasons ID");
        assert(sh["dEquips"][0]==0x0223);
        puts("PASS departure: shared wand and full u16 Rod of Seasons C/D payloads");
    } else if (!strcmp(mode,"apply")) {
        nlohmann::json sh;
        sh["cEquips"]=nlohmann::json::array({0xD0,0x0223,0xFF});
        sh["dEquips"]=nlohmann::json::array({0x0223,0xD0,0xFF,0xFF});
        ApplyEquips(sh);
        assert(getC(1)==0xD0 && "arrival must resolve custom NEI ownership, not only native inventory");
        assert(slotC(1)==WAND_SLOT);
        assert(getC(2)==0x0223 && slotC(2)==0xFF);
        assert(getD(0)==0x0223 && slotD(0)==ROD_D_SLOT);
        assert(getD(1)==0xD0 && slotD(1)==WAND_SLOT);
        checkOtherForm();
        puts("PASS arrival: owned NEI cells, marker/shadow/slot coherence and shared MM buttons");
    } else {
        nlohmann::json sh;
        sh["cEquips"]=nlohmann::json::array({0xD0,0x0223,0xFF});
        // A quest-only EXT item has no shared inventory route: retain it.
        setC(1,0x0201,0xFF); setD(0,0x0202,0xFF);
        sh["dEquips"]=nlohmann::json::array({0x0223}); ApplyEquips(sh);
        assert(getC(1)==0x0201 && getD(0)==0x0202);
        reset();
        ApplyEquips(sh); assert(getC(1)==0xFF && getC(2)==0xFF && getD(0)==0xFF);
        own();
        sh["cEquips"]=nlohmann::json::array({-48,0x10223,"invalid"});
        sh["dEquips"]=nlohmann::json::array({0x10223,0x0201,-48,nullptr});
        ApplyEquips(sh);
        assert(getC(1)==0xFF && getC(2)==0xFF && getD(0)==0xFF && getD(1)==0xFF);
        // A native arrival clears old shadow metadata.
        ownBow(); setC(2,0x0223,0xFF);
        sh["cEquips"]=nlohmann::json::array({0xFF,0x03}); ApplyEquips(sh);
        assert(getC(2)==NATIVE_BOW && slotC(2)==NATIVE_BOW_SLOT && shadowC(2)==0);
        puts("PASS policy: local quest items, missing ownership, malformed IDs and native shadow clearing");
    }
    return 0;
}
'''

MM = r'''
#define MM_INV gSaveContext.save.saveInfo.inventory
#define MM_EQ gSaveContext.save.saveInfo.equips
static constexpr uint8_t kSlotOcarina=SLOT_OCARINA;
#define WAND_SLOT 75
#define ROD_D_SLOT 95
#define NATIVE_BOW ITEM_BOW
#define NATIVE_BOW_SLOT SLOT_BOW
static void reset() {
    gSaveContext={}; nei={};
    memset(MM_INV.items,0xFF,sizeof(MM_INV.items));
    memset(MM_EQ.buttonItems,0xFF,sizeof(MM_EQ.buttonItems));
    memset(MM_EQ.cButtonSlots,0xFF,sizeof(MM_EQ.cButtonSlots));
    memset(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems,0xFF,sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.dpadItems));
    for (auto& item:nei.ownedItems) item=ITEM_NONE;
    // Native MM C and D buttons are shared in form 0; only B is form-specific.
    gSaveContext.save.playerForm=PLAYER_FORM_DEKU;
}
static void setC(int b,uint16_t item,uint8_t slot) {
    if(item>=0x200) ExtButton_SetItem(0,b,item); else MM_EQ.buttonItems[0][b]=item;
    MM_EQ.cButtonSlots[0][b]=slot;
}
static void setD(int b,uint16_t item,uint8_t slot) { ExtButton_SetDpadItem(0,b,item); gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots[0][b]=slot; }
static uint16_t getC(int b) {return ExtButton_GetItem(0,b);}
static uint16_t getD(int b) {return ExtButton_GetDpadItem(0,b);}
static uint8_t slotC(int b) {return MM_EQ.cButtonSlots[0][b];}
static uint8_t slotD(int b) {return gSaveContext.save.shipSaveInfo.dpadEquips.dpadSlots[0][b];}
static uint16_t shadowC(int b) {return gSaveContext.save.shipSaveInfo.extButtons.items[0][b];}
static void ownBow() {MM_INV.items[SLOT_BOW]=ITEM_BOW;}
static void checkOtherForm() {assert(MM_EQ.buttonItems[PLAYER_FORM_DEKU][1]==0xFF);}
'''

OOT = r'''
#define WAND_SLOT 27
#define ROD_D_SLOT 0xFF
#define NATIVE_BOW ITEM_BOW
#define NATIVE_BOW_SLOT SLOT_BOW
static void reset() {
    gSaveContext={}; nei={};
    memset(gSaveContext.inventory.items,0xFF,sizeof(gSaveContext.inventory.items));
    memset(gSaveContext.equips.buttonItems,0xFF,sizeof(gSaveContext.equips.buttonItems));
    memset(gSaveContext.equips.cButtonSlots,0xFF,sizeof(gSaveContext.equips.cButtonSlots));
    for (auto& item:nei.ownedItems) item=ITEM_NONE;
}
static void setC(int b,uint16_t item,uint8_t slot) {
    if(item>=0x200) ExtButton_SetItem(b,item); else gSaveContext.equips.buttonItems[b]=item;
    gSaveContext.equips.cButtonSlots[b-1]=slot;
}
static void setD(int b,uint16_t item,uint8_t slot) {setC(b+4,item,slot);}
static uint16_t getC(int b) {return ExtButton_GetItem(b);}
static uint16_t getD(int b) {return ExtButton_GetItem(b+4);}
static uint8_t slotC(int b) {return gSaveContext.equips.cButtonSlots[b-1];}
static uint8_t slotD(int b) {return gSaveContext.equips.cButtonSlots[b+3];}
static uint16_t shadowC(int b) {return gSaveContext.ship.extButtons.items[b];}
static void ownBow() {gSaveContext.inventory.items[SLOT_BOW]=ITEM_BOW;}
static void checkOtherForm() {}
'''

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref")
    parser.add_argument("--sanitize",action="store_true")
    parser.add_argument("--json-include",type=Path,default=Path("/usr/include"))
    args=parser.parse_args()
    def read(path):
        if args.source_ref:
            return subprocess.check_output(["git","show",args.source_ref+":"+path],cwd=ROOT,text=True)
        return (ROOT/path).read_text()
    failures=0
    for host,fleet in (("mm","mm/2s2h/FleetShipCombo"),("soh","soh/soh/FleetShipCombo")):
        sync=read(fleet+"/FleetSync.cpp")
        buttons=read(host+"/mods/ext_buttons/ext_buttons.cpp")
        names=["ExtButton_GetItem","ExtButton_SetItem","ExtButton_ClearItem"]
        if host=="mm": names += ["ExtButton_GetDpadItem","ExtButton_SetDpadItem"]
        production="\n".join(function(buttons,n) for n in names)
        if host=="mm": production += "\n"+function(sync,"MmButtonCanonical")
        production += "\n"+"\n".join(function(sync,n) for n in ("ExtractEquips","FindInvSlot","ApplyOneButton","ApplyEquips"))
        code=PREFIX+(MM if host=="mm" else OOT)+production+COMMON_CHECKS
        flags=["-std=c++20","-w","-DF3DEX_GBI_2","-DCOMBO_BUILD","-DCONTROLLERBUTTONS_T=uint32_t"]
        flags += ["-I"+str(ROOT/p) for p in (host,host+"/include",host+"/include/PR",host+"/src",host+"/assets",host+"/2s2h","libultraship/include","combo","combo/menu",fleet)]
        flags += ["-I"+str(args.json_include)]
        if args.sanitize: flags += ["-fsanitize=address,undefined","-fno-omit-frame-pointer","-g"]
        with tempfile.TemporaryDirectory(prefix="combo-button-"+host+"-") as td:
            path=Path(td); source=path/"test.cpp"; binary=path/"test"; source.write_text(code)
            subprocess.run([os.environ.get("CXX","c++"),*flags,str(source),"-o",str(binary)],check=True)
            env=dict(os.environ,ASAN_OPTIONS="detect_leaks=0",UBSAN_OPTIONS="halt_on_error=1")
            for mode in ("extract","apply","policy"):
                result=subprocess.run([str(binary),mode],env=env)
                failures += bool(result.returncode)
                print(host,mode,"FAIL" if result.returncode else "PASS",flush=True)
    return int(bool(failures))

if __name__=="__main__":
    raise SystemExit(main())
