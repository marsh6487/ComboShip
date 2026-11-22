#pragma once

#include "randomizerEnums.h"

namespace OotWallet {

inline constexpr int NativeFloor(bool tycoonEnabled) {
    return tycoonEnabled ? 3 : 2;
}

inline constexpr RandomizerGet NextUpgrade(int tier, bool hasWallet, bool tycoonEnabled, bool zfgEnabled, bool hasZfg,
                                           bool infiniteEnabled, bool hasInfinite) {
    if (!hasWallet)
        return RG_CHILD_WALLET;
    if (tier < 1)
        return RG_ADULT_WALLET;
    if (tier < 2)
        return RG_GIANT_WALLET;
    if (tier < NativeFloor(tycoonEnabled))
        return RG_TYCOON_WALLET;
    if (zfgEnabled && !hasZfg)
        return RG_ZFG_WALLET;
    if (infiniteEnabled && !hasInfinite)
        return RG_WALLET_INF;
    return RG_BLUE_RUPEE;
}

} // namespace OotWallet
