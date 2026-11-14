#include "../../../../combo/render/ItemGetLightShaft.h"
#include "../../../../combo/render/ItemGetLightShaftBridge.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/frame_interpolation.h"
#include "z64.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
extern PlayState* gPlayState;
}
#include <libultraship/bridge/consolevariablebridge.h>

#define GI_SHAFT_ALLOC(gfx, bytes) Graph_Alloc(gfx, bytes)
#define GI_SHAFT_SETUP_XLU(gfx) Gfx_SetupDL_25Xlu(gfx)
#define GI_SHAFT_LOAD_MATRIX(gfx) \
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfx), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH)
#include "../../../../combo/render/ItemGetLightShaftRenderer.inc"

namespace {
ItemGetLightShaft::Marker sMarker;
ItemGetLightShaft::State sState;

void Reset() {
    sMarker.Begin();
    sState.Reset();
}
bool CanMark(PlayState* play, Player* player) {
    return play && play == gPlayState && play->state.gfxCtx && player && player == GET_PLAYER(play) &&
           player->actor.id == ACTOR_PLAYER && (player->stateFlags1 & PLAYER_STATE1_GETTING_ITEM) &&
           CVarGetInteger(ItemGetLightShaft::kCVar, 0) && play->pauseCtx.state == 0 && play->pauseCtx.debugState == 0 &&
           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF;
}
void Draw() {
    PlayState* play = gPlayState;
    if (!play) {
        Reset();
        return;
    }
    ItemGetLightShaft::Input input;
    input.enabled = CVarGetInteger(ItemGetLightShaft::kCVar, 0) != 0;
    input.paused = play->pauseCtx.state != 0 || play->pauseCtx.debugState != 0;
    input.transitioning = play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF;
    input.playIdentity = reinterpret_cast<uintptr_t>(play);
    input.scene = play->sceneNum;
    input.room = play->roomCtx.curRoom.num;
    input.frame = play->gameplayFrames;
    input.seconds = std::clamp<int>(R_UPDATE_RATE, 1, 3) / 60.f;
    input.presentation = sMarker.Consume();
    ItemGetLightShaft_Draw(play, sState.Step(input));
}
void Register() {
    COND_HOOK(OnPlayDrawBegin, true, [] { sMarker.Begin(); });
    COND_HOOK(OnPlayDrawEnd, true, Draw);
    COND_HOOK(OnSceneInit, true, [](int16_t) { Reset(); });
    COND_HOOK(OnPlayDestroy, true, Reset);
    COND_HOOK(OnLoadGame, true, [](int32_t) { Reset(); });
    COND_HOOK(OnExitGame, true, [](int32_t) { Reset(); });
}
static RegisterShipInitFunc init(Register);
} // namespace

extern "C" void SoHItemGetLightShaft_MarkNative(PlayState* play, Player* player, float itemY) {
    if (!CanMark(play, player) || player->unk_862 <= 0)
        return;
    const auto& origin = player->actor.world.pos;
    sMarker.Mark({ true, { origin.x, origin.y, origin.z }, Player_GetHeight(player), itemY - origin.y });
}
