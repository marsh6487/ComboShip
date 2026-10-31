#ifndef COMBO_BOSS_SOUL_RECEIPT_ICON_H
#define COMBO_BOSS_SOUL_RECEIPT_ICON_H

// Include after MM's RandoItemId declarations. The map-screen soul skull is
// RGBA16 16x16; the receipt ABI accepts RGBA32 or IA8. Keep notification/GI
// artwork unchanged and use MM's established Captain skull receipt substitute
// (RGBA32 32x32) for the generic soul family, including Majora.
static inline int ComboMmSoulReceiptUsesSkull(int item) {
    return item == RI_SOUL_BOSS_MAJORA || (item >= RI_SOUL_ENEMY_ALIEN && item <= RI_SOUL_ENEMY_WOLFOS) ||
           (item >= RI_SOUL_OOT_BOSS_BARINADE && item <= RI_SOUL_OOT_BOSS_VOLVAGIA);
}

static inline const char* ComboMmSoulReceiptIconPath(int item, const char* notificationIcon,
                                                     const char* alignedSkullIcon) {
    return ComboMmSoulReceiptUsesSkull(item) ? alignedSkullIcon : notificationIcon;
}

#endif
