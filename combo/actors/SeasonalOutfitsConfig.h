#pragma once

#include "SeasonalOutfits.h"
#include "libultraship/bridge/consolevariablebridge.h"

namespace SeasonalOutfits {
inline void MigrateOutfitSetConfig() {
    constexpr const char* legacySet = "gEnhancements.SeasonalOutfits.Set";
    constexpr const char* legacyIndex = "gEnhancements.SeasonalOutfits.SetIndex";

    if (!CVarExists(SEASONAL_OUTFITS_SET_CVAR) && CVarExists(legacySet)) {
        CVarCopy(legacySet, SEASONAL_OUTFITS_SET_CVAR);
    }
    // The enable switch is a JSON value. Remove its old child keys before saving.
    CVarClear(legacySet);
    CVarClear(legacyIndex);
}
} // namespace SeasonalOutfits
