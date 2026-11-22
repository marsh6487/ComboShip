#include "PickupCounterDraw.h"
#include "SongEventGlyphAtlas.h"
#include "WorldHudViewport.h"
#include "gui/UiVisibility.h"
#include "rando/SongEventDisplay.h"
#include "rando/SongEvents.h"
#include <ship/window/gui/GuiWindow.h>
#include <cmath>

#ifdef COMBO_BUILD
#include "menu/ComboMenuSharedContext.h"
#endif

namespace {
namespace Songs = ComboRando::SongEvents;
namespace Atlas = ComboRando::SongEvents::Atlas;
constexpr float kSongMarkerDistance = 300.0f;
constexpr float kSongMarkerFullOpacity = 185.0f;
constexpr double kBobSeconds = 4.0;
static_assert(std::size(Atlas::kIconRegions) == std::size(Songs::SongLabels));
static_assert(std::size(Atlas::kLabelRegions) == std::size(Songs::SongLabels));
static_assert(std::size(Atlas::kGlintRegions) == std::size(Songs::SongLabels));
bool sSongMarkerRegistered = false;

bool CanDrawSongMarkers(const PlayState* play) {
    return SongMarkerHost::IsEnabled() && play && play->state.running && !UiVisibility_IsHidden() &&
           gSaveContext.gameMode == GAMEMODE_NORMAL && play->roomCtx.curRoom.num >= 0 && play->pauseCtx.state == 0 &&
           play->csCtx.state == CS_STATE_IDLE && play->transitionTrigger == TRANS_TRIGGER_OFF &&
           play->transitionMode == TRANS_MODE_OFF && play->gameOverCtx.state == GAMEOVER_INACTIVE &&
           play->msgCtx.msgMode == MSGMODE_NONE;
}

void DrawSongMarker(ImDrawList* draw, ImTextureID texture, PickupCounter::Point anchor, int song, float scale,
                    float alpha, double time, bool showName) {
    const auto& icon = Atlas::kIconRegions[song];
    const auto& label = Atlas::kLabelRegions[song];
    const float texel = scale * 0.5f; // The atlas is authored at twice its display size.
    const float iconWidth = icon.width * texel;
    const float iconHeight = icon.height * texel;
    const float labelWidth = showName ? label.width * texel : 0.f;
    const float gap = showName ? 5.f * scale : 0.f;
    const float x = anchor.x - (iconWidth + gap + labelWidth) * 0.5f;
    const float cycle = static_cast<float>(std::fmod(time, kBobSeconds) / kBobSeconds);
    const float bob = 4.f * scale * std::sin(6.2831853f * cycle);
    const float centerY = anchor.y - 14.f * scale + bob;
    const float y = centerY - iconHeight * 0.5f;
    const ImU32 tint = PickupCounter::Ink(255, 255, 255, alpha);
    draw->AddImage(texture, ImVec2(x, y), ImVec2(x + iconWidth, y + iconHeight), icon.Uv(0, 0), icon.Uv(1, 1), tint);
    if (showName) {
        const float labelX = x + iconWidth + gap;
        const float halfLabelHeight = label.height * texel * 0.5f;
        draw->AddImage(texture, ImVec2(labelX, centerY - halfLabelHeight),
                       ImVec2(labelX + labelWidth, centerY + halfLabelHeight), label.Uv(0, 0), label.Uv(1, 1), tint);
    }

    const auto& glint = Atlas::kGlintRegions[song];
    const float sweep = 1.3f * cycle - 0.15f;
    for (int band = -1; band <= 1; ++band) {
        const float left = std::clamp(sweep + (band - 0.5f) * 0.075f, 0.f, 1.f);
        const float right = std::clamp(sweep + (band + 0.5f) * 0.075f, 0.f, 1.f);
        if (right > left) {
            draw->AddImage(texture, ImVec2(x + left * iconWidth, y), ImVec2(x + right * iconWidth, y + iconHeight),
                           glint.Uv(left, 0), glint.Uv(right, 1),
                           PickupCounter::Ink(255, 255, 255, alpha * (band == 0 ? 0.25f : 0.10f)));
        }
    }
}

class SongEventMarkerWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override {
    }
    void UpdateElement() override {
    }
    void DrawElement() override {
    }

    void Draw() override {
        PlayState* play = gPlayState;
        if (!CanDrawSongMarkers(play))
            return;
#ifdef COMBO_BUILD
        ComboMenuContext::UseSharedImGuiContext();
#endif
        Player* player = GET_PLAYER(play);
        WorldHud::Viewport viewport;
        if (!player || (player->stateFlags1 & PLAYER_STATE1_DEAD) || Player_InCsMode(play) ||
            !WorldHud::GetViewport(viewport) || viewport.clip.width < 1 || viewport.clip.height < 1)
            return;

        const float scale = viewport.image.height / 720.0f;
        const auto& clip = viewport.clip;
        const bool mirrored = SongMarkerHost::IsMirrored();
        const bool showName = Songs::ShowSongEventName();
        // ImGui advances on rendered frames, independent from 20 Hz game updates.
        const double time = ImGui::GetTime();
        ImTextureID atlas{};
        auto* draw = ImGui::GetForegroundDrawList(viewport.imguiViewport);
        draw->PushClipRect(ImVec2(clip.x, clip.y), ImVec2(clip.x + clip.width, clip.y + clip.height), true);
        for (int category : SongMarkerHost::kActorCategories) {
            for (Actor* actor = SongMarkerHost::FirstActor(play, category); actor; actor = actor->next) {
                if (actor->room >= 0 && actor->room != play->roomCtx.curRoom.num)
                    continue;
                Vec3f position;
                const int song = SongMarkerHost::MarkerSong(actor, player, play, position);
                if (song < 0 || static_cast<size_t>(song) >= std::size(Atlas::kIconRegions))
                    continue;

                const Vec3f& from = player->actor.world.pos;
                const float dx = position.x - from.x, dy = position.y - from.y, dz = position.z - from.z;
                const float distanceSq = dx * dx + dy * dy + dz * dz;
                if (distanceSq >= kSongMarkerDistance * kSongMarkerDistance)
                    continue;
                PickupCounter::Point anchor;
                if (!viewport.Project(play->viewProjectionMtxF, position, mirrored, anchor))
                    continue;
                Vec3f eye = play->view.eye;
                Vec3f hit;
                CollisionPoly* poly = nullptr;
                s32 bgId;
                if (BgCheck_CameraLineTest2(&play->colCtx, &eye, &position, &hit, &poly, true, true, true, false,
                                            &bgId))
                    continue;

                const float alpha = std::min(1.f, (kSongMarkerDistance - std::sqrt(distanceSq)) /
                                                      (kSongMarkerDistance - kSongMarkerFullOpacity));
                if (!atlas)
                    atlas = Atlas::GetTexture(*viewport.gui, *viewport.interpreter);
                if (atlas)
                    DrawSongMarker(draw, atlas, anchor, song, scale, alpha, time, showName);
            }
        }
        draw->PopClipRect();
    }
};

void EnsureSongEventMarkerWindow() {
    if (sSongMarkerRegistered || !CanDrawSongMarkers(gPlayState))
        return;
    auto* context = Ship::Context::GetRawInstance();
    auto window = context ? context->GetWindow() : nullptr;
    auto gui = window ? window->GetGui() : nullptr;
    if (gui) {
        gui->AddGuiWindow(std::make_shared<SongEventMarkerWindow>(SongMarkerHost::kCvar, true, SongMarkerHost::kName));
        sSongMarkerRegistered = true;
    }
}
} // namespace
