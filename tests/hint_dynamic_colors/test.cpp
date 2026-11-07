#include "global.h"
#include <nlohmann/json_fwd.hpp>
#include "2s2h/Rando/Rando.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "overlays/actors/ovl_En_Js/z_en_js.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>

SaveContext gSaveContext{};
static PlayState play;
static Player player;
static Actor child;
PlayState* gPlayState = &play;
u32 gBitFlags[32]{};
u8 gItemSlots[77]{};
static std::array<int, SENSOR_DESIRE_SLOTS> desires;
static std::vector<int> desireReads;
static std::vector<std::pair<RandoItemId, RandoCheckId>> locationReads;
static CustomMessage::Entry presented;
static int submissions;
static uint32_t maskCount;
static std::vector<bool> emptySelections;
static std::vector<std::string> previews;
static int cvarWrites, menuSaves;
#define HAS_ITEM(item) (INV_CONTENT(item) == item)

extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    const std::string prefix = CVAR_SENSOR_DESIRE_PREFIX;
    if (!std::string(key).starts_with(prefix)) return fallback;
    const int slot = std::stoi(key + prefix.size());
    desireReads.push_back(slot);
    return desires.at(slot);
}
extern "C" void CVarSetInteger(const char*, int32_t) { ++cvarWrites; }
namespace ImGui {
void TextWrapped(const char*, ...) {}
bool BeginCombo(const char*, const char* preview) { previews.emplace_back(preview); return true; }
bool InputTextWithHint(const char*, const char*, char*, size_t) { return false; }
bool Selectable(const char* text, bool selected) {
    if (std::string(text) == "(empty)") emptySelections.push_back(selected);
    return false;
}
void EndCombo() {}
}
namespace Ship {
struct SensorGui { void SaveConsoleVariablesNextFrame() { ++menuSaves; } };
struct SensorWindow { SensorGui* GetGui() { static SensorGui gui; return &gui; } };
struct Context {
    static Context* GetRawInstance() { static Context context; return &context; }
    SensorWindow* GetWindow() { static SensorWindow window; return &window; }
};
}
namespace Rando::StaticData {
std::map<RandoCheckId, RandoStaticCheck> Checks;
std::map<RandoItemId, RandoStaticItem> Items;
std::string GetItemName(RandoItemId item, bool, RandoCheckId
#ifdef COMBO_BUILD
                        , bool
#endif
) {
    switch (item) {
        case RI_OOT_NEI_SHOVEL: return "Shovel";
        case RI_BOMB_BAG_20: return "Bomb Bag";
        case RI_BOW: return "Hero's Bow";
        case RI_MASK_DEKU: return "Deku Mask";
        case RI_REMAINS_ODOLWA: return "Odolwa's Remains";
        case RI_REMAINS_GOHT: return "Goht's Remains";
        case RI_REMAINS_GYORG: return "Gyorg's Remains";
        case RI_REMAINS_TWINMOLD: return "Twinmold's Remains";
        default: assert(false); return {};
    }
}
}
namespace Rando {
std::string GetHintLocationText(RandoItemId item, RandoCheckId check, bool) {
    locationReads.emplace_back(item, check);
    return "at Gerudo Fortress (OOT)";
}
namespace Logic { uint32_t MoonMaskCount() { return maskCount; } }
}
namespace CustomMessage {
Entry LoadVanillaMessageTableEntry(u16) {
    Entry entry;
    entry.msg = "Vanilla play prompt";
    return entry;
}
void LoadCustomMessageIntoFont(Entry entry) { presented = std::move(entry); ++submissions; }
void StartTextbox(std::string text, Entry entry) {
    entry.msg = std::move(text);
    LoadCustomMessageIntoFont(std::move(entry));
}
}
/* COLOR_ENCODER */
/* PRODUCTION */

