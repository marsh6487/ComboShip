// Included once per native host after its inventory/receipt adapters. The two
// games own separate state and window names; neither reads the dormant save.
#include "PickupCounterDraw.h"
#include "gui/UiVisibility.h"
#include <fast/Fast3dGui.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <ship/window/gui/GuiWindow.h>
#include <memory>

namespace {
namespace PC = PickupCounter;
PC::State sPickupCounters;
uint32_t sPickupReceipts = 0;
bool sPickupRegistered = false;

void ResetPickupCounters() {
    sPickupCounters.Reset();
    sPickupReceipts = 0;
}

bool PickupSessionValid() {
    return PickupCounterHost::IsForeground() && CVarGetInteger(PC::kEnabled, 0) && gPlayState &&
           gPlayState->state.running && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           gPlayState->transitionTrigger == TRANS_TRIGGER_OFF && gPlayState->transitionMode == TRANS_MODE_OFF &&
           gPlayState->gameOverCtx.state == GAMEOVER_INACTIVE;
}

void NotePickup(PC::Kind kind) {
    if (PickupSessionValid()) {
        sPickupReceipts |= PC::Mask(kind);
    }
}

class PickupCounterWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void InitElement() override {
    }
    void UpdateElement() override {
        // Gui skips Draw for True Hidden UI; state and receipts must still be
        // drained, or hiding the HUD could replay old pickups when it returns.
        if (!PickupSessionValid()) {
            ResetPickupCounters();
            positioned = false;
            return;
        }
        sPickupCounters.Sample(PickupCounterHost::ReadSnapshot(), sPickupReceipts);
        sPickupReceipts = 0;
        if (UiVisibility_IsHidden()) {
            sPickupCounters.Advance(ImGui::GetIO().DeltaTime);
        }
    }
    void DrawElement() override {
    }
    void Draw() override {
        if (!PickupSessionValid() || sPickupCounters.Empty() || UiVisibility_IsHidden() || !gPlayState ||
            gPlayState->pauseCtx.state != 0 || gPlayState->csCtx.state != CS_STATE_IDLE ||
            Player_InCsMode(gPlayState) || ImGui::GetCurrentContext() == nullptr) {
            positioned = false;
            return;
        }
        auto* context = Ship::Context::GetRawInstance();
        auto window = context ? context->GetWindow() : nullptr;
        auto gui = window ? std::dynamic_pointer_cast<Fast::Fast3dGui>(window->GetGui()) : nullptr;
        auto fastWindow = std::dynamic_pointer_cast<Fast::Fast3dWindow>(window);
        auto interpreter = fastWindow ? fastWindow->GetInterpreterWeak().lock() : nullptr;
        if (!gui || !interpreter || gui->GetMenuOrMenubarVisible()) {
            positioned = false;
            return;
        }
        Player* player = GET_PLAYER(gPlayState);
        if (!player || (player->stateFlags1 & PLAYER_STATE1_DEAD) || !player->actor.draw) {
            positioned = false;
            return;
        }
        const auto* viewport = ImGui::GetMainViewport();
        if (!viewport) {
            return;
        }
        const auto& game = interpreter->mGameWindowViewport;
        PC::Rect rect{ viewport->Pos.x + game.x, viewport->Pos.y + game.y, static_cast<float>(game.width),
                       static_cast<float>(game.height) };
        const PC::Rect gameBounds = rect;
        // Match DrawGame's displayed image, including aspect correction and
        // integer scaling. mGameWindowViewport alone includes the black bars.
        const bool lowRes = CVarGetInteger(CVAR_LOW_RES_MODE, 0) == 1;
        const bool advanced = CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 0) != 0;
        if (rect.width < 1 || rect.height < 1 || interpreter->mCurDimensions.width == 0 ||
            interpreter->mCurDimensions.height == 0) {
            return;
        }
        const float renderWidth = interpreter->mCurDimensions.width;
        const float renderHeight = interpreter->mCurDimensions.height;
        if (lowRes) {
            const float width = rect.height * 4.0f / 3.0f;
            rect.x += std::floor((rect.width - width) * 0.5f);
            rect.width = width;
        } else if (advanced) {
            if (CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".PixelPerfectMode", 0)) {
                const int fit =
                    std::max(1, static_cast<int>(std::min(rect.width / renderWidth, rect.height / renderHeight)));
                const bool automatic =
                    CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.FitAutomatically", 0);
                int factor = CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.Factor", 1);
                if (automatic) {
                    factor = fit + CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.ExceedBoundsBy", 0);
                } else if (CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.NeverExceedBounds", 1)) {
                    factor = std::min(factor, fit);
                }
                const float width = renderWidth * std::max(1, factor);
                const float height = renderHeight * std::max(1, factor);
                rect.x += std::floor((rect.width - width) * 0.5f);
                rect.y += std::floor((rect.height - height) * 0.5f);
                rect.width = width;
                rect.height = height;
            } else if (!CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".IgnoreAspectCorrection", 0)) {
                const float fit = std::min(rect.width / renderWidth, rect.height / renderHeight);
                const float width = renderWidth * fit;
                const float height = renderHeight * fit;
                rect.x += std::floor((rect.width - width) * 0.5f);
                rect.y += std::floor((rect.height - height) * 0.5f);
                rect.width = width;
                rect.height = height;
            }
        }
        // The actor's focus follows both ages, custom models and MM forms.
        const Vec3f focus = player->actor.focus.pos;
        const MtxF& matrix = gPlayState->viewProjectionMtxF;
        const float x = matrix.xx * focus.x + matrix.xy * focus.y + matrix.xz * focus.z + matrix.xw;
        const float y = matrix.yx * focus.x + matrix.yy * focus.y + matrix.yz * focus.z + matrix.yw;
        const float w = matrix.wx * focus.x + matrix.wy * focus.y + matrix.wz * focus.z + matrix.ww;
        PC::Point anchor;
        if (!PC::Project(interpreter->AdjXForAspectRatio(x), y, w, rect, PickupCounterHost::IsMirrored(), anchor)) {
            sPickupCounters.Advance(ImGui::GetIO().DeltaTime);
            positioned = false;
            return;
        }
        const float dt = ImGui::GetIO().DeltaTime;
        const float clipX = std::max(rect.x, gameBounds.x), clipY = std::max(rect.y, gameBounds.y);
        const PC::Rect clip{ clipX, clipY, std::min(rect.x + rect.width, gameBounds.x + gameBounds.width) - clipX,
                             std::min(rect.y + rect.height, gameBounds.y + gameBounds.height) - clipY };
        if (clip.width < 1 || clip.height < 1) {
            return;
        }
        const float configured = CVarGetFloat(PC::kScale, 1.0f);
        const float uiScale = std::isfinite(configured) ? std::clamp(configured, 0.65f, 1.5f) : 1.0f;
        const auto rows = sPickupCounters.VisibleRows();
        size_t count = 0;
        for (const auto& row : rows) {
            count += row.active;
        }
        const float stackHeight = PC::kRowHeight + (count - 1) * PC::kRowStride;
        const float scale = std::min(
            { rect.height / 720.0f * uiScale, clip.width / (PC::kRowWidth + 24), clip.height / (stackHeight + 24) });
        PC::Point target = PC::Place(anchor, clip, PC::kRowWidth * scale, stackHeight * scale, scale);
        const float distance = std::hypot(target.x - position.x, target.y - position.y);
        if (!positioned || distance > 160 * scale) {
            position = target;
            positioned = true;
        } else {
            const float follow = 1 - std::exp(-24.0f * std::clamp(dt, 0.0f, 0.1f));
            position.x += (target.x - position.x) * follow;
            position.y += (target.y - position.y) * follow;
        }
        position = PC::Constrain(position, clip, PC::kRowWidth * scale, stackHeight * scale, scale);
        auto* draw = ImGui::GetForegroundDrawList(const_cast<ImGuiViewport*>(viewport));
        draw->PushClipRect(ImVec2(clip.x, clip.y), ImVec2(clip.x + clip.width, clip.y + clip.height), true);
        float rowY = position.y;
        for (const auto& row : rows) {
            if (!row.active) {
                continue;
            }
            ImTextureID texture{};
            const int item = PickupCounterHost::IconItem(row.kind);
            if (item >= 0) {
                const char* path = reinterpret_cast<const char*>(gItemIcons[item]);
                // Existing tracker preloads own these textures. Missing packs
                // fall back to the authored vector glyph, never a null load.
                if (path) {
                    texture = gui->GetTextureByName(path);
                }
            }
            PC::DrawRow(draw, row, ImVec2(position.x, rowY), scale, texture, PickupCounterHost::Font());
            rowY += PC::kRowStride * scale;
        }
        draw->PopClipRect();
        sPickupCounters.Advance(dt);
    }

  private:
    PC::Point position;
    bool positioned = false;
};

void EnsurePickupCounterWindow() {
    if (sPickupRegistered) {
        return;
    }
    auto* context = Ship::Context::GetRawInstance();
    auto window = context ? context->GetWindow() : nullptr;
    auto gui = window ? window->GetGui() : nullptr;
    if (!gui) {
        return;
    }
    auto hud =
        std::make_shared<PickupCounterWindow>(PickupCounterHost::kWindowCvar, true, PickupCounterHost::kWindowName);
    gui->AddGuiWindow(hud);
    sPickupRegistered = true;
}
} // namespace
