#ifdef COMBO_BUILD
#include "ComboSoloForest.h"
#include "entrances/SoloForest.h"
#include "entrances/LostWoodsEntrance.h"
#include "entrances/SoloForestResources.h"
#include "entrances/SoloForestScroll.h"
#include "global.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/SaveManager.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include "../../../../mm/2s2h/Enhancements/Graphics/ComboForestPortalEffect.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

extern "C" {
u8 ResourceMgr_FileExists(const char* path);
}

namespace {
struct ForestOrigin {
    int entrance;
    Vec3f pos;
    s16 yaw;
    s8 room;
};
ForestOrigin DefaultForestOrigin() {
    return { ENTR_LOST_WOODS_SOUTH_EXIT, { 800, 0, 160 }, -32768, 1 };
}
ForestOrigin sForestOrigin = DefaultForestOrigin(), sGroundedOrigin = sForestOrigin;
int sForestSlot = -1;
u32 sForestSeed;
bool sForestResume = false, sForestArmed = false, sOriginStaged = false, sLocalReturning = false, sForestTitle = false;
RespawnData sForestPreservedReturn;
alignas(2) constexpr char kForestTitle[] = "__OTR__custom/skiejer_forest/title";
nlohmann::json sForestPending;
bool sForestLoadPending = true;
u32 ForestSeed() {
    return IS_RANDO ? Rando::Context::GetInstance()->GetSeed() : 0;
}
bool ForestPack() {
    return std::all_of(std::begin(kSoloForestResources), std::end(kSoloForestResources),
                       [](const char* path) { return ResourceMgr_FileExists(path); });
}
bool ForestVisit(const PlayState* play) {
    return play &&
           ComboSoloForest_IsOotVisit(play->sceneNum, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                      gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.cutsceneIndex);
}
bool OriginScene(const PlayState* play) {
    return play && play->sceneNum == SCENE_LOST_WOODS && play->roomCtx.curRoom.num == 1;
}
bool ForestSafe(const PlayState* play) {
    return gComboGameMode == COMBO_GAME_OOT && play && play->state.running &&
           gSaveContext.gameMode == GAMEMODE_NORMAL && ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) &&
           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF &&
           play->pauseCtx.state == 0 && play->gameOverCtx.state == GAMEOVER_INACTIVE &&
           play->csCtx.state == CS_STATE_IDLE && !gSaveContext.cutsceneTrigger;
}
void ForestTransition(PlayState* play, int entrance) {
    play->nextEntranceIndex = entrance;
    play->transitionType = gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK;
    play->transitionTrigger = TRANS_TRIGGER_START;
}
void StageForestOrigin() {
    if (sOriginStaged)
        return;
    auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    sForestPreservedReturn = arrival;
    sOriginStaged = true;
    arrival = {};
    arrival.entranceIndex = sForestOrigin.entrance;
    arrival.roomIndex = sForestOrigin.room;
    arrival.pos = sForestOrigin.pos;
    arrival.yaw = sForestOrigin.yaw;
    arrival.playerParams = (PLAYER_START_MODE_IDLE << 8) | 0xFF;
    arrival.data = 1;
    gSaveContext.respawnFlag = 2;
}
void ForestPlayerUpdate() {
    auto* play = gPlayState;
    if (!ForestSafe(play) || Player_InCsMode(play))
        return;
    if (sOriginStaged) {
        gSaveContext.respawn[RESPAWN_MODE_RETURN] = sForestPreservedReturn;
        sOriginStaged = false;
    }
    if (sLocalReturning && OriginScene(play)) {
        sLocalReturning = false;
        sForestArmed = false;
        return;
    }
    auto* player = GET_PLAYER(play);
    if (!player || sForestSlot != gSaveContext.fileNum || sForestSeed != ForestSeed())
        return;
    const auto pos = player->actor.world.pos;
    if (ForestVisit(play)) {
        if (play->roomCtx.curRoom.num != 0)
            return;
        const float dx = pos.x - COMBO_SOLO_FOREST_STUMP_X, dz = pos.z - COMBO_SOLO_FOREST_STUMP_Z;
        if (dx * dx + dz * dz > 14400)
            sForestArmed = true;
        if (sForestArmed && ComboSoloForest_AtStump(pos.x, pos.y, pos.z)) {
            sForestArmed = false;
            sLocalReturning = true;
            ForestTransition(play, sForestOrigin.entrance);
        }
        return;
    }
    if (!OriginScene(play) || !ForestPack())
        return;
    const bool inside = ComboLostWoods_InOotFairyTunnel(play->sceneNum, play->roomCtx.curRoom.num, pos.x, pos.y, pos.z);
    if (!inside) {
        sForestArmed = true;
        if (player->actor.floorPoly && player->actor.floorPoly->normal.y >= 23170 &&
            std::isfinite(player->actor.floorHeight) && std::abs(pos.y - player->actor.floorHeight) < 35)
            sGroundedOrigin = { gSaveContext.entranceIndex, pos, player->actor.shape.rot.y, 1 };
    } else if (sForestArmed) {
        SaveManager::Instance->ThreadPoolWait();
        sForestOrigin = sGroundedOrigin;
        sForestArmed = false;
        ForestTransition(play, COMBO_SOLO_FOREST_OOT_ENTRANCE);
    }
}
void LoadForestState(const nlohmann::json& value, int slot) {
    SaveManager::Instance->ThreadPoolWait();
    sForestSlot = slot;
    sForestSeed = ForestSeed();
    sForestResume = false;
    sOriginStaged = sLocalReturning = sForestArmed = sForestTitle = false;
    sForestOrigin = DefaultForestOrigin();
    if (gComboGameMode != COMBO_GAME_OOT || !ComboLostWoods_IsPlayableSlot(slot)) {
        sForestSlot = -1;
        return;
    }
    if (value.is_object() && value.value("seed", nlohmann::json()) == sForestSeed) {
        try {
            const auto point = value.at("origin");
            const int entrance = point.at("entrance"), yaw = point.at("yaw");
            const Vec3f pos = { point.at("x"), point.at("y"), point.at("z") };
            if (entrance >= 0 && entrance < ENTR_MAX && gEntranceTable[entrance].scene == SCENE_LOST_WOODS &&
                point.at("room") == 1 && std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z) &&
                std::abs(pos.x) < 4096 && std::abs(pos.y) < 4096 && std::abs(pos.z) < 4096 && yaw >= INT16_MIN &&
                yaw <= INT16_MAX)
                sForestOrigin = { entrance, pos, static_cast<s16>(yaw), 1 };
        } catch (const nlohmann::json::exception&) {}
        sForestResume = value.value("savedInside", nlohmann::json()) == true;
    }
    sGroundedOrigin = sForestOrigin;
}
void SaveForestState(const SaveContext& snapshot, int, bool) {
    if (gComboGameMode != COMBO_GAME_OOT || sForestSlot != snapshot.fileNum)
        return;
    const nlohmann::json value = { { "version", 1 },
                                   { "seed", sForestSeed },
                                   { "savedInside", snapshot.entranceIndex >= ENTR_SKIEJER_FOREST &&
                                                        snapshot.entranceIndex <= ENTR_SKIEJER_FOREST_3 },
                                   { "origin",
                                     { { "entrance", sForestOrigin.entrance },
                                       { "room", sForestOrigin.room },
                                       { "x", sForestOrigin.pos.x },
                                       { "y", sForestOrigin.pos.y },
                                       { "z", sForestOrigin.pos.z },
                                       { "yaw", sForestOrigin.yaw } } } };
    SaveManager::Instance->SaveData("native", value);
}
void NormalizeForestEntry() {
    if (gComboGameMode != COMBO_GAME_OOT || !ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum))
        return;
    const bool matches = sForestSlot == gSaveContext.fileNum && sForestSeed == ForestSeed();
    if (sForestResume) {
        sForestResume = false;
        if (matches) {
            gSaveContext.entranceIndex = COMBO_SOLO_FOREST_OOT_ENTRANCE;
            gSaveContext.respawnFlag = 0;
        }
    }
    if (gSaveContext.entranceIndex >= ENTR_SKIEJER_FOREST_EXIT &&
        gSaveContext.entranceIndex <= ENTR_SKIEJER_FOREST_EXIT_3) {
        if (!matches) {
            SaveManager::Instance->ThreadPoolWait();
            sForestOrigin = DefaultForestOrigin();
        }
        sLocalReturning = true;
        gSaveContext.entranceIndex = sForestOrigin.entrance;
        gSaveContext.cutsceneIndex = 0;
        StageForestOrigin();
        return;
    }
    if (sLocalReturning && matches && gSaveContext.entranceIndex == sForestOrigin.entrance) {
        StageForestOrigin();
        return;
    }
    if (gSaveContext.entranceIndex < ENTR_SKIEJER_FOREST || gSaveContext.entranceIndex > ENTR_SKIEJER_FOREST_3)
        return;
    gSaveContext.cutsceneIndex = 0;
    gSaveContext.cutsceneTrigger = 0;
    if (!ForestPack()) {
        if (!matches) {
            SaveManager::Instance->ThreadPoolWait();
            sForestOrigin = DefaultForestOrigin();
        }
        sLocalReturning = true;
        gSaveContext.entranceIndex = sForestOrigin.entrance;
        StageForestOrigin();
        return;
    }
    const auto& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    if (gSaveContext.respawnFlag == 2 && arrival.entranceIndex >= ENTR_SKIEJER_FOREST &&
        arrival.entranceIndex <= ENTR_SKIEJER_FOREST_3 && arrival.roomIndex == 0 && std::isfinite(arrival.pos.x) &&
        std::isfinite(arrival.pos.y) && std::isfinite(arrival.pos.z))
        return;
    gSaveContext.respawnFlag = 0; // void/death/reload starts at the safe private room-zero spawn
}

