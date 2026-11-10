#pragma once
#include "CrossEntranceMmCatalogue.h"

// Include after MM's native entrance/region enums. The 40 paired boundaries are
// the reversible outdoor exits in Rando/Logic/Regions/{Central,East,North,South,
// West,MilkRoad,TerminaField}. Source conditions remain owned by that native graph.
// Authored ComboShip passages, owl/statue travel, save starts and Song of Soaring
// are separate travel mechanisms and are deliberately absent from this catalogue.
namespace ComboSoloMmOverworld {
using ComboRando::CrossDungeon::Dungeon;
using ComboRando::CrossDungeon::Pool;

struct Portal {
    const char* id;
    int32_t entrance, exit;
    RandoRegionId region, sourceRegion;
    int scene, sourceScene;
    const char* sourceLabel;
};

inline const std::array<Portal, 40>& Portals() {
    static const std::array<Portal, 40> portals = { {
        { "mm:overworld:west_clock_town:0", ENTRANCE(WEST_CLOCK_TOWN, 0), ENTRANCE(TERMINA_FIELD, 0),
          RR_CLOCK_TOWN_WEST, RR_TERMINA_FIELD, SCENE_ICHIBA, SCENE_00KEIKOKU, "Termina Field to West Clock Town" },
        { "mm:overworld:road_to_southern_swamp:0", ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 0), ENTRANCE(TERMINA_FIELD, 1),
          RR_ROAD_TO_SOUTHERN_SWAMP, RR_TERMINA_FIELD, SCENE_24KEMONOMITI, SCENE_00KEIKOKU,
          "Termina Field to Southern Swamp Road" },
        { "mm:overworld:great_bay_coast:0", ENTRANCE(GREAT_BAY_COAST, 0), ENTRANCE(TERMINA_FIELD, 2),
          RR_GREAT_BAY_COAST, RR_TERMINA_FIELD_BEFORE_GREAT_BAY_COAST, SCENE_30GYOSON, SCENE_00KEIKOKU,
          "Termina Field to Great Bay Coast" },
        { "mm:overworld:path_to_mountain_village:0", ENTRANCE(PATH_TO_MOUNTAIN_VILLAGE, 0), ENTRANCE(TERMINA_FIELD, 3),
          RR_PATH_TO_MOUNTAIN_VILLAGE_LOWER, RR_TERMINA_FIELD_BEFORE_PATH_TO_MOUNTAIN_VILLAGE, SCENE_13HUBUKINOMITI,
          SCENE_00KEIKOKU, "Termina Field to Mountain Village Path" },
        { "mm:overworld:road_to_ikana:0", ENTRANCE(ROAD_TO_IKANA, 0), ENTRANCE(TERMINA_FIELD, 4),
          RR_ROAD_TO_IKANA_FIELD_SIDE, RR_TERMINA_FIELD, SCENE_IKANAMAE, SCENE_00KEIKOKU,
          "Termina Field to Ikana Road" },
        { "mm:overworld:milk_road:0", ENTRANCE(MILK_ROAD, 0), ENTRANCE(TERMINA_FIELD, 5), RR_MILK_ROAD,
          RR_TERMINA_FIELD, SCENE_ROMANYMAE, SCENE_00KEIKOKU, "Termina Field to Milk Road" },
        { "mm:overworld:south_clock_town:1", ENTRANCE(SOUTH_CLOCK_TOWN, 1), ENTRANCE(TERMINA_FIELD, 6),
          RR_CLOCK_TOWN_SOUTH, RR_TERMINA_FIELD, SCENE_CLOCKTOWER, SCENE_00KEIKOKU,
          "Termina Field to South Clock Town" },
        { "mm:overworld:east_clock_town:0", ENTRANCE(EAST_CLOCK_TOWN, 0), ENTRANCE(TERMINA_FIELD, 7),
          RR_CLOCK_TOWN_EAST, RR_TERMINA_FIELD, SCENE_TOWN, SCENE_00KEIKOKU, "Termina Field to East Clock Town" },
        { "mm:overworld:north_clock_town:0", ENTRANCE(NORTH_CLOCK_TOWN, 0), ENTRANCE(TERMINA_FIELD, 8),
          RR_CLOCK_TOWN_NORTH, RR_TERMINA_FIELD, SCENE_BACKTOWN, SCENE_00KEIKOKU, "Termina Field to North Clock Town" },
        { "mm:overworld:east_clock_town:3", ENTRANCE(EAST_CLOCK_TOWN, 3), ENTRANCE(SOUTH_CLOCK_TOWN, 2),
          RR_CLOCK_TOWN_EAST, RR_CLOCK_TOWN_SOUTH, SCENE_TOWN, SCENE_CLOCKTOWER,
          "South Clock Town to East Clock Town (passage 1)" },
        { "mm:overworld:west_clock_town:2", ENTRANCE(WEST_CLOCK_TOWN, 2), ENTRANCE(SOUTH_CLOCK_TOWN, 3),
          RR_CLOCK_TOWN_WEST, RR_CLOCK_TOWN_SOUTH, SCENE_ICHIBA, SCENE_CLOCKTOWER,
          "South Clock Town to West Clock Town (passage 1)" },
        { "mm:overworld:north_clock_town:2", ENTRANCE(NORTH_CLOCK_TOWN, 2), ENTRANCE(SOUTH_CLOCK_TOWN, 4),
          RR_CLOCK_TOWN_NORTH, RR_CLOCK_TOWN_SOUTH, SCENE_BACKTOWN, SCENE_CLOCKTOWER,
          "South Clock Town to North Clock Town" },
        { "mm:overworld:west_clock_town:1", ENTRANCE(WEST_CLOCK_TOWN, 1), ENTRANCE(SOUTH_CLOCK_TOWN, 5),
          RR_CLOCK_TOWN_WEST, RR_CLOCK_TOWN_SOUTH, SCENE_ICHIBA, SCENE_CLOCKTOWER,
          "South Clock Town to West Clock Town (passage 2)" },
        { "mm:overworld:laundry_pool:0", ENTRANCE(LAUNDRY_POOL, 0), ENTRANCE(SOUTH_CLOCK_TOWN, 6),
          RR_CLOCK_TOWN_LAUNDRY, RR_CLOCK_TOWN_SOUTH, SCENE_ALLEY, SCENE_CLOCKTOWER,
          "South Clock Town to Laundry Pool" },
        { "mm:overworld:east_clock_town:1", ENTRANCE(EAST_CLOCK_TOWN, 1), ENTRANCE(SOUTH_CLOCK_TOWN, 7),
          RR_CLOCK_TOWN_EAST, RR_CLOCK_TOWN_SOUTH, SCENE_TOWN, SCENE_CLOCKTOWER,
          "South Clock Town to East Clock Town (passage 2)" },
        { "mm:overworld:north_clock_town:1", ENTRANCE(NORTH_CLOCK_TOWN, 1), ENTRANCE(EAST_CLOCK_TOWN, 5),
          RR_CLOCK_TOWN_NORTH, RR_CLOCK_TOWN_EAST, SCENE_BACKTOWN, SCENE_TOWN, "East Clock Town to North Clock Town" },
        { "mm:overworld:romani_ranch:0", ENTRANCE(ROMANI_RANCH, 0), ENTRANCE(MILK_ROAD, 1), RR_ROMANI_RANCH,
          RR_MILK_ROAD, SCENE_F01, SCENE_ROMANYMAE, "Milk Road to Romani Ranch" },
        { "mm:overworld:gorman_track:3", ENTRANCE(GORMAN_TRACK, 3), ENTRANCE(MILK_ROAD, 2), RR_GORMAN_TRACK_BACK,
          RR_MILK_ROAD_BEHIND_FENCE, SCENE_KOEPONARACE, SCENE_ROMANYMAE, "Milk Road fence to Gorman Track back" },
        { "mm:overworld:gorman_track:0", ENTRANCE(GORMAN_TRACK, 0), ENTRANCE(MILK_ROAD, 3), RR_GORMAN_TRACK_FRONT,
          RR_MILK_ROAD, SCENE_KOEPONARACE, SCENE_ROMANYMAE, "Milk Road to Gorman Track front" },
        { "mm:overworld:southern_swamp_poisoned:0", ENTRANCE(SOUTHERN_SWAMP_POISONED, 0),
          ENTRANCE(ROAD_TO_SOUTHERN_SWAMP, 1), RR_SOUTHERN_SWAMP_NORTH, RR_ROAD_TO_SOUTHERN_SWAMP, SCENE_20SICHITAI,
          SCENE_24KEMONOMITI, "Southern Swamp Road to Southern Swamp" },
        { "mm:overworld:deku_palace:0", ENTRANCE(DEKU_PALACE, 0), ENTRANCE(SOUTHERN_SWAMP_POISONED, 3),
          RR_DEKU_PALACE_OUTSIDE, RR_SOUTHERN_SWAMP_SOUTH, SCENE_22DEKUCITY, SCENE_20SICHITAI,
          "Southern Swamp to Deku Palace gate" },
        { "mm:overworld:deku_palace:5", ENTRANCE(DEKU_PALACE, 5), ENTRANCE(SOUTHERN_SWAMP_POISONED, 4),
          RR_DEKU_PALACE_OUTSIDE, RR_SOUTHERN_SWAMP_SOUTH_UPPER, SCENE_22DEKUCITY, SCENE_20SICHITAI,
          "Southern Swamp to Deku Palace upper passage" },
        { "mm:overworld:woodfall:0", ENTRANCE(WOODFALL, 0), ENTRANCE(SOUTHERN_SWAMP_POISONED, 2), RR_WOODFALL,
          RR_SOUTHERN_SWAMP_SOUTH_UPPER, SCENE_21MITURINMAE, SCENE_20SICHITAI, "Southern Swamp to Woodfall" },
        { "mm:overworld:woods_of_mystery:0", ENTRANCE(WOODS_OF_MYSTERY, 0), ENTRANCE(SOUTHERN_SWAMP_POISONED, 7),
          RR_WOODS_OF_MYSTERY, RR_SOUTHERN_SWAMP_NEAR_WOODS, SCENE_26SARUNOMORI, SCENE_20SICHITAI,
          "Southern Swamp to Woods of Mystery" },
        { "mm:overworld:mountain_village_winter:6", ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 6),
          ENTRANCE(PATH_TO_MOUNTAIN_VILLAGE, 1), RR_MOUNTAIN_VILLAGE, RR_PATH_TO_MOUNTAIN_VILLAGE_UPPER,
          SCENE_10YUKIYAMANOMURA, SCENE_13HUBUKINOMITI, "Mountain Village Path to Mountain Village" },
        { "mm:overworld:path_to_goron_village_winter:0", ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 0),
          ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 2), RR_PATH_TO_GORON_VILLAGE, RR_MOUNTAIN_VILLAGE, SCENE_17SETUGEN,
          SCENE_10YUKIYAMANOMURA, "Mountain Village to Goron Village Path" },
        { "mm:overworld:goron_village_winter:0", ENTRANCE(GORON_VILLAGE_WINTER, 0),
          ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 1), RR_GORON_VILLAGE, RR_PATH_TO_GORON_VILLAGE, SCENE_11GORONNOSATO,
          SCENE_17SETUGEN, "Goron Village Path to Goron Village" },
        { "mm:overworld:goron_racetrack:0", ENTRANCE(GORON_RACETRACK, 0), ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 2),
          RR_GORON_RACETRACK, RR_PATH_TO_GORON_VILLAGE, SCENE_GORONRACE, SCENE_17SETUGEN,
          "Goron Village Path to Goron Racetrack" },
        { "mm:overworld:path_to_snowhead:0", ENTRANCE(PATH_TO_SNOWHEAD, 0), ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 4),
          RR_PATH_TO_SNOWHEAD_LOWER, RR_MOUNTAIN_VILLAGE, SCENE_14YUKIDAMANOMITI, SCENE_10YUKIYAMANOMURA,
          "Mountain Village to Snowhead Path" },
        { "mm:overworld:snowhead:0", ENTRANCE(SNOWHEAD, 0), ENTRANCE(PATH_TO_SNOWHEAD, 1), RR_SNOWHEAD_NEAR_PATH,
          RR_PATH_TO_SNOWHEAD_UPPER, SCENE_12HAKUGINMAE, SCENE_14YUKIDAMANOMITI, "Snowhead Path to Snowhead" },
        { "mm:overworld:zora_cape:0", ENTRANCE(ZORA_CAPE, 0), ENTRANCE(GREAT_BAY_COAST, 1), RR_ZORA_CAPE,
          RR_GREAT_BAY_COAST, SCENE_31MISAKI, SCENE_30GYOSON, "Great Bay Coast to Zora Cape" },
        { "mm:overworld:pinnacle_rock:0", ENTRANCE(PINNACLE_ROCK, 0), ENTRANCE(GREAT_BAY_COAST, 3),
          RR_PINNACLE_ROCK_ENTRANCE, RR_GREAT_BAY_COAST, SCENE_SINKAI, SCENE_30GYOSON,
          "Great Bay Coast to Pinnacle Rock" },
        { "mm:overworld:zora_hall:0", ENTRANCE(ZORA_HALL, 0), ENTRANCE(ZORA_CAPE, 1), RR_ZORA_HALL, RR_ZORA_CAPE,
          SCENE_33ZORACITY, SCENE_31MISAKI, "Zora Cape to Zora Hall front" },
        { "mm:overworld:zora_hall:1", ENTRANCE(ZORA_HALL, 1), ENTRANCE(ZORA_CAPE, 2), RR_ZORA_HALL,
          RR_ZORA_CAPE_BEFORE_GREAT_BAY_TEMPLE, SCENE_33ZORACITY, SCENE_31MISAKI, "Zora Cape to Zora Hall back" },
        { "mm:overworld:waterfall_rapids:0", ENTRANCE(WATERFALL_RAPIDS, 0), ENTRANCE(ZORA_CAPE, 4), RR_WATERFALL_RAPIDS,
          RR_ZORA_CAPE, SCENE_35TAKI, SCENE_31MISAKI, "Zora Cape to Waterfall Rapids" },
        { "mm:overworld:ikana_canyon:0", ENTRANCE(IKANA_CANYON, 0), ENTRANCE(ROAD_TO_IKANA, 1), RR_IKANA_CANYON_LOWER,
          RR_ROAD_TO_IKANA_ABOVE_LEDGE, SCENE_IKANA, SCENE_IKANAMAE, "Ikana Road to Ikana Canyon" },
        { "mm:overworld:ikana_graveyard:0", ENTRANCE(IKANA_GRAVEYARD, 0), ENTRANCE(ROAD_TO_IKANA, 2),
          RR_IKANA_GRAVEYARD_LOWER, RR_ROAD_TO_IKANA_BELOW_LEDGE, SCENE_BOTI, SCENE_IKANAMAE,
          "Ikana Road to Ikana Graveyard" },
        { "mm:overworld:stone_tower:0", ENTRANCE(STONE_TOWER, 0), ENTRANCE(IKANA_CANYON, 3), RR_STONE_TOWER_BOTTOM,
          RR_IKANA_CANYON_UPPER, SCENE_F40, SCENE_IKANA, "Ikana Canyon to Stone Tower" },
        { "mm:overworld:ikana_canyon:14", ENTRANCE(IKANA_CANYON, 14), ENTRANCE(IKANA_CANYON, 13), RR_IKANA_CANYON_CAVE,
          RR_IKANA_CANYON_UPPER, SCENE_IKANA, SCENE_IKANA, "Ikana Canyon to its cave passage" },
        { "mm:overworld:stone_tower_inverted:0", ENTRANCE(STONE_TOWER_INVERTED, 0), ENTRANCE(STONE_TOWER, 1),
          RR_STONE_TOWER_INVERTED_LOWER, RR_STONE_TOWER_TOP, SCENE_F41, SCENE_F40,
          "Stone Tower to Inverted Stone Tower" },
    } };
    return portals;
}

