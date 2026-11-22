#pragma once

#include "PickupCounter.h"
#include <fast/Fast3dGui.h>
#include <fast/interpreter.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>

namespace WorldHud {
struct Viewport {
    std::shared_ptr<Fast::Fast3dGui> gui;
    std::shared_ptr<Fast::Interpreter> interpreter;
    ImGuiViewport* imguiViewport = nullptr;
    PickupCounter::Rect image;
    PickupCounter::Rect clip;

    bool Project(const MtxF& matrix, const Vec3f& point, bool mirrored, PickupCounter::Point& anchor) const {
        const float x = matrix.xx * point.x + matrix.xy * point.y + matrix.xz * point.z + matrix.xw;
        const float y = matrix.yx * point.x + matrix.yy * point.y + matrix.yz * point.z + matrix.yw;
        const float w = matrix.wx * point.x + matrix.wy * point.y + matrix.wz * point.z + matrix.ww;
        return PickupCounter::Project(interpreter->AdjXForAspectRatio(x), y, w, image, mirrored, anchor);
    }
};

inline bool GetViewport(Viewport& result) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return false;
    }
    auto* context = Ship::Context::GetRawInstance();
    auto window = context ? context->GetWindow() : nullptr;
    auto gui = window ? std::dynamic_pointer_cast<Fast::Fast3dGui>(window->GetGui()) : nullptr;
    auto fastWindow = std::dynamic_pointer_cast<Fast::Fast3dWindow>(window);
    auto interpreter = fastWindow ? fastWindow->GetInterpreterWeak().lock() : nullptr;
    if (!gui || !interpreter || gui->GetMenuOrMenubarVisible()) {
        return false;
    }
    auto* viewport = ImGui::GetMainViewport();
    if (!viewport) {
        return false;
    }
    const auto& game = interpreter->mGameWindowViewport;
    PickupCounter::Rect rect{ viewport->Pos.x + game.x, viewport->Pos.y + game.y, static_cast<float>(game.width),
                              static_cast<float>(game.height) };
    const PickupCounter::Rect gameBounds = rect;
    // Match DrawGame's displayed image, including aspect correction and
    // integer scaling. mGameWindowViewport alone includes the black bars.
    const bool lowRes = CVarGetInteger(CVAR_LOW_RES_MODE, 0) == 1;
    const bool advanced = CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 0) != 0;
    if (rect.width < 1 || rect.height < 1 || interpreter->mCurDimensions.width == 0 ||
        interpreter->mCurDimensions.height == 0) {
        return false;
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
            const bool automatic = CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.FitAutomatically", 0);
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
    const float clipX = std::max(rect.x, gameBounds.x), clipY = std::max(rect.y, gameBounds.y);
    result = { gui,
               interpreter,
               viewport,
               rect,
               { clipX, clipY, std::min(rect.x + rect.width, gameBounds.x + gameBounds.width) - clipX,
                 std::min(rect.y + rect.height, gameBounds.y + gameBounds.height) - clipY } };
    return true;
}
} // namespace WorldHud
