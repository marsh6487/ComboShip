#ifndef RANDO_CAPACITY_H
#define RANDO_CAPACITY_H

// Included after SaveContext and the inventory macros in z64save.h. Policy is
// seed-owned: live CVars and cross-game upgrade tiers cannot grant a base bag.
extern u32 gUpgradeMasks[];
extern u8 gUpgradeShifts[];
extern u16 gUpgradeCapacities[][4];
extern u8 gItemSlots[77];

static inline int RandoCapacity_HasBag(int upgrade) {
    if (gSaveContext.save.shipSaveInfo.saveType != SAVETYPE_RANDO)
        return 1;
    RandoOptionId option;
    RandoInf flag;
    if (upgrade == UPG_DEKU_STICKS) {
        option = RO_SHUFFLE_DEKU_STICK_BAG;
        flag = RANDO_INF_DEKU_STICK_BAG;
    } else if (upgrade == UPG_DEKU_NUTS) {
        option = RO_SHUFFLE_DEKU_NUT_BAG;
        flag = RANDO_INF_DEKU_NUT_BAG;
    } else {
        return 1;
    }
    return !gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[option] ||
           (gSaveContext.save.shipSaveInfo.rando.randoInf[flag / 16] & (1u << (flag % 16))) != 0;
}

static inline int RandoCapacity_HasBagForItem(int item) {
    if (item == ITEM_DEKU_STICK || item == ITEM_DEKU_STICKS_5 || item == ITEM_DEKU_STICKS_10)
        return RandoCapacity_HasBag(UPG_DEKU_STICKS);
    if (item == ITEM_DEKU_NUT || item == ITEM_DEKU_NUTS_5 || item == ITEM_DEKU_NUTS_10)
        return RandoCapacity_HasBag(UPG_DEKU_NUTS);
    return 1;
}

static inline int RandoCapacity_HasWallet(void) {
    const RandoInf flag = RANDO_INF_WALLET_CHILD;
    return gSaveContext.save.shipSaveInfo.saveType != SAVETYPE_RANDO ||
           !gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHUFFLE_CHILD_WALLET] ||
           CUR_UPG_VALUE(UPG_WALLET) > 0 ||
           (gSaveContext.save.shipSaveInfo.rando.randoInf[flag / 16] & (1u << (flag % 16))) != 0;
}

static inline int RandoCapacity_Current(int upgrade) {
    if (upgrade == UPG_WALLET && !RandoCapacity_HasWallet())
        return 0;
    return RandoCapacity_HasBag(upgrade) ? gUpgradeCapacities[upgrade][CUR_UPG_VALUE(upgrade)] : 0;
}

static inline void RandoCapacity_RepairBagAmmo(void) {
    if (!RandoCapacity_HasBag(UPG_DEKU_STICKS))
        AMMO(ITEM_DEKU_STICK) = 0;
    if (!RandoCapacity_HasBag(UPG_DEKU_NUTS))
        AMMO(ITEM_DEKU_NUT) = 0;
}

static inline void RandoCapacity_RepairWallet(void) {
    if (!RandoCapacity_HasWallet()) {
        gSaveContext.save.saveInfo.playerData.rupees = 0;
        gSaveContext.rupeeAccumulator = 0;
    }
}

#endif
