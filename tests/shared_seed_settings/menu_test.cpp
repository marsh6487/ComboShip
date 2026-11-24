// Execute the complete shared menu builder. Rendering observes navigation and
// the original exported widgets; native widget callbacks remain game-owned.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <imgui.h>
#include "gui/ComboMenuModel.h"
#include "gui/ComboSoulTracker.h"

static std::string currentId;
static std::vector<std::string> navigation;
static std::vector<int> rendered;
static std::vector<int> renderedColumns;
static std::vector<std::pair<std::string, int>> shuffleGroups;
static const ComboRando::GameMenu* renderedOwner;
static int soulsPanelsDrawn;
static int currentColumn = -1;
static bool expandShuffleGroups;
namespace ImGui {
ImVec2 GetContentRegionAvail() { return {1000, 700}; }
ImGuiStyle& GetStyle() { static ImGuiStyle style; return style; }
ImVec2 CalcTextSize(const char*, const char*, bool, float) { return {100, 20}; }
bool BeginChild(const char*, const ImVec2&, ImGuiChildFlags, ImGuiWindowFlags) { return true; }
void EndChild() {}
void SetWindowFontScale(float) {}
void SeparatorText(const char* label) {
    if (currentColumn >= 0) shuffleGroups.emplace_back(label, currentColumn);
}
void TextUnformatted(const char*, const char*) {}
void TextDisabled(const char*, ...) {}
void TextWrapped(const char*, ...) {}
void PushID(const char* id) { currentId = id; }
void PushID(int id) { currentColumn = id; }
void PopID() { currentColumn = -1; }
bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags) {
    shuffleGroups.emplace_back(label, currentColumn);
    return expandShuffleGroups || (flags & ImGuiTreeNodeFlags_DefaultOpen) != 0;
}
void SameLine(float, float) {}
void Dummy(const ImVec2&) {}
ImVec2 GetCursorScreenPos() { return {}; }
ImDrawList* GetWindowDrawList() { static ImDrawList list(nullptr); return &list; }
void MemFree(void* pointer) { std::free(pointer); }
}
ImGuiStyle::ImGuiStyle() { ScrollbarSize = 0; ItemSpacing = {}; }
ImDrawList::ImDrawList(ImDrawListSharedData*) {}
ImDrawList::~ImDrawList() {}
void ImDrawListSplitter::ClearFreeMemory() {}
void ImDrawList::AddRectFilled(const ImVec2&, const ImVec2&, ImU32, float, ImDrawFlags) {}
namespace ComboEmotes { void DrawSettings() {} }
namespace ComboSync { void DrawSyncSharedPanel() {} }
namespace ComboRando {
void SoulTracker::DrawSettings() { ++soulsPanelsDrawn; }
ComboMenuModel& ComboMenuModel::Get() { static ComboMenuModel model; return model; }
void ComboMenuModel::EnsureLoaded() {}
bool ComboMenu_StyledSidebarEntry(const char*, bool, float) { navigation.push_back(currentId); return false; }
void RenderWidget(const CwWidget& widget, const GameMenu& owner, int = 90) {
    rendered.push_back(widget.index);
    renderedColumns.push_back(currentColumn);
    renderedOwner = &owner;
}
void RenderSidebarWidgets(const CwSidebar& sidebar, const GameMenu& owner) {
    for (int i = 0; i < sidebar.widgetCount; ++i) RenderWidget(sidebar.widgets[i], owner);
}
void DrawComboPlandoPanel() {}
void DrawTrackerSharedPanel() {}
void DrawCheckTrackerSharedPanel() {}
void DrawHintTrackerSharedPanel() {}
void DrawTimersSharedPanel() {}
void DrawNetworkSharedPanel() {}
class ComboMenu {
  public:
    std::string mScope = "randomizer", mHubActive, mSearchQuery;
    void DrawSharedPanel();
    void DrawComboPanel() {}
    void DrawSearchResults(const std::string&) {}
};
/* MENU_HELPERS */
/* SHARED_PANEL */
}

