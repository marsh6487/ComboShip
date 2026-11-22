#pragma once

#include <cstdint>

namespace ComboRando::SoulTracker {

enum class Family : int32_t { Boss, Enemy, Npc, Animal, Misc, Bean, Count };

// Each game visits rows from the requested resident randomizer save. It
// returns zero when that save is unavailable. Names belong to the game module.
using VisitRow = void (*)(void* context, Family family, const char* name, int32_t owned);
using ReadSave = int32_t (*)(int32_t slot, VisitRow visit, void* context);

enum class FirstSlainStatus : int32_t { Unrecorded, Defeated, Collected };

constexpr FirstSlainStatus FirstSlainProgress(bool defeated, bool collected) {
    return collected  ? FirstSlainStatus::Collected
           : defeated ? FirstSlainStatus::Defeated
                      : FirstSlainStatus::Unrecorded;
}

// The separate reader keeps the soul ownership callback compatible with existing modules.
using VisitFirstSlain = void (*)(void* context, const char* name, FirstSlainStatus status);
using ReadFirstSlain = int32_t (*)(int32_t slot, VisitFirstSlain visit, void* context);

} // namespace ComboRando::SoulTracker
