#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace ComboRando {

// Test whether rewards sharing one native boss domain admit a collection order.
// The callback reports how many domain checks are reachable with exactly the
// selected rewards plus the caller's fixed progression assumptions. Reachability
// must be monotone as rewards are added. Previously used checks then remain in
// the reachable set, so its count minus the number collected is unused capacity.
// OoT has at most eight movable dungeon rewards: at most 256 inventory states.
template <class ReachableCount> bool HasNativeRewardPlacement(size_t rewardCount, ReachableCount reachableCount) {
    if (rewardCount > 8)
        return false;
    const auto complete = static_cast<uint16_t>((1u << rewardCount) - 1u);
    std::array<int8_t, 256> memo;
    memo.fill(-1);
    const auto visit = [&](auto&& self, uint16_t collected) -> bool {
        if (collected == complete)
            return true;
        if (memo[collected] >= 0)
            return memo[collected] != 0;
        memo[collected] = 0;
        if (reachableCount(collected) <= static_cast<size_t>(std::popcount(collected)))
            return false;
        for (size_t i = 0; i < rewardCount; ++i) {
            const auto bit = static_cast<uint16_t>(1u << i);
            if (!(collected & bit) && self(self, static_cast<uint16_t>(collected | bit))) {
                memo[collected] = 1;
                return true;
            }
        }
        return false;
    };
    return visit(visit, 0);
}

} // namespace ComboRando
