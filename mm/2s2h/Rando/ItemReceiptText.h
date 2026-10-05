#pragma once
#include "Rando/Types.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#ifdef COMBO_BUILD
#include "ComboExport.h"
#endif

namespace Rando {
bool ApplyItemReceiptText(RandoItemId id, CustomMessage::Entry& entry);
// Read-only seed information; returns no text while the shared setting is disabled.
bool MapCompassInfoEnabled();
std::string GetDungeonMapCompassInfo(int32_t dungeon, bool compass);
#ifdef COMBO_BUILD
bool ApplyForeignItemReceiptText(const char* itemName, CustomMessage::Entry& entry, RandoCheckId check = RC_UNKNOWN);
#endif
// Append attribution after the description while retaining its encoding.
void AppendReceiptSource(CustomMessage::Entry& entry, const std::string& source);
} // namespace Rando

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT int32_t MM_GetSeedItemIconInfo(const char* name, CwItemIconInfo* out);
extern "C" COMBO_EXPORT int32_t MM_GetDungeonRewardIconInfo(int32_t dungeon, CwItemIconInfo* out);
extern "C" COMBO_EXPORT int32_t MM_GetDungeonRewardName(int32_t dungeon, char* buffer, uint32_t capacity);
#endif
