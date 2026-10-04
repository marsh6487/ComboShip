#pragma once
#include "Rando/Types.h"
#include "2s2h/CustomMessage/CustomMessage.h"

namespace Rando {
bool ApplyItemReceiptText(RandoItemId id, CustomMessage::Entry& entry);
#ifdef COMBO_BUILD
bool ApplyForeignItemReceiptText(const char* itemName, CustomMessage::Entry& entry, RandoCheckId check = RC_UNKNOWN);
#endif
// Append attribution after the description while retaining its encoding.
void AppendReceiptSource(CustomMessage::Entry& entry, const std::string& source);
} // namespace Rando
