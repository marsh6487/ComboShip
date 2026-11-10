#pragma once
#include "Types.h"
extern "C" {
#include "z64scene.h"
#include "z64actor.h"
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
struct Source {
    RandoCheckId check;
    RandoCheckType type;
    s16 scene;
    s8 setup;
    s8 room;
    s16 index;
    s8 slice;
    s16 actorId;
};
inline constexpr Source Sources[] = {
#define MM_ENVIRONMENT_PLACEMENT(id, type, scene, setup, room, index, slice, actorId) \
    { id, type, scene, setup, room, index, slice, actorId },
#include "MMEnvironmentPlacements.inc"
#undef MM_ENVIRONMENT_PLACEMENT
};
inline constexpr RandoOptionId OptionForType(RandoCheckType type) {
    switch (type) {
        case RCTYPE_ICICLE:
            return RO_SHUFFLE_ICICLE_DROPS;
        case RCTYPE_ROCK:
            return RO_SHUFFLE_ROCK_DROPS;
        case RCTYPE_BOULDER:
            return RO_SHUFFLE_BOULDER_DROPS;
        case RCTYPE_RED_BOULDER:
            return RO_SHUFFLE_RED_BOULDER_DROPS;
        case RCTYPE_SOIL:
            return RO_SHUFFLE_SOIL_DROPS;
        default:
            return RO_MAX;
    }
}
inline constexpr bool RequiresIronBoots(RandoCheckId check) {
    switch (check) {
#define MM_IRON_BOOT_ROCK(id) case id:
#include "MMEnvironmentIronBootRocks.inc"
#undef MM_IRON_BOOT_ROCK
        return true;
        default:
            return false;
    }
}
inline constexpr bool SourceEligible(RandoCheckId check, bool hasOotGameplay, bool startsWithIronBoots) {
    return !RequiresIronBoots(check) || hasOotGameplay || startsWithIronBoots;
}
template <class Save> bool SourceEligibleForSeed(RandoCheckId check, bool hasOotGameplay, const Save& save) {
    bool startsWithIronBoots = false;
    for (auto item : save.randoStartingItems)
        if (item == RI_OOT_IRON_BOOTS)
            startsWithIronBoots = true;
    return SourceEligible(check, hasOotGameplay, startsWithIronBoots);
}
inline constexpr s8 NormalizeSetup(s16 scene, s32 layer) {
    s8 extra = 0;
    switch (scene) {
        case SCENE_IKANA:
            extra = 3;
            break;
        case SCENE_13HUBUKINOMITI:
        case SCENE_16GORON_HOUSE:
        case SCENE_33ZORACITY:
        case SCENE_30GYOSON:
        case SCENE_31MISAKI:
        case SCENE_DEKU_KING:
        case SCENE_14YUKIDAMANOMITI:
        case SCENE_12HAKUGINMAE:
        case SCENE_GORONRACE:
        case SCENE_BACKTOWN:
            extra = 1;
            break;
        case SCENE_21MITURINMAE:
            extra = 2;
            break;
        default:
            break;
    }
    return layer > 0 && layer <= extra ? layer : 0;
}
inline constexpr RandoCheckId Identify(s16 scene, s8 setup, s8 room, s16 index, s8 slice, s16 actorId) {
    for (const auto& p : Sources)
        if (p.scene == scene && p.setup == setup && p.room == room && p.index == index && p.slice == slice &&
            p.actorId == actorId)
            return p.check;
    return RC_UNKNOWN;
}
inline constexpr RandoCheckId IdentifyIcicle(s16 scene, s8 room, s16 index) {
    return Identify(scene, 0, room, index, 0, ACTOR_BG_ICICLE);
}
inline constexpr bool MatchesParams(s16 actorId, s16 params) {
    switch (actorId) {
        case ACTOR_EN_ISHI:
            return (params & 1) == 0;
        case ACTOR_BG_ICICLE:
            return (params & 3) == 0 || (params & 3) == 3;
        case ACTOR_OBJ_MURE2:
            return (params & 3) == 2;
        case ACTOR_OBJ_SWPRIZE:
        case ACTOR_OBJ_BOMBIWA:
        case ACTOR_OBJ_HAMISHI:
            return true;
        default:
            return false;
    }
}
template <class Check> bool CanDrop(const Check& check) {
    return check.shuffled && !check.obtained && !check.cycleObtained && !check.eligible;
}
} // namespace Rando::MMEnvironment
