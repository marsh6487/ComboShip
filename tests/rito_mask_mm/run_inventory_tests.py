#!/usr/bin/env python3
"""Execute the native inventory mapping and cursor against an independently owned Rito mask."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('fixture', ROOT / 'tests/mm_pause_inventory/run_tests.py')
fixture = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fixture)

inv = (ROOT / 'mm/mods/extended_inventory.c').read_text()
ui = (ROOT / 'mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c').read_text()
start = inv.index('static const uint8_t sOotPage0Map[24]')
mapping = inv[start:inv.index('\n};', start) + 3]
prefix = fixture.PREFIX.replace('void ItemGrantAudit_Begin', '#ifdef COMBO_BUILD\nvoid ItemGrantAudit_Begin').replace('void ItemGrantAudit_End(void) {}', 'void ItemGrantAudit_End(void) {}\n#endif')
prefix = prefix.replace('uint8_t ExtInv_GetOotSlotItem(int slot) { return ITEM_NONE; }', 'uint8_t ExtInv_GetOotSlotItem(int slot) { return slot == VSLOT_OOT_MASKS && OotMask_OwnedCount() ? OotMask_CellItem() : ITEM_NONE; }')
code = prefix + mapping + r'''
+#define OOT_MASK_COUNT 9
+#define CUSTOM_FORM_RITO 4
+int CustomForms_ActiveForm(){return nei.ritoMaskOwned && nei.activeCustomForm == 5 ? 4 : -1;}
+void CustomForms_Deactivate(){nei.activeCustomForm=0;}
+u16 ExtButton_GetItem(s32 f,s32 b){return BUTTON_ITEM_EQUIP(f,b)==ITEM_EXT_BUTTON?EXT_BUTTON_ITEM(f,b):BUTTON_ITEM_EQUIP(f,b);}
+void ExtButton_ClearItem(s32 f,s32 b){BUTTON_ITEM_EQUIP(f,b)=ITEM_NONE;EXT_BUTTON_ITEM(f,b)=0;}
+u16 ExtButton_GetDpadItem(s32 f,s32 b){return DPAD_BUTTON_ITEM_EQUIP(f,b)==ITEM_EXT_BUTTON?gSaveContext.save.shipSaveInfo.dpadEquips.extItems[f][b]:DPAD_BUTTON_ITEM_EQUIP(f,b);}
+void ExtButton_SetDpadItem(s32 f,s32 b,u16 item){DPAD_BUTTON_ITEM_EQUIP(f,b)=item>=0x200?ITEM_EXT_BUTTON:item;gSaveContext.save.shipSaveInfo.dpadEquips.extItems[f][b]=item>=0x200?item:0;}
+'''.replace('\n+','\n')
for name in ('RitoMask_IsOwned', 'RitoMask_UsesCornerWheel', 'RitoMask_BremenCellItem', 'RitoMask_Grant',
             'OotMask_IsOwnedIndex', 'OotMask_OwnedCount', 'OotMask_OwnedAt', 'OotMask_CursorIndex',
             'OotMask_NeighborIndex', 'OotMask_CursorStep', 'OotMask_CellItem',
             'TravelTools_IsOwned', 'MagicMirror_IsOwned', 'ExtInv_Reset', 'ExtInv_Update',
             'ExtInv_CanSwitchPage', 'ExtInv_SwitchPage', 'ExtInv_GetCurrentPage', 'ExtInv_GetMaxPages',
             'ExtInv_IsCustomItemsEnabled', 'ExtInv_IsMmMasksEnabled', 'ExtInv_GetInventorySlot', 'ExtInv_ClampPage', 'RitoMask_Remove'):
    if name == 'RitoMask_IsOwned' and 'uint8_t RitoMask_IsOwned(' not in inv:
        continue
    code += fixture.function(inv, name) + '\n'
code += fixture.function(ui, 'KaleidoScope_UpdateItemCursor')
code += fixture.CHECKS[:fixture.CHECKS.index('int main(')]
code += r'''
int main() {
    assert(EXT_ITEM_RITO_MASK != EXT_ITEM_MAGIC_MIRROR);
    for (int combined = 0; combined < 2; ++combined) {
        reset(); gComboGameMode = combined ? COMBO_GAME_BOTH : COMBO_GAME_MM;
        RitoMask_Grant();
        assert(ExtInv_GetMaxPages() == 3 && "Rito must not add any inventory page");
        const int slot = combined ? VSLOT_OOT_MASKS : 55;
        assert(ExtInv_GetSlotItem(slot) == 0x225 && "mask alone must be selectable, full-width");
        assert(gSaveContext.save.saveInfo.inventory.items[SLOT_MASK_BREMEN] == ITEM_NONE);
        assert(nei.ootSpellsOwned == 0 && nei.ootMasksOwned == 0);
        sExtInvState.currentPage = combined ? 0 : 2;
        play.pauseCtx.cursorPoint[PAUSE_ITEM] = play.pauseCtx.cursorSlot[PAUSE_ITEM] = combined ? 23 : 7;
        play.pauseCtx.cursorXIndex[PAUSE_ITEM] = combined ? 5 : 1;
        play.pauseCtx.cursorYIndex[PAUSE_ITEM] = combined ? 3 : 1;
        play.state.input[0].press.button = BTN_CLEFT;
        KaleidoScope_UpdateItemCursor(&play);
        assert(play.pauseCtx.equipTargetItem == 0x225);
        // Every native mask remains in its original store. Solo's selector only changes display.
        for (int i = 0; i < 24; ++i) gSaveContext.save.saveInfo.inventory.items[24+i] = ITEM_MASK_DEKU+i;
        gSaveContext.save.saveInfo.inventory.items[SLOT_MASK_BREMEN] = ITEM_MASK_BREMEN;
        if (!combined) {
            nei.ritoMaskSelected = 0; assert(ExtInv_GetSlotItem(55) == ITEM_MASK_BREMEN);
            nei.ritoMaskSelected = 1; assert(ExtInv_GetSlotItem(55) == 0x225);
            assert(gSaveContext.save.saveInfo.inventory.items[SLOT_MASK_BREMEN] == ITEM_MASK_BREMEN);
        } else {
            nei.ootMasksOwned = 1; nei.ootMaskCursor = 0;
            assert(ExtInv_GetSlotItem(VSLOT_OOT_MASKS) == ITEM_OOT_MASK_PLACEHOLDER);
            OotMask_CursorStep(1); assert(OotMask_CursorIndex() == 8);
            assert(ExtInv_GetSlotItem(VSLOT_OOT_MASKS) == 0x225);
            OotMask_CursorStep(1); assert(OotMask_CursorIndex() == 0);
        }
        nei.activeCustomForm=5;
        BUTTON_ITEM_EQUIP(0,EQUIP_SLOT_C_LEFT)=ITEM_EXT_BUTTON;
        EXT_BUTTON_ITEM(0,EQUIP_SLOT_C_LEFT)=EXT_ITEM_RITO_MASK;
        ExtButton_SetDpadItem(0,EQUIP_SLOT_D_RIGHT,EXT_ITEM_RITO_MASK);
        RitoMask_Remove();
        assert(!RitoMask_IsOwned() && nei.activeCustomForm==0);
        assert(ExtButton_GetItem(0,EQUIP_SLOT_C_LEFT)==ITEM_NONE && ExtButton_GetDpadItem(0,EQUIP_SLOT_D_RIGHT)==ITEM_NONE);
        RitoMask_Grant(); assert(nei.activeCustomForm==0 && "regrant must not restore a removed transformation");
        for (int i = 0; i < 24; ++i) assert(gSaveContext.save.saveInfo.inventory.items[24+i] == ITEM_MASK_DEKU+i || i == SLOT_MASK_BREMEN-24);
    }
    puts("PASS MM mode placement, independent grants, u16 cursor equip, Bremen coexistence and corner wheel cycling");
}
'''
with tempfile.TemporaryDirectory(prefix='rito-mm-inventory-') as td:
    source = Path(td) / 'test.cpp'; binary = Path(td) / 'test'
    source.write_text(code)
    includes = ['-I' + str(ROOT / p) for p in ('mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h','libultraship/include','combo','combo/menu')]
    subprocess.run(['c++','-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t',*includes,str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
