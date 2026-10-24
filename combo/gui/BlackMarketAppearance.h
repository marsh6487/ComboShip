#pragma once
#include "BlackMarketGrannyResources.h"
#include "BlackMarketLock.h"
#include "WhatsNew.h"
#include <imgui.h>
#include <ship/window/Window.h>

namespace BlackMarketUI {
inline void DrawAppearance() {
    const bool grannyAvailable = BlackMarketGranny::DependenciesAvailable();
    bool grannyInterior = CVarGetInteger(BlackMarketGranny::kCvar, 0) != 0;
    // A saved preference can always be turned off, even after the pack is removed.
    ImGui::BeginDisabled(!grannyAvailable && !grannyInterior);
    {
        WhatsNew::Scope releaseHighlight(BlackMarketGranny::kCvar, "Regular Granny Interior",
                                         CVarGetInteger(WhatsNew::kEnabledCvar, 1) != 0,
                                         !grannyAvailable && !grannyInterior);
        if (ImGui::Checkbox("Regular Granny Interior", &grannyInterior)) {
            CVarSetInteger(BlackMarketGranny::kCvar, grannyInterior ? 1 : 0);
            Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        }
    }
    ImGui::EndDisabled();
    ImGui::SetItemTooltip(
        "Use Djipi's original Granny room with the raised counter. Requires the installed "
        "Djipi 3DE scene maps and 3DS textures. Applies the next time you enter; unchecked keeps the magical room.");
    if (!grannyAvailable)
        ImGui::TextWrapped("Regular interior requires Djipi's scene maps and 3DS textures, plus OoT resources.");
    if (grannyInterior) {
        const char* error = CVarGetString(BlackMarketGranny::kErrorCvar, "");
        if (error && error[0])
            ImGui::TextWrapped("Regular interior unavailable: %s Magical room retained.", error);
    }
    bool animatedSeals = CVarGetInteger(BlackMarketVisual::kSealsCvar, 1) != 0;
    {
        WhatsNew::Scope releaseHighlight(BlackMarketVisual::kSealsCvar, "Animated Item Seals",
                                         CVarGetInteger(WhatsNew::kEnabledCvar, 1) != 0);
        if (ImGui::Checkbox("Animated Item Seals", &animatedSeals)) {
            CVarSetInteger(BlackMarketVisual::kSealsCvar, animatedSeals ? 1 : 0);
            Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
        }
    }
    ImGui::SetItemTooltip("Show the scrolling X around locked offers. This visual preference applies immediately "
                          "in either interior; purchase requirements remain the same.");
}
} // namespace BlackMarketUI
