// Included once per native host after its inventory/receipt adapters. The two
// games own separate state and window names; neither reads the dormant save.
#include "PickupCounterDraw.h"
#include "gui/UiVisibility.h"
#include "WorldHudViewport.h"
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
    explicit PickupCounterWindow(void (*drawSceneIcons)())
        : GuiWindow(PickupCounterHost::kWindowCvar, true, PickupCounterHost::kWindowName),
          drawSceneIcons(drawSceneIcons) {
    }
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
        // World icons have their own settings and render below the pickup rows.
        if (drawSceneIcons) {
            drawSceneIcons();
        }
        if (!PickupSessionValid() || sPickupCounters.Empty() || UiVisibility_IsHidden() || !gPlayState ||
            gPlayState->pauseCtx.state != 0 || gPlayState->csCtx.state != CS_STATE_IDLE ||
            Player_InCsMode(gPlayState) || ImGui::GetCurrentContext() == nullptr) {
            positioned = false;
            return;
        }
        WorldHud::Viewport viewport;
        if (!WorldHud::GetViewport(viewport)) {
            positioned = false;
            return;
        }
        Player* player = GET_PLAYER(gPlayState);
        if (!player || (player->stateFlags1 & PLAYER_STATE1_DEAD) || !player->actor.draw) {
            positioned = false;
            return;
        }
        // The actor's focus follows both ages, custom models and MM forms.
        PC::Point anchor;
        if (!viewport.Project(gPlayState->viewProjectionMtxF, player->actor.focus.pos, PickupCounterHost::IsMirrored(),
                              anchor)) {
            sPickupCounters.Advance(ImGui::GetIO().DeltaTime);
            positioned = false;
            return;
        }
        const float dt = ImGui::GetIO().DeltaTime;
        const PC::Rect& clip = viewport.clip;
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
        const float scale = std::min({ viewport.image.height / 720.0f * uiScale, clip.width / (PC::kRowWidth + 24),
                                       clip.height / (stackHeight + 24) });
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
        auto* draw = ImGui::GetForegroundDrawList(viewport.imguiViewport);
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
                    texture = viewport.gui->GetTextureByName(path);
                }
            }
            PC::DrawRow(draw, row, ImVec2(position.x, rowY), scale, texture, PickupCounterHost::Font());
            rowY += PC::kRowStride * scale;
        }
        draw->PopClipRect();
        sPickupCounters.Advance(dt);
    }

  private:
    void (*drawSceneIcons)();
    PC::Point position;
    bool positioned = false;
};

void EnsurePickupCounterWindow(void (*drawSceneIcons)() = nullptr) {
    if (sPickupRegistered) {
        return;
    }
    auto* context = Ship::Context::GetRawInstance();
    auto window = context ? context->GetWindow() : nullptr;
    auto gui = window ? window->GetGui() : nullptr;
    if (!gui) {
        return;
    }
    gui->AddGuiWindow(std::make_shared<PickupCounterWindow>(drawSceneIcons));
    sPickupRegistered = true;
}
} // namespace
