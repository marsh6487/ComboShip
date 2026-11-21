#pragma once

#ifdef COMBO_BUILD
#include "Rando/EntranceTrackerNames.h"
#include <ship/window/gui/GuiWindow.h>

namespace Rando::EntranceTracker {
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
