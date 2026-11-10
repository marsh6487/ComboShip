// Included beside the combined adapter in BenPort.cpp. Local routes have their
// own state and can never queue a foreign arrival or ask the launcher to switch.
#include "rando/SoloEntranceMmRuntime.h"
#include "Rando/SoloEntrances.h"
#include "rando/SoloEntranceMmOverworld.h"
#include <memory>

namespace ComboSoloMmRuntime {
using namespace ComboRando::CrossDungeon;
static RuntimeState state(1);
static CheckpointState checkpoint;
static int32_t localArrival = -1;
static bool bossDeathRecoveryRequested = false;

static void ClearRuntime() {
    state.Install(nullptr);
    checkpoint.Arrive(-1, false);
    localArrival = -1;
    bossDeathRecoveryRequested = false;
}

static bool Playable() {
    return gComboGameMode == COMBO_GAME_MM && IS_RANDO && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           gSaveContext.fileNum >= 0 && gSaveContext.fileNum <= 2;
}
static void RememberCheckpoint() {
    if (Playable())
        gSaveContext.save.shipSaveInfo.rando.soloEntrances.checkpoint = checkpoint.Saved();
}
static bool AllowsInteriorTransition(int32_t index, int scene) {
    for (const auto& portal : ComboCrossMmCatalogue::Interiors()) {
        if (!state.FindEntrance(portal.entrance))
            continue;
        RandoRegionId expected = RR_MAX;
        if (index == portal.entrance)
            expected = portal.sourceRegion;
        else if (index == portal.exit)
            expected = portal.region;
        if (expected == RR_MAX)
            continue;
        const auto region = Rando::Logic::Regions.find(expected);
        // Native minigames and story skips also reuse interior entry indices
        // while already inside. Shuffle only the authored source door/passage.
        return region != Rando::Logic::Regions.end() &&
               ComboSoloMmOverworld::CanonicalScene(scene) ==
                   ComboSoloMmOverworld::CanonicalScene(region->second.sceneId);
    }
    return true;
}
static int32_t RecoverBoss(int32_t arena) {
    const auto recovery = state.RecoveryForBoss({ 1, arena });
    if (!recovery || recovery->source.game != 1 || recovery->parent.game != 1)
        return -1;
    checkpoint.Arrive(recovery->parent.index, true);
    RememberCheckpoint();
    return recovery->parent.index;
}
} // namespace ComboSoloMmRuntime

extern "C" COMBO_EXPORT int MM_SetSoloEntranceRuntimeSeed(const char* raw) try {
    if (gComboGameMode != COMBO_GAME_MM)
        return 0;
    // The codec verifies the complete canonical MM catalogue and every
    // endpoint/recovery field. It only writes this temporary native save.
    auto candidate = std::make_unique<RandoSaveInfo>();
    Rando::SoloEntrances::ApplySeed(*candidate, nlohmann::json::parse(raw ? raw : "null"));
    const auto seed = Rando::SoloEntrances::Seed(*candidate);
    const auto routes = Rando::SoloEntrances::BuildRoutes(*candidate);
    const auto catalogue = Rando::SoloEntrances::CatalogueForSave(*candidate);
    using namespace ComboSoloMmRuntime;
    state.InstallValidated(seed, routes, catalogue);
    checkpoint.Arrive(-1, false);
    localArrival = -1;
    bossDeathRecoveryRequested = false;
    return 1;
} catch (const std::exception& error) {
    SPDLOG_ERROR("[SoloEntrances] MM runtime seed rejected: {}", error.what());
    return 0;
}

