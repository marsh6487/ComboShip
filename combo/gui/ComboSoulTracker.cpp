#include "ComboSoulTracker.h"
#include "ComboTrackerCommon.h"
#include "ComboMenuSharedContext.h"
#include "ComboWidgetStyle.h"
#include "SoulTrackerView.h"

#include <memory>

namespace ComboRando::SoulTracker {
namespace {

constexpr const char* WindowName = "Souls Tracker";
constexpr const char* VisibilityCvar = "gCombo.SoulTracker.Enabled";

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
                static const auto readFirstSlain =
                    reinterpret_cast<ReadFirstSlain>(Combo_ResolveSym("soh", "SOH_ReadFirstSlainSave"));
                DrawFirstSlain(slot, readFirstSlain);
                DrawGame(slot, read, mShowMissing);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Majora's Mask")) {
                static const auto read =
                    reinterpret_cast<ReadSave>(Combo_ResolveSym("2ship", "MM_ReadSoulTrackerSave"));
                static const auto readFirstSlain =
                    reinterpret_cast<ReadFirstSlain>(Combo_ResolveSym("2ship", "MM_ReadFirstSlainSave"));
                DrawFirstSlain(slot, readFirstSlain);
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
