#pragma once

#ifdef COMBO_BUILD
#include <string>
#include <string_view>

namespace ComboRando::CrossDungeon {
struct Dungeon;
} // namespace ComboRando::CrossDungeon

namespace Rando::EntranceTracker {
struct TrackerDoorway {
    std::string_view id;
    const char* source;
    const char* arrival;
};

inline const TrackerDoorway* NamedDoorway(std::string_view id) {
    constexpr static TrackerDoorway names[] = {
        { "mm:ocean-spider-house", "Oceanside Spider House doorway (Great Bay Coast)", "Oceanside Spider House" },
        { "mm:stonetower", "Stone Tower Temple doorway", "Stone Tower Temple (normal)" },
        { "mm:inverted", "Inverted Stone Tower Temple doorway", "Inverted Stone Tower Temple" },
        { "mm:beneath-well-front", "Beneath the Well front doorway (Ikana Canyon)", "Beneath the Well (front)" },
        { "mm:beneath-well-back", "Beneath the Well back doorway (Ikana Castle)", "Beneath the Well (castle side)" },
    };
    for (const auto& name : names)
        if (name.id == id)
            return &name;
    return nullptr;
}

std::string PortalSourceName(const ComboRando::CrossDungeon::Dungeon& portal);
} // namespace Rando::EntranceTracker
#endif
