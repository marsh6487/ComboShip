#include "Rando/Logic/Logic.h"
#include "ComboGameMode.h"
#include "entrances/SoloForest.h"
#include "combo/rando/CrossEntranceMmCatalogue.h"
#include "combo/rando/SoloEntranceMmOverworld.h"
#include "tests/test_require.h"
#include <cstdio>
#include <memory>
#include <numeric>

#define COMBO_EXPORT
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_DEBUG(...) ((void)0)

extern "C" {
SaveContext gSaveContext{};
PlayState* gPlayState = nullptr;
int gComboGameMode = COMBO_GAME_MM;
int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
void Play_Init(GameState*) {}
NeiSaveData* Nei_Save(void) { return &gSaveContext.save.shipSaveInfo.nei; }
s32 gSceneEntranceTableCount = 1;
SceneEntranceTableEntry sSceneEntranceTable[1]{};
s16 Play_GetOriginalSceneId(s16 scene) { return scene; }
void Flags_SetWeekEventReg(s32 flag) {
    gSaveContext.save.saveInfo.weekEventReg[flag >> 8] |= flag & 0xff;
}
}

static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
static PlayState play;
static int foreignCalls = 0;
static int ForeignQueue(int32_t) { ++foreignCalls; return 1; }
static void ReturnPeer(int) { ++foreignCalls; }
static void (*gComboReturnCallback)(int) = ReturnPeer;
static void* Combo_ResolveSym(const char*, const char*) {
    ++foreignCalls;
    return reinterpret_cast<void*>(ForeignQueue);
}
extern "C" void MM_QueueFwHandoff(void) { ++foreignCalls; }
extern "C" void Combo_MM_RequestCrossGameTitleCard(void) {}
static int giantsBookkeepingCalls = 0;
static SceneId lastGiantScene;
// Giant/Oath flag mutation is independently tested with the real implementation
// in cross_dungeon/mm_boss_warp_behavior_test.py. Here it is the transport boundary.
static void handleGiantsCheck(SceneId scene) { ++giantsBookkeepingCalls; lastGiantScene = scene; }
#include "native_giants.inc"

#include "combo/rando/CrossDungeonMmRuntime.inc.h"
#if __has_include("combo/rando/SoloEntranceMmRuntime.inc.h")
#include "combo/rando/SoloEntranceMmRuntime.inc.h"
#endif
#include "native_regions.inc"
#include "native_save_load.inc"
#include "native_hooks.inc"
#include "mm/2s2h/ComboFaroresWind.cpp"
#include "native_fw.inc"
#include "native_reloads.inc"

static void LoadNativeDungeonFile() {
    gSaveContext = {};
    gSaveContext.fileNum = 0;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.nextCutsceneIndex = 0xffef;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    gSaveContext.save.shipSaveInfo.rando.finalSeed = 73;
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_SHUFFLE_DUNGEON_ENTRANCES] = 1;
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = -1;
    auto& saved = gSaveContext.save.shipSaveInfo.rando.soloEntrances;
    saved.version = 2;
    saved.seed = 73;
    saved.options = 1; // Dungeon pool only.
    saved.count = 5;
    const uint16_t permutation[] = { 1, 2, 0, 4, 3 };
    std::copy(std::begin(permutation), std::end(permutation), saved.permutation);
    const uint16_t reverse[] = { 2, 0, 1, 4, 3 };
    std::copy(std::begin(reverse), std::end(reverse), saved.reversePermutation);
    saved.checkpoint = -1;
    saved.faroresWindCheckpoint = -1;
    gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 0);
    GameInteractor_ExecuteOnSaveLoad(0);
}

