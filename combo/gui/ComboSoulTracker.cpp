#include "ComboSoulTracker.h"
#include "ComboTrackerCommon.h"
#include "ComboMenuSharedContext.h"
#include "ComboWidgetStyle.h"
#include "rando/SoulTrackerData.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace ComboRando::SoulTracker {
namespace {

constexpr const char* WindowName = "Souls Tracker";
constexpr const char* VisibilityCvar = "gCombo.SoulTracker.Enabled";
constexpr std::array<const char*, static_cast<size_t>(Family::Count)> FamilyNames = {
    "Bosses", "Enemies", "NPCs / Shopkeepers", "Animals", "Miscellaneous", "Bean Patches",
};

struct Row {
    const char* name;
    bool owned;
};
using SoulRows = std::array<std::vector<Row>, FamilyNames.size()>;

void DrawGame(int slot, ReadSave readSave, bool showMissing) {
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

class SoulWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void Draw() override {
        if (!IsVisible())
            return;
        ComboMenuContext::UseSharedImGuiContext();
        GuiWindow::Draw();
    }

  protected:
    void InitElement() override {
    }
    void UpdateElement() override {
    }
    void DrawElement() override {
        const int slot = ComboTracker::OotActiveSlot();
        if (slot < 0) {
            ImGui::TextDisabled("Load a randomizer save to view your souls.");
            return;
        }

        ImGui::Checkbox("Show missing souls", &mShowMissing);
        if (ImGui::BeginTabBar("##SoulGames")) {
            if (ImGui::BeginTabItem("Ocarina of Time")) {
                static const auto read = reinterpret_cast<ReadSave>(Combo_ResolveSym("soh", "SOH_ReadSoulTrackerSave"));
                DrawGame(slot, read, mShowMissing);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Majora's Mask")) {
                static const auto read =
                    reinterpret_cast<ReadSave>(Combo_ResolveSym("2ship", "MM_ReadSoulTrackerSave"));
                DrawGame(slot, read, mShowMissing);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

  private:
    bool mShowMissing = false;
};

std::shared_ptr<SoulWindow> sWindow;

} // namespace

void RegisterWindow() {
    auto gui = Ship::Context::GetRawInstance()->GetWindow()->GetGui();
    sWindow = std::make_shared<SoulWindow>(VisibilityCvar, WindowName, ImVec2(440.0f, 450.0f),
                                           ImGuiWindowFlags_NoFocusOnAppearing);
    gui->AddGuiWindow(sWindow);
}

void DrawSettings() {
    if (!sWindow)
        return;
    ComboMenu_PushButton(ComboMenu_ThemeColor());
    if (ImGui::Button(sWindow->IsVisible() ? "Close Souls Tracker" : "Open Souls Tracker"))
        sWindow->ToggleVisibility();
    ComboMenu_PopButton();
}

} // namespace ComboRando::SoulTracker
