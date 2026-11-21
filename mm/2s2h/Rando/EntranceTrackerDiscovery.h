#pragma once

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <span>

namespace Rando::EntranceTracker {
template <typename Save> std::span<const uint16_t> Discovered(const Save& save) {
    return { save.discoveredEntrances,
             std::min<std::size_t>(save.discoveredEntranceCount, std::size(save.discoveredEntrances)) };
}

template <typename Save> void Discover(Save& save, int32_t source) {
    if (source < 0 || source > UINT16_MAX || save.discoveredEntranceCount >= std::size(save.discoveredEntrances))
        return;
    const auto entries = Discovered(save);
    if (std::find(entries.begin(), entries.end(), source) == entries.end())
        save.discoveredEntrances[save.discoveredEntranceCount++] = static_cast<uint16_t>(source);
}
} // namespace Rando::EntranceTracker
