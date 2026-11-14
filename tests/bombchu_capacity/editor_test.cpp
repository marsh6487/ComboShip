#include "tests/test_require.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
extern "C" {
#include "global.h"
#include "functions.h"
SaveContext gSaveContext;
PlayState* gPlayState;
s32 Inventory_ReplaceItem(PlayState*, u8, u8) {
    std::abort();
}
}
using std::find;
std::vector<ItemId> safeItemsForInventorySlot[SLOT_MASK_FIERCE_DEITY + 1];
#include "editor.inc"

static void Reset(int capacity) {
    gSaveContext = {};
    std::memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
                sizeof(gSaveContext.save.saveInfo.inventory.items));
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] = capacity;
}

int main() {
    safeItemsForInventorySlot[SLOT_BOMB] = { ITEM_BOMB };
    safeItemsForInventorySlot[SLOT_BOMBCHU] = { ITEM_BOMBCHU };
    safeItemsForInventorySlot[SLOT_BOW] = { ITEM_BOW };
    Reset(50);
    NextItemInSlot(SLOT_BOMBCHU);
    REQUIRE(AMMO(ITEM_BOMBCHU) == 50 && CUR_UPG_VALUE(UPG_BOMB_BAG) == 0);
    REQUIRE(INV_CONTENT(ITEM_BOMB) == ITEM_NONE);
    AMMO(ITEM_BOMBCHU) = 13;
    NextItemInSlot(SLOT_BOMB);
    for (int tier : { 1, 2, 3 }) {
        REQUIRE(CUR_UPG_VALUE(UPG_BOMB_BAG) == unsigned(tier));
        REQUIRE(AMMO(ITEM_BOMB) == 10 + tier * 10);
        REQUIRE(AMMO(ITEM_BOMBCHU) == 13 && INV_CONTENT(ITEM_BOMBCHU) == ITEM_BOMBCHU);
        NextItemInSlot(SLOT_BOMB);
    }
    REQUIRE(CUR_UPG_VALUE(UPG_BOMB_BAG) == 0 && INV_CONTENT(ITEM_BOMB) == ITEM_NONE);
    REQUIRE(AMMO(ITEM_BOMBCHU) == 13 && INV_CONTENT(ITEM_BOMBCHU) == ITEM_BOMBCHU);
    NativeCheatRefill();
    REQUIRE(AMMO(ITEM_BOMBCHU) == 50 && CUR_UPG_VALUE(UPG_BOMB_BAG) == 0);

    Reset(20);
    INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
    for (int capacity : { 30, 50 }) {
        NextItemInSlot(SLOT_BOMBCHU);
        REQUIRE(gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] == capacity);
        REQUIRE(AMMO(ITEM_BOMBCHU) == capacity && CUR_UPG_VALUE(UPG_BOMB_BAG) == 0);
    }
    gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_INFINITE_BOMBCHUS] = 1;
    NextItemInSlot(SLOT_BOMBCHU);
    REQUIRE(INV_CONTENT(ITEM_BOMBCHU) == ITEM_NONE && AMMO(ITEM_BOMBCHU) == 0);
    REQUIRE(gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] == 0);
    REQUIRE(gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_INFINITE_BOMBCHUS] == 0);
    // Ordinary MM editor behavior still cycles the shared native Bomb Bag.
    NextItemInSlot(SLOT_BOMBCHU);
    REQUIRE(CUR_UPG_VALUE(UPG_BOMB_BAG) == 1 && AMMO(ITEM_BOMBCHU) == 20);
    NextItemInSlot(SLOT_BOMBCHU);
    REQUIRE(CUR_UPG_VALUE(UPG_BOMB_BAG) == 2 && AMMO(ITEM_BOMBCHU) == 30);
    std::puts("PASS native Bombchu editor/cheat: independent tiers, native bag isolation and ordinary controls");
}