static std::vector<ComboRando::CrossDungeon::Dungeon> ConfigureNativeMap(uint16_t options, uint32_t policy = 0) {
    using namespace ComboRando::CrossDungeon;
    gSaveContext = {};
    play = {};
    interactor.events.clear();
    giantsBookkeepingCalls = 0;
    gSaveContext.fileNum = 0;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.nextCutsceneIndex = 0xffef;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = -1;
    gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 0);
    auto& rando = gSaveContext.save.shipSaveInfo.rando;
    rando.finalSeed = 73;
    const RandoOptionId optionIds[] = {
        RO_SHUFFLE_DUNGEON_ENTRANCES, RO_SHUFFLE_EXPANDED_DUNGEON_ENTRANCES, RO_SHUFFLE_BOSS_ENTRANCES,
        RO_SHUFFLE_INTERIOR_ENTRANCES, RO_SHUFFLE_SHOP_ENTRANCES, RO_SHUFFLE_DOOR_INTERIORS,
        RO_SHUFFLE_GROTTO_ENTRANCES, RO_SHUFFLE_OVERWORLD_ENTRANCES
    };
    const RandoOptionId policyIds[] = {
        RO_DECOUPLED_ENTRANCES, RO_MIX_ENTRANCE_POOLS, RO_MIX_DUNGEON_ENTRANCES, RO_MIX_BOSS_ENTRANCES,
        RO_MIX_INTERIOR_ENTRANCES, RO_MIX_SHOP_ENTRANCES, RO_MIX_GROTTO_ENTRANCES, RO_MIX_OVERWORLD_ENTRANCES
    };
    for (size_t i = 0; i < std::size(optionIds); ++i) {
        rando.randoSaveOptions[optionIds[i]] = (options >> i) & 1;
        rando.randoSaveOptions[policyIds[i]] = (policy >> i) & 1;
    }
    PoolOptions selected;
    selected.dungeons = options & 1;
    selected.expanded = options & 2;
    selected.bosses = options & 4;
    selected.interiors = options & 8;
    selected.shops = options & 16;
    selected.doorInteriors = options & 32;
    selected.grottos = options & 64;
    auto catalogue = ComboCrossMmCatalogue::Catalogue(selected);
    if (options & 128)
        ComboSoloMmOverworld::Append(catalogue);
    auto& saved = rando.soloEntrances;
    saved.version = 2;
    saved.seed = rando.finalSeed;
    saved.options = options;
    saved.policy = policy;
    saved.count = catalogue.size();
    saved.checkpoint = -1;
    saved.faroresWindCheckpoint = -1;
    std::iota(saved.permutation, saved.permutation + saved.count, 0);
    std::iota(saved.reversePermutation, saved.reversePermutation + saved.count, 0);
    return catalogue;
}

static size_t Slot(const std::vector<ComboRando::CrossDungeon::Dungeon>& catalogue, int32_t entrance) {
    for (size_t i = 0; i < catalogue.size(); ++i)
        if (catalogue[i].entrance.index == entrance)
            return i;
    REQUIRE(false);
    return 0;
}

static void SwapEntrances(const std::vector<ComboRando::CrossDungeon::Dungeon>& catalogue, int32_t a, int32_t b) {
    auto& saved = gSaveContext.save.shipSaveInfo.rando.soloEntrances;
    std::swap(saved.permutation[Slot(catalogue, a)], saved.permutation[Slot(catalogue, b)]);
    for (size_t i = 0; i < saved.count; ++i)
        saved.reversePermutation[saved.permutation[i]] = i;
}

static int32_t Transition(int32_t source) {
    REQUIRE(!Combo_MM_CrossDungeonTransition(&source));
    REQUIRE(foreignCalls == 0);
    return source;
}

static void ObserveArrival(int scene, int32_t entrance) {
    play.sceneId = scene;
    gSaveContext.save.entrance = entrance;
    GameInteractor_ExecuteOnSceneInit(scene, (entrance >> 4) & 0x1f);
}

static void TestNativeRestore() {
    LoadNativeDungeonFile();
    const int32_t arrived = Transition(ENTRANCE(WOODFALL_TEMPLE, 0));
    ObserveArrival(SCENE_HAKUGIN, arrived);
    REQUIRE(gSaveContext.save.shipSaveInfo.rando.soloEntrances.checkpoint == ENTRANCE(SNOWHEAD_TEMPLE, 0));
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == ENTRANCE(SNOWHEAD_TEMPLE, 0));

    // Native enhancement hooks restore physical respawn buffers. The actual
    // selected-file bridge must bind/rebuild Solo state after all such hooks.
    bool nativeHookRan = false;
    const auto hook = interactor.RegisterGameHook<GameInteractor::OnSaveLoad>([&](s16) {
        nativeHookRan = true;
        gSaveContext.respawnFlag = 3;
        gSaveContext.save.entrance = ENTRANCE(WOODFALL_TEMPLE, 0);
    });
    gSaveContext.save.isOwlSave = true;
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = ENTRANCE(WOODFALL_TEMPLE, 0);
    gSaveContext.save.day = 2;
    gSaveContext.save.time = 0x8a00;
    GameInteractor_ExecuteOnSaveLoad(0);
    REQUIRE(nativeHookRan);
    REQUIRE(gSaveContext.save.entrance == ENTRANCE(SNOWHEAD_TEMPLE, 0));
    REQUIRE(gSaveContext.respawnFlag == 0);
    REQUIRE(gSaveContext.save.shipSaveInfo.pauseSaveEntrance == -1);
    REQUIRE(gSaveContext.save.day == 2);
    REQUIRE(gSaveContext.save.time == 0x8a00);
    interactor.UnregisterGameHook<GameInteractor::OnSaveLoad>(hook);

    // End-of-cycle and SoT files deliberately retain the native Clock Town
    // start; a preceding physical temple checkpoint must not replace it.
    gSaveContext.save.isOwlSave = false;
    gSaveContext.save.entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 0);
    GameInteractor_ExecuteOnSaveLoad(0);
    REQUIRE(gSaveContext.save.entrance == ENTRANCE(SOUTH_CLOCK_TOWN, 0));
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == -1);
    REQUIRE(gSaveContext.save.shipSaveInfo.rando.soloEntrances.checkpoint == -1);
}

