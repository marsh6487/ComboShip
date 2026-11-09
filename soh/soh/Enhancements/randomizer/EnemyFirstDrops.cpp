#include "EnemyFirstDrops.h"
#include "static_data.h"
#include "dungeon.h"
#include "location_access.h"
#include "rando/CrossEntranceBoss.h"

namespace Rando::EnemyFirstDrops {
bool SeedUsesMQ(SceneID scene) {
    auto context = Context::GetInstance();
    auto* dungeon = context ? context->GetDungeonFromScene(scene) : nullptr;
    return dungeon && dungeon->IsMQ();
}
bool HasSeededCheck(RandomizerCheck check) {
    auto context = Context::GetInstance();
    return context && context->IsLocationShuffled(check);
}
void RefreshSeedExclusions() {
    auto context = Context::GetInstance();
    if (context)
        context->UpdateEnemyDropExclusions();
}

void RegisterLocations() {
    for (const auto& entry : Catalog) {
        // These checks can be claimed in multiple scenes, including both world
        // and dungeon instances. They must NEVER inherit a witness dungeon's
        // ownership: Own Dungeon keys cannot be placed on a global type check.
        // Hyrule Field is the existing non-dungeon tracker/exclusion bucket;
        // the explicit Global label and SCENE_ID_MAX avoid a false physical scene.
        // A first-type reward is global: describe the action rather than one
        // witness dungeon. Distinct keys also preserve spoiler hint identities.
        StaticData::hintTextTable[entry.hint] = HintText(CustomMessage(
            std::string("They say that defeating your first #") + entry.name + "# rewards #[[1]]#.",
            std::string("Man sagt, der erste besiegte #") + entry.name + "# bringt #[[1]]#.",
            std::string("Selon la rumeur, vaincre votre premier #") + entry.name + "# rapporte #[[1]]#."));
        const auto name = std::string("Global Enemy Drop: ") + entry.name;
        *StaticData::GetLocation(entry.check) =
            Location::Base(entry.check, RCQUEST_BOTH, RCTYPE_ENEMY_DROP, RCAREA_HYRULE_FIELD, ACTOR_ID_MAX,
                           SCENE_ID_MAX, 0, std::string(name), std::string(name), entry.hint, RG_BLUE_RUPEE,
                           SpoilerCollectionCheck::RandomizerInf(entry.flag));
    }
}

void RegisterAccess() {
#define ENEMY_DROP_WITNESS(key, region, scene, quest, condition)                                                   \
    areaTable[region].locations.push_back(LOCATION(RC_ENEMY_DROP_##key, ctx->GetOption(RSK_SHUFFLE_ENEMY_DROPS) && \
                                                                            logic->HasItem(RG_SOUL_ENEMY_##key) && \
                                                                            (condition)));
#include "EnemyFirstDropsWitnesses.inc"
#include "EnemyFirstDropsAdditionalWitnesses.inc"
#undef ENEMY_DROP_WITNESS
}
} // namespace Rando::EnemyFirstDrops
