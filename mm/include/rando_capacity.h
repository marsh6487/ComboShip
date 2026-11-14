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

static inline int RandoCapacity_InfiniteFlag(RandoInf flag) {
    return gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO &&
           (gSaveContext.save.shipSaveInfo.rando.randoInf[flag / 16] & (1u << (flag % 16))) != 0;
}

static inline int RandoCapacity_HasIndependentBombchuBag(void) {
    return gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] > 0;
}

static inline void RandoCapacity_GrantBombchuBag(void) {
    if (gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] < 20)
        gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY] = 20;
}

static inline int RandoCapacity_InfiniteAmmo(int item) {
    RandoInf flag;
    int upgrade;
    switch (item) {
        case ITEM_BOW:
            flag = RANDO_INF_INFINITE_BOW;
            upgrade = UPG_QUIVER;
            break;
        case ITEM_BOMBCHU:
            // An imported Bombchu Bag has its own capacity and infinite flag.
            // Native MM Bombchus still use the native Bomb Bag when no such
            // independent bag has been obtained.
            if (RandoCapacity_HasIndependentBombchuBag())
                return gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_INFINITE_BOMBCHUS] != 0 &&
                       INV_CONTENT(ITEM_BOMBCHU) == ITEM_BOMBCHU;
            // fallthrough
        case ITEM_BOMB:
            flag = RANDO_INF_INFINITE_BOMB_BAG;
            upgrade = UPG_BOMB_BAG;
            break;
        case ITEM_DEKU_STICK:
            flag = RANDO_INF_INFINITE_STICKS;
            upgrade = UPG_DEKU_STICKS;
            break;
        case ITEM_DEKU_NUT:
            flag = RANDO_INF_INFINITE_NUTS;
            upgrade = UPG_DEKU_NUTS;
            break;
        default:
            return 0;
    }
    return RandoCapacity_InfiniteFlag(flag) && RandoCapacity_HasBag(upgrade) && CUR_UPG_VALUE(upgrade) > 0 &&
           INV_CONTENT(item) == item;
}

static inline int RandoCapacity_InfiniteMagic(void) {
    return RandoCapacity_InfiniteFlag(RANDO_INF_INFINITE_MAGIC) &&
           gSaveContext.save.saveInfo.playerData.isMagicAcquired && gSaveContext.magicCapacity > 0;
}

static inline int RandoCapacity_InfiniteWallet(void) {
    return RandoCapacity_InfiniteFlag(RANDO_INF_INFINITE_WALLET) && RandoCapacity_HasWallet();
}

static inline int RandoCapacity_CanSpendMagic(int amount) {
    return RandoCapacity_InfiniteMagic() || gSaveContext.save.saveInfo.playerData.magic >= amount;
}

static inline int RandoCapacity_Current(int upgrade) {
    if (upgrade == UPG_WALLET && !RandoCapacity_HasWallet())
        return 0;
    if (upgrade == UPG_WALLET && RandoCapacity_InfiniteWallet()) {
        const int finite = gUpgradeCapacities[upgrade][CUR_UPG_VALUE(upgrade)];
        const int tycoon = gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHUFFLE_TYCOON_WALLET];
        int ceiling = gUpgradeCapacities[upgrade][tycoon ? 3 : 2];
        // The oracle does not initialize HUD hooks, and FleetSync may restore
        // native tier 3 to 999. Keep the owned endpoint's configured ceiling
        // deterministic without changing finite tiers or the shared table.
        if (tycoon && ceiling < 5000)
            ceiling = 5000;
        return finite > ceiling ? finite : ceiling;
    }
    if ((upgrade == UPG_QUIVER && RandoCapacity_InfiniteAmmo(ITEM_BOW)) ||
        (upgrade == UPG_BOMB_BAG &&
         (RandoCapacity_InfiniteAmmo(ITEM_BOMB) ||
          (!RandoCapacity_HasIndependentBombchuBag() && RandoCapacity_InfiniteAmmo(ITEM_BOMBCHU)))) ||
        (upgrade == UPG_DEKU_STICKS && RandoCapacity_InfiniteAmmo(ITEM_DEKU_STICK)) ||
        (upgrade == UPG_DEKU_NUTS && RandoCapacity_InfiniteAmmo(ITEM_DEKU_NUT)))
        return gUpgradeCapacities[upgrade][3];
    return RandoCapacity_HasBag(upgrade) ? gUpgradeCapacities[upgrade][CUR_UPG_VALUE(upgrade)] : 0;
}

static inline int RandoCapacity_BombchuCapacity(void) {
    const int independent = gSaveContext.save.shipSaveInfo.nei.comboObtained[FC_OOT_BOMBCHU_CAPACITY];
    return independent > 0 ? (independent > 50 ? 50 : independent) : RandoCapacity_Current(UPG_BOMB_BAG);
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

static inline void RandoCapacity_RepairInfinite(void) {
    const int items[] = { ITEM_BOW, ITEM_BOMB, ITEM_BOMBCHU, ITEM_DEKU_STICK, ITEM_DEKU_NUT };
    const int upgrades[] = { UPG_QUIVER, UPG_BOMB_BAG, UPG_BOMB_BAG, UPG_DEKU_STICKS, UPG_DEKU_NUTS };
    int i;
    for (i = 0; i < 5; ++i)
        if (RandoCapacity_InfiniteAmmo(items[i]))
            AMMO(items[i]) =
                items[i] == ITEM_BOMBCHU ? RandoCapacity_BombchuCapacity() : RandoCapacity_Current(upgrades[i]);
    if (RandoCapacity_InfiniteMagic())
        gSaveContext.save.saveInfo.playerData.magic = gSaveContext.magicCapacity;
    if (RandoCapacity_InfiniteWallet()) {
        gSaveContext.save.saveInfo.playerData.rupees = RandoCapacity_Current(UPG_WALLET);
        gSaveContext.rupeeAccumulator = 0;
    }
}

#endif