static int failures;
static void Check(bool ok, const char* message) {
    if (!ok) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
static bool Has(const char* key) {
    return std::find(navigation.begin(), navigation.end(), key) != navigation.end();
}
static void Draw(ComboRando::ComboMenu& menu, const char* active) {
    navigation.clear(); rendered.clear(); renderedOwner = nullptr;
    renderedColumns.clear(); shuffleGroups.clear();
    soulsPanelsDrawn = 0;
    menu.mHubActive = active;
    menu.DrawSharedPanel();
}
int main() {
    using namespace ComboRando;
    CwWidget stats[2]{};
    stats[0].index = 101; stats[0].cvar = "gRandoSettings.QuarterHeart";
    stats[1].index = 102; stats[1].cvar = "gRandoSettings.SpeedUpgradeRequired";
    CwWidget hints[2]{};
    hints[0].index = 201; hints[0].cvar = "gRandoSettings.MapsCompassesGiveInformation";
    hints[1].index = 202; hints[1].cvar = "gRandoSettings.GossipStoneHints";
    std::vector<CwWidget> shuffles;
    const auto addShuffle = [&](int index, int column, const char* cvar, const char* heading = nullptr) {
        CwWidget widget{};
        widget.index = index;
        widget.column = column;
        widget.cvar = cvar;
        widget.name = heading;
        widget.kind = heading ? CW_SEPARATOR_TEXT : CW_CHECKBOX;
        shuffles.push_back(widget);
    };
    addShuffle(400, 0, nullptr, "Shuffle Items");
    addShuffle(401, 0, "gRandoSettings.ShuffleRitoMask");
    addShuffle(402, 0, "gRandoSettings.ShuffleClock");
    addShuffle(403, 0, "gRandoEnhancements.SongEventGlyphDisplay");
    addShuffle(404, 0, "gRandoSettings.ShufflePots");
    addShuffle(405, 0, "gRandoSettings.Fishsanity");
    addShuffle(406, 0, "gRandoSettings.FishsanityPondCount");
    addShuffle(407, 0, "gRandoSettings.ShuffleFairySpots");
    addShuffle(408, 1, nullptr, "Shuffle Shops & Merchants");
    addShuffle(409, 1, "gRandoSettings.Shopsanity");
    addShuffle(410, 1, "gRandoSettings.ShopsanityPrices");
    addShuffle(411, 2, nullptr, "Souls");
    addShuffle(412, 2, "gRandoSettings.ShuffleEnemySouls");
    addShuffle(413, 2, "gRandoSettings.ShuffleAnimalSouls");
    addShuffle(414, 2, nullptr, "Additional Items");
    addShuffle(415, 2, "gRandoSettings.ShuffleChildWallet");
    addShuffle(416, 2, "gRandoSettings.ShuffleSwim");
    addShuffle(417, 2, "gRandoSettings.ShuffleBeanSouls");
    CwSidebar ootSides[] = {{"Stat Upgrades", 2, stats, 2}, {"Hints/Traps", 2, hints, 2},
                            {"Shuffles", 3, shuffles.data(), static_cast<int>(shuffles.size())}};
    CwSection ootSection{"Randomizer", "", ootSides, 3};
    CwMenu ootMenu{1, &ootSection, 1};
    CwWidget mmWidget{}; mmWidget.index = 301;
    CwSidebar mmSide{"Hints", 1, &mmWidget, 1};
    CwSection mmSection{"Rando", "", &mmSide, 1};
    CwMenu mmMenu{1, &mmSection, 1};
    auto& model = ComboMenuModel::Get();
    auto& oot = const_cast<GameMenu&>(model.Oot());
    auto& mm = const_cast<GameMenu&>(model.Mm());
    oot.loaded = mm.loaded = true; oot.menu = &ootMenu; mm.menu = &mmMenu;
    ComboMenu menu;
    Draw(menu, "Shared Seed Settings/Stat Upgrades");
    Check(Has("Shared Seed Settings/Stat Upgrades"), "RPG controls belong to Shared Seed Settings");
    Check(!Has("OOT Randomizer/Stat Upgrades"), "RPG sidebar no longer claims OoT-only scope");
    Check(rendered == std::vector<int>({101, 102}) && renderedOwner == &oot,
          "shared RPG preserves both original native widgets and their owner");
    Draw(menu, "OOT Randomizer/Hints/Traps");
    Check(!Has("Shared Seed Settings/Map & Compass Information"), "map/compass information has no standalone sidebar");
    Check(rendered == std::vector<int>({201, 202}) && renderedOwner == &oot,
          "Hints/Traps renders map/compass information once alongside native hints");
    Check(hints[0].index == 201 && ootSides[1].widgetCount == 2,
          "presentation filtering never mutates the exported native menu");
    Draw(menu, "MM Randomizer/Hints");
    Check(rendered == std::vector<int>({301}) && renderedOwner == &mm,
          "MM native hint sidebar keeps its original ownership");
    Draw(menu, "OOT Randomizer/Shuffles");
    Check(rendered == std::vector<int>({401, 412, 413}) && renderedColumns == std::vector<int>({0, 2, 2}),
          "Shuffles defaults to gear alongside a dedicated, always visible Souls column");
    Check(shuffleGroups == std::vector<std::pair<std::string, int>>({
              {"Gear & Quest Items", 0}, {"Songs & Time", 0}, {"Abilities", 0}, {"Wallets & Upgrades", 0},
              {"World Checks", 1}, {"Fishing", 1}, {"Fairy Checks", 1}, {"Shops & Merchants", 1}, {"Souls", 2}}),
          "Shuffles presents the expected groups in each column");
    expandShuffleGroups = true;
    Draw(menu, "OOT Randomizer/Shuffles");
    Check(rendered == std::vector<int>({401, 402, 403, 416, 415, 404, 417, 405, 406, 407, 409, 410, 412, 413}) &&
              renderedColumns == std::vector<int>({0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2}) &&
              renderedOwner == &oot,
          "expanding each group exposes all original widgets with dependencies and native ownership intact");
    oot.loaded = false; oot.menu = nullptr;
    Draw(menu, "Shared Seed Settings/Map & Compass Information");
    Check(!Has("Shared Seed Settings/Map & Compass Information") && rendered == std::vector<int>({301}),
          "unavailable donor falls back to available navigation safely");
    menu.mScope = "settings";
    Draw(menu, "Settings/Souls Tracker");
    Check(Has("Settings/Souls Tracker") && soulsPanelsDrawn == 1 && rendered.empty(),
          "shared settings dispatch the Souls Tracker to its own panel even when OoT is unavailable");
    if (!failures) std::cout << "PASS shared seed menu: native ownership, unique controls and unavailable-host recovery\n";
    return failures != 0;
}