static void TestGrottoRestore() {
    const auto catalogue = ConfigureNativeMap(64);
    SwapEntrances(catalogue, ENTRANCE(GROTTOS, 18), ENTRANCE(GROTTOS, 19));
    GameInteractor_ExecuteOnSaveLoad(0);
    gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = 0x9a;
    REQUIRE(Transition(ENTRANCE(GROTTOS, 4)) == ENTRANCE(GROTTOS, 4));
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].data == 0x3f);
    ObserveArrival(SCENE_KAKUSIANA, ENTRANCE(GROTTOS, 4));
    play.roomCtx.curRoom.num = 4;
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == ENTRANCE(GROTTOS, 19));

    gSaveContext.save.isOwlSave = true;
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = ENTRANCE(GROTTOS, 4);
    const auto hook = interactor.RegisterGameHook<GameInteractor::OnSaveLoad>([](s16) {
        gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = 0xff;
    });
    GameInteractor_ExecuteOnSaveLoad(0);
    REQUIRE(gSaveContext.save.entrance == ENTRANCE(GROTTOS, 4));
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].data == 0x3f);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].pos.x == 1012);
    interactor.UnregisterGameHook<GameInteractor::OnSaveLoad>(hook);

    gSaveContext.respawnFlag = 4;
    REQUIRE(Transition(0xffff) == ENTRANCE(TERMINA_FIELD, 0));
    REQUIRE(gSaveContext.respawnFlag == 4);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].playerParams == 0x04ff);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].pos.x == 2367);
    ObserveArrival(SCENE_00KEIKOKU, ENTRANCE(TERMINA_FIELD, 0));
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == -1);
}

static void TestBossIngressRecovery() {
    const auto catalogue = ConfigureNativeMap(4);
    SwapEntrances(catalogue, ENTRANCE(ODOLWAS_LAIR, 0), ENTRANCE(GOHTS_LAIR, 0));
    GameInteractor_ExecuteOnSaveLoad(0);
    REQUIRE(Transition(ENTRANCE(ODOLWAS_LAIR, 0)) == ENTRANCE(GOHTS_LAIR, 0));
    ObserveArrival(SCENE_HAKUGIN_BS, ENTRANCE(GOHTS_LAIR, 0));
    REQUIRE(Combo_MM_SoloEntranceBossSource(9) == 8);
    Combo_MM_RequestCrossBossDeathRecovery();
    gSaveContext.respawnFlag = -6;
    REQUIRE(Transition(ENTRANCE(GOHTS_LAIR, 0)) == ENTRANCE(WOODFALL_TEMPLE, 0));
    REQUIRE(gSaveContext.respawnFlag == 0);
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == ENTRANCE(WOODFALL_TEMPLE, 0));
}

static void TestMixedDecoupledBossRecovery() {
    // A hole leads to Goht. A decoupled blue warp returns through a different
    // hole, while death must recover to the actual ingress hole's safe floor.
    const auto catalogue = ConfigureNativeMap(4 | 64, 1 | 2 | 8 | 64);
    SwapEntrances(catalogue, ENTRANCE(GROTTOS, 18), ENTRANCE(GOHTS_LAIR, 0));
    auto& saved = gSaveContext.save.shipSaveInfo.rando.soloEntrances;
    std::swap(saved.reversePermutation[Slot(catalogue, ENTRANCE(GOHTS_LAIR, 0))],
              saved.reversePermutation[Slot(catalogue, ENTRANCE(GROTTOS, 19))]);
    GameInteractor_ExecuteOnSaveLoad(0);
    for (int boss = 8; boss < 12; ++boss)
        REQUIRE(Combo_MM_SoloEntranceBossSource(boss) == boss);
    gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = 0x9a;
    REQUIRE(Transition(ENTRANCE(GROTTOS, 4)) == ENTRANCE(GOHTS_LAIR, 0));
    ObserveArrival(SCENE_HAKUGIN_BS, ENTRANCE(GOHTS_LAIR, 0));
    Combo_MM_RequestCrossBossDeathRecovery();
    gSaveContext.respawnFlag = -6;
    REQUIRE(Transition(ENTRANCE(GOHTS_LAIR, 0)) == ENTRANCE(TERMINA_FIELD, 0));
    REQUIRE(gSaveContext.respawnFlag == 4);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].pos.x == 2367);

    gSaveContext.respawnFlag = 0;
    gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = 0x9a;
    REQUIRE(Transition(ENTRANCE(GROTTOS, 4)) == ENTRANCE(GOHTS_LAIR, 0));
    ObserveArrival(SCENE_HAKUGIN_BS, ENTRANCE(GOHTS_LAIR, 0));
    REQUIRE(Transition(ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 7)) == ENTRANCE(TERMINA_FIELD, 0));
    REQUIRE(gSaveContext.respawnFlag == 4);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].pos.x == 1012);
}

