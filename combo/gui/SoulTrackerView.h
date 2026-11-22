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

inline constexpr ImU32 FirstSlainAccent = IM_COL32(234, 197, 120, 255);
inline constexpr char FirstSlainHeading[] = "First Slain Enemies";
inline constexpr char FirstSlainLegend[] = "Gold marks a recorded defeat.";

inline constexpr std::array<const char*, 3> FirstSlainLabels = { "Unrecorded", "Defeated", "Collected" };

inline void DrawFirstSlain(int slot, ReadFirstSlain read) {
    if (!read)
        return;

    struct DropRow {
        const char* name;
        FirstSlainStatus status;
    };
    std::vector<DropRow> rows;
    const auto append = [](void* context, const char* name, FirstSlainStatus status) {
        static_cast<std::vector<DropRow>*>(context)->push_back({ name, status });
    };
    if (!read(slot, append, &rows))
        return;
    const auto recorded = std::count_if(rows.begin(), rows.end(),
                                        [](const DropRow& row) { return row.status != FirstSlainStatus::Unrecorded; });
    const auto header = std::string(FirstSlainHeading) + " (" + std::to_string(recorded) + "/" +
                        std::to_string(rows.size()) + ")###FirstSlain";
    if (!ImGui::CollapsingHeader(header.c_str()))
        return;

    const auto gold = ImGui::ColorConvertU32ToFloat4(FirstSlainAccent);
    ImGui::TextColored(gold, "%s", FirstSlainLegend);
    if (rows.empty())
        ImGui::TextDisabled("No first-slay checks in this seed.");
    for (const auto& [name, status] : rows) {
        const char* label = FirstSlainLabels[static_cast<size_t>(status)];
        if (status == FirstSlainStatus::Unrecorded)
            ImGui::TextDisabled("%s (%s)", name, label);
        else
            ImGui::TextColored(gold, "%s (%s)", name, label);
    }
}

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