extern "C" int Combo_MM_SoloEntranceRuntimeEnabled(void) {
    return ComboSoloMmRuntime::Playable() && ComboSoloMmRuntime::state.Enabled();
}
extern "C" int Combo_MM_SoloEntranceBossEnabled(void) {
    return Combo_MM_SoloEntranceRuntimeEnabled() &&
           ComboSoloMmRuntime::state.HasPool(ComboRando::CrossDungeon::Pool::Boss);
}
extern "C" int Combo_MM_SoloEntranceBossSource(int physicalBoss) {
    if (!Combo_MM_SoloEntranceBossEnabled() || physicalBoss < 8 || physicalBoss >= 12)
        return -1;
    try {
        return Rando::SoloEntrances::BossSourceForSave(gSaveContext.save.shipSaveInfo.rando, physicalBoss);
    } catch (const std::exception& error) {
        SPDLOG_ERROR("[SoloEntrances] MM boss source rejected: {}", error.what());
        return -1;
    }
}
extern "C" int32_t Combo_MM_SoloEntranceSavedCheckpoint(void) {
    return Combo_MM_SoloEntranceRuntimeEnabled() ? ComboSoloMmRuntime::checkpoint.Saved() : -1;
}
extern "C" void Combo_MM_SoloEntranceRestoreCheckpoint(int32_t index) {
    if (gComboGameMode != COMBO_GAME_MM)
        return;
    using namespace ComboSoloMmRuntime;
    const bool known = state.Enabled() && state.IsRecoveryEntrance(index) && ComboCrossMmRuntime::ValidEntrance(index);
    checkpoint.Arrive(known ? index : -1, known);
    localArrival = -1;
    bossDeathRecoveryRequested = false;
    RememberCheckpoint();
}
extern "C" void Combo_MM_SoloEntranceRequestBossDeathRecovery(void) {
    using namespace ComboSoloMmRuntime;
    bossDeathRecoveryRequested =
        Combo_MM_SoloEntranceBossEnabled() && state.RecoveryForBoss({ 1, checkpoint.Saved() }).has_value();
}
extern "C" void Combo_MM_SoloEntranceRestoreWarpCheckpoint(int32_t index) {
    if (!Combo_MM_SoloEntranceRuntimeEnabled())
        return;
    Combo_MM_SoloEntranceRestoreCheckpoint(index);
    const int32_t logical = ComboSoloMmRuntime::checkpoint.Saved();
    if (!ComboCrossMmCatalogue::FindGrottoByEntry(logical))
        return;
    // Positioned spells already own their physical TOP point and arrival mode.
    // Restore the shared grotto's content and outside-hole identity alongside
    // that point, without permuting or replacing the caller's actual recall.
    const auto top = gSaveContext.respawn[RESPAWN_MODE_TOP];
    const auto mode = gSaveContext.respawnFlag;
    ComboMmMaterializeGrotto(logical);
    gSaveContext.respawn[RESPAWN_MODE_TOP] = top;
    gSaveContext.respawnFlag = mode;
}
extern "C" void Combo_MM_SoloEntranceObserveScene(int scene) {
    if (!Combo_MM_SoloEntranceRuntimeEnabled())
        return;
    using namespace ComboSoloMmRuntime;
    const bool owned = ComboCrossMmRuntime::OwnsScene(state, scene);
    if (localArrival >= 0) {
        checkpoint.Arrive(localArrival, owned);
        localArrival = -1;
    }
    checkpoint.ObserveScene(owned);
    RememberCheckpoint();
}

extern "C" void Combo_MM_SoloEntranceOnPlayDestroy(void) {
    if (gComboGameMode != COMBO_GAME_MM)
        return;
    // Ordinary portals recreate PlayState too; keep their queued arrival until
    // OnSceneInit. Reset/file-select/title exits discard only live routing state.
    // The saved POD remains available for the next actual selected-file load.
    if (!gPlayState || gPlayState->state.init != Play_Init)
        ComboSoloMmRuntime::ClearRuntime();
}

