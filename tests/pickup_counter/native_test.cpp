#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include "combo/hud/PickupCounter.h"
#include "z64.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
SaveContext gSaveContext;
PlayState* gPlayState = nullptr;
}

#ifndef PICKUP_TEST_MM
static std::set<int> flags;
static int bombchuSetting = RO_BOMBCHU_BAG_PROGRESSIVE;
extern "C" s32 Flags_GetRandomizerInf(RandomizerInf flag) {
    return flags.contains(flag);
}
extern "C" u8 Randomizer_GetSettingValue(RandomizerSettingKey key) {
    assert(key == RSK_BOMBCHU_BAG);
    return bombchuSetting;
}
#endif

#include "native_inventory.inc"
namespace PickupCounterHost {
using namespace PickupCounter;
#include "native_adapters.inc"
}
using namespace PickupCounter;

static void Upgrade(int upgrade, int tier) {
#ifdef PICKUP_TEST_MM
    auto& bits = gSaveContext.save.saveInfo.inventory.upgrades;
#else
    auto& bits = gSaveContext.inventory.upgrades;
#endif
    bits = (bits & ~gUpgradeMasks[upgrade]) | (tier << gUpgradeShifts[upgrade]);
}

int main() {
#ifdef PICKUP_TEST_MM
    std::memset(gSaveContext.save.saveInfo.inventory.items, ITEM_NONE,
                sizeof(gSaveContext.save.saveInfo.inventory.items));
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    auto& nativeRupees = gSaveContext.save.saveInfo.playerData.rupees;
    auto flag = [](RandoInf value) {
        gSaveContext.save.shipSaveInfo.rando.randoInf[value / 16] |= 1u << (value % 16);
    };
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHUFFLE_CHILD_WALLET] = 1;
#else
    std::memset(gSaveContext.inventory.items, ITEM_NONE, sizeof(gSaveContext.inventory.items));
    gSaveContext.ship.quest.id = QUEST_RANDOMIZER;
    auto& nativeRupees = gSaveContext.rupees;
    auto flag = [](RandomizerInf value) { flags.insert(value); };
#endif
    assert(PickupCounterHost::ReadSnapshot()[Index(Kind::Rupees)].capacity == 0);
#ifdef PICKUP_TEST_MM
    flag(RANDO_INF_WALLET_CHILD);
#else
    flag(RAND_INF_HAS_WALLET);
#endif
    Upgrade(UPG_WALLET, 1);
    nativeRupees = 100;
    gSaveContext.rupeeAccumulator = 20;
    auto first = PickupCounterHost::ReadSnapshot();
    assert(first[Index(Kind::Rupees)].current == 120);
    assert(first[Index(Kind::Rupees)].capacity == 200);
    nativeRupees = 101;
    gSaveContext.rupeeAccumulator = 19;
    assert(PickupCounterHost::ReadSnapshot()[Index(Kind::Rupees)].current == 120);
    nativeRupees = 200;
    gSaveContext.rupeeAccumulator = 50;
    assert(Sanitize(PickupCounterHost::ReadSnapshot()[Index(Kind::Rupees)]).current == 200);
    assert(PickupCounterHost::ReceiptKind(ITEM_RUPEE_GREEN) == Kind::Rupees);
    assert(PickupCounterHost::ReceiptKind(ITEM_RUPEE_PURPLE) == Kind::Rupees);
    assert(PickupCounterHost::ReceiptKind(ITEM_BOMBS_5) == Kind::Bombs);
    assert(PickupCounterHost::ReceiptKind(ITEM_BOMBCHUS_5) == Kind::Bombchus);
    assert(PickupCounterHost::ReceiptKind(ITEM_NONE) == Kind::Count);

    // The ordinary Bomb Bag is absent; Bombchus retain their independent 20.
    Upgrade(UPG_BOMB_BAG, 0);
    INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
    AMMO(ITEM_BOMBCHU) = 19;
#ifdef PICKUP_TEST_MM
    gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] = 20;
#else
    gSaveContext.ship.quest.data.randomizer.bombchuUpgradeLevel = 1;
#endif
    auto snapshot = PickupCounterHost::ReadSnapshot();
    assert(snapshot[Index(Kind::Bombs)].capacity == 0);
    assert(snapshot[Index(Kind::Bombchus)].capacity == 20);
    assert(snapshot[Index(Kind::Bombchus)].current == 19);
    Upgrade(UPG_BOMB_BAG, 3);
    assert(PickupCounterHost::ReadSnapshot()[Index(Kind::Bombchus)].capacity == 20);

    // The native host's actual upgrade tables drive every supported tier.
    INV_CONTENT(ITEM_BOW) = ITEM_BOW;
    for (int tier = 0; tier <= 3; ++tier) {
        Upgrade(UPG_QUIVER, tier);
        snapshot = PickupCounterHost::ReadSnapshot();
        assert(snapshot[Index(Kind::Arrows)].capacity == gUpgradeCapacities[UPG_QUIVER][tier]);
    }
    AMMO(ITEM_BOW) = 30;
#ifdef PICKUP_TEST_MM
    flag(RANDO_INF_INFINITE_BOW);
    assert(PickupCounterHost::ReceiptKind(ITEM_ARROWS_10) == Kind::Arrows);
    assert(PickupCounterHost::ReceiptKind(ITEM_POWDER_KEG) == Kind::PowderKeg);
    assert(PickupCounterHost::ReceiptKind(ITEM_DEKU_NUTS_10) == Kind::Nuts);
    assert(PickupCounterHost::ReadSnapshot()[Index(Kind::Seeds)].capacity == 0);
    INV_CONTENT(ITEM_POWDER_KEG) = ITEM_POWDER_KEG;
    AMMO(ITEM_POWDER_KEG) = 1;
    assert(PickupCounterHost::ReadSnapshot()[Index(Kind::PowderKeg)].capacity == 1);
    assert(PickupCounterHost::IconItem(Kind::Bombchus) == ITEM_BOMBCHU);
    std::cout << "PASS MM native receipts, wallet accumulator, absent bags, independent Bombchus and upgrades\n";
#else
    flag(RAND_INF_HAS_INFINITE_QUIVER);
    assert(PickupCounterHost::ReceiptKind(ITEM_ARROWS_SMALL) == Kind::Arrows);
    assert(PickupCounterHost::ReceiptKind(ITEM_SEEDS_30) == Kind::Seeds);
    assert(PickupCounterHost::ReceiptKind(ITEM_NUTS_10) == Kind::Nuts);
    assert(PickupCounterHost::ReadSnapshot()[Index(Kind::PowderKeg)].capacity == 0);
    for (int tier = 0; tier <= 3; ++tier) {
        gSaveContext.ship.quest.data.randomizer.bombchuUpgradeLevel = tier;
        const int expected[] = { 0, 20, 30, 50 };
        assert(PickupCounterHost::ReadSnapshot()[Index(Kind::Bombchus)].capacity == expected[tier]);
    }
    bombchuSetting = RO_BOMBCHU_BAG_NONE;
    assert(PickupCounterHost::ReadSnapshot()[Index(Kind::Bombchus)].capacity == 50);
    assert(PickupCounterHost::IconItem(Kind::Seeds) == ITEM_SEEDS);
    std::cout << "PASS OoT native receipts, wallet accumulator, independent Bombchus and upgrades\n";
#endif
    assert(PickupCounterHost::ReadSnapshot()[Index(Kind::Arrows)].infinite);
}
