#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "soh/Enhancements/item-tables/ItemTableTypes.h"
#ifdef COMBO_BUILD
#include "rando/StartupCheckReceipt.h"
#ifdef __cplusplus
extern "C" {
#endif
bool ComboStartupOot_RestoreFrozenDraw(const CwStartupReceipt* receipt, GetItemEntry* entry);
bool ComboStartupOot_PrepareContextChange(void);
void ComboStartupOot_BeginFile(void);
bool ComboStartupOot_Capture(const GetItemEntry* entry, int32_t check);
bool ComboStartupOot_IsReceipt(const GetItemEntry* entry);
void ComboStartupOot_TextStarted(PlayState* play, const GetItemEntry* entry);
const char* ComboStartupOot_FrozenForeignName(const GetItemEntry* entry);
#ifdef __cplusplus
}
#endif
#endif