extern "C" void Combo_MM_SoloEntranceOnSaveLoad(void) {
    if (gComboGameMode != COMBO_GAME_MM)
        return;
    using namespace ComboSoloMmRuntime;
    // Clear the preceding file even when this load is vanilla, legacy, invalid
    // or disabled. Deserializing file-select previews never reaches this hook.
    ClearRuntime();
    if (!Playable())
        return;
    try {
        const auto& saved = gSaveContext.save.shipSaveInfo.rando;
        const int32_t savedCheckpoint = saved.soloEntrances.checkpoint;
        const auto seed = Rando::SoloEntrances::Seed(saved);
        if (!MM_SetSoloEntranceRuntimeSeed(seed.dump().c_str()) || !state.Enabled())
            return;

        // Native new-cycle loads keep the Clock Town/SoT start. Pause/auto/owl
        // resumes may need a safe physical temple or logical grotto checkpoint.
        // This runs after the native OnSaveLoad respawn-data hooks.
        const bool resume = gSaveContext.save.isOwlSave && gSaveContext.save.shipSaveInfo.pauseSaveEntrance >= 0;
        Combo_MM_SoloEntranceRestoreCheckpoint(resume ? savedCheckpoint : -1);
        if (!resume || checkpoint.Saved() < 0)
            return;
        const int32_t bossRecovery = RecoverBoss(checkpoint.Saved());
        const int32_t physical = bossRecovery >= 0 ? bossRecovery : checkpoint.Saved();
        gSaveContext.respawnFlag = 0;
        gSaveContext.save.entrance = ComboMmMaterializeGrotto(physical);
        gSaveContext.save.cutsceneIndex = 0;
        gSaveContext.nextCutsceneIndex = 0xFFEF;
        // Player_Init must not replace a materialized grotto jump with its
        // generic pause-save start mode. The composed destination is final.
        gSaveContext.save.shipSaveInfo.pauseSaveEntrance = -1;
        gSaveContext.save.isOwlSave = false;
    } catch (const std::exception& error) {
        ClearRuntime();
        SPDLOG_ERROR("[SoloEntrances] MM save routing disabled: {}", error.what());
    }
}

extern "C" int Combo_MM_SoloEntranceTransition(int32_t* index) {
    if (!index || !Combo_MM_SoloEntranceRuntimeEnabled())
        return 0;
    using namespace ComboSoloMmRuntime;
    if (bossDeathRecoveryRequested) {
        bossDeathRecoveryRequested = false;
        const int32_t recovery = RecoverBoss(checkpoint.Saved());
        if (recovery >= 0) {
            gSaveContext.respawnFlag = 0;
            *index = ComboMmMaterializeGrotto(recovery);
            gSaveContext.save.cutsceneIndex = 0;
            gSaveContext.nextCutsceneIndex = 0xFFEF;
            return 0;
        }
    }
    const int scene = gPlayState ? gPlayState->sceneId : -1;
    const int32_t canonicalExit = ComboMmCanonicalGrottoExit(*index, scene);
    const bool grottoExit = canonicalExit != *index && ComboCrossMmCatalogue::FindGrottoByExit(canonicalExit);
    // Physical death/void/Farore/Soaring returns bypass permutation. Grotto
    // mode4 is a real portal exit and therefore uses its logical paired route.
    if (gSaveContext.respawnFlag != 0 && !(gSaveContext.respawnFlag == 4 && grottoExit))
        return 0;
    const int32_t source =
        ComboSoloMmOverworld::Canonicalize(grottoExit ? canonicalExit : ComboMmCanonicalGrottoLoad(*index));
    if (!ComboSoloMmOverworld::AllowsTransition(source, scene, gSaveContext.nextCutsceneIndex,
                                                gPlayState ? gPlayState->transitionType : -1))
        return 0;
    if (!AllowsInteriorTransition(source, scene))
        return 0;
    auto route = state.Lookup(Phase::BeforeNative, source);
    if (!route && *index == ENTRANCE(GIANTS_CHAMBER, 0)) {
        int32_t entrance = 0, cutscene = 0;
        if (Combo_MM_GiantsEscape(&entrance, &cutscene, false)) {
            auto escape = state.Lookup(Phase::BeforeNative, entrance);
            if (escape && Combo_MM_GiantsEscape(&entrance, &cutscene, true)) {
                *index = entrance;
                gSaveContext.nextCutsceneIndex = cutscene;
                route = escape;
            }
        }
    }
    if (!route)
        route = state.Lookup(Phase::AfterNative, source);
    if (!route || route->to.game != 1)
        return 0;
    if (route->to.index != *index) {
        gSaveContext.nextCutsceneIndex = 0xFFEF;
        gSaveContext.save.cutsceneIndex = 0;
    }
    localArrival =
        route->arrivalCheckpoint ? route->arrivalCheckpoint->index : state.RecoveryForArrival(route->to.index);
    if (localArrival < 0)
        checkpoint.Arrive(-1, false);
    RememberCheckpoint();
    // Exactly one already-composed leg, including boss/dungeon combinations.
    // A physical hole exit contributed mode4 at the source. Only the chosen
    // destination may create a positioned return, otherwise native spawning
    // would reuse that source hole's coordinates in an unrelated scene.
    gSaveContext.respawnFlag = 0;
    *index = ComboMmMaterializeGrotto(route->to.index);
    return 0;
}
