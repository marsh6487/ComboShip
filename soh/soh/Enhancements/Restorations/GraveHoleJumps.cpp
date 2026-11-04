#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <libultraship/bridge/consolevariablebridge.h>

#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"
#include "soh/resource/type/Scene.h"
#include "soh/resource/type/scenecommand/SceneCommand.h"
#include "soh/resource/type/scenecommand/SetCollisionHeader.h"
#include "soh/unbound/SceneDB.h"
#include <spdlog/spdlog.h>
#include <vector>

extern "C" {
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
int ResourceMgr_IsModAsset(const char* path);
}

#define CVAR_GRAVE_HOLE_NAME CVAR_ENHANCEMENT("GraveHoles")
#define GRAVE_HOLES_DEFAULT 0
#define CVAR_GRAVE_HOLE_VALUE CVarGetInteger(CVAR_GRAVE_HOLE_NAME, GRAVE_HOLES_DEFAULT)
#define CUSTOM_SURFACE_TYPE 32 // Marker in the vanilla mapping, not an authored table index.

const static std::array<std::pair<std::pair<u16, u16>, std::pair<u16, u16>>, 6> graveyardGeometryPatches = { {
    // { { startPolygon, endPolygon }, { originalSurfaceType, patchedSurfaceType } }
    { { 487, 509 }, { 20, CUSTOM_SURFACE_TYPE } }, // Floor around graves
    { { 651, 658 }, { 20, CUSTOM_SURFACE_TYPE } }, // Floor around Royal Family Tomb
    { { 613, 620 }, { 0, 15 } },                   // Grave ledges (Hylian Shield)
    { { 623, 630 }, { 0, 15 } },                   // Grave ledges (Redead)
    { { 633, 640 }, { 0, 15 } },                   // Grave ledges (Dampe)
    { { 643, 650 }, { 0, 15 } },                   // Grave ledges (Royal Family)
} };

// Pin the resource only while the live scene uses it. Restore before Alt resources unload.
static std::shared_ptr<SOH::CollisionHeader> sCollisionOwner;
static CollisionHeader* sCollisionHeader;
static SurfaceType* sOriginalSurfaceTypes;
static std::vector<SurfaceType> sSurfaceTypes;
static std::vector<std::pair<size_t, u16>> sOriginalPolyTypes;

static void RestoreGraveyardGeometry() {
    if (sCollisionOwner && sCollisionHeader != nullptr) {
        for (const auto& entry : sOriginalPolyTypes) {
            sCollisionHeader->polyList[entry.first].type = entry.second;
        }
        sCollisionHeader->surfaceTypeList = sOriginalSurfaceTypes;
    }
    sOriginalPolyTypes.clear();
    sSurfaceTypes.clear();
    sOriginalSurfaceTypes = nullptr;
    sCollisionHeader = nullptr;
    sCollisionOwner.reset();
}

void ApplyGraveyardGeometryPatches() {
    RestoreGraveyardGeometry();
    if (gPlayState == nullptr || gPlayState->sceneNum != SCENE_GRAVEYARD || CVAR_GRAVE_HOLE_VALUE) {
        return;
    }
    const std::string scenePath = SceneDB::Instance->GetScenePath(SCENE_GRAVEYARD);
    auto sceneOwner = Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(scenePath);
    SOH::Scene* scene = static_cast<SOH::Scene*>(sceneOwner.get());
    if (scene == nullptr) {
        return;
    }
    for (const auto& cmd : scene->commands) {
        if (cmd->cmdId != SOH::SceneCommandID::SetCollisionHeader) {
            continue;
        }
        auto* sceneCmd = static_cast<SOH::SetCollisionHeader*>(cmd.get());
        // A mod can preserve vanilla material numbers while changing the mesh.
        // Never infer canonical geometry from those numbers alone.
        if (sceneCmd->fileName.empty() || ResourceMgr_IsModAsset(sceneCmd->fileName.c_str())) {
            return;
        }
        auto owner = sceneCmd->collisionHeader;
        auto* header = static_cast<CollisionHeader*>(sceneCmd->GetRawPointer());
        // A different setup/resource is not ours to patch. Custom tables and polygon
        // numbering must retain all authored surfaces, including indices above 32.
        if (!owner || header == nullptr || header != gPlayState->colCtx.colHeader ||
            (owner->surfaceTypesCount != 31 && owner->surfaceTypesCount != 32) || header->surfaceTypeList == nullptr ||
            header->polyList == nullptr) {
            return;
        }
        for (const auto& patch : graveyardGeometryPatches) {
            if (patch.first.second >= header->numPolygons) {
                return;
            }
            for (size_t i = patch.first.first; i <= patch.first.second; ++i) {
                if (header->polyList[i].type != patch.second.first) {
                    return;
                }
            }
        }
        sCollisionOwner = owner;
        sCollisionHeader = header;
        sOriginalSurfaceTypes = header->surfaceTypeList;
        sSurfaceTypes.assign(sOriginalSurfaceTypes, sOriginalSurfaceTypes + owner->surfaceTypesCount);
        const u16 customSurface = static_cast<u16>(sSurfaceTypes.size());
        sSurfaceTypes.push_back(SurfaceType_Unpack(0x24000004, 0xFC8));
        for (const auto& patch : graveyardGeometryPatches) {
            for (size_t i = patch.first.first; i <= patch.first.second; ++i) {
                sOriginalPolyTypes.emplace_back(i, header->polyList[i].type);
                header->polyList[i].type =
                    patch.second.second == CUSTOM_SURFACE_TYPE ? customSurface : patch.second.second;
            }
        }
        header->surfaceTypeList = sSurfaceTypes.data();
        return;
    }
}

void RegisterGraveHoleJumps() {
    COND_HOOK(OnPlayDestroy, true, []() { RestoreGraveyardGeometry(); });
    COND_HOOK(OnSceneInit, true, [](int32_t) { ApplyGraveyardGeometryPatches(); });
    // Settings changed during gameplay apply to this scene without caching its pointers.
    ApplyGraveyardGeometryPatches();
}

static RegisterShipInitFunc initFunc(RegisterGraveHoleJumps, { CVAR_GRAVE_HOLE_NAME });
