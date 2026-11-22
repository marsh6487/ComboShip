#pragma once
#include "CrossDungeonRuntime.h"

namespace ComboRando::CrossDungeon {
// OoT projects both shuffle layers before exposing an arrival. The MM tab can
// display the physical source instead of the intermediate native override.
using VisitMmTrackerArrival = void (*)(void* context, const char* source, int32_t destination);
using ReadMmTrackerArrivals = void (*)(int32_t slot, int32_t reveal, VisitMmTrackerArrival visit, void* context);

// This is the load pipeline's projection, not a traversal of its route graph.
// Viewing a tracker must never queue a handoff, materialize a grotto, or own a checkpoint.
inline Endpoint ComposeTrackerDestination(const RuntimeState& state, bool enabled, Endpoint physical,
                                          int32_t nativeOverride) {
    if (enabled && physical.game == 0 && state.Enabled()) {
        if (const auto before = state.Lookup(Phase::BeforeNative, physical.index))
            return before->to;
        if (const auto after = state.Lookup(Phase::AfterNative, nativeOverride))
            return after->to;
    }
    return { physical.game, nativeOverride };
}
inline const Dungeon* FindTrackerPortal(const std::vector<Dungeon>& catalogue, Endpoint endpoint) {
    for (const auto& portal : catalogue)
        if (portal.entrance == endpoint || (portal.hasReverse && portal.exit == endpoint) ||
            std::find(portal.escapeExits.begin(), portal.escapeExits.end(), endpoint) != portal.escapeExits.end())
            return &portal;
    return nullptr;
}
} // namespace ComboRando::CrossDungeon
