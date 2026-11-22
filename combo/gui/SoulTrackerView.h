#pragma once

#include "../rando/SoulTrackerData.h"
#include <imgui.h>
#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace ComboRando::SoulTracker {

inline constexpr std::array<const char*, static_cast<size_t>(Family::Count)> FamilyNames = {
    "Bosses", "Enemies", "NPCs / Shopkeepers", "Animals", "Miscellaneous", "Bean Patches",
};

struct Row {
    const char* name;
    bool owned;
};
using SoulRows = std::array<std::vector<Row>, FamilyNames.size()>;

inline void DrawGame(int slot, ReadSave readSave, bool showMissing) {
    SoulRows rows;
    const auto append = [](void* context, Family family, const char* name, int32_t owned) {
        auto& groups = *static_cast<SoulRows*>(context);
        groups[static_cast<size_t>(family)].push_back({ name, owned != 0 });
    };
    if (!readSave || !readSave(slot, append, &rows)) {
        ImGui::TextDisabled("This game's randomizer save is unavailable.");
        return;
    }

    size_t ownedTotal = 0;
    size_t rowTotal = 0;
    std::array<size_t, FamilyNames.size()> ownedCounts{};
    for (size_t family = 0; family < rows.size(); ++family) {
        const auto& group = rows[family];
        ownedCounts[family] = std::count_if(group.begin(), group.end(), [](const Row& row) { return row.owned; });
        ownedTotal += ownedCounts[family];
        rowTotal += group.size();
    }
    if (rowTotal == 0) {
        ImGui::TextDisabled("No souls are shuffled in this game.");
        return;
    }
    ImGui::Text("%zu / %zu collected", ownedTotal, rowTotal);
    if (!showMissing && ownedTotal == 0) {
        ImGui::TextDisabled("No souls collected yet.");
        return;
    }

    for (size_t family = 0; family < rows.size(); ++family) {
        const auto& group = rows[family];
        if (group.empty() || (!showMissing && ownedCounts[family] == 0))
            continue;
        const char* name = FamilyNames[family];
        const auto label = std::string(name) + " (" + std::to_string(ownedCounts[family]) + "/" +
                           std::to_string(group.size()) + ")###" + name;
        if (!ImGui::CollapsingHeader(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            continue;
        for (const auto& row : group) {
            if (row.owned)
                ImGui::TextUnformatted(row.name);
            else if (showMissing)
                ImGui::TextDisabled("%s (missing)", row.name);
        }
    }
}

} // namespace ComboRando::SoulTracker