static void TestSeasonalOverworld() {
    const auto catalogue = ConfigureNativeMap(128);
    SwapEntrances(catalogue, ENTRANCE(SOUTHERN_SWAMP_POISONED, 0), ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 6));
    SwapEntrances(catalogue, ENTRANCE(WOODFALL, 0), ENTRANCE(GORON_VILLAGE_WINTER, 0));
    GameInteractor_ExecuteOnSaveLoad(0);
    play.sceneId = Rando::Logic::Regions.at(RR_ROAD_TO_SOUTHERN_SWAMP).sceneId;
    REQUIRE(Transition(ENTRANCE(SOUTHERN_SWAMP_CLEARED, 0)) == ENTRANCE(MOUNTAIN_VILLAGE_WINTER, 6));
    play.sceneId = Rando::Logic::Regions.at(RR_PATH_TO_MOUNTAIN_VILLAGE_UPPER).sceneId;
    REQUIRE(Transition(ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 6)) == ENTRANCE(SOUTHERN_SWAMP_POISONED, 0));
    play.sceneId = Rando::Logic::Regions.at(RR_WOODFALL).sceneId;
    REQUIRE(Transition(ENTRANCE(SOUTHERN_SWAMP_CLEARED, 2)) == ENTRANCE(PATH_TO_GORON_VILLAGE_WINTER, 1));
    play.sceneId = Rando::Logic::Regions.at(RR_GORON_VILLAGE).sceneId;
    REQUIRE(Transition(ENTRANCE(PATH_TO_GORON_VILLAGE_SPRING, 1)) == ENTRANCE(SOUTHERN_SWAMP_POISONED, 2));
    for (int32_t fixed : { ENTRANCE(SOUTH_CLOCK_TOWN, 0), ENTRANCE(SOUTH_CLOCK_TOWN, 9),
                           ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 0), ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 7),
                           ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 8) })
        REQUIRE(Transition(fixed) == fixed);
    gSaveContext.respawnFlag = -6;
    REQUIRE(Transition(ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 6)) == ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 6));
}

static void TestOverworldReloadContexts() {
    const auto catalogue = ConfigureNativeMap(128);
    SwapEntrances(catalogue, ENTRANCE(ROMANI_RANCH, 0), ENTRANCE(STONE_TOWER_INVERTED, 0));
    SwapEntrances(catalogue, ENTRANCE(IKANA_CANYON, 14), ENTRANCE(GREAT_BAY_COAST, 0));
    SwapEntrances(catalogue, ENTRANCE(WOODS_OF_MYSTERY, 0), ENTRANCE(SNOWHEAD, 0));
    GameInteractor_ExecuteOnSaveLoad(0);
    play.sceneId = SCENE_F01;
    EnMa4_BeginHorsebackGame(nullptr, &play);
    REQUIRE(Transition(play.nextEntrance) == ENTRANCE(ROMANI_RANCH, 0));
    REQUIRE(gSaveContext.nextCutsceneIndex == 0xfff0);

    play.sceneId = SCENE_IKANA;
    const auto reload = NativeCurseReload();
    play.nextEntrance = reload.entrance;
    play.transitionType = reload.transitionType;
    gSaveContext.nextCutsceneIndex = reload.cutsceneIndex;
    REQUIRE(Transition(play.nextEntrance) == ENTRANCE(IKANA_CANYON, 14));
    REQUIRE(gSaveContext.nextCutsceneIndex == 0);

    play.sceneId = COMBO_SOLO_FOREST_MM_SCENE;
    gSaveContext.nextCutsceneIndex = 0xffef;
    ForestTransition(&play, ENTRANCE(WOODS_OF_MYSTERY, 0));
    REQUIRE(gSaveContext.respawnFlag == 0); // Positional staging occurs later at Play_Init.
    REQUIRE(Transition(play.nextEntrance) == ENTRANCE(WOODS_OF_MYSTERY, 0));

    // The same entrance numbers still shuffle when walking their real links.
    play.sceneId = SCENE_ROMANYMAE;
    gSaveContext.nextCutsceneIndex = 0xffef;
    REQUIRE(Transition(ENTRANCE(ROMANI_RANCH, 0)) == ENTRANCE(STONE_TOWER_INVERTED, 0));
    play.sceneId = SCENE_IKANA;
    REQUIRE(Transition(ENTRANCE(IKANA_CANYON, 14)) == ENTRANCE(GREAT_BAY_COAST, 0));

    // Stone Tower's intentional inversion uses cutscene index 0 too. Preserve
    // its shuffle participation in each direction instead of banning all CS0.
    play.sceneId = SCENE_F40;
    NativeTowerInversion(&play);
    REQUIRE(Transition(play.nextEntrance) == ENTRANCE(ROMANI_RANCH, 0));
    play.sceneId = SCENE_F41;
    NativeTowerInversion(&play);
    REQUIRE(Transition(play.nextEntrance) == ENTRANCE(MILK_ROAD, 1));
}