inline void Append(std::vector<Dungeon>& out) {
    for (const auto& portal : Portals()) {
        Dungeon dungeon;
        dungeon.id = portal.id;
        dungeon.entrance = { 1, portal.entrance };
        dungeon.exit = dungeon.returnTo = { 1, portal.exit };
        dungeon.pool = Pool::Overworld;
        out.push_back(std::move(dungeon));
    }
}

inline int32_t Canonicalize(int32_t index) {
    if (index < 0 || index > 0xffff)
        return index;
    int scene = index >> 9;
    switch (scene) {
        case ENTR_SCENE_SOUTHERN_SWAMP_CLEARED:
            scene = ENTR_SCENE_SOUTHERN_SWAMP_POISONED;
            break;
        case ENTR_SCENE_MOUNTAIN_VILLAGE_SPRING:
            scene = ENTR_SCENE_MOUNTAIN_VILLAGE_WINTER;
            break;
        case ENTR_SCENE_GORON_VILLAGE_SPRING:
            scene = ENTR_SCENE_GORON_VILLAGE_WINTER;
            break;
        case ENTR_SCENE_PATH_TO_GORON_VILLAGE_SPRING:
            scene = ENTR_SCENE_PATH_TO_GORON_VILLAGE_WINTER;
            break;
        default:
            return index;
    }
    const int32_t canonical = (scene << 9) | (index & 0x1ff);
    const auto matches = [canonical](const auto& portal) {
        return portal.entrance == canonical || portal.exit == canonical;
    };
    for (const auto& portal : Portals())
        if (matches(portal))
            return canonical;
    for (const auto& portal : ComboCrossMmCatalogue::Interiors())
        if (matches(portal))
            return canonical;
    for (const auto& portal : ComboCrossMmCatalogue::ExpandedDungeons())
        if (matches(portal))
            return canonical;
    // In particular, preserve the Goht blue warp (spring spawn 7), owl spawn 0
    // and authored mountain forest spawn 8. They are not these paired portals.
    return index;
}

