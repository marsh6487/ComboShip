#ifndef COMBO_ITEM_ICON_OWNERSHIP_H
#define COMBO_ITEM_ICON_OWNERSHIP_H
#include <string.h>

#define COMBO_IKANA_SHIELD_ICON "__OTR__icon_item_static_yar/gItemIconMirrorShieldTex"

// This is MM's Mirror Shield, independent of which game's catalog carries its
// extended-equipment identity. Cold aliases must never choose OoT's shield.
static inline int ComboIconIsIkanaShieldName(const char* name) {
    static const char* names[] = { "Shield of Ikana",      "Ikana Mirror Shield",  "Ikana Shield",
                                   "MM Mirror Shield",     "Mirror Shield (MM)",   "Mirror Shield (Ikana)",
                                   "Shield of Ikana (MM)", "Shield of Ikana (OOT)" };
    if (!name)
        return 0;
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        if (!strcmp(name, names[i]))
            return 1;
    return 0;
}

static inline int ComboIconUsesMmOwnership(const char* path) {
    return path && !strcmp(path, COMBO_IKANA_SHIELD_ICON);
}
#endif
