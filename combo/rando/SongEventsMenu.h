#pragma once
#include "SongEvents.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
#include <string>
#include <libultraship/bridge/consolevariablebridge.h>
namespace ComboRando::SongEvents {
inline std::string PlandoKey(Game game, unsigned event) {
    return std::string(game == Game::Oot ? "gRandoSettings.SongEventPlando." : "gRando.Options.SongEventPlando.") +
           (game == Game::Oot ? OotNames[event] : MmNames[event]);
}
inline bool DrawPlando(Game game) {
    if (!ImGui::CollapsingHeader("Song Event Assignments"))
        return false;
    ImGui::TextWrapped("Random uses the seed. Overrides are saved in the generated song-event mapping.");
    bool changed = false;
    for (unsigned event = 0; event < Defaults(game).size(); ++event) {
        auto key = PlandoKey(game, event);
        int value = CVarGetInteger(key.c_str(), -1);
        const char* preview = value >= 0 && value < 20 && Playable(game, value) ? SongLabels[value] : "Random";
        ImGui::PushID(key.c_str());
        if (ImGui::BeginCombo(game == Game::Oot ? OotLabels[event] : MmLabels[event], preview)) {
            if (ImGui::Selectable("Random", value < 0)) {
                CVarSetInteger(key.c_str(), -1);
                changed = true;
            }
            for (unsigned song = 0; song < 20; ++song) {
                if (Playable(game, song) && ImGui::Selectable(SongLabels[song], value == int(song))) {
                    CVarSetInteger(key.c_str(), song);
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopID();
    }
    if (changed)
        CVarSave();
    return changed;
}
} // namespace ComboRando::SongEvents
