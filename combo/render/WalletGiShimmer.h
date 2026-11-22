#pragma once

#include <stdint.h>

typedef enum {
    CW_WALLET_CHILD,
    CW_WALLET_ADULT,
    CW_WALLET_GIANT,
    CW_WALLET_TYCOON,
    CW_WALLET_ZFG,
    CW_WALLET_INFINITE,
    CW_WALLET_COUNT,
} ComboWalletGiTier;

static inline int ComboWalletGi_ShimmerColor(int tier, uint8_t color[4]) {
    static const uint8_t palettes[CW_WALLET_COUNT][4] = {
        { 177, 239, 119, 255 }, // Child
        { 105, 197, 255, 255 }, // Adult
        { 255, 138, 98, 255 },  // Giant
        { 213, 128, 255, 255 }, // Tycoon
        { 255, 216, 122, 255 }, // ZFG
        { 163, 243, 232, 255 }, // Infinite
    };
    if (tier < 0 || tier >= CW_WALLET_COUNT || color == 0) {
        return 0;
    }
    for (int i = 0; i < 4; ++i) {
        color[i] = palettes[tier][i];
    }
    return 1;
}
