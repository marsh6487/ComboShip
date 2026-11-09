#include "EnemyFirstDrops.h"
#include "static_data.h"
#include "location_access.h"
#include "rando/CrossEntranceBoss.h"

namespace Rando::EnemyFirstDrops {
void RegisterLocations() {
    for (const auto& entry : Catalog) {
        // These checks can be claimed in multiple scenes, including both world
        // and dungeon instances. They must NEVER inherit a witness dungeon's
        // ownership: Own Dungeon keys cannot be placed on a global type check.
        // Hyrule Field is the existing non-dungeon tracker/exclusion bucket;
        // the explicit Global label and SCENE_ID_MAX avoid a false physical scene.
        const auto name = std::string("Global Enemy Drop: ") + entry.name;
        *StaticData::GetLocation(entry.check) =
            Location::Base(entry.check, RCQUEST_BOTH, RCTYPE_ENEMY_DROP, RCAREA_HYRULE_FIELD, ACTOR_ID_MAX,
                           SCENE_ID_MAX, 0, std::string(name), std::string(name), RHT_NONE, RG_BLUE_RUPEE,
                           SpoilerCollectionCheck::RandomizerInf(entry.flag));
    }
}

void RegisterAccess() {
#define ENEMY_DROP_WITNESS(key, region, condition)                                                                 \
    areaTable[region].locations.push_back(LOCATION(RC_ENEMY_DROP_##key, ctx->GetOption(RSK_SHUFFLE_ENEMY_DROPS) && \
                                                                            logic->HasItem(RG_SOUL_ENEMY_##key) && \
                                                                            (condition)));
#include "EnemyFirstDropsWitnesses.inc"
#include "EnemyFirstDropsAdditionalWitnesses.inc"
#undef ENEMY_DROP_WITNESS
}
} // namespace Rando::EnemyFirstDrops
