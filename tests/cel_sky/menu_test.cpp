// Execute the real menu builders through a narrow UI registration surface.
// Catches a missing AddMenuElements registration, wrong feature defaults,
// ineffective master toggles, and a reset that clears another feature's state.
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <memory>
#define CVAR_ENHANCEMENT(s) "gEnhancements." s
#define CVAR_SETTING(s) "gSettings." s
#define CVAR_DEVELOPER_TOOLS(s) "gDeveloperTools." s
std::map<std::string, double> cvars;
int CVarGetInteger(const char* key, int fallback) { return cvars.contains(key) ? cvars[key] : fallback; }
void CVarClear(const char* key) { cvars.erase(key); }
struct ImVec2 { float x, y; ImVec2(float x, float y): x(x), y(y) {} };
namespace ImGui {
ImVec2 GetContentRegionAvail() { return {500, 500}; }
ImVec2 CalcTextSize(const char*) { return {8, 10}; }
void SameLine() {} void SetCursorPosY(float) {} float GetCursorPosY() { return 0; }
}
namespace Ship {
struct FakeGui { int saves = 0; void SaveConsoleVariablesNextFrame() { saves++; } };
struct Context {
    FakeGui gui;
    static Context* GetRawInstance() { static Context context; return &context; }
    Context* GetWindow() { return this; } FakeGui* GetGui() { return &gui; }
};
}
namespace UIWidgets {
constexpr int THEME_COLOR = 0;
struct Options {
    double value = 0;
    bool disabled = false;
    const char* disabledTooltip = nullptr;
    Options& DefaultValue(double v) { value = v; return *this; }
    Options& Tooltip(const char*) { return *this; } Options& Min(double) { return *this; }
    Options& Max(double) { return *this; } Options& IsPercentage() { return *this; }
    Options& Format(const char*) { return *this; } Options& ShowButtons(bool) { return *this; }
    Options& Size(ImVec2) { return *this; } Options& Padding(ImVec2) { return *this; }
    Options& Color(int) { return *this; }
};
using CheckboxOptions = Options; using FloatSliderOptions = Options;
using IntSliderOptions = Options; using ButtonOptions = Options;
void CVarSliderFloat(const char*, const char*, const Options&) {}
bool Button(const char*, const Options&) { return false; }
}
namespace SohGui {
using namespace UIWidgets;
constexpr int SECTION_COLUMN_1 = 0, SECTION_COLUMN_2 = 1;
enum WidgetType { WIDGET_CVAR_CHECKBOX, WIDGET_SEPARATOR_TEXT, WIDGET_SEPARATOR, WIDGET_BUTTON, WIDGET_CVAR_SLIDER_FLOAT, WIDGET_CVAR_SLIDER_INT, WIDGET_CUSTOM };
struct WidgetPath { std::string sectionName, sidebarName; int column; };
struct WidgetInfo {
    std::string sidebar, name, key;
    bool isHidden = false;
    double defaultValue = 0;
    std::shared_ptr<UIWidgets::Options> options = std::make_shared<UIWidgets::Options>();
    std::function<void(WidgetInfo&)> preFunc, callback, custom;
    WidgetInfo& CVar(const char* key) { this->key = key; return *this; }
    WidgetInfo& RaceDisable(bool) { return *this; }
    WidgetInfo& Options(const UIWidgets::Options& options) { defaultValue = options.value; return *this; }
    WidgetInfo& PreFunc(std::function<void(WidgetInfo&)> fn) { preFunc = fn; return *this; }
    WidgetInfo& Callback(std::function<void(WidgetInfo&)> fn) { callback = fn; return *this; }
    WidgetInfo& CustomFunction(std::function<void(WidgetInfo&)> fn) { custom = fn; return *this; }
};
namespace MenuInit { std::vector<std::function<void()>>& GetInitFuncs() { static std::vector<std::function<void()>> init; return init; } }
struct SohMenu {
    std::deque<WidgetInfo> widgets;
    bool mMenuElementsInitialized = false;
    void AddMenuEntry(const char*, const char*) {} void AddSidebarEntry(const char*, const char*, int) {}
    void AddMenuElements(); void AddMenuWindWakerStyle();
    void AddMenuSettings() {} void AddMenuEnhancements() {} void AddMenuRandomizer() {}
    void AddMenuNetwork() {} void AddMenuDevTools() {} void InsertSidebarSearch() {}
    WidgetInfo& AddWidget(WidgetPath& path, const char* name, WidgetType) {
        widgets.emplace_back(); auto& w = widgets.back(); w.sidebar = path.sidebarName; w.name = name; return w;
    }
    WidgetInfo& Find(const char* key) { for (auto& w: widgets) if (w.key == key) return w; std::fprintf(stderr,"FAIL: menu missing %s\n",key); std::exit(1); }
};
/* PRODUCTION_MENU */
/* PRODUCTION_MENU_REGISTRATION */
}
static void Require(bool value, const char* why) { if (!value) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); } }
int main() {
    SohGui::SohMenu menu; menu.AddMenuElements();
    auto& cel = menu.Find(CVAR_ENHANCEMENT("Graphics.ToonLighting.Enabled"));
    auto& shadows = menu.Find(CVAR_ENHANCEMENT("Graphics.WorldShadows.Enabled"));
    auto& lights = menu.Find(CVAR_ENHANCEMENT("Graphics.WorldLighting.Enabled"));
    auto& sky = menu.Find(CVAR_ENHANCEMENT("Graphics.WWSky.Enabled"));
    Require(cel.defaultValue == 1 && shadows.defaultValue == 0 && lights.defaultValue == 0 && sky.defaultValue == 0, "donor defaults retained");
    auto& stars = menu.Find(CVAR_ENHANCEMENT("Graphics.WWNightSky.StarCount"));
    stars.preFunc(stars); Require(stars.isHidden, "sky master hides enabled sublayer tuning");
    cvars[CVAR_ENHANCEMENT("Graphics.WWSky.Enabled")] = 1;
    stars.preFunc(stars); Require(!stars.isHidden, "sky master reveals enabled sublayer tuning");
    cvars[CVAR_ENHANCEMENT("Graphics.WWNightSky.Enabled")] = 0;
    stars.preFunc(stars); Require(stars.isHidden, "star sublayer hides its tuning");
    cvars[CVAR_ENHANCEMENT("Graphics.ToonLighting.RampCenter")] = .9;
    cvars[CVAR_ENHANCEMENT("Graphics.WorldShadows.Opacity")] = .75;
    bool reset = false;
    for (auto& w: menu.widgets) if (w.sidebar == "Cel Shading" && w.name == "Reset All to Defaults") { w.callback(w); reset = true; }
    Require(reset && !cvars.contains(CVAR_ENHANCEMENT("Graphics.ToonLighting.RampCenter")), "cel reset clears its tuning");
    Require(cvars.at(CVAR_ENHANCEMENT("Graphics.WorldShadows.Opacity")) == .75, "cel reset preserves shadow tuning");
    Require(Ship::Context::GetRawInstance()->gui.saves == 1, "reset persists once");
    std::puts("cel sky menu registration: PASS");
}
