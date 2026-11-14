#include "../../../../combo/render/ItemGetLightShaft.h"
#include "../../../../combo/render/ItemGetLightShaftBridge.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "mods/items/logic/adult_link_render.h"
#include "global.h"
#include <libultraship/bridge/consolevariablebridge.h>

void CustomItem00_Update(Actor* actor, PlayState* play);

#define GI_SHAFT_ALLOC(gfx, bytes) GRAPH_ALLOC(gfx, bytes)
#define GI_SHAFT_SETUP_XLU(gfx) Gfx_SetupDL25_Xlu(gfx)
#define GI_SHAFT_LOAD_MATRIX(gfx) MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfx)
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
           player->actor.id == ACTOR_PLAYER && (player->stateFlags1 & PLAYER_STATE1_400) &&
           CVarGetInteger(ItemGetLightShaft::kCVar, 0) && play->pauseCtx.state == PAUSE_STATE_OFF &&
           play->transitionTrigger == TRANS_TRIGGER_OFF && play->transitionMode == TRANS_MODE_OFF;
}
void Mark(PlayState* play, Player* player, float itemY) {
    if (!CanMark(play, player))
        return;
    const auto& origin = player->actor.world.pos;
    const bool adult = AdultLink_UsesAdultItemPresentation(player);
    const float height = adult ? AdultLink_GetItemPresentationHeight() : Player_GetHeight(player);
    const float offset = adult ? AdultLink_GetItemPresentationYOffset() : 0.f;
    sMarker.Mark({ true, { origin.x, origin.y + offset, origin.z }, height, itemY - origin.y - offset });
}
void MarkCustom(Actor* actor) {
    PlayState* play = gPlayState;
    if (!play || !actor || actor->id != ACTOR_EN_ITEM00 || actor->params != ITEM00_NOTHING ||
        actor->update != CustomItem00_Update || !actor->draw || !(actor->home.rot.x & CustomItem::GIVE_ITEM_CUTSCENE) ||
        !(actor->home.rot.x & CustomItem::CALLED_ACTION) ||
        (actor->home.rot.x & (CustomItem::GIVE_OVERHEAD | CustomItem::KILL_ON_TOUCH)) ||
        reinterpret_cast<EnItem00*>(actor)->unk152 < 20 || actor->scale.y <= 0.f)
        return;
    Mark(play, GET_PLAYER(play), actor->world.pos.y + actor->shape.yOffset * actor->scale.y);
}
void Draw() {
    PlayState* play = gPlayState;
    if (!play) {
        Reset();
        return;
    }
    ItemGetLightShaft::Input input;
    input.enabled = CVarGetInteger(ItemGetLightShaft::kCVar, 0) != 0;
    input.paused = play->pauseCtx.state != PAUSE_STATE_OFF;
    input.transitioning = play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF;
    input.playIdentity = reinterpret_cast<uintptr_t>(play);
    input.scene = play->sceneId;
    input.room = play->roomCtx.curRoom.num;
    input.frame = play->gameplayFrames;
    input.seconds = std::clamp<int>(R_UPDATE_RATE, 1, 3) / 60.f;
    input.presentation = sMarker.Consume();
    ItemGetLightShaft_Draw(play, sState.Step(input));
}
void Register() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldStart>([] { sMarker.Begin(); });
    GameInteractor::Instance->RegisterGameHookForID<GameInteractor::OnActorDraw>(ACTOR_EN_ITEM00, MarkCustom);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldEnd>(Draw);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s8, s8) { Reset(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(Reset);
}
static RegisterShipInitFunc init(Register);
} // namespace

extern "C" void MMItemGetLightShaft_MarkNative(PlayState* play, Player* player, float itemY) {
    if (CanMark(play, player) && player->getItemDrawIdPlusOne > GID_NONE + 1)
        Mark(play, player, itemY);
}
