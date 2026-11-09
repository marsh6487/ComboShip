#pragma once
#include <stdint.h>
#include "../../../combo/ComboExport.h"
#ifdef __cplusplus
extern "C" {
#endif
int MM_ShareSoulReceipt(int item);
COMBO_EXPORT void MM_GrantSharedSoulForSlot(const char* name, int sourceSlot, uint32_t sourceMask);
COMBO_EXPORT const char* MM_ExtractSharedSoulState(void);
void MM_ApplySharedSoulState(const char* state);
#ifdef __cplusplus
}
#endif
