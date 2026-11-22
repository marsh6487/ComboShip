#pragma once

#include "NeiGiPresentation.h"
#include "ComboCollectibleShimmer.h"
#include "combo/render/WalletGiShimmer.h"
#include "randomizerEnums.h"
#include "z64item.h"
#include <libultraship/bridge/consolevariablebridge.h>

static inline int OotWalletGi_Tier(const GetItemEntry* entry) {
    // Use the presentation identity, since trap disguises can use a
    // different draw entry from the item that will actually be granted.
    if (entry->drawModIndex == MOD_RANDOMIZER) {
        switch (entry->drawItemId) {
            case RG_CHILD_WALLET:
                return CW_WALLET_CHILD;
            case RG_TYCOON_WALLET:
                return CW_WALLET_TYCOON;
            case RG_ZFG_WALLET:
                return CW_WALLET_ZFG;
            case RG_WALLET_INF:
                return CW_WALLET_INFINITE;
        }
    } else if (entry->drawModIndex == MOD_NONE) {
        switch (entry->drawItemId) {
            case ITEM_WALLET_ADULT:
                return CW_WALLET_ADULT;
            case ITEM_WALLET_GIANT:
                return CW_WALLET_GIANT;
        }
    }
    return -1;
}

static inline int OotWalletGi_Shimmer(int tier, uint8_t color[4]) {
    return CVarGetInteger(CVAR_NEI_GI_EFFECTS, 0) && ComboWalletGi_ShimmerColor(tier, color) ? CW_ITEM_SHIMMER_TINTED
                                                                                             : CW_ITEM_SHIMMER_NONE;
}
