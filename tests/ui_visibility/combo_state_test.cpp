#include <algorithm>
#include <cassert>
#include <cstdio>
#include <imgui.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

static bool hidden;
static int activeSlot;
static int foreground = 1;
static std::map<std::string, float> cvars;
static ImGuiContext *sharedContext;
static ImGuiContext *otherContext;
int UiVisibility_IsHidden() { return hidden; }
int CVarGetInteger(const char *name, int fallback) {
  const auto it = cvars.find(name);
  return it == cvars.end() ? fallback : static_cast<int>(it->second);
}
float CVarGetFloat(const char *name, float fallback) {
  const auto it = cvars.find(name);
  return it == cvars.end() ? fallback : it->second;
}
void CVarSetInteger(const char *name, int value) { cvars[name] = value; }
void CVarSetFloat(const char *name, float value) { cvars[name] = value; }

namespace Ship {
struct Gui {
  int saves = 0;
  ImGuiContext *GetImGuiContext() { return sharedContext; }
  void SaveConsoleVariablesNextFrame() { ++saves; }
};
struct Window {
  Gui gui;
  Gui *GetGui() { return &gui; }
};
struct Context {
  Window window;
  static Context *GetRawInstance() {
    static Context context;
    return &context;
  }
  Window *GetWindow() { return &window; }
};
struct CrossRMRegistry {
  static bool Get(const char *) { return true; }
};
struct GuiWindow {
  bool visible = true;
  virtual ~GuiWindow() = default;
  virtual void Draw() {}
  virtual void InitElement() {}
  virtual void UpdateElement() {}
  virtual void DrawElement() {}
  bool IsVisible() { return visible; }
  void Update() { UpdateElement(); }
};
} // namespace Ship

namespace ComboUI {
int GetForegroundGame() { return foreground; }
} // namespace ComboUI
namespace ComboTracker {
constexpr int kSwapCount = 2;
struct Win {
  const char *cvar;
  const char *name;
};
struct Kind {
  int column;
  const char *enabledCvar;
  const char *hideBgCvar;
  const char *mmPlacedCvar;
};
static const Kind kKinds[] = {{0, "items", "items.hideBg", "items.placed"},
                              {2, "checks", "checks.hideBg", "checks.placed"}};
static const Win kTrackers[2][4] = {
    {{"oot.items", "OOT items"},
     {"oot.items.settings", "OOT item settings"},
     {"oot.checks", "OOT checks"},
     {"oot.checks.settings", "OOT check settings"}},
    {{"mm.items", "MM items"},
     {"mm.items.settings", "MM item settings"},
     {"mm.checks", "MM checks"},
     {"mm.checks.settings", "MM check settings"}},
};
int OotActiveSlot() { return activeSlot; }
bool ForegroundPaused(int) { return true; }
void SetTracker(const Win &window, bool shown) {
  CVarSetInteger(window.cvar, shown);
}
} // namespace ComboTracker

namespace tracker_case {
using ComboTracker::kKinds;
using ComboTracker::kTrackers;
using ComboTracker::OotActiveSlot;
struct Peek {
  bool holdArmed = false;
  bool held = false;
};
static Peek sPeeks[ComboTracker::kSwapCount];
struct PausedOnly {
  const char *ootCvar;
  const char *ootWindowTypeCvar;
  int ootWindowTypeDefault;
  const char *mmCvar;
};
constexpr PausedOnly kPausedOnly[] = {
    {"oot.items.paused", "oot.items.type", 0, "mm.items.paused"},
    {"oot.checks.paused", "oot.checks.type", 0, "mm.checks.paused"},
};
const char *KindWindowName(const ComboTracker::Kind &kind, int game) {
  return kTrackers[game][kind.column].name;
}
void EnsureMmSaveLoadedForDormantDraw() {}
void SkipSizingFrame(const char *) {}
void PlaceMmWindowOnFirstShow(int) {}
class SwapWindow : public Ship::GuiWindow {
public:
  void UpdateElement() override;
};
/* TRACKER_FUNCTIONS */
void Test() {
  cvars.clear();
  hidden = false;
  foreground = 1;
  cvars["items"] = 1;
  cvars["checks"] = 1;
  cvars["items.hideBg"] = 1;
  sPeeks[0] = {true, true}; // A hold began before L hid the UI.
  SwapWindow window;
  window.Update();
  assert(CVarGetInteger("oot.items", 0) == 1 && sPeeks[0].held);
  hidden = true;
  ImGui::SetCurrentContext(otherContext);
  window.Update(); // The GUI deliberately never calls hidden Draw().
  assert(CVarGetInteger("mm.items", 0) == 1);
  assert(CVarGetInteger("oot.items", 0) == 0);
  assert(CVarGetInteger("oot.checks", 0) == 1);
  assert(!sPeeks[0].holdArmed && !sPeeks[0].held);
  assert(CVarGetInteger("items", 0) == 1 && CVarGetInteger("checks", 0) == 1);
  foreground = 0;
  window.Update();
  assert(CVarGetInteger("oot.items", 0) == 1 &&
         CVarGetInteger("mm.items", 1) == 0);
  assert(ImGui::GetCurrentContext() == sharedContext);
  std::puts("PASS combo hidden tracker reconciliation and stale-hold release");
}
} // namespace tracker_case

