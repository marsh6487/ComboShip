#include "soh/Enhancements/randomizer/RocsFeatherCycle.h"
#include "vanilla/ComboVanillaMode.h"
#include "mods/nei_save.h"
#include "functions.h"
#include "variables.h"
#include "macros.h"

void VanillaRocsFeather_Grant(void) {
    NeiSaveData* nei = Nei_Save();
    if (INV_CONTENT(ITEM_NAYRUS_LOVE) == ITEM_NAYRUS_LOVE)
        nei->separateRocsFlags |= 2;
    nei->separateRocsFlags |= 1;
    if (INV_CONTENT(ITEM_NAYRUS_LOVE) == ITEM_NONE)
        INV_CONTENT(ITEM_NAYRUS_LOVE) = ITEM_ROCS_FEATHER;
}

static uint8_t VanillaRocsOtherItem(void) {
    NeiSaveData* nei = Nei_Save();
    const uint8_t item = INV_CONTENT(ITEM_NAYRUS_LOVE);
    if (item == ITEM_NAYRUS_LOVE)
        nei->separateRocsFlags |= 2;
    if (item == ITEM_NAYRUS_LOVE && (nei->separateRocsFlags & 1))
        return ITEM_ROCS_FEATHER;
    if (item == ITEM_ROCS_FEATHER && (nei->separateRocsFlags & 2))
        return ITEM_NAYRUS_LOVE;
    return ITEM_NONE;
}

uint8_t Enhancement_GetNextNayrusItem() {
    if (Combo_IsVanilla())
        return VanillaRocsOtherItem();
    if (INV_CONTENT(ITEM_NAYRUS_LOVE) == ITEM_NAYRUS_LOVE && Flags_GetRandomizerInf(RAND_INF_OBTAINED_ROCS_FEATHER)) {
        return ITEM_ROCS_FEATHER;
    }
    if (INV_CONTENT(ITEM_NAYRUS_LOVE) == ITEM_ROCS_FEATHER && Flags_GetRandomizerInf(RAND_INF_OBTAINED_NAYRUS_LOVE)) {
        return ITEM_NAYRUS_LOVE;
    }
    return ITEM_NONE;
}

uint8_t Enhancement_GetPrevNayrusItem() {
    if (Combo_IsVanilla())
        return VanillaRocsOtherItem();
    if (INV_CONTENT(ITEM_NAYRUS_LOVE) == ITEM_NAYRUS_LOVE && Flags_GetRandomizerInf(RAND_INF_OBTAINED_ROCS_FEATHER)) {
        return ITEM_ROCS_FEATHER;
    }
    if (INV_CONTENT(ITEM_NAYRUS_LOVE) == ITEM_ROCS_FEATHER && Flags_GetRandomizerInf(RAND_INF_OBTAINED_NAYRUS_LOVE)) {
        return ITEM_NAYRUS_LOVE;
    }

    return ITEM_NONE;
}
