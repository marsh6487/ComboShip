// Included after ComboCrossMm's oracle HostState and reachable/time maps are defined.
#include "rando/CrossEntranceBoss.h"
#include "Rando/SoloEntrances.h"
extern "C" int Combo_MM_SoloEntranceBossSource(int physicalBoss);
extern "C" COMBO_EXPORT void MM_ApplyCrossBossClear(int sourceBossId, int dormant);
namespace ComboCrossMm {
static ComboRando::CrossDungeon::BossCompletionState bossCompletion;
static const int32_t bossEntrances[4] = { ENTRANCE(ODOLWAS_LAIR, 0), ENTRANCE(GOHTS_LAIR, 0), ENTRANCE(GYORGS_LAIR, 0),
                                          ENTRANCE(TWINMOLDS_LAIR, 0) };
static int BossFromScene(int scene) {
    switch (scene) {
        case SCENE_MITURIN_BS:
            return COMBO_BOSS_ODOLWA;
        case SCENE_HAKUGIN_BS:
            return COMBO_BOSS_GOHT;
        case SCENE_SEA_BS:
            return COMBO_BOSS_GYORG;
        case SCENE_INISIE_BS:
            return COMBO_BOSS_TWINMOLD;
        default:
            return -1;
    }
}
static bool BossLogicEnabled() {
    return Combo_IsCombinedGame() && state.HasPool(Pool::Boss);
}
static void AppendBossCatalogue(std::vector<Dungeon>& out) {
    ComboCrossMmCatalogue::AppendBossCatalogue(out);
}
} // namespace ComboCrossMm
extern "C" COMBO_EXPORT void MM_SetCrossBossNotify(void (*notify)(int)) {
    ComboCrossMm::bossCompletion.SetNotify(notify);
}
extern "C" COMBO_EXPORT void MM_SetCrossBossCompletedMask(uint16_t mask) {
    ComboCrossMm::bossCompletion.SetCompletedMask(mask);
}
extern "C" int Combo_MM_CrossBossLogicalClear(int id, int nativeValue) {
    using namespace ComboCrossMm;
    if (gComboGameMode == COMBO_GAME_MM) {
        if (searching || Rando::gItemGrantSimulation)
            return Rando::SoloEntrances::LogicalClear(id, nativeValue);
        // Solo blue warps set the source temple's native weekly flag. Its world
        // effect still resets each cycle even though completion is remembered.
        return nativeValue;
    }
    if (searching && BossLogicEnabled())
        return id >= 0 && id < COMBO_BOSS_COUNT && (state.logicalClears & (1u << id));
    // MM world effects reset each cycle; loading persistent completion must not restore spring.
    return bossCompletion.LogicalClear(id, nativeValue != 0, !searching && Combo_MM_CrossBossEnabled()) && nativeValue;
}
extern "C" int Combo_MM_CrossBossWarp(int scene) {
    if (gComboGameMode == COMBO_GAME_MM) {
        const int source = Combo_MM_SoloEntranceBossSource(ComboCrossMm::BossFromScene(scene));
        if (source < COMBO_BOSS_ODOLWA || source > COMBO_BOSS_TWINMOLD)
            return 0;
        MM_ApplyCrossBossClear(source, 0);
        return 1; // Suppress native physical-temple clear; the source owns the completion.
    }
    return ComboCrossMm::bossCompletion.Warp(ComboCrossMm::BossFromScene(scene), Combo_MM_CrossBossEnabled());
}
extern "C" COMBO_EXPORT void MM_ApplyCrossBossClear(int sourceBossId, int dormant) try {
    using namespace ComboCrossMm;
    if (sourceBossId < 8 || sourceBossId >= 12 || gSaveContext.fileNum < 0 || gSaveContext.fileNum > 2)
        return;
    if (gComboGameMode == COMBO_GAME_MM)
        gSaveContext.save.shipSaveInfo.rando.soloEntrances.completedBossMask |= 1u << sourceBossId;
    else
        bossCompletion.SetCompletedMask(bossCompletion.CompletedMask() | (1u << sourceBossId));
    switch (sourceBossId) {
        case COMBO_BOSS_ODOLWA:
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE);
            break;
        case COMBO_BOSS_GOHT:
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE);
            break;
        case COMBO_BOSS_GYORG:
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE);
            break;
        case COMBO_BOSS_TWINMOLD:
            SET_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE);
            break;
    }
    if (dormant || !gPlayState)
        SaveManager_SaveCurrentForCombo();
} catch (const std::exception& e) {
    SPDLOG_ERROR("[CrossBoss] MM source clear could not be saved: {}", e.what());
} catch (...) { SPDLOG_ERROR("[CrossBoss] MM source clear failed"); }

extern "C" COMBO_EXPORT uint16_t MM_GetCrossBossDefeated(void) {
    const RandoRegionId rooms[] = { RR_WOODFALL_TEMPLE_BOSS_ROOM, RR_SNOWHEAD_TEMPLE_BOSS_ROOM,
                                    RR_GREAT_BAY_TEMPLE_BOSS_ROOM, RR_STONE_TOWER_TEMPLE_BOSS_ROOM };
    const ActorId actors[] = { ACTOR_BOSS_01, ACTOR_BOSS_HAKUGIN, ACTOR_BOSS_03, ACTOR_BOSS_02 };
    uint16_t defeated = 0;
    for (int i = 0; i < 4; ++i) {
        if (!ComboCrossMm::reachable.contains(rooms[i]))
            continue;
        Rando::Logic::SetCurrentRegionTime(ComboCrossMm::timeStates, rooms[i]);
        if (Rando::Logic::CanKillEnemy(actors[i]))
            defeated |= 1u << (i + 8);
    }
    return defeated;
}