void DrawForestPortal() {
    auto* play = gPlayState;
    if (!ForestVisit(play) || play->roomCtx.curRoom.num != 0 || !ForestSafe(play))
        return;
    if (sForestTitle) {
        auto& title = play->actorCtx.titleCtx;
        if (!title.durationTimer && !title.delayTimer && !title.alpha) {
            title = {};
            title.texture = const_cast<char*>(kForestTitle);
            title.isBossCard = false;
            title.hasTranslation = false;
            title.x = 160;
            title.y = 180;
            title.width = 144;
            title.height = 24;
            title.durationTimer = 80;
            title.delayTimer = 20;
        }
        sForestTitle = false;
    }
    using namespace ComboForestPortalEffect;
    std::array<Vertex, kVertexCount> shape;
    Build(play->gameplayFrames, true, shape);
    auto* vertices = static_cast<Vtx*>(Graph_Alloc(play->state.gfxCtx, sizeof(Vtx) * kVertexCount));
    if (!vertices)
        return;
    for (int i = 0; i < kVertexCount; ++i) {
        vertices[i] = {};
        vertices[i].v.ob[0] = shape[i].x;
        vertices[i].v.ob[1] = shape[i].y;
        vertices[i].v.ob[2] = 10;
        vertices[i].v.cn[0] = shape[i].r;
        vertices[i].v.cn[1] = shape[i].g;
        vertices[i].v.cn[2] = shape[i].b;
        vertices[i].v.cn[3] = shape[i].a;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(COMBO_SOLO_FOREST_STUMP_X, 0, COMBO_SOLO_FOREST_STUMP_Z, MTXMODE_NEW);
    Matrix_RotateY(COMBO_SOLO_FOREST_STUMP_YAW * (6.28318530718f / 65536.0f), MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, __FILE__, __LINE__),
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_FOG | G_CULL_BOTH);
    gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_PASS, G_RM_AA_ZB_XLU_SURF2);
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_SHADE, G_CC_SHADE);
    for (int i = 0; i < kRimSegments; ++i) {
        gSPVertex(POLY_XLU_DISP++, reinterpret_cast<uintptr_t>(&vertices[i * 4]), 4, 0);
        gSP2Triangles(POLY_XLU_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    }
    for (int i = 0; i < kGlints; ++i) {
        gSPVertex(POLY_XLU_DISP++, reinterpret_cast<uintptr_t>(&vertices[kRimSegments * 4 + i * 5]), 5, 0);
        gSP2Triangles(POLY_XLU_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
        gSP2Triangles(POLY_XLU_DISP++, 0, 3, 4, 0, 0, 4, 1, 0);
    }
    Matrix_Pop();
    gDPPipeSync(POLY_XLU_DISP++);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}
void ResetForestFile() {
    // InitFile also runs under saveMtx. Never wait for a worker that needs
    // that mutex, or publish worker-facing slot/seed/origin state here.
    sForestPending = nullptr;
    sForestLoadPending = true;
    sForestResume = sForestArmed = sOriginStaged = sLocalReturning = sForestTitle = false;
}
void BindForestFile(int slot) {
    if (gComboGameMode != COMBO_GAME_OOT)
        return;
    // Native age/warp notifications reuse OnLoadGame. Only an actual native
    // file initialization/load can rebind its pending saved origin/resume.
    if (!sForestLoadPending && sForestSlot == slot && sForestSeed == ForestSeed())
        return;
    LoadForestState(sForestPending, slot); // waits outside LoadFile's saveMtx
    sForestLoadPending = false;
}
void RegisterSoloForest() {
    if (gComboGameMode != COMBO_GAME_OOT)
        return;
    SaveManager::Instance->AddInitFunction([](bool) { ResetForestFile(); });
    SaveManager::Instance->AddLoadFunction("soloForest", 1, [] {
        SaveManager::Instance->LoadData("native", sForestPending);
        sForestLoadPending = true;
    });
    SaveManager::Instance->AddSaveFunction("soloForest", 1, SaveForestState, true, SECTION_PARENT_NONE);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(BindForestFile);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16) {
        sForestArmed = false;
        sForestTitle = ForestVisit(gPlayState);
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(ForestPlayerUpdate);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawEnd>(DrawForestPortal);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(
        [] { sForestArmed = sForestTitle = false; });
}
RegisterShipInitFunc init(RegisterSoloForest);
} // namespace
extern "C" void ComboSoloForestOot_NormalizeEntry(void) {
    NormalizeForestEntry();
}
extern "C" void ComboSoloForestOot_DrawMaterials(PlayState* play) {
    if (!ForestVisit(play) || !ResourceMgr_FileExists(SoloForestScroll::kMarker))
        return;
    const u32 frame = play->gameplayFrames;
    OPEN_DISPS(play->state.gfxCtx);
    for (const auto& binding : SoloForestScroll::kBindings) {
        const auto& first = binding.layers[0];
        const auto& second = binding.layers[1];
        Gfx* scroll = binding.count == 1
                          ? Gfx_TexScrollEx(play->state.gfxCtx, frame * first.x, frame * first.y, first.width,
                                            first.height, first.x, first.y)
                          : Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, frame * first.x, frame * first.y, first.width,
                                               first.height, 1, frame * second.x, frame * second.y, second.width,
                                               second.height, first.x, first.y, second.x, second.y);
        gSPSegment(POLY_OPA_DISP++, binding.segment, reinterpret_cast<uintptr_t>(scroll));
        gSPSegment(POLY_XLU_DISP++, binding.segment, reinterpret_cast<uintptr_t>(scroll));
    }
    CLOSE_DISPS(play->state.gfxCtx);
}
#endif
