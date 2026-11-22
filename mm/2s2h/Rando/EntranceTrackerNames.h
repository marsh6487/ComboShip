#pragma once

#ifdef COMBO_BUILD
#include <string>

namespace ComboRando::CrossDungeon {
struct Dungeon;
} // namespace ComboRando::CrossDungeon

namespace Rando::EntranceTracker {
std::string PortalSourceName(const ComboRando::CrossDungeon::Dungeon& portal);
} // namespace Rando::EntranceTracker
#endif
