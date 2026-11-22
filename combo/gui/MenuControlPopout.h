#pragma once
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <ship/window/gui/Gui.h>
#include <ship/window/gui/GuiWindow.h>
#include <memory>
#include <set>
#include <string>

namespace ComboMenuPopout {
class ControlWindow final : public Ship::GuiWindow {
    std::weak_ptr<Ship::Menu> menu;
    std::string section, sidebar;
    std::set<std::string> cvars;
    bool (*foreground)();
    void (*drawExtra)();

  public:
    ControlWindow(std::shared_ptr<Ship::Menu> source, const char* name, const char* cvar, const char* sectionName,
                  const char* sidebarName, std::set<std::string> selected, ImVec2 size, bool (*isForeground)(),
                  void (*extraControls)())
        : GuiWindow(cvar, name, size), menu(source), section(sectionName), sidebar(sidebarName),
          cvars(std::move(selected)), foreground(isForeground), drawExtra(extraControls) {
    }
    void Rebind(std::shared_ptr<Ship::Menu> source) {
        menu = source;
    }
    void InitElement() override {
    }
    void UpdateElement() override {
    }
    void Draw() override {
        if (!foreground || foreground())
            Ship::GuiWindow::Draw();
    }
    void DrawElement() override {
        if (auto source = menu.lock()) {
            source->DrawPopoutControls(section, sidebar, cvars);
            if (drawExtra)
                drawExtra();
        }
    }
};

inline void EnsureWindow(std::shared_ptr<Ship::Menu> menu, const char* name, const char* cvar, const char* section,
                         const char* sidebar, std::set<std::string> cvars, ImVec2 size, bool (*foreground)(),
                         void (*drawExtra)() = nullptr) {
    auto context = Ship::Context::GetRawInstance();
    if (!menu || !context || !context->GetWindow() || !context->GetWindow()->GetGui())
        return;
    auto gui = context->GetWindow()->GetGui();
    if (auto existing = std::dynamic_pointer_cast<ControlWindow>(gui->GetGuiWindow(name)))
        existing->Rebind(menu);
    else if (!gui->GetGuiWindow(name))
        gui->AddGuiWindow(std::make_shared<ControlWindow>(menu, name, cvar, section, sidebar, std::move(cvars), size,
                                                          foreground, drawExtra));
}
} // namespace ComboMenuPopout
