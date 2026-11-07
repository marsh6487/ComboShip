// Execute the production DrawMenu route with real headless ImGui. Window and
// application services are boundaries; hidden draws must not reach them.
#include <cassert>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#define CVAR_IMGUI_CONTROLLER_NAV "gSettings.ControllerNavigation"

static int hidden;
extern "C" int UiVisibility_IsHidden(void) { return hidden; }

namespace Ship {
class Gui;
class Config {};
class ConsoleVariables {
public:
  int GetInteger(const char *, int value) { return value; }
};
class MouseStateManager {
public:
  unsigned captures = 0;
  void UpdateMouseCapture() { ++captures; }
};
class Window {
public:
  std::shared_ptr<Gui> gui;
  std::shared_ptr<MouseStateManager> mouse =
      std::make_shared<MouseStateManager>();
  int GetWidth() { return 640; }
  int GetHeight() { return 480; }
  auto GetMouseStateManager() { return mouse; }
  auto GetGui() { return gui; }
};
class Context {
public:
  static Context *GetRawInstance() {
    static Context context;
    return &context;
  }
  std::shared_ptr<Window> window = std::make_shared<Window>();
  auto GetWindow() { return window; }
  auto GetConfig() { return std::make_shared<Config>(); }
  auto GetConsoleVariables() { return std::make_shared<ConsoleVariables>(); }
};
class GuiWindow {
public:
  unsigned updates = 0, draws = 0;
  bool visible = true;
  virtual ~GuiWindow() = default;
  virtual void Update() { ++updates; }
  virtual void Draw() {
    if (visible)
      ++draws;
  }
  bool IsVisible() { return visible; }
  void ToggleVisibility() { visible = !visible; }
};
class ConsoleWindow : public GuiWindow {
public:
  void Dispatch(const char *) { assert(false); }
};
class Gui {
public:
  std::shared_ptr<GuiWindow> menu = std::make_shared<GuiWindow>();
  std::shared_ptr<GuiWindow> bar = std::make_shared<GuiWindow>();
  std::map<std::string, std::shared_ptr<GuiWindow>> mGuiWindows;
  ImVec2 mTemporaryWindowPos;
  ImGuiIO *mImGuiIo = &ImGui::GetIO();
  unsigned framebuffers = 0, submissions = 0, floating = 0, saves = 0;
  auto GetMenu() { return menu; }
  auto GetMenuBar() { return bar; }
  bool GetMenuOrMenubarVisible() {
    return menu->IsVisible() || bar->IsVisible();
  }
  auto GetGuiWindow(const std::string &name) { return mGuiWindows.at(name); }
  void UpdateGamepadNavigation() {}
  void DrawMenu();
  void EndDraw();
  void DrawGame() { ++framebuffers; }
  void EndFrame() { ++submissions; }
  void DrawFloatingWindows() { ++floating; }
  void CheckSaveCvars() { ++saves; }
};
/* PRODUCTION_WINDOW_POLICY */
/* PRODUCTION_DRAW_MENU */
} // namespace Ship

int main() {
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(640, 480);
  io.DeltaTime = 1.0f / 60.0f;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.IniFilename = nullptr;
  io.Fonts->Build();
  auto gui = std::make_shared<Ship::Gui>();
  Ship::Context::GetRawInstance()->window->gui = gui;
  const std::vector<std::string> passive = {"Stats",
                                            "Stats##Soh",
                                            "Item Tracker",
                                            "Check Tracker##MM",
                                            "Notifications Window",
                                            "Notifications Window##MM",
                                            "Additional Timers",
                                            "Display Overlay",
                                            "Input Viewer",
                                            "Cane Wheel HUD"};
  const std::vector<std::string> settings = {
      "Input Viewer Settings", "Item Tracker Settings##MM",
      "Time Splits Settings Window", "Configure Controller",
      "2S2H Input Editor"};
  const std::vector<std::string> recovery = {"FileBrowser", "Modal Window",
                                             "Modal Window##MM", "2ship (MM)"};
  for (const auto &group : {passive, settings, recovery})
    for (const auto &name : group)
      gui->mGuiWindows[name] = std::make_shared<Ship::GuiWindow>();
  gui->mGuiWindows["SDLAddRemoveDeviceEventHandler"] =
      std::make_shared<Ship::GuiWindow>();
  for (bool isHidden : {false, true}) {
    for (bool menuOpen : {false, true}) {
      hidden = isHidden;
      gui->menu->visible = menuOpen;
      gui->bar->visible = false;
      std::map<std::string, unsigned> prior;
      for (auto &[name, window] : gui->mGuiWindows)
        prior[name] = window->draws;
      ImGui::NewFrame();
      gui->DrawMenu();
      ImGui::Render();
      for (const auto &name : passive)
        assert(gui->mGuiWindows[name]->draws == prior[name] + !isHidden);
      for (const auto &name : settings)
        assert(gui->mGuiWindows[name]->draws ==
               prior[name] + (!isHidden || menuOpen));
      for (const auto &name : recovery)
        assert(gui->mGuiWindows[name]->draws == prior[name] + 1);
    }
  }
  for (auto &[name, window] : gui->mGuiWindows) {
    assert(window->updates ==
           4); // Includes hotplug and notification expiry while hidden.
    assert(window->visible); // Suppression never calls Hide/SetVisibility.
  }
  assert(gui->menu->updates == 4 && gui->bar->updates == 4 &&
         gui->menu->draws == 2);
  // F1 recovery stays on the production input route with True Hidden armed.
  hidden = true;
  io.AddKeyEvent(ImGuiKey_F1, true);
  ImGui::NewFrame();
  gui->DrawMenu();
  ImGui::Render();
  assert(gui->bar->visible && gui->bar->draws == 1);
  assert(Ship::Context::GetRawInstance()->window->mouse->captures == 1);
  gui->EndDraw();
  assert(gui->framebuffers == 1 && gui->submissions == 1 &&
         gui->floating == 1 && gui->saves == 1);
  Ship::Context::GetRawInstance()->window->gui.reset();
  gui.reset();
  ImGui::DestroyContext();
  std::cout << "PASS GUI routing: passive suppression, live updates, "
               "menu/settings/popup/framebuffer recovery\n";
}
