#pragma once
#include "Types.h"
extern "C" {
#include "z64scene.h"
}
namespace Rando::MMEnvironment {
struct Placement {
    RandoCheckId check;
    s16 scene;
    s8 room;
    s16 index;
    RandoRegionId region;
};
inline constexpr Placement Icicles[] = {
#define MM_ICICLE(id, scene, room, index, region) { id, scene, room, index, region },
#include "MMEnvironmentIcicles.inc"
#undef MM_ICICLE
};
inline constexpr RandoCheckId IdentifyIcicle(s16 scene, s8 room, s16 index) {
    for (const auto& placement : Icicles)
        if (placement.scene == scene && placement.room == room && placement.index == index)
            return placement.check;
    return RC_UNKNOWN;
}
template <class Check> bool CanDrop(const Check& check) {
    // Permanent receipt is authoritative even after Song of Time clears cycleObtained.
    return check.shuffled && !check.obtained && !check.cycleObtained && !check.eligible;
}
} // namespace Rando::MMEnvironment
