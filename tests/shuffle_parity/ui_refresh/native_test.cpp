#include <map>
#include <string>
#include <cstdint>
static std::map<std::string, int32_t> values;
#include "foundation.inc"
#include "2s2h/ShipInit.hpp"
#include "Rando/SongEvents.h"
#include "gui/WhatsNew.h"
#include "Rando/VanillaCompatibility.h"
#include <imgui_internal.h>

// Only storage, GUI scheduling and ResourceManager acquisition are boundaries.
// Native dispatch, real pools/catalogues, UI widgets and ImGui input run below.
static int saves;
extern "C" void CVarSetInteger(const char* key, int32_t value) { values[key] = value; }
extern "C" void CVarClear(const char* key) { values.erase(key); }
namespace Ship {
#ifdef COMBO_BUILD
OwnRMScope::OwnRMScope(const char*) {}
OwnRMScope::~OwnRMScope() = default;
#endif
struct Gui { void SaveConsoleVariablesNextFrame() { ++saves; } };
struct Window { Gui* GetGui() { static Gui gui; return &gui; } };
struct Context {
    static Context* GetRawInstance() { static Context context; return &context; }
    Window* GetWindow() { static Window window; return &window; }
};
}
namespace Rando { std::vector<RandoItemId> GetStartingItemsFromConfig() { return {}; } }
void ClearIncompatibleSetting();
#include "metrics.inc"
static bool PassesCheckGroupFilter(RandoCheckId) { return true; }
static std::string CheckListLabel(RandoCheckId id) { return Rando::StaticData::Checks.at(id).name; }
#include "exclusion.inc"
extern "C" bool Ship_IsCStringEmpty(const char* text) { return !text || !*text; }
namespace UIWidgets {
enum class Colors { LightBlue };
enum class LabelPositions { Near, Far, Above, None };
using LabelPosition = LabelPositions;
enum class ComponentAlignments { Left, Right };
using ComponentAlignment = ComponentAlignments;
const std::map<Colors, ImVec4> ColorValues{{Colors::LightBlue, ImVec4(.3f,.6f,.8f,1)}};
struct WidgetOptions { const char* tooltip = ""; bool disabled = false; const char* disabledTooltip = ""; };
struct CheckboxOptions : WidgetOptions {
    bool defaultValue = false;
    ComponentAlignment alignment = ComponentAlignment::Left;
    LabelPosition labelPosition = LabelPosition::Near;
    ImVec2 padding = ImVec2(10,8);
    Colors color = Colors::LightBlue;
};
std::string WrappedText(const char*, unsigned int = 80);
std::string WrappedText(const std::string&, unsigned int = 80);
#include "widgets.inc"
}
using namespace UIWidgets;
#include "compatibility.inc"
static void DrawRusty() {
#include "rusty.inc"
}
static void DrawRings() {
#include "rings.inc"
}
struct FrameResult { ImGuiID id; ImVec2 min,max; bool accented; };
template<class Draw> static FrameResult Frame(Draw draw, ImVec2 mouse = {-100,-100}, bool down = false) {
    auto& io = ImGui::GetIO();
    io.AddMousePosEvent(mouse.x, mouse.y); io.AddMouseButtonEvent(0, down);
    ImGui::NewFrame();
    ImGui::SetNextWindowPos({20,20}); ImGui::SetNextWindowSize({700,500});
    ImGui::Begin("Pool refresh", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
    const int colors = GImGui->ColorStack.Size;
    draw();
    FrameResult result{ImGui::GetItemID(),ImGui::GetItemRectMin(),ImGui::GetItemRectMax(),false};
    const ImU32 gold = ImGui::ColorConvertFloat4ToU32(WhatsNew::Accent(WhatsNew::Kind::New));
    for (auto& vertex : ImGui::GetWindowDrawList()->VtxBuffer) result.accented |= vertex.col == gold;
    REQUIRE(GImGui->ColorStack.Size == colors);
    ImGui::End(); ImGui::Render();
    return result;
}
template<class Draw> static void Click(Draw draw, int row = 0) {
    Frame(draw); // first frame establishes the window
    auto box = Frame(draw);
    ImVec2 hit{box.min.x+8, (box.min.y+box.max.y)*.5f};
    // Rings return the fourth checkbox; request another row by its native size.
    hit.y -= row * (box.max.y-box.min.y+ImGui::GetStyle().ItemSpacing.y);
    Frame(draw,hit); Frame(draw,hit,true); Frame(draw,hit,false);
}
int main() {
    ImGui::CreateContext();
    auto& io = ImGui::GetIO(); io.IniFilename=nullptr; io.DisplaySize={900,600}; io.DeltaTime=1.f/60;
    unsigned char* pixels; int width,height; io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    // Region reachability is outside this UI test. Admit the complete native
    // catalogue into the candidate region; GeneratePools still applies all
    // actual option, source-eligibility and exclusion policies.
    for (auto& [id, check] : Rando::StaticData::Checks)
        if (id != RC_UNKNOWN) Rando::Logic::Regions[static_cast<RandoRegionId>(0)].checks[id] =
            {[] { return true; }, "UI pool candidate membership"};
    ShipInit::InitAll();
    // Actual UI preview must preserve the mapping used by later oracle replay.
    {
        auto previous = gSaveContext.save.shipSaveInfo.rando;
        auto& loaded = gSaveContext.save.shipSaveInfo.rando;
        loaded = {};
        loaded.randoSaveOptions[RO_SHUFFLE_SONG_EVENTS] = 1;
        nlohmann::json mapping = nlohmann::json::array();
        for (unsigned i = 0; i < 13; ++i) mapping.push_back(3);
        Rando::SongEvents::ClearImportedMapping();
        Rando::SongEvents::Import(loaded, {{"songEvents", mapping}});
        RefreshMetrics();
        Rando::SongEvents::RememberSavedMapping(loaded);
        auto replay = loaded;
        Rando::SongEvents::RestoreForOracle(replay);
        REQUIRE(memcmp(loaded.songEvents, replay.songEvents, sizeof(loaded.songEvents)) == 0);
        gSaveContext.save.shipSaveInfo.rando = previous;
        Rando::SongEvents::ClearImportedMapping();
    }

    const char* rock = Rando::StaticData::Options[RO_SHUFFLE_ROCK_DROPS].cvar;
    ImGuiTextFilter filter;
    const auto check=RC_SWAMP_SPIDER_HOUSE_ROCK_ENTRANCE_1;
    REQUIRE(!IsCheckShownAsShuffled(check,filter));
    const int initialChecks=checksInPool;
    auto rockControl=[&]{CVarCheckbox("Small Rocks",rock,{});};
    Click(rockControl);
    REQUIRE(CVarGetInteger(rock,0)==1 && checksInPool>initialChecks);
    REQUIRE(IsCheckShownAsShuffled(check,filter));
    Click(rockControl);
    REQUIRE(!IsCheckShownAsShuffled(check,filter) && checksInPool==initialChecks);
    const char* npc=Rando::StaticData::Options[RO_SHUFFLE_NPC_SOULS].cvar;
    const int initialItems=itemsInPool;
    auto npcControl=[&]{CVarCheckbox("NPC Souls",npc,{});};
    Click(npcControl); REQUIRE(itemsInPool>initialItems);
    Click(npcControl); REQUIRE(itemsInPool==initialItems);
    auto before=checkPoolGeneration;
    const int beforeRings=itemsInPool, beforeSaves=saves;
    Click(DrawRings); // Stone Tower replaces four keys with one ring
    REQUIRE(CVarGetInteger(Rando::StaticData::Options[RO_SHUFFLE_KEY_RINGS].cvar,0)==8);
    REQUIRE(saves==beforeSaves+1 && checkPoolGeneration==before+1 && itemsInPool==beforeRings-3);
    // Dispatch every actual native option, rather than matching registry text.
    for (auto& [id, option] : Rando::StaticData::Options) {
        auto generation=checkPoolGeneration; ShipInit::Init(option.cvar);
        if (checkPoolGeneration != generation+1) std::fprintf(stderr,"Missing refresh: %s\n",option.cvar);
        REQUIRE(checkPoolGeneration == generation+1);
    }
    values[WhatsNew::kEnabledCvar]=0; auto plain=Frame(DrawRusty);
    values[WhatsNew::kEnabledCvar]=1; auto marked=Frame(DrawRusty);
    REQUIRE(!plain.accented && marked.accented && plain.id==marked.id);
    REQUIRE(plain.min.x==marked.min.x && plain.max.x==marked.max.x && plain.max.y==marked.max.y);
    values[Rando::StaticData::Options[RO_LOGIC].cvar]=RO_LOGIC_VANILLA;
    const int rustySaves=saves;
    auto locked=Frame(DrawRusty); Click(DrawRusty);
    REQUIRE(!locked.accented && saves==rustySaves);
    REQUIRE(CVarGetInteger(Rando::StaticData::Options[RO_SHUFFLE_RUSTY_KEYS].cvar,0)==0);
    values[Rando::StaticData::Options[RO_LOGIC].cvar]=RO_LOGIC_GLITCHLESS;
    const auto* oot=WhatsNew::Find("gRandoSettings.LockOverworldDoors");
    REQUIRE(oot && oot->kind==WhatsNew::Kind::Updated);
    values[Rando::StaticData::Options[RO_PLACEMENT_SMALL_KEYS].cvar]=RO_DUNGEON_ITEM_VANILLA;
    before=checkPoolGeneration; const int disabledSaves=saves;
    Click(DrawRings);
    REQUIRE(checkPoolGeneration==before && saves==disabledSaves);
    REQUIRE(CVarGetInteger(Rando::StaticData::Options[RO_SHUFFLE_KEY_RINGS].cvar,0)==8);
    // A config import can set preferences without the Logic combobox callback.
    // Its actual ShipInit refresh must normalize the visible settings too.
    values[Rando::StaticData::Options[RO_LOGIC].cvar]=RO_LOGIC_VANILLA;
    for(auto option:Rando::VanillaCompatibility::UnsupportedOptions)
        values[Rando::StaticData::Options[option].cvar]=1;
    ShipInit::Init(Rando::StaticData::Options[RO_LOGIC].cvar);
    for(auto option:Rando::VanillaCompatibility::UnsupportedOptions)
        REQUIRE(CVarGetInteger(Rando::StaticData::Options[option].cvar,0)==0);
    REQUIRE(!IsCheckShownAsShuffled(check,filter));
    ImGui::DestroyContext();
    std::puts("PASS real native UI input, all option dispatch, pool/exclusion refresh, saved rings and accents");
}
