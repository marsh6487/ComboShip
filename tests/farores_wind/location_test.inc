#include "gameplay/FaroresWindLocation.h"
#include <algorithm>
#include <cstdio>
#include <utility>

#ifndef TEST_MM
static constexpr int localEntrance = 0x123;
#endif
static constexpr int foreignEntrance = 0x5670;
static std::string displayedText;
static bool longLabel;

#ifdef TEST_MM
#include "2s2h/CustomMessage/CustomMessage.h"
#include "entrances/BlackMarketEntrance.h"

extern "C" const char* SOH_GetFwLocationName(int32_t entrance) {
    return entrance == foreignEntrance ? "Water Temple" : nullptr;
}

extern "C" const char* Ship_GetSceneName(s16 scene) {
    if (scene == SCENE_CLOCKTOWER)
        return longLabel ? "An extremely long custom Clock Town name with an "
                           "embedded\ncontrol code"
                         : "South Clock Town";
    return "Mountain Village";
}

extern "C" f32 Ship_GetCharFontWidthNES(u8 character) {
    return sNESFontWidths[character - ' '];
}

void CustomMessage::StartTextbox(std::string body, Entry options) {
    assert(!options.autoFormat);
    displayedText = std::move(body);
}

#else
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/util.h"

extern "C" const char* MM_GetFwLocationName(int32_t entrance) {
    return entrance == foreignEntrance ? "Snowhead Temple" : nullptr;
}

extern "C" int32_t EntranceDB_IsIndexValid(int32_t entrance, int32_t layer) {
    return entrance >= 0 && layer >= 0 && (int64_t)entrance + layer < ENTR_MAX;
}

extern "C" int32_t SceneDB_IsValid(int32_t scene) {
    return scene >= 0 && scene < SCENE_ID_MAX;
}

const std::string& SohUtils::GetSceneName(int32_t scene) {
    static const std::string longName = "An extremely long custom Forest Temple "
                                        "name with an embedded\ncontrol code";
    static const std::string local = "Forest Temple";
    static const std::string other = "Hyrule Field";
    return scene == SCENE_FOREST_TEMPLE ? (longLabel ? longName : local) : other;
}

CustomMessage::CustomMessage(std::string body, TextBoxType type_, TextBoxPosition position_)
    : type(type_), position(position_) {
    messages[0] = std::move(body);
}

CustomMessage CustomMessage::LoadVanillaMessageTableEntry(uint16_t textId) {
    assert(textId == 0x003B);
    return CustomMessage("Farore invoked!\001\034Retour\001Dissiper\001Quitter\002", TEXTBOX_TYPE_BLUE,
                         TEXTBOX_POS_BOTTOM);
}

const std::string CustomMessage::GetForCurrentLanguage(MessageFormat format) const {
    assert(format == MF_RAW);
    return messages[0];
}

std::string CustomMessage::THREE_WAY_CHOICE() {
    return "\034";
}
std::string CustomMessage::NEWLINE() {
    return "\001";
}
const TextBoxType& CustomMessage::GetTextBoxType() const {
    return type;
}
const TextBoxPosition& CustomMessage::GetTextBoxPosition() const {
    return position;
}
void CustomMessage::LoadIntoFont() {
    assert(type == TEXTBOX_TYPE_BLUE && position == TEXTBOX_POS_BOTTOM);
    displayedText = messages[0];
}
#endif

// NATIVE_FW_LOCATION_MESSAGE

static std::string OpenLocationMessage() {
    displayedText.clear();
#ifdef TEST_MM
    OotSpells_OpenFwTextbox();
    assert(displayedText.ends_with("\002\021\303Return to the Warp "
                                   "Point\021Dispel the Warp Point\021Exit\277"));
    assert(std::count(displayedText.begin(), displayedText.end(), '\021') == 3);
    return displayedText.substr(0, displayedText.find('\002'));
#else
    uint16_t textId = 0x003B;
    bool loadNative = true;
    ShowDestination(&textId, &loadNative);
    assert(textId == 0x003B);
    if (loadNative) {
        assert(displayedText.empty());
        return {};
    }
    assert(displayedText.ends_with("\001\034Retour\001Dissiper\001Quitter\002"));
    assert(std::count(displayedText.begin(), displayedText.end(), '\001') == 3);
    return displayedText.substr(0, displayedText.find('\001'));
#endif
}

