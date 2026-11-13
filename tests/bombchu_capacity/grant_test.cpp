#include <nlohmann/json.hpp>
#include "Rando/Rando.h"
#include "Rando/StaticData/StaticData.h"
#include "combo/rando/SharedItems.h"
#include "tests/test_require.h"
#include <cstdio>
#include <cstring>
extern "C" {
#include "functions.h"
SaveContext gSaveContext;
PlayState* gPlayState;
u8 NativeBombBagGrant(PlayState*, u8);
static bool sharedBag;
int Combo_MM_BombchuBagShared(void) {
    return sharedBag;
}
u8 Item_Give(PlayState* play, u8 item) {
    return NativeBombBagGrant(play, item);
}
}
namespace Rando::StaticData {
std::map<RandoItemId, RandoStaticItem> Items;
}
static int saved;
void SaveManager_SaveCurrentForCombo() {
    ++saved;
}
#include "grant.inc"

static void Reset() {
    gSaveContext = {};
    std::memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
                sizeof(gSaveContext.save.saveInfo.inventory.items));
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    saved = 0;
}

int main() {
    Rando::StaticData::Items[RI_BOMB_BAG_20].itemId = ITEM_BOMB_BAG_20;
    Rando::StaticData::Items[RI_BOMB_BAG_30].itemId = ITEM_BOMB_BAG_30;
    Rando::StaticData::Items[RI_BOMB_BAG_40].itemId = ITEM_BOMB_BAG_40;
    for (int capacity : { 0, 20, 30, 50 }) {
        Reset();
        auto& registry = gSaveContext.save.shipSaveInfo.nei.comboObtained;
        registry[FC_OOT_BOMBCHU_CAPACITY] = capacity;
        AMMO(ITEM_BOMBCHU) = 1;
        NativeGive(RI_OOT_BOMBCHU_BAG);
        REQUIRE(registry[FC_OOT_BOMBCHU_CAPACITY] == (capacity ? capacity : 20));
        REQUIRE(INV_CONTENT(ITEM_BOMBCHU) == ITEM_BOMBCHU && AMMO(ITEM_BOMBCHU) == 20);
        REQUIRE(CUR_UPG_VALUE(UPG_BOMB_BAG) == 0 && INV_CONTENT(ITEM_BOMB) == ITEM_NONE);
        Inventory_ChangeAmmo(ITEM_BOMBCHU, -1);
        REQUIRE(AMMO(ITEM_BOMBCHU) == 19);
        for (bool shared : { false, true }) {
            sharedBag = shared;
            for (RandoItemId bag : { RI_BOMB_BAG_20, RI_BOMB_BAG_30, RI_BOMB_BAG_40 }) {
                AMMO(ITEM_BOMBCHU) = 7;
                NativeGive(bag);
                REQUIRE(AMMO(ITEM_BOMBCHU) == 7);
                REQUIRE(AMMO(ITEM_BOMB) == CUR_CAPACITY(UPG_BOMB_BAG));
                REQUIRE(registry[FC_OOT_BOMBCHU_CAPACITY] == (capacity ? capacity : 20));
            }
        }
    }
    Reset();
    sharedBag = false;
    NativeGive(RI_BOMB_BAG_20);
    REQUIRE(AMMO(ITEM_BOMBCHU) == 20 && INV_CONTENT(ITEM_BOMBCHU) == ITEM_BOMBCHU);
    REQUIRE(gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] == 0);
#ifdef COMBO_BUILD
    // Native Bombchus alone are not ownership of the independent shared bag.
    REQUIRE(MM_GetSharedTier(ComboRando::SF_BOMBCHU_BAG) == 0);
    AMMO(ITEM_BOMBCHU) = 5;
    NativeRaiseSharedTier(ComboRando::SF_BOMBCHU_BAG, 1);
    REQUIRE(MM_GetSharedTier(ComboRando::SF_BOMBCHU_BAG) == 1);
    REQUIRE(gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] == 20);
    REQUIRE(AMMO(ITEM_BOMBCHU) == 20 && saved == 1);
    Inventory_ChangeAmmo(ITEM_BOMBCHU, -1);
    for (int i = 0; i < 20; ++i)
        NativeRaiseSharedTier(ComboRando::SF_BOMBCHU_BAG, 1);
    REQUIRE(AMMO(ITEM_BOMBCHU) == 19 && saved == 1);
    gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] = 50;
    NativeRaiseSharedTier(ComboRando::SF_BOMBCHU_BAG, 1);
    REQUIRE(AMMO(ITEM_BOMBCHU) == 19 && saved == 1);
    REQUIRE(gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] == 50);
    Reset();
    NativeRaiseSharedTier(ComboRando::SF_BOMBCHU_BAG, 1);
    REQUIRE(AMMO(ITEM_BOMBCHU) == 20 && CUR_UPG_VALUE(UPG_BOMB_BAG) == 0);
    REQUIRE(INV_CONTENT(ITEM_BOMB) == ITEM_NONE);
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_VANILLA;
    REQUIRE(MM_GetSharedTier(ComboRando::SF_BOMBCHU_BAG) == 0);
#endif
    for (bool shared : { false, true }) {
        Reset();
        sharedBag = shared;
        gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] = 20;
        INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
        for (RandoItemId ammo : { RI_BOMBCHU, RI_BOMBCHU_5, RI_BOMBCHU_10 })
            REQUIRE(NativeRandoBombchuObtainable(ammo));
    }
    Reset();
    sharedBag = false;
    REQUIRE(!NativeRandoBombchuObtainable(RI_BOMBCHU_5));
    Inventory_ChangeUpgrade(UPG_BOMB_BAG, 1);
    INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
    REQUIRE(NativeRandoBombchuObtainable(RI_BOMBCHU_5));
    sharedBag = true;
#ifdef COMBO_BUILD
    REQUIRE(!NativeRandoBombchuObtainable(RI_BOMBCHU_5));
#else
    REQUIRE(NativeRandoBombchuObtainable(RI_BOMBCHU_5));
#endif
    std::puts("PASS native Bombchu grants: independent ownership, native bag isolation and shared-tier replay");
}
