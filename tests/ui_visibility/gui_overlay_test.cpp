// Native GameOverlay lifetime/submission boundary. The text renderer and CVar
// storage are external services; hidden frames must not call either service.
#include <cassert>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

static int hidden, textCalls, cvarReads;
extern "C" int UiVisibility_IsHidden(void) { return hidden; }

namespace Ship {
enum class OverlayType { TEXT, IMAGE, NOTIFICATION };
struct Overlay {
  OverlayType Type;
  std::string Value;
  float fadeTime;
  float duration;
};
enum class ConsoleVariableType { Float, Integer, String, Color, Color24 };
struct CVar {
  ConsoleVariableType Type = ConsoleVariableType::Integer;
  float Float = 0;
  int Integer = 0;
  const char *String = "";
  struct {
    unsigned r, g, b, a;
  } Color, Color24;
};
struct ConsoleVariables {
  auto Get(const char *) {
    ++cvarReads;
    return std::make_shared<CVar>();
  }
};
struct Context {
  static Context *GetRawInstance() {
    static Context context;
    return &context;
  }
  auto GetConsoleVariables() { return std::make_shared<ConsoleVariables>(); }
};
class GameOverlay {
public:
  std::unordered_map<std::string, Overlay> mRegisteredOverlays;
  bool mNeedsCleanup = false;
  void CleanupNotifications();
  void Draw();
  void TextDraw(float, float, bool, ImVec4, const char *, ...) { ++textCalls; }
  float GetStringWidth(const char *) { return 10; }
  float GetScreenWidth() { return 640; }
  float GetScreenHeight() { return 480; }
};
/* PRODUCTION_OVERLAY */
} // namespace Ship

int main() {
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(640, 480);
  io.DeltaTime = 1.0f / 60.0f;
  io.IniFilename = nullptr;
  io.Fonts->Build();
  Ship::GameOverlay overlay;
  overlay.mRegisteredOverlays["brief"] = {Ship::OverlayType::NOTIFICATION,
                                          "brief notice", .10f, .10f};
  overlay.mRegisteredOverlays["long"] = {Ship::OverlayType::NOTIFICATION,
                                         "long notice", 1.0f, 1.0f};
  overlay.mNeedsCleanup = true;
  hidden = true;
  ImGui::NewFrame();
  overlay.Draw();
  assert(textCalls == 0 && cvarReads == 0);
  assert(overlay.mRegisteredOverlays["brief"].duration < .10f);
  assert(ImGui::FindWindowByName("GameOverlay") == nullptr);
  ImGui::Render();
  // Registered TEXT is kept without accessing CVar storage or its text
  // renderer.
  overlay.mRegisteredOverlays["text"] = {Ship::OverlayType::TEXT, "gTest.Label",
                                         0, 0};
  ImGui::NewFrame();
  overlay.Draw();
  ImGui::Render();
  assert(overlay.mRegisteredOverlays.count("brief") == 0);
  assert(overlay.mRegisteredOverlays.count("text") == 1);
  assert(textCalls == 0 && cvarReads == 0);
  overlay.mRegisteredOverlays.erase(
      "text"); // The unrelated existing visible TEXT free is outside this test.
  hidden = false;
  ImGui::NewFrame();
  overlay.Draw();
  ImGui::Render();
  assert(textCalls == 1 && overlay.mRegisteredOverlays.count("brief") == 0);
  assert(overlay.mRegisteredOverlays["long"].duration < .91f);
  hidden = true;
  for (int frame = 0; frame < 30; ++frame) {
    ImGui::NewFrame();
    overlay.Draw();
    ImGui::Render();
  }
  assert(overlay.mRegisteredOverlays.empty() && textCalls == 1);
  ImGui::DestroyContext();
  std::cout << "PASS hidden overlay lifetime: no text/CVar access, expiry "
               "cleanup, no stale replay on reveal\n";
}
