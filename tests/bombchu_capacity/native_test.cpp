#include "tests/test_require.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
extern "C" {
#include "global.h"
#include "functions.h"
#include "overlays/actors/ovl_En_GirlA/z_en_girla.h"
SaveContext gSaveContext;
u8 NativeBombchuRefill(PlayState*, u8);
int NativeBombchuFull0(void);
int NativeBombchuFull1(void);
int NativeBombchuFull2(void);
s32 EnGirlA_CanBuyBombchus(PlayState*, EnGirlA*);
}

static void Reset(int independent = 0, int nativeTier = 0) {
    gSaveContext = {};
    std::memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
                sizeof(gSaveContext.save.saveInfo.inventory.items));
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] = independent;
    Inventory_ChangeUpgrade(UPG_BOMB_BAG, nativeTier);
    INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
}

static void NativeInfinite(bool enabled) {
    const int flag = RANDO_INF_INFINITE_BOMB_BAG;
    auto& word = gSaveContext.save.shipSaveInfo.rando.randoInf[flag / 16];
    if (enabled)
        word |= 1u << (flag % 16);
    else
        word &= ~(1u << (flag % 16));
}

int main() {
    // The reported failure: imported 20 with no native Bomb Bag must spend one.
    Reset(20);
    AMMO(ITEM_BOMBCHU) = 20;
    Inventory_ChangeAmmo(ITEM_BOMBCHU, -1);
    REQUIRE(AMMO(ITEM_BOMBCHU) == 19);

    int cases = 0;
    for (int independent : { 20, 30, 50 }) {
        for (int nativeTier : { 0, 1, 2, 3 }) {
            for (bool nativeInfinite : { false, true }) {
                for (bool foreignInfinite : { false, true }) {
                    Reset(independent, nativeTier);
                    NativeInfinite(nativeInfinite);
                    auto& registry = gSaveContext.save.shipSaveInfo.nei.comboObtained;
                    registry[FC_OOT_INFINITE_BOMBCHUS] = foreignInfinite;
                    INV_CONTENT(ITEM_BOMB) = ITEM_BOMB;
                    AMMO(ITEM_BOMBCHU) = independent;
                    AMMO(ITEM_BOMB) = 7;
                    Inventory_ChangeAmmo(ITEM_BOMBCHU, -1);
                    REQUIRE(AMMO(ITEM_BOMBCHU) == independent - int(!foreignInfinite));
                    REQUIRE(AMMO(ITEM_BOMB) == 7);
                    REQUIRE(CUR_UPG_VALUE(UPG_BOMB_BAG) == unsigned(nativeTier));
                    const int nativeCapacity = nativeTier ? (nativeInfinite ? 40 : 10 + nativeTier * 10) : 0;
                    REQUIRE(CUR_CAPACITY(UPG_BOMB_BAG) == nativeCapacity);
                    REQUIRE(bool(RandoCapacity_InfiniteAmmo(ITEM_BOMBCHU)) == foreignInfinite);
                    REQUIRE(bool(RandoCapacity_InfiniteAmmo(ITEM_BOMB)) == (nativeInfinite && nativeTier > 0));
                    Inventory_ChangeAmmo(ITEM_BOMBCHU, 32767);
                    REQUIRE(AMMO(ITEM_BOMBCHU) == independent);
                    Inventory_ChangeAmmo(ITEM_BOMBCHU, -32768);
                    REQUIRE(AMMO(ITEM_BOMBCHU) == (foreignInfinite ? independent : 0));
                    AMMO(ITEM_BOMBCHU) = 3;
                    RandoCapacity_RepairInfinite();
                    REQUIRE(AMMO(ITEM_BOMBCHU) == (foreignInfinite ? independent : 3));
                    ++cases;
                }
            }
            Reset(independent, nativeTier);
            for (int item : { ITEM_BOMBCHU, ITEM_BOMBCHUS_20, ITEM_BOMBCHUS_10, ITEM_BOMBCHUS_1, ITEM_BOMBCHUS_5 }) {
                AMMO(ITEM_BOMBCHU) = independent - 1;
                NativeBombchuRefill(nullptr, item);
                REQUIRE(AMMO(ITEM_BOMBCHU) == independent);
                REQUIRE(INV_CONTENT(ITEM_BOMBCHU) == ITEM_BOMBCHU);
                REQUIRE(CUR_UPG_VALUE(UPG_BOMB_BAG) == unsigned(nativeTier));
            }
            for (int item : { ITEM_BOMBCHUS_20, ITEM_BOMBCHUS_10, ITEM_BOMBCHUS_1, ITEM_BOMBCHUS_5 })
                REQUIRE(Item_CheckObtainability(item) != ITEM_NONE);
            static PlayState play{};
            play.msgCtx.unk1206C = 10;
            gSaveContext.save.saveInfo.playerData.rupees = 50;
            AMMO(ITEM_BOMBCHU) = independent;
            REQUIRE(EnGirlA_CanBuyBombchus(&play, nullptr) == CANBUY_RESULT_NO_ROOM);
            AMMO(ITEM_BOMBCHU) = independent - 1;
            REQUIRE(EnGirlA_CanBuyBombchus(&play, nullptr) == CANBUY_RESULT_SUCCESS_2);
            gSaveContext.save.saveInfo.playerData.rupees = 0;
            REQUIRE(EnGirlA_CanBuyBombchus(&play, nullptr) == CANBUY_RESULT_NEED_RUPEES);
            for (auto full : { NativeBombchuFull0, NativeBombchuFull1, NativeBombchuFull2 }) {
                AMMO(ITEM_BOMBCHU) = independent;
                REQUIRE(full());
                AMMO(ITEM_BOMBCHU) = independent - 1;
                REQUIRE(!full());
            }
        }
    }
    // No independent capacity: retain native MM's 0/20/30/40 fallback.
    for (int tier : { 0, 1, 2, 3 }) {
        Reset(0, tier);
        const int maximum = tier ? 10 + 10 * tier : 0;
        AMMO(ITEM_BOMBCHU) = maximum;
        Inventory_ChangeAmmo(ITEM_BOMBCHU, -1);
        REQUIRE(AMMO(ITEM_BOMBCHU) == (maximum ? maximum - 1 : 0));
        Inventory_ChangeAmmo(ITEM_BOMBCHU, 32767);
        REQUIRE(AMMO(ITEM_BOMBCHU) == maximum);
        // A stray foreign infinity flag cannot grant an independent bag.
        gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_INFINITE_BOMBCHUS] = 1;
        REQUIRE(!RandoCapacity_InfiniteAmmo(ITEM_BOMBCHU));
    }
    // Native infinite Bomb Bag still supports Bombchus when bombs are absent.
    Reset(0, 1);
    NativeInfinite(true);
    REQUIRE(INV_CONTENT(ITEM_BOMB) == ITEM_NONE);
    RandoCapacity_RepairInfinite();
    REQUIRE(AMMO(ITEM_BOMBCHU) == 40 && CUR_CAPACITY(UPG_BOMB_BAG) == 40);
    Inventory_ChangeAmmo(ITEM_BOMBCHU, -1);
    REQUIRE(AMMO(ITEM_BOMBCHU) == 40);
    // Ordinary saves retain their finite native behavior.
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_VANILLA;
    Inventory_ChangeAmmo(ITEM_BOMBCHU, -1);
    REQUIRE(AMMO(ITEM_BOMBCHU) == 20);
    std::printf("PASS native Bombchu capacity: %d independent/infinity combinations, refills, shops, HUD/pause and "
                "native controls\n",
                cases);
}
