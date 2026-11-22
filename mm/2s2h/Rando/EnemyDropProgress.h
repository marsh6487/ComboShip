#pragma once

#include "Types.h"
#include <cstdint>

namespace Rando::EnemyDropProgress {

// MM enemy drop check IDs form a contiguous, saved range.
static_assert(RC_ENEMY_DROP_WOLFOS - RC_ENEMY_DROP_ALIEN < 64);
constexpr uint64_t DefeatedBit(RandoCheckId check) {
    if (check < RC_ENEMY_DROP_ALIEN || check > RC_ENEMY_DROP_WOLFOS)
        return 0;
    return uint64_t{ 1 } << (check - RC_ENEMY_DROP_ALIEN);
}

} // namespace Rando::EnemyDropProgress
