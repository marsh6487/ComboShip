#pragma once

#include "Types.h"
#include "../../../combo/render/WalletGiShimmer.h"

static inline int MM_WalletGi_ShimmerColor(RandoItemId id, uint8_t color[4]) {
    switch (id) {
        case RI_WALLET_CHILD:
            return ComboWalletGi_ShimmerColor(CW_WALLET_CHILD, color);
        case RI_WALLET_ADULT:
            return ComboWalletGi_ShimmerColor(CW_WALLET_ADULT, color);
        case RI_WALLET_GIANT:
            return ComboWalletGi_ShimmerColor(CW_WALLET_GIANT, color);
        case RI_WALLET_TYCOON:
            return ComboWalletGi_ShimmerColor(CW_WALLET_TYCOON, color);
        case RI_WALLET_ZFG:
            return ComboWalletGi_ShimmerColor(CW_WALLET_ZFG, color);
        case RI_INFINITE_WALLET:
            return ComboWalletGi_ShimmerColor(CW_WALLET_INFINITE, color);
        default:
            return 0;
    }
}
