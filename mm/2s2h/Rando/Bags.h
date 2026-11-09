#pragma once
#include "Rando/Rando.h"
#include <algorithm>
extern "C" {
#include "functions.h"
}

namespace Rando::Bags {
struct Definition {
    RandoItemId item;
    int inventoryItem;
    int upgrade;
    RandoInf flag;
    RandoOptionId option;
};
inline constexpr Definition Catalog[] = {
    { RI_DEKU_STICK_BAG, ITEM_DEKU_STICK, UPG_DEKU_STICKS, RANDO_INF_DEKU_STICK_BAG, RO_SHUFFLE_DEKU_STICK_BAG },
    { RI_DEKU_NUT_BAG, ITEM_DEKU_NUT, UPG_DEKU_NUTS, RANDO_INF_DEKU_NUT_BAG, RO_SHUFFLE_DEKU_NUT_BAG },
};
inline const Definition* Find(RandoItemId item) {
    for (const auto& bag : Catalog)
        if (bag.item == item)
            return &bag;
    return nullptr;
}
inline bool Give(RandoItemId item) {
    const auto* bag = Find(item);
    if (!bag)
        return false;
    if (!Flags_GetRandoInf(bag->flag)) {
        Flags_SetRandoInf(bag->flag);
        Inventory_ChangeUpgrade(bag->upgrade, std::max(1u, CUR_UPG_VALUE(bag->upgrade)));
        INV_CONTENT(bag->inventoryItem) = bag->inventoryItem;
        AMMO(bag->inventoryItem) = CUR_CAPACITY(bag->upgrade);
    }
    return true;
}
inline bool Remove(RandoItemId item) {
    const auto* bag = Find(item);
    if (!bag)
        return false;
    Flags_ClearRandoInf(bag->flag);
    RandoCapacity_RepairBagAmmo();
    return true;
}
// The foreign capacity IDs remain stable. New bag seeds treat their first
// upgrade as 20 sticks / 30 nuts even if it arrives before the base bag.
inline bool GiveCapacity(RandoItemId item) {
    int which;
    if (item == RI_OOT_PROGRESSIVE_STICK_CAPACITY)
        which = 0;
    else if (item == RI_OOT_PROGRESSIVE_NUT_CAPACITY)
        which = 1;
    else
        return false;
    const auto& bag = Catalog[which];
    const unsigned baseline = RANDO_SAVE_OPTIONS[bag.option] ? 1 : 0;
    const unsigned tier = std::min(3u, std::max(baseline, CUR_UPG_VALUE(bag.upgrade)) + 1);
    Inventory_ChangeUpgrade(bag.upgrade, tier);
    INV_CONTENT(bag.inventoryItem) = bag.inventoryItem;
    AMMO(bag.inventoryItem) = CUR_CAPACITY(bag.upgrade);
    return true;
}
} // namespace Rando::Bags
