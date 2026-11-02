#ifndef COMBO_LANTERN_RECEIPT_ICON_H
#define COMBO_LANTERN_RECEIPT_ICON_H

/* Capture the caught fire, rather than consulting the equipped lantern again
 * while the textbox is displayed. All five authored icons are RGBA32 32x32. */
static inline const char* ComboLanternReceiptIcon(unsigned fireType) {
    switch (fireType) {
        case 1:
            return "__OTR__textures/icon_item_custom/gItemIconLanternFireTex";
        case 2:
            return "__OTR__textures/icon_item_custom/gItemIconLanternBlueTex";
        case 3:
            return "__OTR__textures/icon_item_custom/gItemIconLanternPoeTex";
        case 4:
            return "__OTR__textures/icon_item_custom/gItemIconLanternGreenTex";
        default:
            return "__OTR__textures/icon_item_custom/gItemIconLanternTex";
    }
}

#endif
