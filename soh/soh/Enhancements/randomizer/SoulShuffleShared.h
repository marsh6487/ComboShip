#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int SOH_ShareSoulReceipt(int item);
void SOH_GrantSharedSoulForSlot(const char* name, int sourceSlot, uint32_t sourceMask);
const char* SOH_ExtractSharedSoulState(void);
void SOH_ApplySharedSoulState(const char* state);
#ifdef __cplusplus
}
#endif