static void Reset() {
    gSaveContext = {};
    play = {}; player = {}; child = {};
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.talkActor = &child;
    for (int bit = 0; bit < 32; ++bit) gBitFlags[bit] = uint32_t(1) << bit;
    gItemSlots[ITEM_MASK_DEKU] = SLOT_MASK_DEKU;
    gItemSlots[ITEM_MASK_GORON] = SLOT_MASK_GORON;
    gItemSlots[ITEM_MASK_ZORA] = SLOT_MASK_ZORA;
    std::fill(std::begin(gSaveContext.save.saveInfo.inventory.items),
              std::end(gSaveContext.save.saveInfo.inventory.items), ITEM_NONE);
    desires.fill(RI_NONE);
    desireReads.clear(); locationReads.clear();
    Rando::StaticData::Checks.clear();
    presented = {}; submissions = 0; maskCount = 0;
    emptySelections.clear(); previews.clear(); cvarWrites = menuSaves = 0;
}
static void AddCheck(RandoCheckId check, RandoItemId item, bool shuffled, bool obtained, bool cycle) {
    Rando::StaticData::Checks[check] = {};
    RANDO_SAVE_CHECKS[check].randoItemId = item;
    RANDO_SAVE_CHECKS[check].shuffled = shuffled;
    RANDO_SAVE_CHECKS[check].obtained = obtained;
    RANDO_SAVE_CHECKS[check].cycleObtained = cycle;
}
static std::string Encoded() {
    std::string text = presented.msg;
    CustomMessage::ReplaceColorChars(&text);
    return text;
}
static void Sensor() {
    Reset();
    desires = {RI_NONE, -1, RI_OOT_NEI_SHOVEL, RI_BOW, RI_MAX};
    AddCheck(RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST, RI_OOT_NEI_SHOVEL, false, false, false);
    AddCheck(RC_BENEATH_THE_GRAVEYARD_CHEST, RI_OOT_NEI_SHOVEL, true, true, false);
    AddCheck(RC_BENEATH_THE_GRAVEYARD_DAMPE_CHEST, RI_OOT_NEI_SHOVEL, true, false, true);
    AddCheck(RC_BENEATH_THE_WELL_KEESE_CHEST, RI_OOT_NEI_SHOVEL, true, false, false);
    AddCheck(RC_CLOCK_TOWN_EAST_UPPER_CHEST, RI_BOW, true, false, false);
    const SaveContext before = gSaveContext;
    assert(SensorRune_PrepareHint() == 1);
    assert((desireReads == std::vector<int>{0, 1, 2}));
    assert((locationReads == std::vector<std::pair<RandoItemId, RandoCheckId>>{
        {RI_OOT_NEI_SHOVEL, RC_BENEATH_THE_WELL_KEESE_CHEST}}));
    SensorRune_OpenHint();
    const char expected[] = "The slate senses \x02" "Shovel\x00 \x04"
                            "at Gerudo Fortress (OOT)\x00...";
    assert(Encoded() == std::string(expected, sizeof(expected) - 1) &&
           "sensor item must be green and its location yellow, with prose restored to white");
    assert(submissions == 1 && presented.autoFormat && presented.nextMessageID == 0xFFFF);
    assert(!std::memcmp(&gSaveContext, &before, sizeof(before)));
    // The earlier wished-for Shovel takes priority over an outstanding Bow.
    // Once collected, the next wish becomes answerable without charging here.
    RANDO_SAVE_CHECKS[RC_BENEATH_THE_WELL_KEESE_CHEST].obtained = true;
    assert(SensorRune_PrepareHint() == 1 && locationReads.back() == std::make_pair(RI_BOW, RC_CLOCK_TOWN_EAST_UPPER_CHEST));
    SensorRune_OpenHint();
    const char nextExpected[] = "The slate senses \x02" "Hero's Bow\x00 \x04"
                                "at Gerudo Fortress (OOT)\x00...";
    assert(Encoded() == std::string(nextExpected, sizeof(nextExpected) - 1) && submissions == 2);
    RANDO_SAVE_CHECKS[RC_CLOCK_TOWN_EAST_UPPER_CHEST].obtained = true;
    assert(SensorRune_PrepareHint() == 0 && sSensorHintText.empty() && submissions == 2);
    std::cout << "PASS sensor: green item/yellow location bytes; wish order, outstanding-check gate and save preserved\n";
}
static void Remains() {
    constexpr QuestItem quests[] = {QUEST_REMAINS_ODOLWA, QUEST_REMAINS_GOHT, QUEST_REMAINS_GYORG, QUEST_REMAINS_TWINMOLD};
    constexpr const char* names[] = {"Odolwa's Remains", "Goht's Remains", "Gyorg's Remains", "Twinmold's Remains"};
    for (int type = 1; type <= 4; ++type) {
        Reset(); child.params = type;
        RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] = RO_ACCESS_TRIALS_REMAINS;
        const SaveContext before = gSaveContext;
        u16 id = 0x2215; bool table = true;
        OverrideSubJsText(&id, &table);
        const std::string expected = "You need to find " + std::string(1, '\x02') + names[type - 1] +
                                     std::string(1, '\x00') + " before you can play...";
        assert(Encoded() == expected && "moon-child missing-remains item must be green with a white prose tail");
        assert(!table && id == 0x2215 && presented.nextMessageID == 0x2216 && submissions == 1);
        assert(!std::memcmp(&gSaveContext, &before, sizeof(before)));
        SET_QUEST_ITEM(quests[type - 1]);
        table = true; OverrideSubJsText(&id, &table);
        assert(presented.msg == "Vanilla play prompt" && presented.nextMessageID == 0xFFFF);
    }
    Reset(); player.talkActor = nullptr;
    u16 id = 0x2215; bool table = true; OverrideSubJsText(&id, &table);
    assert(table && submissions == 0);
    std::cout << "PASS moon remains: all four green item spans, ownership gates, next IDs and save preserved\n";
}
static void Forms() {
    constexpr ItemId masks[] = {ITEM_MASK_DEKU, ITEM_MASK_GORON, ITEM_MASK_ZORA};
    constexpr const char* names[] = {"the Deku Mask", "the Goron Mask", "the Zora Mask"};
    for (int type = 1; type <= 3; ++type) {
        Reset(); child.params = type;
        RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] = RO_ACCESS_TRIALS_FORMS;
        const SaveContext before = gSaveContext;
        u16 id = 0x2215; bool table = true; OverrideSubJsText(&id, &table);
        const std::string expected = "You need to find " + std::string(1, '\x02') + names[type - 1] +
                                     std::string(1, '\x00') + " before you can play...";
        assert(Encoded() == expected && "moon-child missing transformation mask must use the native green item palette");
        assert(!table && presented.nextMessageID == 0x2216 && !std::memcmp(&gSaveContext, &before, sizeof(before)));
        INV_CONTENT(masks[type - 1]) = masks[type - 1];
        OverrideSubJsText(&id, &table);
        assert(presented.msg == "Vanilla play prompt" && presented.nextMessageID == 0xFFFF);
    }
    Reset(); child.params = 4; RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] = RO_ACCESS_TRIALS_FORMS;
    u16 id = 0x2215; bool table = true; OverrideSubJsText(&id, &table);
    assert(presented.msg == "Vanilla play prompt" && presented.nextMessageID == 0xFFFF);
    // The separate numeric-mask gate and dismissal keep their original text.
    child.params = 2; RANDO_SAVE_OPTIONS[RO_ACCESS_TRIALS] = RO_ACCESS_TRIALS_20_MASKS;
    OverrideSubJsText(&id, &table);
    assert(presented.msg == "You need more masks to play... Come back when you have at least 6...");
    id = 0x2216; OverrideSubJsText(&id, &table);
    assert(presented.msg == "...\xE0");
    std::cout << "PASS moon forms: three green mask spans, possession gates, type4/count/dismissal behavior preserved\n";
}
static void Selectors() {
    // These ordinary wishes precede RI_NONE in the native enum; none is an
    // ordinal upper/lower range boundary.
    bool allAnswered = true;
    for (const auto item : {RI_BOMB_BAG_20, RI_BOW, RI_MASK_DEKU}) {
        Reset(); desires[0] = item;
        AddCheck(RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST, item, true, false, false);
        const SaveContext before = gSaveContext;
        if (SensorRune_PrepareHint() != 1) {
            std::cerr << "Rejected ordinary Sensor wish: " << Rando::StaticData::GetItemName(item) << '\n';
            allAnswered = false;
            continue;
        }
        SensorRune_OpenHint();
        assert(presented.msg.find(Rando::StaticData::GetItemName(item)) != std::string::npos);
        assert(!std::memcmp(&gSaveContext, &before, sizeof(before)));
    }
    assert(allAnswered && "ordinary item/mask wishes before RI_NONE must remain answerable");
    Reset();
    for (int id = RI_UNKNOWN + 1; id < RI_MAX; ++id) {
        desires[0] = id;
        const auto want = id == RI_NONE ? RI_NONE : static_cast<RandoItemId>(id);
        assert(SensorGetDesire(0) == want);
    }
    for (int id : {std::numeric_limits<int>::min(), -1, int(RI_UNKNOWN), int(RI_NONE), int(RI_MAX),
                   std::numeric_limits<int>::max()}) {
        desires[0] = id;
        assert(SensorGetDesire(0) == RI_NONE);
    }
    const auto reads = desireReads.size();
    assert(SensorGetDesire(-1) == RI_NONE && SensorGetDesire(SENSOR_DESIRE_SLOTS) == RI_NONE &&
           desireReads.size() == reads);
    std::cout << "PASS sensor selectors: ordinary wishes, complete ID range and invalid/sentinel/slot bounds\n";
}
static void Picker() {
    Reset();
    // Execute the real picker helpers and callback with a controlled item
    // catalogue; only ImGui/window persistence are presentation boundaries.
    const auto addItem = [](RandoItemId id, const char* name) {
        Rando::StaticData::Items[id] = {};
        Rando::StaticData::Items[id].name = name;
    };
    addItem(RI_UNKNOWN, "Unknown"); addItem(RI_NONE, "literally nothing");
    addItem(RI_BOMB_BAG_20, "Bomb Bag"); addItem(RI_BOW, "Hero's Bow");
    addItem(RI_MASK_DEKU, "Deku Mask"); addItem(RI_OOT_NEI_SHOVEL, "Shovel");
    addItem(RI_OOT_NEI_MAGIC_MIRROR, "Magic Mirror");
    addItem(RI_MAX, "Bad high ID");
    addItem(RI_HOOKSHOT, ""); addItem(RI_ARROWS_10, nullptr);
    const std::vector<std::pair<int32_t, std::string>> wantChoices = {
        {RI_BOMB_BAG_20, "Bomb Bag"}, {RI_MASK_DEKU, "Deku Mask"}, {RI_BOW, "Hero's Bow"},
        {RI_OOT_NEI_MAGIC_MIRROR, "Magic Mirror"}, {RI_OOT_NEI_SHOVEL, "Shovel"}};
    assert(SensorItemChoices() == wantChoices && "picker must include ordinary items and preserve alphabetical display order");
    desires = {RI_BOMB_BAG_20, RI_BOW, RI_MASK_DEKU, RI_NONE, RI_UNKNOWN};
    DrawSensorDesirePicker();
    assert((previews == std::vector<std::string>{"Bomb Bag", "Hero's Bow", "Deku Mask", "(empty)", "(empty)"}));
    assert((emptySelections == std::vector<bool>{false, false, false, true, true}));
    previews.clear(); emptySelections.clear();
    desires = {std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), -1, RI_MAX, RI_OOT_NEI_SHOVEL};
    DrawSensorDesirePicker();
    assert((previews == std::vector<std::string>{"(empty)", "(empty)", "(empty)", "(empty)", "Shovel"}));
    assert((emptySelections == std::vector<bool>{true, true, true, true, false}));
    assert(cvarWrites == 0 && menuSaves == 0 && "reading wishes must not persist or change their order");
    std::cout << "PASS sensor picker: real sorted choices, preview/empty state, no order/CVar/persistence mutation\n";
}
int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string test = argv[1];
    if (test == "sensor") Sensor();
    else if (test == "remains") Remains();
    else if (test == "forms") Forms();
    else if (test == "selectors") Selectors();
    else if (test == "picker") Picker();
    else assert(false);
}