inline int CanonicalScene(int scene) {
    // Play_GetOriginalSceneId also collapses both Stone Tower orientations;
    // those are distinct physical source scenes for entrance shuffling.
    switch (scene) {
        case SCENE_20SICHITAI2:
            return SCENE_20SICHITAI;
        case SCENE_10YUKIYAMANOMURA2:
            return SCENE_10YUKIYAMANOMURA;
        case SCENE_11GORONNOSATO2:
            return SCENE_11GORONNOSATO;
        case SCENE_17SETUGEN2:
            return SCENE_17SETUGEN;
        default:
            return scene;
    }
}

inline bool AllowsTransition(int32_t index, int sourceScene, int nextCutsceneIndex, int transitionType) {
    index = Canonicalize(index);
    // Solo Triforce completion and Majora's ending use the same native index
    // as the West Clock Town exit. Preserve the ending even if its final item
    // was found in that source scene (Rando/GiveItem.cpp).
    if (index == ENTRANCE(TERMINA_FIELD, 0) && nextCutsceneIndex == 0xFFF7)
        return false;
    // The story-skip enhancement heals Sharp then reloads his cave to refresh
    // actors. The real cave walk is still a selected same-scene portal.
    if (index == ENTRANCE(IKANA_CANYON, 14) && sourceScene == SCENE_IKANA && nextCutsceneIndex == 0 &&
        transitionType == TRANS_TYPE_INSTANT)
        return false;
    for (const auto& portal : Portals()) {
        if (index == portal.entrance)
            return CanonicalScene(sourceScene) == portal.sourceScene;
        if (index == portal.exit)
            return CanonicalScene(sourceScene) == portal.scene;
    }
    // Other selected categories have their own source-context policy.
    return true;
}

inline RandoRegionId RegionForEntry(int32_t index) {
    index = Canonicalize(index);
    for (const auto& portal : Portals())
        if (portal.entrance == index)
            return portal.region;
    return RR_MAX;
}

inline RandoRegionId RegionForReturn(int32_t index) {
    index = Canonicalize(index);
    for (const auto& portal : Portals())
        if (portal.exit == index)
            return portal.sourceRegion;
    return RR_MAX;
}

inline const char* SourceLabel(int32_t index) {
    index = Canonicalize(index);
    for (const auto& portal : Portals())
        if (portal.entrance == index)
            return portal.sourceLabel;
    return nullptr;
}
} // namespace ComboSoloMmOverworld
