#pragma once

// Native item hint initialization runs before item-table construction. Use the
// catalogue's native EN/FR/DE names rather than reading an uninitialized Item.
// Every clarity falls back to this individual name; no variant RNG is consumed.
namespace Rando::SoulHints {
template <class HintTable> inline void Register(HintTable& table) {
#define SOUL_HINT(item, hint, english, french, german) table[hint] = HintText(CustomMessage(english, german, french));
#include "SoulHintCatalog.inc"
#undef SOUL_HINT
}
} // namespace Rando::SoulHints
