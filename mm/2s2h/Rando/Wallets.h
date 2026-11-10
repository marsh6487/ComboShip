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
inline bool Give(RandoItemId item) {
    if (item != RI_WALLET_CHILD)
        return false;
    if (!Flags_GetRandoInf(RANDO_INF_WALLET_CHILD)) {
        Flags_SetRandoInf(RANDO_INF_WALLET_CHILD);
        gSaveContext.rupeeAccumulator = CUR_CAPACITY(UPG_WALLET) - gSaveContext.save.saveInfo.playerData.rupees;
    }
    return true;
}
inline bool Remove(RandoItemId item) {
    if (item != RI_WALLET_CHILD)
        return false;
    Flags_ClearRandoInf(RANDO_INF_WALLET_CHILD);
    RandoCapacity_RepairWallet();
    return true;
}
} // namespace Rando::Wallets
