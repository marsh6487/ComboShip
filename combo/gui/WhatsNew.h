#pragma once

// Release-specific presentation only. Keep the actual widget labels, IDs, CVars,
// callbacks and gameplay defaults intact. Replace this inventory for the next release.
#include <algorithm>
#include <cstring>
#include <imgui.h>

namespace WhatsNew {
inline constexpr char kEnabledCvar[] = "gSettings.Menu.WhatsNew";
inline constexpr char kToggleTooltip[] =
    "Highlight this cumulative release's new controls in gold and updated controls in cyan. "
    "Hover a NEW or UPDATED marker to see what changed. Turn off to restore normal menu styling.";

enum class Kind { New, Updated };
struct Entry {
    const char* key;
    Kind kind;
    const char* note;
};

inline constexpr Entry kControls[]{
    { kEnabledCvar, Kind::New, "Optional release highlights, remembered with your menu preferences." },
    { "gEnhancements.AlwaysThawedZorasDomain", Kind::New,
      "Adult Zora's Domain can retain flowing water and installed water textures without changing progression." },
    { "gEnhancements.MmIceArrows", Kind::New,
      "OoT Ice Arrows can create MM-style water platforms and frozen Octorok blocks." },
    { "gEnhancements.DinFireShield", Kind::Updated,
      "Fire-shield stance, orientation and ground clearance have been refined in both games." },
    { "gEnhancements.DinFireShieldSfx", Kind::Updated, "The fire shield retains its optional guarding sound." },
    { "gEnhancements.DinFireSwordDamage", Kind::Updated,
      "Force fire independently of arrows; normal fire imbues unlock with Fire Arrows in the equipment selector." },
    { "gEnhancements.IceSwordDamage", Kind::New,
      "Force ice independently of arrows; normal ice imbues unlock with Ice Arrows in the equipment selector." },
    { "gEnhancements.LightSwordDamage", Kind::New,
      "Force light independently of arrows; normal light imbues unlock with Light Arrows in the equipment selector." },
    { "gMods.TransformMasks.Enabled", Kind::Updated,
      "OoT supports the native Deku, Goron, Zora and Fierce Deity transformation animations and sounds." },
    { "gMods.TransformMasks.InstantTransform", Kind::Updated,
      "Leave Instant Transform off to play the native transformation animation and sound sequence." },
    { "gRandoSettings.MapsCompassesGiveInformation", Kind::Updated,
      "Map and compass information follows the recorded dungeon, entrance and boss-reward ownership." },
    { "gRandoSettings.RodOfSeasons", Kind::Updated,
      "Season presentation includes distinct blush spring grass, forest foliage, weather and audio integration." },
    { "gRando.Options.RO_ROD_OF_SEASONS", Kind::Updated,
      "Shared season modes retain the cumulative foliage, grass, forest-weather and audio integrations." },
    { "gRandoSettings.StartingRodOfSeasons", Kind::Updated,
      "The starting Rod follows the selected individual, Rod or gated season mode." },
    { "gRando.Options.RO_STARTING_ROD_OF_SEASONS", Kind::Updated,
      "The shared starting Rod follows the selected individual, Rod or gated season mode." },
    { "gCombo.BlackMarket.RegularGrannyInterior", Kind::New,
      "Choose the installed Granny interior while retaining the native Black Market shop behavior." },
    { "gCombo.BlackMarket.AnimatedItemSeals", Kind::New,
      "Scrolling X seals mark locked Black Market offers without changing purchase requirements." },
    { "gCombo.Emotes.Enabled", Kind::New,
      "Eight emotes have animation, native sound, volume and shortcut controls in both games and solo modes." },
};

// These custom sections have no backing CVar. Never use substring/prefix matching:
// an older unrelated control must not inherit a release marker by accident.
inline constexpr Entry kSections[]{
    { "Black Market Appearance", Kind::New,
      "Black Market interiors, shelves and animated offer seals have dedicated presentation controls." },
};

inline const Entry* Find(const char* cvar, const char* label = nullptr) {
    if (cvar && cvar[0]) {
        for (const auto& entry : kControls) {
            if (std::strcmp(cvar, entry.key) == 0)
                return &entry;
        }
        return nullptr;
    }
    if (label) {
        for (const auto& entry : kSections) {
            if (std::strcmp(label, entry.key) == 0)
                return &entry;
        }
    }
    return nullptr;
}

inline ImVec4 Accent(Kind kind) {
    return kind == Kind::New ? ImVec4(1.0f, 0.82f, 0.36f, 1.0f) : ImVec4(0.40f, 0.88f, 1.0f, 1.0f);
}

// The marker is a non-interactive text item before the real widget. The original
// widget stays the last item for its tooltip, callbacks and keyboard navigation.
// A scope also restores the color on native early returns and exceptions.
class Scope {
  public:
    Scope(const char* cvar, const char* label, bool enabled, bool disabled = false) {
        const Entry* entry = enabled && !disabled ? Find(cvar, label) : nullptr;
        if (!entry)
            return;
        const ImVec4 normal = ImGui::GetStyle().Colors[ImGuiCol_Text];
        ImGui::PushStyleColor(ImGuiCol_Text, Accent(entry->kind));
        mActive = true;
        const char* marker = entry->kind == Kind::New ? "NEW" : "UPDATED";
        const auto& style = ImGui::GetStyle();
        // Native checkboxes push eight pixels of vertical padding per side,
        // enlarging their square after this shared measurement.
        const float controlWidth = std::max(ImGui::GetFrameHeight(), ImGui::GetTextLineHeight() + 16.0f);
        const float inlineWidth = label ? ImGui::CalcTextSize(label, nullptr, true).x + ImGui::CalcTextSize(marker).x +
                                              controlWidth + style.ItemInnerSpacing.x * 3.0f
                                        : 0.0f;
        const bool inlineMarker = label && inlineWidth <= ImGui::GetContentRegionAvail().x;
        ImGui::TextUnformatted(marker);
        if (ImGui::IsItemHovered()) {
            ImGui::PushStyleColor(ImGuiCol_Text, normal);
            ImGui::SetTooltip("%s: %s", marker, entry->note);
            ImGui::PopStyleColor();
        }
        if (inlineMarker)
            ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
    }
    ~Scope() {
        if (mActive)
            ImGui::PopStyleColor();
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

  private:
    bool mActive = false;
};
} // namespace WhatsNew
