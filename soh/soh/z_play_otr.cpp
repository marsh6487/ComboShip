#include "ResourceManagerHelpers.h"
#include "soh/unbound/SceneDB.h"
#include <libultraship/libultraship.h>
#include "soh/resource/type/Scene.h"
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManagerScope.h>
#include <stdexcept>
#include <ship/utils/StringHelper.h>
#include <spdlog/spdlog.h>

#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/audio/SceneRainPolicy.h"

extern "C" {
#include "functions.h"
#include "variables.h"
}

extern "C" void Play_InitScene(PlayState* play, s32 spawn);
extern "C" void Play_InitEnvironment(PlayState* play, s16 skyboxId);
void OTRPlay_InitScene(PlayState* play, s32 spawn);
s32 OTRScene_ExecuteCommands(PlayState* play, SOH::Scene* scene);

extern "C" void ActorCatalogue_LogLifecycle(const char* stage, int params, int type, int pose, int modelObjectId,
                                            int modelSlot, int animationObjectId, int animationSlot) {
    SPDLOG_INFO(
        "[ActorCatalogueProbe] stage={} params={:#06x} type={} pose={} modelObject={} modelSlot={} animationObject={} "
        "animationSlot={}",
        stage, params & 0xFFFF, type, pose, modelObjectId, modelSlot, animationObjectId, animationSlot);
}

extern "C" void GlobalOutdoorRain_Log(const char* message) {
    SPDLOG_INFO("{}", message);
}

// LUS::OTRResource* OTRPlay_LoadFile(PlayState* play, RomFile* file) {
Ship::IResource* OTRPlay_LoadFile(PlayState* play, const char* fileName) {
    // ComboShip: the resource manager loads async and rethrows on .get(); during an MM->OOT resume a
    // scene/resource can fail to load, and the uncaught throw took the whole process down with no
    // backtrace. Catch it here, log which file failed and why, and return nullptr so the caller hits
    // its existing fallback instead of dying silently.
    try {
#ifdef COMBO_BUILD
        LUSLOG_INFO("[SceneSpawn] owner path='%s' active-is-oot=%d", fileName,
                    Ship::Context::GetRawInstance()->GetResourceManager() == Ship::CrossRMRegistry::GetOrActive("oot"));
        Ship::OwnRMScope ownerScope("oot");
#endif
        auto resources = Ship::Context::GetRawInstance()->GetResourceManager();
        auto archives = resources->GetArchiveManager();
        LUSLOG_INFO("[SceneSpawn] lookup path='%s' alt=%d present=%d alt-present=%d", fileName,
                    resources->IsAltAssetsEnabled(), archives->HasFile(fileName),
                    archives->HasFile(std::string("alt/") + fileName));
        auto res = Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(fileName);
        auto scene = std::dynamic_pointer_cast<SOH::Scene>(res);
        LUSLOG_INFO("[SceneSpawn] resource path='%s' resolved='%s' result=%s", fileName,
                    res ? res->GetInitData()->Path.c_str() : "(none)",
                    scene ? "scene" : (res ? "wrong-type" : "missing"));
        return scene.get();
    } catch (const std::exception& e) {
        lusprintf(__FILE__, __LINE__, 2, "[ComboShip] OTRPlay_LoadFile FAILED for '%s': %s\n",
                  fileName ? fileName : "(null)", e.what());
        return nullptr;
    } catch (...) {
        lusprintf(__FILE__, __LINE__, 2, "[ComboShip] OTRPlay_LoadFile FAILED for '%s': unknown exception\n",
                  fileName ? fileName : "(null)");
        return nullptr;
    }
}

