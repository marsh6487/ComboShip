#pragma once
#include "Rando/Rando.h"
extern "C" {
#include "functions.h"
}

namespace Rando::InfiniteUpgrades {
struct Definition {
    RandoItemId item;
    RandoInf flag;
};
inline constexpr Definition Catalog[] = {
    { RI_INFINITE_BOW, RANDO_INF_INFINITE_BOW },       { RI_INFINITE_BOMB_BAG, RANDO_INF_INFINITE_BOMB_BAG },
    { RI_INFINITE_STICKS, RANDO_INF_INFINITE_STICKS }, { RI_INFINITE_NUTS, RANDO_INF_INFINITE_NUTS },
    { RI_INFINITE_MAGIC, RANDO_INF_INFINITE_MAGIC },   { RI_INFINITE_WALLET, RANDO_INF_INFINITE_WALLET },
};
inline const Definition* Find(RandoItemId item) {
    for (const auto& upgrade : Catalog)
        if (upgrade.item == item)
            return &upgrade;
    return nullptr;
}
inline bool Give(RandoItemId item) {
    const auto* upgrade = Find(item);
    if (!upgrade)
        return false;
    Flags_SetRandoInf(upgrade->flag);
    RandoCapacity_RepairInfinite();
    return true;
}
inline bool Remove(RandoItemId item) {
    const auto* upgrade = Find(item);
    if (!upgrade)
        return false;
    Flags_ClearRandoInf(upgrade->flag);
    const int items[] = { ITEM_BOW, ITEM_BOMB, ITEM_BOMBCHU, ITEM_DEKU_STICK, ITEM_DEKU_NUT };
    const int upgrades[] = { UPG_QUIVER, UPG_BOMB_BAG, UPG_BOMB_BAG, UPG_DEKU_STICKS, UPG_DEKU_NUTS };
    for (int i = 0; i < 5; ++i)
        AMMO(items[i]) = CLAMP(AMMO(items[i]), 0, CUR_CAPACITY(upgrades[i]));
    const int total = gSaveContext.save.saveInfo.playerData.rupees + gSaveContext.rupeeAccumulator;
    gSaveContext.save.saveInfo.playerData.rupees = CLAMP(total, 0, CUR_CAPACITY(UPG_WALLET));
    gSaveContext.rupeeAccumulator = 0;
    return true;
}
} // namespace Rando::InfiniteUpgrades
