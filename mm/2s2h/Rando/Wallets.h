#pragma once
#include "Rando/Rando.h"
extern "C" {
#include "functions.h"
}

namespace Rando::Wallets {
// Keep shared wallet ABI tiers 0..3 unchanged. Shared Wallets already uses
// the normal base wallet in OoT; normalize MM's generated snapshot likewise.
inline void NormalizeOptions(RandoSaveInfo& save, bool sharedWallets) {
    if (sharedWallets)
        save.randoSaveOptions[RO_SHUFFLE_CHILD_WALLET] = RO_GENERIC_OFF;
}
inline unsigned HighestNativeUpgrade() {
    return RANDO_SAVE_OPTIONS[RO_SHUFFLE_TYCOON_WALLET] == RO_GENERIC_YES ? 3 : 2;
}

inline RandoItemId NextProgressiveItem() {
    if (RandoCapacity_HasZfgWallet()) {
        return RI_JUNK;
    }
    if (!RandoCapacity_HasWallet()) {
        return RI_WALLET_CHILD;
    }
    const auto tier = CUR_UPG_VALUE(UPG_WALLET);
    if (tier == 0) {
        return RI_WALLET_ADULT;
    }
    if (tier == 1) {
        return RI_WALLET_GIANT;
    }
    if (tier < HighestNativeUpgrade()) {
        return RI_WALLET_TYCOON;
    }
    return RANDO_SAVE_OPTIONS[RO_SHUFFLE_ZFG_WALLET] == RO_GENERIC_YES ? RI_WALLET_ZFG : RI_JUNK;
}

inline bool Give(RandoItemId item) {
    if (item == RI_WALLET_ZFG) {
        if (!Flags_GetRandoInf(RANDO_INF_WALLET_ZFG)) {
            // Preserve the earned native tier and supply its floor for a direct grant.
            const unsigned precedingTier = HighestNativeUpgrade();
            if (CUR_UPG_VALUE(UPG_WALLET) < precedingTier) {
                Inventory_ChangeUpgrade(UPG_WALLET, precedingTier);
            }
            Flags_SetRandoInf(RANDO_INF_WALLET_ZFG);
            gSaveContext.rupeeAccumulator = CUR_CAPACITY(UPG_WALLET) - gSaveContext.save.saveInfo.playerData.rupees;
        }
        return true;
    }
    if (item != RI_WALLET_CHILD)
        return false;
    if (!Flags_GetRandoInf(RANDO_INF_WALLET_CHILD)) {
        Flags_SetRandoInf(RANDO_INF_WALLET_CHILD);
        gSaveContext.rupeeAccumulator = CUR_CAPACITY(UPG_WALLET) - gSaveContext.save.saveInfo.playerData.rupees;
    }
    return true;
}
inline bool Remove(RandoItemId item) {
    if (item == RI_WALLET_ZFG) {
        if (Flags_GetRandoInf(RANDO_INF_WALLET_ZFG)) {
            Flags_ClearRandoInf(RANDO_INF_WALLET_ZFG);
            const int capacity = CUR_CAPACITY(UPG_WALLET);
            auto& balance = gSaveContext.save.saveInfo.playerData.rupees;
            if (balance > capacity) {
                balance = capacity;
            }
            const int room = capacity - balance;
            if (gSaveContext.rupeeAccumulator > room) {
                gSaveContext.rupeeAccumulator = room;
            }
        }
        return true;
    }
    if (item != RI_WALLET_CHILD)
        return false;
    Flags_ClearRandoInf(RANDO_INF_WALLET_CHILD);
    RandoCapacity_RepairWallet();
    return true;
}
} // namespace Rando::Wallets