static void TestFirstAndRepeatBossWarps() {
    const int32_t bosses[] = { ENTRANCE(ODOLWAS_LAIR, 0), ENTRANCE(GOHTS_LAIR, 0),
                               ENTRANCE(GYORGS_LAIR, 0), ENTRANCE(TWINMOLDS_LAIR, 0) };
    const int scenes[] = { SCENE_MITURIN_BS, SCENE_HAKUGIN_BS, SCENE_SEA_BS, SCENE_INISIE_BS };
    const int32_t escapes[] = { ENTRANCE(WOODFALL_TEMPLE, 1), ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 7),
                                ENTRANCE(ZORA_CAPE, 9), ENTRANCE(IKANA_CANYON, 15) };
    const int cutscenes[] = { 0, 0, 0xfff0, 0xfff2 };
    for (size_t boss = 0; boss < std::size(bosses); ++boss) {
        const auto catalogue = ConfigureNativeMap(4 | 64, 2 | 8 | 64);
        SwapEntrances(catalogue, ENTRANCE(GROTTOS, 18), bosses[boss]);
        GameInteractor_ExecuteOnSaveLoad(0);
        gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = 0x9a;
        REQUIRE(Transition(ENTRANCE(GROTTOS, 4)) == bosses[boss]);
        ObserveArrival(scenes[boss], bosses[boss]);
        GIEventTransition firstWarp{};
        firstWarp.entrance = escapes[boss];
        firstWarp.cutsceneIndex = cutscenes[boss];
        firstWarp.transitionType = scenes[boss];
        interactor.events.emplace_back(firstWarp);
        interactor.events.emplace_back(GIEventGiveItem{});
        REQUIRE(Transition(ENTRANCE(GIANTS_CHAMBER, 0)) == ENTRANCE(TERMINA_FIELD, 0));
        REQUIRE(gSaveContext.respawnFlag == 4);
        REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].pos.x == 2367);
        REQUIRE(gSaveContext.nextCutsceneIndex == 0xffef);
        REQUIRE(giantsBookkeepingCalls == 1 && lastGiantScene == scenes[boss]);
        REQUIRE(interactor.events.size() == 1 && std::holds_alternative<GIEventGiveItem>(interactor.events.front()));

        gSaveContext.respawnFlag = 0;
        gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = 0x9a;
        REQUIRE(Transition(ENTRANCE(GROTTOS, 4)) == bosses[boss]);
        ObserveArrival(scenes[boss], bosses[boss]);
        gSaveContext.nextCutsceneIndex = cutscenes[boss];
        REQUIRE(Transition(escapes[boss]) == ENTRANCE(TERMINA_FIELD, 0));
        REQUIRE(gSaveContext.respawnFlag == 4 && gSaveContext.nextCutsceneIndex == 0xffef);
        REQUIRE(giantsBookkeepingCalls == 1); // Repeat warps never re-free a Giant.
    }
}

static void TestInteriorReloadContexts() {
    const auto catalogue = ConfigureNativeMap(8);
    SwapEntrances(catalogue, ENTRANCE(MUSIC_BOX_HOUSE, 0), ENTRANCE(DOGGY_RACETRACK, 0));
    SwapEntrances(catalogue, ENTRANCE(MOUNTAIN_SMITHY, 0), ENTRANCE(POST_OFFICE, 0));
    GameInteractor_ExecuteOnSaveLoad(0);
    play.sceneId = SCENE_MUSICHOUSE;
    skipHealingPamelasFather();
    REQUIRE(interactor.events.size() == 1);
    const auto reload = std::get<GIEventTransition>(interactor.events.front());
    play.nextEntrance = reload.entrance;
    play.transitionType = reload.transitionType;
    gSaveContext.nextCutsceneIndex = reload.cutsceneIndex;
    REQUIRE(Transition(play.nextEntrance) == ENTRANCE(MUSIC_BOX_HOUSE, 0));
    REQUIRE(gSaveContext.nextCutsceneIndex == 0);

    play.sceneId = SCENE_IKANA;
    gSaveContext.nextCutsceneIndex = 0xffef;
    REQUIRE(Transition(ENTRANCE(MUSIC_BOX_HOUSE, 0)) == ENTRANCE(DOGGY_RACETRACK, 0));
    play.sceneId = Rando::Logic::Regions.at(RR_DOGGY_RACETRACK).sceneId;
    REQUIRE(Transition(ENTRANCE(ROMANI_RANCH, 5)) == ENTRANCE(IKANA_CANYON, 2));
    // A seasonal native exterior remains the same authored source door.
    play.sceneId = SCENE_10YUKIYAMANOMURA2;
    REQUIRE(Transition(ENTRANCE(MOUNTAIN_SMITHY, 0)) == ENTRANCE(POST_OFFICE, 0));
    play.sceneId = Rando::Logic::Regions.at(RR_MOUNTAIN_SMITHY).sceneId;
    REQUIRE(Transition(ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 1)) == ENTRANCE(WEST_CLOCK_TOWN, 7));
}

