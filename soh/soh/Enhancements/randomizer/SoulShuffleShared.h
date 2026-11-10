#pragma once
#include <stdint.h>
#include "../../../../combo/ComboExport.h"
#ifdef __cplusplus
extern "C" {
#endif
int SOH_ShareSoulReceipt(int item);
COMBO_EXPORT void SOH_GrantSharedSoulForSlot(const char* name, int sourceSlot, uint32_t sourceMask);
COMBO_EXPORT const char* SOH_ExtractSharedSoulState(void);
void SOH_ApplySharedSoulState(const char* state);
#ifdef __cplusplus
}
#endif
