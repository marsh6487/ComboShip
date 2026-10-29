#pragma once

#include <stdbool.h>
#include "../item-tables/ItemTableTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

GetItemCategory Randomizer_AdjustItemCategory(GetItemEntry item);
// Chest-only appearance; keep item/progression categories and other containers unchanged.
bool Randomizer_IsChestStrayFairy(GetItemEntry item);

#ifdef __cplusplus
}
#endif