static void TestLifecycle() {
    LoadNativeDungeonFile();
    const int32_t target = Transition(ENTRANCE(WOODFALL_TEMPLE, 0));
    play.state.init = Play_Init;
    GameInteractor_ExecuteOnPlayDestroy();
    // Every ordinary portal also destroys PlayState. Its queued local arrival
    // must survive until the replacement play state reports its physical scene.
    REQUIRE(Combo_MM_SoloEntranceRuntimeEnabled());
    ObserveArrival(SCENE_HAKUGIN, target);
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == ENTRANCE(SNOWHEAD_TEMPLE, 0));
    GameInteractor_ExecuteBeforeEndOfCycleSave();
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == -1);
    REQUIRE(gSaveContext.save.shipSaveInfo.rando.soloEntrances.checkpoint == -1);
    ObserveArrival(SCENE_HAKUGIN, target);
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == -1);

    Transition(ENTRANCE(WOODFALL_TEMPLE, 0));
    ObserveArrival(SCENE_HAKUGIN, target);
    GameInteractor_ExecuteBeforeMoonCrash();
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == -1);
    REQUIRE(gSaveContext.save.shipSaveInfo.rando.soloEntrances.checkpoint == -1);

    Transition(ENTRANCE(WOODFALL_TEMPLE, 0));
    ObserveArrival(SCENE_HAKUGIN, target);
    play.state.init = nullptr; // Title/file-select/reset/teardown, rather than another Play_Init.
    GameInteractor_ExecuteOnPlayDestroy();
    REQUIRE(!Combo_MM_SoloEntranceRuntimeEnabled());
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == -1);
    // Teardown clears only live queues. The already-saved recovery stays owned
    // by this file and can be reconstructed on its next actual selected load.
    REQUIRE(gSaveContext.save.shipSaveInfo.rando.soloEntrances.checkpoint == ENTRANCE(SNOWHEAD_TEMPLE, 0));
    gSaveContext.save.isOwlSave = true;
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = target;
    GameInteractor_ExecuteOnSaveLoad(0);
    REQUIRE(Combo_MM_SoloEntranceRuntimeEnabled());
    REQUIRE(gSaveContext.save.entrance == ENTRANCE(SNOWHEAD_TEMPLE, 0));
}

static void TestMixedTempleGrottoBossCycle() {
    const auto catalogue = ConfigureNativeMap(1 | 4 | 64, 2 | 4 | 8 | 64);
    SwapEntrances(catalogue, ENTRANCE(WOODFALL_TEMPLE, 0), ENTRANCE(GROTTOS, 18));
    SwapEntrances(catalogue, ENTRANCE(GROTTOS, 18), ENTRANCE(GOHTS_LAIR, 0));
    GameInteractor_ExecuteOnSaveLoad(0);
    play.sceneId = SCENE_21MITURINMAE;
    REQUIRE(Transition(ENTRANCE(WOODFALL_TEMPLE, 0)) == ENTRANCE(GROTTOS, 4));
    REQUIRE(static_cast<uint8_t>(gSaveContext.respawn[RESPAWN_MODE_UNK_3].data) == 0x9a);
    ObserveArrival(SCENE_KAKUSIANA, ENTRANCE(GROTTOS, 4));
    play.roomCtx.curRoom.num = 4;
    gSaveContext.respawnFlag = 4;
    REQUIRE(Transition(0xffff) == ENTRANCE(WOODFALL, 1));
    // Mode4 belongs to the native hole that was left. Its coordinates must not
    // leak into the unrelated temple exterior chosen by this mixed return.
    REQUIRE(gSaveContext.respawnFlag == 0);
    ObserveArrival(SCENE_21MITURINMAE, ENTRANCE(WOODFALL, 1));

    gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = 0x9a;
    REQUIRE(Transition(ENTRANCE(GROTTOS, 4)) == ENTRANCE(GOHTS_LAIR, 0));
    ObserveArrival(SCENE_HAKUGIN_BS, ENTRANCE(GOHTS_LAIR, 0));
    REQUIRE(Transition(ENTRANCE(MOUNTAIN_VILLAGE_SPRING, 7)) == ENTRANCE(TERMINA_FIELD, 0));
    REQUIRE(gSaveContext.respawnFlag == 4);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].pos.x == 2367);

    gSaveContext.respawnFlag = 0;
    play.sceneId = SCENE_HAKUGIN;
    REQUIRE(Transition(ENTRANCE(GOHTS_LAIR, 0)) == ENTRANCE(WOODFALL_TEMPLE, 0));
    ObserveArrival(SCENE_MITURIN, ENTRANCE(WOODFALL_TEMPLE, 0));
    REQUIRE(Transition(ENTRANCE(WOODFALL, 1)) == ENTRANCE(SNOWHEAD_TEMPLE, 0));
    ObserveArrival(SCENE_HAKUGIN, ENTRANCE(SNOWHEAD_TEMPLE, 0));
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == ENTRANCE(SNOWHEAD_TEMPLE, 0));
    REQUIRE(gSaveContext.respawnFlag == 0);
}

