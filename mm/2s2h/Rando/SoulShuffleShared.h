#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int MM_ShareSoulReceipt(int item);
void MM_GrantSharedSoulForSlot(const char* name, int sourceSlot, uint32_t sourceMask);
const char* MM_ExtractSharedSoulState(void);
void MM_ApplySharedSoulState(const char* state);
#ifdef __cplusplus
}
#endif
