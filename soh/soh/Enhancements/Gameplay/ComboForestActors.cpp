#ifdef COMBO_BUILD
#include "ComboForestActors.h"
#include "entrances/SoloForest.h"
#include "global.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/ShipInit.hpp"
#include <cmath>

namespace {
ComboForestActors::Visit sVisit;
bool ForestVisit(PlayState* play) {
    return play &&
           ComboSoloForest_IsOotVisit(play->sceneNum, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                      gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.cutsceneIndex);
}
bool Safe(PlayState* play) {
    return ForestVisit(play) && play->state.running && play->transitionTrigger == TRANS_TRIGGER_OFF &&
           play->transitionMode == TRANS_MODE_OFF && play->pauseCtx.state == 0 &&
           play->gameOverCtx.state == GAMEOVER_INACTIVE && play->csCtx.state == CS_STATE_IDLE &&
           !gSaveContext.cutsceneTrigger && !Player_InCsMode(play);
}
Actor* Find(PlayState* play, int id, int params) {
    for (const auto& list : play->actorCtx.actorLists)
        for (Actor* actor = list.head; actor; actor = actor->next)
            if (actor->id == id && actor->params == params && actor->update &&
                (actor->room == play->roomCtx.curRoom.num || actor->room == -1))
                return actor;
    return nullptr;
}
bool Spawn(PlayState* play, int id, int params, float x, float z, float lift, s16 yaw) {
    auto* player = GET_PLAYER(play);
    if (!player)
        return false;
    // Stay below the upper forest rooms when sampling this room's floor.
    Vec3f probe = { x, play->roomCtx.curRoom.num == 0 ? 120.0f : 500.0f, z };
    CollisionPoly* floor = nullptr;
    s32 bg;
    float y = BgCheck_EntityRaycastFloor5(play, &play->colCtx, &floor, &bg, &player->actor, &probe);
    if (!floor || !std::isfinite(y) || y <= BGCHECK_Y_MIN)
        return false;
    return Actor_Spawn(&play->actorCtx, play, id, x, y + lift, z, 0, yaw, 0, params) != nullptr;
}
void Update() {
    auto* play = gPlayState;
    if (!ForestVisit(play)) {
        sVisit.Reset();
        return;
    }
    if (!Safe(play) || !sVisit.Ready(play->roomCtx.curRoom.num, play->roomCtx.curRoom.segment, play->roomCtx.status))
        return;
    using namespace ComboForestActors;
    auto ensure = [&](unsigned bit, int id, int params, float x, float z, float lift, s16 yaw) {
        sVisit.Ensure(
            bit, [&] { return Find(play, id, params) != nullptr; },
            [&] { return Spawn(play, id, params, x, z, lift, yaw); });
    };
    if (play->roomCtx.curRoom.num == 0) {
        ensure(1, kViewer, kSaria, COMBO_SOLO_FOREST_STUMP_X + 100, COMBO_SOLO_FOREST_STUMP_Z + 70, 0, 0);
        const float angle = COMBO_SOLO_FOREST_STUMP_YAW * (6.28318530718f / 65536.0f);
        const float sine = std::sin(angle), cosine = std::cos(angle);
        auto* beacon = Find(play, kViewer, kSaria);
        const float dx = beacon ? beacon->world.pos.x - COMBO_SOLO_FOREST_STUMP_X : 100.0f;
        const float dz = beacon ? beacon->world.pos.z - COMBO_SOLO_FOREST_STUMP_Z : 70.0f;
        const float lateral = dx * cosine - dz * sine <= 0 ? 145.0f : -145.0f;
        ensure(2, ACTOR_EN_HORSE, kHorse, COMBO_SOLO_FOREST_STUMP_X + lateral * cosine - 90.0f * sine,
               COMBO_SOLO_FOREST_STUMP_Z - lateral * sine - 90.0f * cosine, 0,
               static_cast<s16>(COMBO_SOLO_FOREST_STUMP_YAW + 0x8000));
        // Shimmer follows matching authored placements without moving them.
        auto* saria = Find(play, kViewer, kSaria);
        if (saria && play->gameplayFrames % 6 == 0) {
            float angle = play->gameplayFrames * 0.12f;
            Vec3f pos = saria->world.pos;
            pos.x += std::sin(angle) * 24;
            pos.z += std::cos(angle) * 24;
            pos.y += 12 + (play->gameplayFrames % 48);
            Vec3f velocity = { 0, 0.25f, 0 }, accel = { 0, 0.01f, 0 };
            Color_RGBA8 prim = { 130, 255, 140, 190 }, env = { 20, 130, 45, 0 };
            EffectSsKiraKira_SpawnDispersed(play, &pos, &velocity, &accel, &prim, &env, 500, 24);
        }
    } else {
        ensure(4, kViewer, kSkullKid, 2200, 200, 35, -0x5000);
    }
}
void OpenText(uint16_t*, bool* loadFromTable) {
    auto* play = gPlayState;
    // The viewer sets textId on each update. Intercept only the actual forest speaker.
    if (!ForestVisit(play) || play->roomCtx.curRoom.num != 0 || !loadFromTable)
        return;
    auto* player = GET_PLAYER(play);
    auto* speaker = player ? player->talkActor : nullptr;
    if (!speaker || speaker != Find(play, ComboForestActors::kViewer, ComboForestActors::kSaria))
        return;
    CustomMessage message("These woods are a bridge\nbetween worlds...\nThe forest will guide you home.");
    message.AutoFormat();
    message.LoadIntoFont();
    *loadFromTable = false;
}
void Register() {
    if (gComboGameMode != COMBO_GAME_OOT)
        return;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(Update);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16) { sVisit.Reset(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] { sVisit.Reset(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(OpenText);
}
RegisterShipInitFunc init(Register);
} // namespace
#endif