static void TestNativeFaroresWind() {
    const auto catalogue = ConfigureNativeMap(4);
    SwapEntrances(catalogue, ENTRANCE(ODOLWAS_LAIR, 0), ENTRANCE(GOHTS_LAIR, 0));
    GameInteractor_ExecuteOnSaveLoad(0);
    REQUIRE(Transition(ENTRANCE(ODOLWAS_LAIR, 0)) == ENTRANCE(GOHTS_LAIR, 0));
    ObserveArrival(SCENE_HAKUGIN_BS, ENTRANCE(GOHTS_LAIR, 0));
    REQUIRE(OotSpells_SceneAllowsFarores(&play));
    auto& down = gSaveContext.respawn[RESPAWN_MODE_DOWN];
    down.entrance = ENTRANCE(GOHTS_LAIR, 0);
    down.roomIndex = 0;
    down.pos = { 111, 222, -333 };
    down.yaw = -300;
    down.playerParams = PLAYER_PARAMS(0xff, PLAYER_START_MODE_D);
    down.tempSwitchFlags = 55;
    down.tempCollectFlags = 77;
    NativeCapturePoint(); // Actual native cast's persist/publish code.

    // Another arena becomes the active checkpoint before recalling the point.
    REQUIRE(Transition(ENTRANCE(GOHTS_LAIR, 0)) == ENTRANCE(ODOLWAS_LAIR, 0));
    ObserveArrival(SCENE_MITURIN_BS, ENTRANCE(ODOLWAS_LAIR, 0));
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == ENTRANCE(ODOLWAS_LAIR, 0));
    REQUIRE(ComboFw_RequestReturn() == 0);
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == ENTRANCE(GOHTS_LAIR, 0));
    NativeRecallPoint(&play); // Actual native recall preserves the physical point.
    REQUIRE(Transition(play.nextEntrance) == ENTRANCE(GOHTS_LAIR, 0));
    const auto& top = gSaveContext.respawn[RESPAWN_MODE_TOP];
    REQUIRE(gSaveContext.respawnFlag == 3);
    REQUIRE(top.pos.x == 111 && top.pos.y == 222 && top.pos.z == -333);
    REQUIRE(top.yaw == -300 && top.roomIndex == 0);
    REQUIRE(top.tempSwitchFlags == 55 && top.tempCollectFlags == 77);
    ObserveArrival(SCENE_HAKUGIN_BS, ENTRANCE(GOHTS_LAIR, 0));
    Combo_MM_RequestCrossBossDeathRecovery();
    gSaveContext.respawnFlag = -6;
    REQUIRE(Transition(ENTRANCE(GOHTS_LAIR, 0)) == ENTRANCE(WOODFALL_TEMPLE, 0));

    ComboFw_ClearPoint();
    REQUIRE(gSaveContext.save.shipSaveInfo.rando.soloEntrances.faroresWindCheckpoint == -1);
    play.sceneId = SCENE_KAKUSIANA;
    REQUIRE(!OotSpells_SceneAllowsFarores(&play)); // Preserve the existing grotto cast ban.
    REQUIRE(foreignCalls == 0);
}