namespace ComboRando {
struct GameMenu {
  void (*applyCVarChange)(const char *) = nullptr;
};
struct ComboMenuModel {
  static ComboMenuModel &Get() {
    static ComboMenuModel model;
    return model;
  }
  void EnsureLoaded() {}
  GameMenu Mm() { return {}; }
  GameMenu Oot() { return {}; }
};
} // namespace ComboRando
namespace sync_case {
constexpr const char *kCvarEnabled = "sync";
enum SyncType { SYNC_INT, SYNC_FLOAT, SYNC_TEXT_SPEED };
enum SyncGroup { GROUP_GAMEPLAY };
struct SyncPair {
  const char *mm;
  const char *oot;
  SyncType type;
  SyncGroup group;
  float mmMin;
  float mmMax;
};
struct Group {
  const char *cvar;
};
static const Group kGroups[] = {{"sync.gameplay"}};
static const SyncPair kPairs[] = {
    {"mm.fastText", "oot.textSpeed", SYNC_TEXT_SPEED, GROUP_GAMEPLAY, 0, 0}};
constexpr size_t kPairCount = 1;
struct Shadow {
  float mm = 0;
  float oot = 0;
  bool armed = false;
};
static Shadow sShadow[kPairCount];
/* SYNC_FUNCTIONS */
/* SYNC_CLASS */
void Test() {
  cvars.clear();
  hidden = true;
  cvars["oot.textSpeed"] = 1;
  SyncWindow window;
  window.Update(); // Record initial settings without forcing agreement.
  cvars["mm.fastText"] = 1;
  ImGui::SetCurrentContext(otherContext);
  window.Update();
  assert(CVarGetInteger("oot.textSpeed", 0) == 6);
  assert(CVarGetInteger("mm.fastText", 0) == 1);
  assert(ImGui::GetCurrentContext() == sharedContext);
  std::puts("PASS combo hidden settings changes reach the other game");
}
} // namespace sync_case

namespace timer_case {
constexpr ComboTracker::Win kNativeOverlays[] = {
    {"native.oot.timer", "OOT timer"},
    {"native.mm.timer", "MM timer"},
};
class ComboTimersWindow : public Ship::GuiWindow {
public:
  void UpdateElement() override;
};
/* TIMER_FUNCTIONS */
void Test() {
  cvars.clear();
  hidden = true;
  cvars["native.oot.timer"] = 1;
  cvars["native.mm.timer"] = 2;
  cvars["combo.timer"] = 1;
  ComboTimersWindow window;
  ImGui::SetCurrentContext(otherContext);
  window.Update();
  assert(CVarGetInteger("native.oot.timer", 1) == 0);
  assert(CVarGetInteger("native.mm.timer", 1) == 0);
  assert(CVarGetInteger("combo.timer", 0) == 1);
  assert(ImGui::GetCurrentContext() == sharedContext);
  std::puts("PASS combo hidden native timer suppression retains combined timer "
            "intent");
}
} // namespace timer_case

namespace notes_case {
static const char *(*sGet)(int);
static void (*sSet)(int, const char *);
static std::string sBuf;
static int sLoadedSlot = -1;
static bool sDirty;
static int sDirtySlot = -1;
static double sLastEdit;
static std::map<int, std::string> notes = {{0, "first"}, {1, "second"}};
namespace ComboNotes {
void FlushPending();
}
class ComboNotesWindow : public Ship::GuiWindow {
public:
  void UpdateElement() override;
};
/* NOTES_FUNCTIONS */
void Edit(const char *text) {
  sBuf = text;
  sDirty = true;
  sDirtySlot = sLoadedSlot;
  sLastEdit = ImGui::GetTime();
}
void Test() {
  sGet = [](int slot) { return notes[slot].c_str(); };
  sSet = [](int slot, const char *value) { notes[slot] = value; };
  ComboNotesWindow window;
  activeSlot = 0;
  hidden = false;
  ImGui::SetCurrentContext(otherContext);
  window.Update();
  assert(sBuf == "first");
  Edit("saved when L hides the editor");
  hidden = true;
  window.Update();
  assert(notes[0] == "saved when L hides the editor" && !sDirty &&
         window.IsVisible());
  Edit("saved to original slot");
  activeSlot = 1;
  window.Update();
  assert(notes[0] == "saved to original slot" && sBuf == "second");
  hidden = false;
  Edit("debounced without drawing");
  sLastEdit = ImGui::GetTime() - 3.0;
  window.Update();
  assert(notes[1] == "debounced without drawing" && !sDirty);
  Edit("saved on actual close");
  window.visible = false;
  window.Update();
  assert(notes[1] == "saved on actual close" && sLoadedSlot == -1);
  assert(ImGui::GetCurrentContext() == sharedContext);
  std::puts("PASS combo notes hidden focus, debounce, slot change and close "
            "persistence");
}
} // namespace notes_case

int main(int argc, char **argv) {
  sharedContext = ImGui::CreateContext();
  otherContext = ImGui::CreateContext();
  ImGui::SetCurrentContext(sharedContext);
  ImGui::GetIO().DisplaySize = ImVec2(1280, 720);
  ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
  unsigned char *pixels;
  int width, height;
  ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  ImGui::NewFrame();
  const std::string selected = argc > 1 ? argv[1] : "all";
  if (selected == "all" || selected == "tracker")
    tracker_case::Test();
  if (selected == "all" || selected == "sync")
    sync_case::Test();
  if (selected == "all" || selected == "timer")
    timer_case::Test();
  if (selected == "all" || selected == "notes")
    notes_case::Test();
  ImGui::Render();
  assert(ImGui::GetDrawData()->TotalVtxCount == 0);
  ImGui::DestroyContext(sharedContext);
  ImGui::DestroyContext(otherContext);
}
