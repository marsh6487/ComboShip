#pragma once
#include "z64.h"
constexpr u16 ITEM_DINS_FIRE = 0xA0, ITEM_ROCS_FEATHER = 0xA6, ITEM_BOOMERANG = 0xEE, ITEM_HAMMER = 0xED,
              ITEM_OOT_MASK_PLACEHOLDER = 0xB0;
constexpr u16 ITEM_PICTOGRAPH_BOX = 0xD, ITEM_POWDER_KEG = 0xC, SLOT_LENS_OF_TRUTH = 14, SLOT_BOMB = 6;
struct NeiSaveData {
    u8 powerKegOwned{}, powerKegMode{};
};
inline NeiSaveData* Nei_Save() {
    static NeiSaveData save;
    return &save;
}