static void TestPositionedWarpGrottoMetadata() {
    const auto catalogue = ConfigureNativeMap(64);
    SwapEntrances(catalogue, ENTRANCE(GROTTOS, 18), ENTRANCE(GROTTOS, 19));
    GameInteractor_ExecuteOnSaveLoad(0);
    // The positioned spell was set in logical grotto19, then grotto18 was
    // visited. Both use physical room4 but have different content and exit data.
    gSaveContext.respawn[RESPAWN_MODE_UNK_3].data = 0x3f;
    REQUIRE(Transition(ENTRANCE(GROTTOS, 4)) == ENTRANCE(GROTTOS, 4));
    ObserveArrival(SCENE_KAKUSIANA, ENTRANCE(GROTTOS, 4));
    play.roomCtx.curRoom.num = 4;
    REQUIRE(static_cast<uint8_t>(gSaveContext.respawn[RESPAWN_MODE_UNK_3].data) == 0x9a);
    auto& top = gSaveContext.respawn[RESPAWN_MODE_TOP];
    top.entrance = ENTRANCE(GROTTOS, 4);
    top.roomIndex = 4;
    top.pos = { 123, 45, -67 };
    top.yaw = -1234;
    top.tempSwitchFlags = 55;
    top.tempCollectFlags = 77;
    top.unk_18 = 99;
    gSaveContext.respawnFlag = 3;
    const auto point = top;
    Combo_MM_SoloEntranceRestoreWarpCheckpoint(ENTRANCE(GROTTOS, 19));
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].data == 0x3f);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].pos.x == 1012);
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_DOWN].entrance == ENTRANCE(TERMINA_FIELD, 6));
    REQUIRE(gSaveContext.respawnFlag == 3);
    REQUIRE(std::memcmp(&point, &top, sizeof(point)) == 0);
    REQUIRE(Transition(ENTRANCE(GROTTOS, 4)) == ENTRANCE(GROTTOS, 4));
    ObserveArrival(SCENE_KAKUSIANA, ENTRANCE(GROTTOS, 4));
    REQUIRE(Combo_MM_SaveCrossDungeonCheckpoint() == ENTRANCE(GROTTOS, 19));
    gSaveContext.respawnFlag = 4;
    REQUIRE(Transition(0xffff) == ENTRANCE(TERMINA_FIELD, 0));
    REQUIRE(gSaveContext.respawn[RESPAWN_MODE_UNK_3].pos.x == 2367);
}

int main() {
    InitNativeRegions();
    RegisterNativeHooks();
    gPlayState = &play;
    LoadNativeDungeonFile();
    int32_t entrance = ENTRANCE(WOODFALL_TEMPLE, 0);
    // Break caught: the native save-load path leaves Solo MM routing disabled,
    // despite the selected save containing a valid dungeon permutation.
    REQUIRE(Combo_MM_CrossDungeonTransition(&entrance) == 0);
    REQUIRE(entrance == ENTRANCE(SNOWHEAD_TEMPLE, 0));
    REQUIRE(foreignCalls == 0);

    entrance = ENTRANCE(SNOWHEAD, 1);
    REQUIRE(Combo_MM_CrossDungeonTransition(&entrance) == 0);
    REQUIRE(entrance == ENTRANCE(WOODFALL, 1));

    // Void/death and owl warp-to-entrance destinations are already physical.
    // They must not traverse the entrance permutation again.
    for (int flag : { 1, -1, -2, -6, 2, 3 }) {
        gSaveContext.respawnFlag = flag;
        entrance = ENTRANCE(WOODFALL_TEMPLE, 0);
        REQUIRE(!Combo_MM_CrossDungeonTransition(&entrance));
        REQUIRE(entrance == ENTRANCE(WOODFALL_TEMPLE, 0));
    }
    gSaveContext.respawnFlag = 0;
    entrance = ENTRANCE(SOUTH_CLOCK_TOWN, 9);
    REQUIRE(!Combo_MM_CrossDungeonTransition(&entrance));
    REQUIRE(entrance == ENTRANCE(SOUTH_CLOCK_TOWN, 9));

    TestNativeRestore();
    TestGrottoRestore();
    TestBossIngressRecovery();
    TestMixedDecoupledBossRecovery();
    TestSeasonalOverworld();
    TestOverworldReloadContexts();
    TestFirstAndRepeatBossWarps();
    TestInteriorReloadContexts();
    TestMixedTempleGrottoBossCycle();
    TestLifecycle();
    TestNativeFaroresWind();
    TestPositionedWarpGrottoMetadata();
    LoadNativeDungeonFile();

    // Session gates, foreign queue entrypoints and normal vanilla files stay
    // closed independently of stale local runtime state.
    REQUIRE(!MM_QueueCrossDungeonArrival(ENTRANCE(SNOWHEAD_TEMPLE, 0)));
    REQUIRE(!MM_QueueCrossBossRecovery(0, 100));
    REQUIRE(!MM_SetCrossDungeonRuntimeSeed("null"));
    REQUIRE(!Combo_MM_CompleteCrossDungeonTransition());
    for (int mode : { COMBO_GAME_OOT, COMBO_GAME_BOTH }) {
        gComboGameMode = mode;
        entrance = ENTRANCE(WOODFALL_TEMPLE, 0);
        REQUIRE(!Combo_MM_CrossDungeonTransition(&entrance));
        REQUIRE(entrance == ENTRANCE(WOODFALL_TEMPLE, 0));
    }
    gComboGameMode = COMBO_GAME_MM;
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_VANILLA;
    GameInteractor_ExecuteOnSaveLoad(0);
    entrance = ENTRANCE(WOODFALL_TEMPLE, 0);
    REQUIRE(!Combo_MM_CrossDungeonTransition(&entrance));
    REQUIRE(entrance == ENTRANCE(WOODFALL_TEMPLE, 0));
    REQUIRE(foreignCalls == 0);
    std::puts("PASS Solo MM native load/lifecycle, grotto replay, mixed/decoupled bosses, first/repeat warps, FW and gates");
    return 0;
}