static void TestLocationMessage() {
#ifdef TEST_MM
    auto& sceneTable = sSceneEntranceTable[9];
    // Native point has spawn 3. Shared point later uses physical spawn 2.
    EntranceTableEntry local = { SCENE_CLOCKTOWER, 3, 0 };
    EntranceTableEntry other = { SCENE_10YUKIYAMANOMURA, 2, 0 };
    EntranceTableEntry* spawns[4] = { nullptr, nullptr, &other, &local };
    const auto oldTable = sceneTable;
    sceneTable.table = spawns;
    sceneTable.tableCount = 4;
    const int otherEntrance = 0x1220;
    const char* expectedNative = "Return point: South Clock Town";
    const char* expectedShared = "Return point: Mountain Village";
    const char* expectedForeign = "Return point: Water Temple (OoT)";
    const char* fallback = "You cast Farore's Wind!";
#else
    gEntranceTable[localEntrance].scene = SCENE_FOREST_TEMPLE;
    const int otherEntrance = 0x222;
    gEntranceTable[otherEntrance].scene = SCENE_HYRULE_FIELD;
    gSaveContext.language = LANGUAGE_FRA;
    const char* expectedNative = "Point de retour : Forest Temple";
    const char* expectedShared = "Point de retour : Hyrule Field";
    const char* expectedForeign = "Point de retour : Snowhead Temple (MM)";
    const char* fallback = "";
#endif

    gSaveContext.fileNum = 1;
    setNativePoint();
    assert(OpenLocationMessage() == expectedNative);
    assert(states[1] == -1); // Display must not write legacy metadata.

    const ComboFwCallbacks callbacks{ readPoint, writePoint, request };
    SET_CALLBACKS(&callbacks);
    assert(OpenLocationMessage() == expectedNative);
    assert(states[1] == -1);

    longLabel = true;
    auto shortened = OpenLocationMessage();
    assert(shortened.ends_with("...") && shortened.find('\n') == std::string::npos);
    longLabel = false;

    ComboFwPoint saved{ ownGame, otherEntrance, 0, 0, ownGame == 0 ? 0 : -1, 0, 0, 0, 0, 0, 0 };
    writePoint(1, &saved);
    assert(OpenLocationMessage() == expectedShared);
#ifdef TEST_MM
    other.sceneId = COMBO_BLACK_MARKET_SCENE;
    assert(OpenLocationMessage().find("Skiejer's Black Market") != std::string::npos);
    other.sceneId = SCENE_10YUKIYAMANOMURA;
#endif

#ifndef _WIN32
    saved.game = 1 - ownGame;
    saved.age = saved.game == 0 ? 1 : -1;
    saved.entrance = foreignEntrance;
    writePoint(1, &saved);
    const std::string foreignHeading = OpenLocationMessage();
    // Some host fonts drop the heading prefix to keep long names on one line.
#ifdef TEST_MM
    assert(foreignHeading == expectedForeign || foreignHeading == "Water Temple (OoT)");
#else
    assert(foreignHeading == expectedForeign || foreignHeading == "Snowhead Temple (MM)");
#endif
#endif

    writePoint(1, nullptr);
    assert(OpenLocationMessage() == fallback && nativeSet());

    saved = { ownGame, 0xFFFF, 0, 0, ownGame == 0 ? 0 : -1, 0, 0, 0, 0, 0, 0 };
    writePoint(1, &saved);
    assert(OpenLocationMessage() == fallback);
    gComboGameMode = ownGame == 0 ? COMBO_GAME_OOT : COMBO_GAME_MM;
    assert(OpenLocationMessage() == expectedNative);
    gComboGameMode = COMBO_GAME_BOTH;

    // Restore the fixture for ordinary return/handoff tests.
    SET_CALLBACKS(nullptr);
    states[1] = -1;
#ifdef TEST_MM
    sceneTable = oldTable;
    nativeNei = {};
#else
    gSaveContext.fw = {};
#endif
    std::puts("PASS saved destination dialogue, foreign game, and unchanged "
              "three-choice rows");
}
