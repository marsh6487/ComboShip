#pragma once

#ifdef COMBO_BUILD
#include <string>
#include <ship/window/gui/GuiWindow.h>

namespace ComboRando::CrossDungeon {
struct Dungeon;
} // namespace ComboRando::CrossDungeon

namespace Rando::EntranceTracker {
std::string PortalSourceName(const ComboRando::CrossDungeon::Dungeon& portal);

class Window final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

  protected:
    void InitElement() override {
    }
    void UpdateElement() override {
    }
    void DrawElement() override;
};
} // namespace Rando::EntranceTracker
#endif
