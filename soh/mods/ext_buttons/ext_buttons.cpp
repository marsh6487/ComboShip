/**
 * ext_buttons.cpp — Extended-button infrastructure accessor API (see ext_buttons.h).
 *
 * Globbed and compiled as its own translation unit by soh/CMakeLists.txt (mods/*.cpp, recursive).
 * Exports C-linkage helpers so the u8 buttonItems marker (ITEM_EXT_BUTTON) and the parallel u16
 * extButtons array stay in lockstep.
 */
#include "ext_buttons.h"
#include "macros.h"
#include "functions.h"
#include "items/helpers/equip_helper.h"
#include <libultraship/bridge/consolevariablebridge.h>
#ifdef COMBO_BUILD
#include "gameplay/MagicMirrorItem.h"
#endif

// gSaveContext is declared in variables.h, which z64.h does not pull in. Declaring the one symbol
// this file needs keeps the TU free of that header's global-variable flood (same as nei_save.cpp).
extern "C" SaveContext gSaveContext;
extern "C" u8 Picto_IsOwned(void);
extern "C" u8 Picto_IsOnLensActive(void);
extern "C" u8 PowerKeg_IsOwned(void);
extern "C" u8 PowerKeg_IsOnBombActive(void);

extern "C" {

u16 ExtButton_GetItem(s32 btn) {
    u8 raw = gSaveContext.equips.buttonItems[btn];
    if (raw == ITEM_EXT_BUTTON) {
        const u16 item = EXT_BUTTON_ITEM(btn);
        return item >= 0x0200 ? item : (u16)ITEM_NONE;
    }
    // Native C/D equips always carry their inventory slot. Preserve B's
    // separate contract and valid zero-valued sticks with SLOT_STICK.
    if (raw == ITEM_STICK && btn > 0 && gSaveContext.equips.cButtonSlots[btn - 1] == SLOT_NONE) {
        return ITEM_NONE;
    }
    return (u16)raw;
}

void ExtButton_SetItem(s32 btn, u16 extId) {
    gSaveContext.equips.buttonItems[btn] = ITEM_EXT_BUTTON;
    EXT_BUTTON_ITEM(btn) = extId;
}

void ExtButton_ClearItem(s32 btn) {
    gSaveContext.equips.buttonItems[btn] = ITEM_NONE;
    EXT_BUTTON_ITEM(btn) = 0;
}

u16 ExtButton_GetInputMask(u16 item) {
    static const u16 buttons[] = { BTN_B, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT, BTN_DUP, BTN_DDOWN, BTN_DLEFT, BTN_DRIGHT };
    const s32 end = CVarGetInteger("gEnhancements.DpadEquips", 0) ? 8 : 4;
    u16 mask = 0;
    for (s32 btn = 1; btn < end; ++btn) {
        const s32 status = btn < 4 ? btn : btn + 1;
        if (ExtButton_GetItem(btn) == item && gSaveContext.buttonStatus[status] != BTN_DISABLED &&
            !ItemInput_ButtonIsClaimed(buttons[btn])) {
            mask |= buttons[btn];
        }
    }
    return mask;
}

static void ExtButton_WriteEquip(s32 btn, u16 item, u8 slot) {
    if (item >= 0x0200) {
        ExtButton_SetItem(btn, item);
        gSaveContext.equips.cButtonSlots[btn - 1] = SLOT_NONE;
    } else {
        ExtButton_ClearItem(btn);
        gSaveContext.equips.buttonItems[btn] = (u8)item;
        gSaveContext.equips.cButtonSlots[btn - 1] = item == ITEM_NONE ? SLOT_NONE : slot;
    }
}

s32 ExtButton_EquipItem(PlayState* play, s32 btn, u16 item, u8 slot) {
    if (btn < 1 || btn >= (s32)ARRAY_COUNT(gSaveContext.equips.buttonItems) || item == ITEM_NONE ||
        item == PAUSE_ITEM_NONE || item == ITEM_SOLD_OUT) {
        return false;
    }
    const bool neiItem = item >= 0x80 || item == ITEM_MM_LETTER_KAFEI || item == ITEM_MM_SPECIAL_DELIVERY ||
                         (item == ITEM_LENS && Picto_IsOwned() && Picto_IsOnLensActive()) ||
                         (item == ITEM_BOMB && PowerKeg_IsOwned() && PowerKeg_IsOnBombActive());
    // Adding a D-pad shortcut preserves C bindings. Moving back to C below
    // consumes existing copies of this exact item across both button banks.
    if (neiItem && btn >= 4) {
        ExtButton_WriteEquip(btn, item, slot);
        return true;
    }
    const u16 displaced = ExtButton_GetItem(btn);
    const u8 displacedSlot = gSaveContext.equips.cButtonSlots[btn - 1];
    bool moved = displaced == item;
    for (s32 other = 1; other < (s32)ARRAY_COUNT(gSaveContext.equips.buttonItems); ++other) {
        if (other == btn) {
            continue;
        }
        const u16 otherItem = ExtButton_GetItem(other);
        // Native equipment virtual slots overlap page-2/3 cell numbers. A
        // native equip may swap only another native binding using that slot.
        const bool same =
            neiItem ? otherItem == item
                    : otherItem < 0x80 && slot != SLOT_NONE && gSaveContext.equips.cButtonSlots[other - 1] == slot;
        const bool bow = item == ITEM_BOW && otherItem >= ITEM_BOW_ARROW_FIRE && otherItem <= ITEM_BOW_ARROW_LIGHT &&
                         !CVarGetInteger("gEnhancements.SeparateArrows", 0);
        if (same || bow) {
            // Move the displaced item once, clearing any additional legacy copies.
            // Full-width NEI identities must survive a native or custom swap.
            ExtButton_WriteEquip(other, moved ? ITEM_NONE : displaced, displacedSlot);
            Interface_LoadItemIcon2(play, other);
            moved = true;
        }
    }
    ExtButton_WriteEquip(btn, item, slot);
    return true;
}

#ifdef COMBO_BUILD
s32 ExtButton_EquipMagicMirror(PlayState* play, s32 btn, u16 item, u8 slot) {
    if (btn < 1 || btn >= (s32)ARRAY_COUNT(gSaveContext.equips.buttonItems)) {
        return false;
    }
    if (item != EXT_ITEM_MAGIC_MIRROR && ExtButton_GetItem(btn) != EXT_ITEM_MAGIC_MIRROR) {
        return false;
    }
    return ExtButton_EquipItem(play, btn, item, slot);
}
#endif

void ExtButton_SaveAgeItems(s32 age) {
    auto& saved = age == LINK_AGE_CHILD ? gSaveContext.ship.childExtButtons : gSaveContext.ship.adultExtButtons;
    for (size_t button = 0; button < ARRAY_COUNT(saved.items); ++button) {
        saved.items[button] = gSaveContext.equips.buttonItems[button] == ITEM_EXT_BUTTON ? EXT_BUTTON_ITEM(button) : 0;
    }
}

void ExtButton_LoadAgeItems(s32 age) {
    const auto& saved = age == LINK_AGE_CHILD ? gSaveContext.ship.childExtButtons : gSaveContext.ship.adultExtButtons;
    for (size_t button = 0; button < ARRAY_COUNT(saved.items); ++button) {
        EXT_BUTTON_ITEM(button) = gSaveContext.equips.buttonItems[button] == ITEM_EXT_BUTTON ? saved.items[button] : 0;
    }
}

void ExtButton_ResetSaveData(void) {
    gSaveContext.ship.extButtons = {};
    gSaveContext.ship.childExtButtons = {};
    gSaveContext.ship.adultExtButtons = {};
}

} // extern "C"
