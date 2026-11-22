#include "2s2h/Enhancements/Enhancements.h"
#include "gui/UiVisibility.h"
#include "hud/PickupCounterDraw.h"
#include "hud/WorldHudViewport.h"

extern "C" {
#include "functions.h"
#include "overlays/actors/ovl_En_Talk_Gibud/z_en_talk_gibud.h"
#include "variables.h"
}

#ifdef COMBO_BUILD
bool Combo_MmIsForeground(void);
#endif

namespace {
constexpr char kEnabled[] = "gEnhancements.Graphics.GibdoRequestIcons";
constexpr float kFullOpacityDistance = 150.0f;
constexpr float kMaxDistance = 240.0f;

bool CanDrawRequests(const PlayState* play) {
#ifdef COMBO_BUILD
    if (!Combo_MmIsForeground())
        return false;
#endif
    return CVarGetInteger(kEnabled, 0) && play && play->sceneId == SCENE_REDEAD && play->state.running &&
           gSaveContext.gameMode == GAMEMODE_NORMAL && play->roomCtx.curRoom.num >= 0 && play->pauseCtx.state == 0 &&
           play->csCtx.state == CS_STATE_IDLE && play->transitionTrigger == TRANS_TRIGGER_OFF &&
           play->transitionMode == TRANS_MODE_OFF && play->gameOverCtx.state == GAMEOVER_INACTIVE &&
           play->msgCtx.msgMode == MSGMODE_NONE && !UiVisibility_IsHidden();
}

void DrawRequest(const WorldHud::Viewport& viewport, ImDrawList* draw, const EnTalkGibudRequestedItem& request,
                 PickupCounter::Point anchor, int tradeMode, float scale, float alpha) {
    const auto icon = viewport.gui->GetTextureByName(reinterpret_cast<const char*>(gItemIcons[request.item]));
    if (!icon)
        return;

    const bool mm3d = tradeMode == GIBDO_TRADE_SEQUENCE_MM3D;
    const ImVec2 center(anchor.x, anchor.y - 15.0f * scale);
    if (mm3d && request.item == ITEM_POTION_BLUE) {
        const auto red = viewport.gui->GetTextureByName(reinterpret_cast<const char*>(gItemIcons[ITEM_POTION_RED]));
        if (red) {
            PickupCounter::DrawItemTexture(draw, icon, ImVec2(center.x - 13.0f * scale, center.y), scale * 0.8f, alpha);
            PickupCounter::DrawItemTexture(draw, red, ImVec2(center.x + 13.0f * scale, center.y), scale * 0.8f, alpha);
            return;
        }
    }

    PickupCounter::DrawItemTexture(draw, icon, center, scale, alpha);
    if (!mm3d && request.amount > 1) {
        char quantity[12];
        std::snprintf(quantity, sizeof(quantity), "%d", request.amount);
        PickupCounter::Text(draw, ImGui::GetFont(), 20.0f * scale,
                            ImVec2(center.x + 8.0f * scale, center.y + 1.0f * scale),
                            PickupCounter::Ink(255, 255, 255, alpha), quantity, alpha);
    }
}

} // namespace

void DrawGibdoRequests() {
    PlayState* play = gPlayState;
    const int tradeMode = CVarGetInteger(GIBDO_TRADE_SEQUENCE_CVAR, GIBDO_TRADE_SEQUENCE_VANILLA);
    if (tradeMode == GIBDO_TRADE_SEQUENCE_NO_TRADE || !CanDrawRequests(play))
        return;
    Player* player = GET_PLAYER(play);
    WorldHud::Viewport viewport;
    if (!player || (player->stateFlags1 & PLAYER_STATE1_DEAD) || Player_InCsMode(play) ||
        !WorldHud::GetViewport(viewport) || viewport.clip.width < 1 || viewport.clip.height < 1)
        return;

    const float scale = viewport.image.height / 720.0f;
    const bool mirrored = CVarGetInteger("gModes.MirroredWorld.State", 0) != 0;
    const auto& clip = viewport.clip;
    auto* draw = ImGui::GetForegroundDrawList(viewport.imguiViewport);
    draw->PushClipRect(ImVec2(clip.x, clip.y), ImVec2(clip.x + clip.width, clip.y + clip.height), true);
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_ENEMY].first; actor; actor = actor->next) {
        const float distanceSq = actor->xyzDistToPlayerSq;
        if (!actor->isDrawn || actor->room != play->roomCtx.curRoom.num || distanceSq >= kMaxDistance * kMaxDistance)
            continue;
        const auto* request = EnTalkGibud_GetPendingRequest(actor, play);
        if (!request)
            continue;

        Vec3f head = actor->focus.pos;
        head.y += 22.0f;
        PickupCounter::Point anchor;
        if (!viewport.Project(play->viewProjectionMtxF, head, mirrored, anchor))
            continue;

        Vec3f hit;
        Vec3f eye = play->view.eye;
        CollisionPoly* poly = nullptr;
        s32 bgId;
        if (BgCheck_CameraLineTest2(&play->colCtx, &eye, &head, &hit, &poly, true, true, true, false, &bgId))
            continue;

        const float distance = std::sqrt(distanceSq);
        const float alpha = std::min(1.0f, (kMaxDistance - distance) / (kMaxDistance - kFullOpacityDistance));
        DrawRequest(viewport, draw, *request, anchor, tradeMode, scale, alpha);
    }
    draw->PopClipRect();
}
