/**
 * ext_buttons.cpp — Extended-button infrastructure accessor API (see ext_buttons.h).
 *
 * Globbed and compiled as its own translation unit by mm/CMakeLists.txt (mods/*.cpp). Exports
 * C-linkage helpers so the u8 buttonItems marker (ITEM_EXT_BUTTON) and the parallel u16
 * extButtons array stay in lockstep.
 */
#include "ext_buttons.h"
#include "mods/extended_equipment.h"
#include "mods/extended_inventory.h"
#include "mods/items/helpers/equip_helper.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#ifdef COMBO_BUILD
#include "gameplay/MagicMirrorItem.h"
#endif

extern "C" {

u16 ExtButton_GetItem(s32 form, s32 btn) {
    u8 raw = BUTTON_ITEM_EQUIP(form, btn);
    if (raw == ITEM_EXT_BUTTON) {
        return EXT_BUTTON_ITEM(form, btn);
    }
    return (u16)raw;
}

void ExtButton_SetItem(s32 form, s32 btn, u16 extId) {
    BUTTON_ITEM_EQUIP(form, btn) = ITEM_EXT_BUTTON;
    EXT_BUTTON_ITEM(form, btn) = extId;
}

void ExtButton_ClearItem(s32 form, s32 btn) {
    BUTTON_ITEM_EQUIP(form, btn) = ITEM_NONE;
    EXT_BUTTON_ITEM(form, btn) = 0;
}

u16 ExtButton_GetDpadItem(s32 form, s32 btn) {
    u8 raw = DPAD_BUTTON_ITEM_EQUIP(form, btn);
    auto& equips = gSaveContext.save.shipSaveInfo.dpadEquips;
    if (raw == ITEM_EXT_BUTTON && equips.extItems[form][btn] >= 0x0200) {
        return equips.extItems[form][btn];
    }
    // Old saves truncated EXT ids but retained the unique page-2 slot band (72..95).
    // Repair only a matching owned slot: a real mushroom's bottle slot never qualifies.
    const u8 slot = DPAD_SLOT_EQUIP(form, btn);
    if (slot >= 72 && slot < 96) {
        const u16 owned = gSaveContext.save.shipSaveInfo.nei.ownedItems[slot - 72];
        if (owned >= 0x0200 && (raw == ITEM_EXT_BUTTON || raw == (u8)owned)) {
            DPAD_BUTTON_ITEM_EQUIP(form, btn) = ITEM_EXT_BUTTON;
            equips.extItems[form][btn] = owned;
            return owned;
        }
    }
    return raw == ITEM_EXT_BUTTON ? ITEM_NONE : raw;
}

void ExtButton_SetDpadItem(s32 form, s32 btn, u16 item) {
    DPAD_BUTTON_ITEM_EQUIP(form, btn) = item >= 0x0200 ? ITEM_EXT_BUTTON : (u8)item;
    gSaveContext.save.shipSaveInfo.dpadEquips.extItems[form][btn] = item >= 0x0200 ? item : 0;
}

u16 ExtButton_GetInputMask(u16 item) {
    static const u16 cButtons[] = { BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT };
    static const u16 dButtons[] = { BTN_DRIGHT, BTN_DLEFT, BTN_DDOWN, BTN_DUP };
    u16 mask = 0;
    for (s32 btn = EQUIP_SLOT_C_LEFT; btn <= EQUIP_SLOT_C_RIGHT; ++btn) {
        if (ExtButton_GetItem(0, btn) == item && gSaveContext.buttonStatus[btn] != BTN_DISABLED) {
            mask |= cButtons[btn - EQUIP_SLOT_C_LEFT];
        }
    }
    if (CVarGetInteger("gEnhancements.Dpad.DpadEquips", 0)) {
        for (s32 btn = EQUIP_SLOT_D_RIGHT; btn <= EQUIP_SLOT_D_UP; ++btn) {
            if (ExtButton_GetDpadItem(0, btn) == item &&
                gSaveContext.shipSaveContext.dpad.status[btn] != BTN_DISABLED &&
                !ItemInput_ButtonIsClaimed(dButtons[btn])) {
                mask |= dButtons[btn];
            }
        }
    }
    return mask;
}

static u16 ExtButton_GetEquipItem(s32 button) {
    return button < PAUSE_EQUIP_D_RIGHT ? ExtButton_GetItem(0, button + EQUIP_SLOT_C_LEFT)
                                        : ExtButton_GetDpadItem(0, button - PAUSE_EQUIP_D_RIGHT);
}

static u8 ExtButton_GetEquipSlot(s32 button) {
    return button < PAUSE_EQUIP_D_RIGHT ? C_SLOT_EQUIP(0, button + EQUIP_SLOT_C_LEFT)
                                        : DPAD_SLOT_EQUIP(0, button - PAUSE_EQUIP_D_RIGHT);
}

static void ExtButton_WriteEquip(PlayState* play, s32 button, u16 item, u8 slot) {
    if (button < PAUSE_EQUIP_D_RIGHT) {
        s32 btn = button + EQUIP_SLOT_C_LEFT;
        if (item >= 0x0200) {
            ExtButton_SetItem(0, btn, item);
        } else {
            ExtButton_ClearItem(0, btn);
            BUTTON_ITEM_EQUIP(0, btn) = (u8)item;
        }
        // Keep the existing C-button contract: EXT cells are not native inventory indices.
        C_SLOT_EQUIP(0, btn) = item >= 0x0200 ? SLOT_NONE : slot;
        Interface_LoadItemIconImpl(play, btn);
    } else {
        s32 btn = button - PAUSE_EQUIP_D_RIGHT;
        ExtButton_SetDpadItem(0, btn, item);
        DPAD_SLOT_EQUIP(0, btn) = slot;
        Interface_Dpad_LoadItemIconImpl(play, btn);
    }
}

static bool ExtButton_IsVirtualOotItem(u16 item) {
    return (item >= ITEM_DINS_FIRE && item <= ITEM_ROCS_FEATHER) || item == ITEM_BOOMERANG || item == ITEM_HAMMER ||
           item == ITEM_OOT_MASK_PLACEHOLDER;
}

s32 ExtButton_EquipItem(PlayState* play, s32 button, u16 item, u8 slot) {
    if (button < PAUSE_EQUIP_C_LEFT || button > PAUSE_EQUIP_D_UP) {
        return false;
    }
    u16 displaced = ExtButton_GetEquipItem(button);
    // Custom cells have a disjoint equip-slot band. Equipment has no inventory
    // slot, and full-width tools carry their identity in the parallel store.
    // Keep native inventory/mask swaps with kaleido's existing owner.
    bool neiItem =
        (slot >= 72 && slot < 96) || item >= 0x0200 || (item >= ITEM_EXT_SWORD_1 && item <= ITEM_EXT_BOOTS_3) ||
        (item == ITEM_PICTOGRAPH_BOX && slot == SLOT_LENS_OF_TRUTH) || (item == ITEM_POWDER_KEG && slot == SLOT_BOMB) ||
        (item == ITEM_BOMB && Nei_Save()->powerKegOwned && Nei_Save()->powerKegMode);
    if (!neiItem && item < 0x0200 && displaced < 0x0200) {
        return false;
    }
    // Kaleido's temporary magic-arrow animation ids resolve to ordinary bow items here.
    // B6/B7 are also the real Roc's Feather/Cape ids in MM. Only a native
    // arrow slot can identify the temporary animation value unambiguously.
    if (item >= 0xB5 && item < 0xB8 && slot >= SLOT_BOW && slot <= SLOT_ARROW_LIGHT) {
        item -= 0xB5 - ITEM_BOW_FIRE;
        slot = SLOT_BOW;
    }
    if (neiItem && button >= PAUSE_EQUIP_D_RIGHT) {
        // D-pad NEI bindings are independent shortcuts. Assigning one must not
        // swap its displaced item into a C slot or remove an existing C binding.
        ExtButton_WriteEquip(play, button, item, slot);
        return true;
    }
    u8 displacedSlot = ExtButton_GetEquipSlot(button);
    bool moved = displaced == item;
    for (s32 other = PAUSE_EQUIP_C_LEFT; other <= PAUSE_EQUIP_D_UP; ++other) {
        if (other == button) {
            continue;
        }
        u16 otherItem = ExtButton_GetEquipItem(other);
        // Match exact identities even when older C/D saves have different slot
        // metadata. Shared progressive cells still use the family checks below.
        if ((otherItem == item && item != ITEM_NONE && item != PAUSE_ITEM_NONE) ||
            (item < 0x0200 && slot != SLOT_NONE && ExtButton_GetEquipSlot(other) == slot &&
             // Page-0 OoT virtual slots and page-1 custom equip slots both use
             // 72..79 in existing saves. A matching byte is not a matching cell
             // across those families. Keep true shared/progressive-cell swaps.
             ExtButton_IsVirtualOotItem(item) == ExtButton_IsVirtualOotItem(otherItem))) {
            // C-button relocation consumes every old shortcut, including a C
            // source preceding D-pad duplicates. Return the displaced item once.
            ExtButton_WriteEquip(play, other, moved ? ITEM_NONE : displaced,
                                 moved ? SLOT_NONE : displacedSlot);
            moved = true;
        }
    }
    ExtButton_WriteEquip(play, button, item, slot);
    return true;
}

} // extern "C"
