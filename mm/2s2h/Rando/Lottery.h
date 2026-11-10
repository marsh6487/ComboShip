#pragma once
#include "Types.h"

namespace Rando::Lottery {
// DAY/CURRENT_DAY remains native, including the midnight-to-dawn portion of a night.
inline constexpr RandoCheckId CheckForDay(bool separateNights, int day) {
    if (day < 1 || day > 3)
        return RC_UNKNOWN;
    if (!separateNights || day == 1)
        return RC_CLOCK_TOWN_WEST_LOTTERY;
    return day == 2 ? RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2 : RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3;
}
inline constexpr bool IsExtraNight(RandoCheckId check) {
    return check == RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_2 || check == RC_CLOCK_TOWN_WEST_LOTTERY_NIGHT_3;
}
inline constexpr bool IsEnabled(RandoCheckId check, bool separateNights) {
    return separateNights || !IsExtraNight(check);
}
template <class Check> bool QueueReward(Check& check, bool separateNights) {
    // Independent rewards are permanent checks; cycle flags must not re-arm them.
    // Legacy mode retains the existing repeat/junk behavior of the native queue.
    if (separateNights && (check.obtained || check.cycleObtained || check.eligible))
        return false;
    check.eligible = true;
    return true;
}
} // namespace Rando::Lottery