extern "C" void OTRPlay_SpawnScene(PlayState* play, s32 sceneId, s32 spawn) {
    LUSLOG_INFO("[SceneSpawn] request scene=%d spawn=%d entrance=%#x layer=%d cutscene=%#x respawn=%d", sceneId, spawn,
                gSaveContext.entranceIndex, gSaveContext.sceneLayer, gSaveContext.cutsceneIndex,
                gSaveContext.respawnFlag);
#ifdef COMBO_BUILD
    Ship::OwnRMScope ownerScope("oot");
#endif
    // A fallback is a new arrival, not a continuation of the failed scene's
    // grotto/Farore/cutscene spawn. Never apply its room or coordinates here.
    for (int attempt = 0;; ++attempt) {
        SceneDB::Entry& scene = SceneDB::Instance->RetrieveEntry(sceneId);
        // MQ resource resolution consults the requested scene during loading.
        play->sceneNum = sceneId;
        std::string scenePath = scene.valid ? SceneDB::Instance->GetScenePath(sceneId) : "";
        play->sceneSegment = scene.valid ? OTRPlay_LoadFile(play, scenePath.c_str()) : nullptr;
        if (play->sceneSegment != nullptr) {
            play->sceneNum = sceneId;
            play->sceneConfig = scene.drawConfig < SDC_MAX ? scene.drawConfig : SDC_DEFAULT;
            break;
        }
        LUSLOG_ERROR("[SceneSpawn] failed scene=%d path='%s' reason=%s attempt=%d", sceneId, scenePath.c_str(),
                     scene.valid ? "resource-load" : "unregistered-scene", attempt);
        if (attempt != 0 || sceneId == SCENE_DODONGOS_CAVERN) {
            throw std::runtime_error("Scene load failed, including recovery destination: " + scenePath);
        }
        sceneId = SCENE_DODONGOS_CAVERN;
        spawn = 0;
        gSaveContext.entranceIndex = ENTR_DODONGOS_CAVERN_ENTRANCE;
        gSaveContext.respawnFlag = 0;
        gSaveContext.sceneLayer = 0;
        gSaveContext.cutsceneIndex = 0;
        gSaveContext.nextCutsceneIndex = 0xFFEF;
        gSaveContext.cutsceneTrigger = 0;
        LUSLOG_INFO("[SceneSpawn] recovery entrance=%#x scene=%d spawn=0 layer=0 respawn=0", gSaveContext.entranceIndex,
                    sceneId);
    }

    // gSegments[2] = VIRTUAL_TO_PHYSICAL(play->sceneSegment);

    OTRPlay_InitScene(play, spawn);
    if (play->linkActorEntry != nullptr) {
        const auto& entry = *play->linkActorEntry;
        LUSLOG_INFO("[SceneSpawn] header scene=%d spawn=%d rooms=%d pos=(%.2f,%.2f,%.2f) params=%#x", play->sceneNum,
                    spawn, play->numRooms, static_cast<double>(entry.pos.x), static_cast<double>(entry.pos.y),
                    static_cast<double>(entry.pos.z), static_cast<unsigned>(static_cast<uint16_t>(entry.params)));
    }
    auto roomSize = func_80096FE8(play, &play->roomCtx);

    osSyncPrintf("ROOM SIZE=%fK\n", roomSize / 1024.0f);

    InitSceneRainPolicy(play, Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager().get(),
                        gSaveContext.sceneLayer, gSaveContext.linkAge);
    GameInteractor_ExecuteOnSceneInit(play->sceneNum);
    SPDLOG_INFO("Scene Init - sceneNum: {0:#x}, entranceIndex: {1:#x}", play->sceneNum, gSaveContext.entranceIndex);
}

void OTRPlay_InitScene(PlayState* play, s32 spawn) {
    // A legal JSON setup may omit collision entirely. Start from owned, queryable empty tables rather
    // than uninitialized PlayState bytes; a collision command replaces this context during setup.
    BgCheck_InitEmpty(&play->colCtx, play);
    play->curSpawn = spawn;
    play->linkActorEntry = nullptr;
    play->unk_11DFC = nullptr;
    play->setupEntranceList = nullptr;
    play->setupExitList = nullptr;
    play->cUpElfMsgs = nullptr;
    play->setupPathList = nullptr;
    play->numSetupActors = 0;
    // SOH [Unbound] the fields an Unbound scene document's commands fill are reset here with the vanilla ones:
    // PlayState is not zero-initialized, a document may omit the command, and the previous scene's command
    // resources are gone with it.
    play->sequenceCtx.unboundSongSeqId = 0;
    play->sceneMaterialAnims = nullptr;
    play->sceneMaterialAnimCount = 0;
    Object_InitBank(play, &play->objectCtx);
    LightContext_Init(play, &play->lightCtx);
    TransitionActor_InitContext(&play->state, &play->transiActorCtx);
    func_80096FD4(play, &play->roomCtx.curRoom);
    YREG(15) = 0;
    gSaveContext.worldMapArea = 0;
    OTRScene_ExecuteCommands(play, (SOH::Scene*)play->sceneSegment);

    GameInteractor_ExecuteAfterSceneCommands(play->sceneNum);
    Play_InitEnvironment(play, play->skyboxId);
    /* auto data = static_cast<LUS::Vertex*>(Ship::Context::GetRawInstance()
                                               ->GetResourceManager()
                                               ->ResourceLoad("object_link_child\\object_link_childVtx_01FE08")
                                               .get());

    auto data2 = ResourceMgr_LoadVtxByCRC(0x68d4ea06044e228f);*/

    volatile int a = 0;
}
