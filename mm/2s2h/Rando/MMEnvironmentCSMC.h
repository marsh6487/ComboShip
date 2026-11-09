#pragma once
#include "Rando/MMEnvironment.h"
#include "combo/rando/ContainerCategoryColors.h"
#include <cmath>
namespace Rando::MMEnvironmentCSMC {
using Category = ComboRando::ContainerCategoryColors::Category;
inline constexpr bool IsEnvironmentalCheck(RandoCheckId check) {
    switch (check) {
#define MM_ICICLE(id, scene, room, index, region) case id:
#include "Rando/MMEnvironmentIcicles.inc"
#undef MM_ICICLE
#define MM_ENVIRONMENT_CHECK(id, type, scene, region) case id:
#include "Rando/MMEnvironmentChecks.inc"
#undef MM_ENVIRONMENT_CHECK
        return true;
        default:
            return false;
    }
}
template <class Check> bool CanMark(RandoCheckId id, const Check& check, bool pending, bool active) {
    // Saved shuffled ownership serves the loaded seed, just as the drop hook
    // does. Generation-menu choices must not change a loaded seed's sources.
    return IsEnvironmentalCheck(id) && MMEnvironment::CanDrop(check) && !check.skipped && !pending && !active;
}
inline constexpr Category CategoryForType(RandoItemType type) {
    switch (type) {
        case RITYPE_JUNK:
            return Category::Junk;
        case RITYPE_LESSER:
            return Category::Lesser;
        case RITYPE_HEALTH:
            return Category::Health;
        case RITYPE_BOSS_KEY:
            return Category::BossKey;
        case RITYPE_SMALL_KEY:
            return Category::SmallKey;
        case RITYPE_SKULLTULA_TOKEN:
            return Category::Token;
        case RITYPE_MASK:
            return Category::Mask;
        case RITYPE_STRAY_FAIRY:
            return Category::StrayFairy;
        default:
            return Category::Major;
    }
}
struct Geometry {
    float height;
    float scale;
    float spacing;
};
inline Geometry GeometryFor(const Actor& actor) {
    float reference = 0.1f, height = 25.0f, scale = 0.6f;
    switch (actor.id) {
        case ACTOR_BG_ICICLE:
            height = 135.0f;
            scale = 0.8f;
            break;
        case ACTOR_OBJ_BOMBIWA:
            height = 150.0f;
            scale = 1.5f;
            break;
        case ACTOR_OBJ_HAMISHI:
            reference = 0.4f;
            height = 90.0f;
            scale = 1.2f;
            break;
        // Swprize is an invisible reward source; its scale is not the soil
        // patch's size. Separate each exact native reward slice on that patch.
        case ACTOR_OBJ_SWPRIZE:
            return { 6.0f, 0.45f, 18.0f };
        default:
            break;
    }
    // Keep the billboard above the native cylinder, so opaque rock/ice
    // geometry cannot hide the marker at its center. Scale follows the source.
    const float ratio = std::fabs(actor.scale.y) / reference;
    return { height * ratio, scale * ratio, 0.0f };
}
} // namespace Rando::MMEnvironmentCSMC
