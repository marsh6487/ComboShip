// Production OoT export; only runtime item/message ownership is replaced.
#include "combo/menu/ComboItemReceiptText.h"
#include "combo/menu/ComboItemReceiptPresentation.h"
#include "soh/soh/Enhancements/custom-message/text.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <unordered_map>
using namespace std::literals::string_literals;
#define SPDLOG_DEBUG(...) ((void)0)
#define COMBO_BUILD
#include "soh/soh/FleetShipCombo/FleetComboIds.h"
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x)                                                      \
  }                                                                            \
  ;
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerCheck.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerSettingKey.h"
constexpr RandomizerCheck RC_RECEIPT_FIXTURE =
    static_cast<RandomizerCheck>(RC_MAX + 1);
constexpr int ITEM_CATEGORY_JUNK = 0, ITEM_CATEGORY_MAJOR = 1,
              TEXT_RANDOMIZER_CUSTOM_ITEM = 0x9000;
constexpr int TEXTBOX_TYPE_BLUE = 2, ITEM_COMPASS = 0x75,
              ITEM_DUNGEON_MAP = 0x76, ITEM_SKULL_TOKEN = 0x71,
              ITEM_CUSTOM = 0xFF;
enum {
#define DEFINE_SCENE(a, b, scene, ...) scene,
#include "soh/include/tables/scene_table.h"
#undef DEFINE_SCENE
};
enum {
#define DEFINE_ENTRANCE(entrance, ...) entrance,
#include "soh/include/tables/entrance_table.h"
#undef DEFINE_ENTRANCE
};
constexpr int RO_MQ_DUNGEONS_NONE = 0, RO_MQ_DUNGEONS_SET_NUMBER = 1,
              MAX_MQ_DUNGEON_COUNT = 12, RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF = 0,
              RO_GENERIC_ON = 1;
struct ReceiptOption {
  int value;
  bool Is(int other) const { return value == other; }
};
struct ReceiptPlacement {
  RandomizerGet item = RG_NONE;
  std::string name;
  mutable Text placedName{};
  RandomizerGet GetPlacedRandomizerGet() const { return item; }
  const Text &GetPlacedItemName() const {
    placedName = Text(name);
    return placedName;
  }
};
struct ReceiptContext {
  int mqMode = 2, mqCount = 6, bossShuffle = 1, dungeonShuffle = 0,
      information = 1;
  bool generated = true, spoiler = false;
  bool IsSeedGenerated() const { return generated; }
  bool IsSpoilerLoaded() const { return spoiler; }
  struct Dungeon {
    bool mq = false;
    bool IsMQ() const { return mq; }
  } dungeon;
  ReceiptContext *GetDungeons() { return this; }
  Dungeon *GetDungeonFromScene(int scene) {
    assert(scene >= SCENE_DEKU_TREE && scene <= SCENE_ICE_CAVERN);
    return &dungeon;
  }
  std::map<RandomizerCheck, ReceiptPlacement> placements;
  ReceiptOption GetOption(int key) const {
    return {key == RSK_MQ_DUNGEON_RANDOM           ? mqMode
            : key == RSK_MQ_DUNGEON_COUNT          ? mqCount
            : key == RSK_SHUFFLE_BOSS_ENTRANCES    ? bossShuffle
            : key == RSK_SHUFFLE_DUNGEON_ENTRANCES ? dungeonShuffle
                                                   : information};
  }
  ReceiptPlacement *GetItemLocation(RandomizerCheck check) {
    return &placements[check];
  }
} receiptContext;
static bool masterQuest = false;
static bool randoActive = true;
bool ResourceMgr_IsSceneMasterQuest(int scene) {
  assert(scene >= SCENE_DEKU_TREE && scene <= SCENE_ICE_CAVERN);
  return randoActive && masterQuest;
}
struct {
  int fileNum = 0, mapIndex = 0;
  struct {
    int gsTokens = 0;
    uint8_t dungeonItems[10]{};
  } inventory;
} gSaveContext;
#define IS_RANDO randoActive
constexpr int DUNGEON_MAP = 2, DUNGEON_COMPASS = 1;
#define CHECK_DUNGEON_ITEM(item, scene)                                        \
  (gSaveContext.inventory.dungeonItems[scene] & (1 << (item)))
constexpr int TEXT_DESC_DUNGEON_MAP_INFO = 0x9300,
              TEXT_DESC_DUNGEON_COMPASS_INFO = 0x9301;
constexpr int ENTRANCE_OVERRIDES_MAX_COUNT = 8;
struct EntranceOverride {
  int16_t index, destination, override, overrideDestination;
} entranceOverrides[8]{};
EntranceOverride *Randomizer_GetEntranceOverrides() {
  return entranceOverrides;
}
bool Entrance_EntranceIsNull(const EntranceOverride *value) {
  return !value->index && !value->destination && !value->override &&
         !value->overrideDestination;
}
namespace EntranceTracker {
struct EntranceData {
  std::string source;
};
std::map<int, EntranceData> data;
const EntranceData *GetEntranceData(int entrance) {
  auto it = data.find(entrance);
  return it == data.end() ? nullptr : &it->second;
}
} // namespace EntranceTracker
struct ReceiptNeiSave {
  uint8_t comboObtained[FC_COMBO_OBTAINED_SIZE]{};
} receiptNeiSave;
ReceiptNeiSave *Nei_Save() { return &receiptNeiSave; }
struct GetItemEntry {
  uint16_t textId;
};
struct MessageTableEntry {
  uint16_t textId;
  uint8_t typePos;
  const char *segment;
  uint32_t msgSize;
};
struct CustomItemMessageEntry {
  int rgId;
  int itemId;
  const char *english;
  const char *german;
  const char *french;
};
static bool stateAdvanced = false, throwOnRead = false;
static int liveResolutionCalls = 0;
struct TraditionalReceiptFixture {
  RandomizerGet item;
  const char* name;
  uint16_t nativeText, expectedText;
};
/* TRADITIONAL_CATALOG */
namespace Rando::StaticData {
std::map<std::string, RandomizerGet> itemNameToEnum = {
    {"Cane of Somaria", RG_CANE_OF_SOMARIA},
    {"Progressive Roc", RG_PROGRESSIVE_ROCS},
    {"Stone of Agony", RG_STONE_OF_AGONY},
    {"Magic Meter", RG_MAGIC_SINGLE},
    {"Enhanced Magic Meter", RG_MAGIC_DOUBLE},
    {"Deku Leaf", RG_DEKU_LEAF},
    {"Power Upgrade", RG_POWER_UPGRADE},
    {"Magic Stat Upgrade", RG_MAGIC_STAT_UPGRADE},
    {"Goht's Remains", RG_MM_REMAINS_GOHT},
    {"Goron Lullaby", RG_MM_SONG_LULLABY},
    {"Goron Lullaby Intro", RG_MM_SONG_LULLABY_INTRO},
    {"New Wave Bossa Nova", RG_MM_SONG_NOVA},
    {"Song of Healing", RG_MM_SONG_HEALING},
    {"Song of Storms (MM)", RG_MM_SONG_STORMS},
    {"Great Deku Tree Compass", RG_DEKU_TREE_COMPASS},
    {"Great Deku Tree Map", RG_DEKU_TREE_MAP},
    {"Forest Temple Small Key", RG_FOREST_TEMPLE_SMALL_KEY},
    {"Forest Temple Boss Key", RG_FOREST_TEMPLE_BOSS_KEY},
    {"Ganon's Castle Boss Key", RG_GANONS_CASTLE_BOSS_KEY},
    {"Minuet of Forest", RG_MINUET_OF_FOREST},
    {"Snowhead Compass", RG_MM_COMPASS_SNOWHEAD},
    {"Woodfall Map", RG_MM_MAP_WOODFALL},
    {"Snowhead Map", RG_MM_MAP_SNOWHEAD},
    {"Great Bay Map", RG_MM_MAP_GREAT_BAY},
    {"Stone Tower Map", RG_MM_MAP_STONE_TOWER},
    {"Ice Cavern Compass", RG_ICE_CAVERN_COMPASS},
    {"Bottom of the Well Compass", RG_BOTTOM_OF_THE_WELL_COMPASS},
    {"Gold Skulltula Token", RG_GOLD_SKULLTULA_TOKEN},
    {"Swamp Gold Skulltula Token", RG_MM_GS_TOKEN_SWAMP},
    {"Ocean Gold Skulltula Token", RG_MM_GS_TOKEN_OCEAN},
    {"Ice Trap", RG_ICE_TRAP},
    {"Green Rupee", RG_GREEN_RUPEE},
    {"None", RG_NONE}};
struct Item {
  RandomizerGet id;
  int GetCategory() {
    return id == RG_GREEN_RUPEE ? ITEM_CATEGORY_JUNK : ITEM_CATEGORY_MAJOR;
  }
  std::string GetName() const {
    for (const auto &[name, rg] : itemNameToEnum)
      if (rg == id)
        return name;
    return "Item";
  }
  std::string GetColor() const { return "%g"; }
  std::shared_ptr<GetItemEntry> GetGIEntryUnresolved() const {
    if (throwOnRead)
      throw 1;
    uint16_t text = id == RG_STONE_OF_AGONY      ? 0x68
                    : id == RG_MAGIC_SINGLE      ? 0xE4
                    : id == RG_MAGIC_DOUBLE      ? 0xE8
                    : id == RG_DEKU_TREE_COMPASS ? 0x67
                    : id == RG_DEKU_TREE_MAP     ? 0x66
                    : id == RG_MINUET_OF_FOREST  ? 0x73
                                                 : TEXT_RANDOMIZER_CUSTOM_ITEM;
    for (const auto& fixture : traditionalReceipts)
      if (fixture.item == id)
        text = fixture.nativeText;
    return std::make_shared<GetItemEntry>(GetItemEntry{text});
  }
  std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet *actual) const {
    ++liveResolutionCalls;
    if (stateAdvanced && id == RG_CANE_OF_SOMARIA)
      *actual = RG_CANE_PACCI_FLIP;
    if (stateAdvanced && id == RG_PROGRESSIVE_ROCS)
      *actual = RG_ROCS_CAPE;
    if (stateAdvanced && id == RG_STONE_OF_AGONY)
      *actual = RG_QUARTZ_OF_MOTION;
    return GetGIEntryUnresolved();
  }
};
Item RetrieveItem(RandomizerGet id) { return {id}; }
} // namespace Rando::StaticData
struct OTRGlobals {
  static OTRGlobals *Instance;
  ReceiptContext *gRandoContext = &receiptContext;
  void *gRandomizer = reinterpret_cast<void *>(1);
};
OTRGlobals globals;
OTRGlobals *OTRGlobals::Instance = &globals;
/* DONOR_MESSAGES */
const CustomItemMessageEntry *GetCustomItemMessage(int rg) {
  static CustomItemMessageEntry localized{};
  for (const auto &msg : receiptMessages) {
    if (msg.rgId != rg)
      continue;
    localized = msg;
    // Production GetCustomItemMessage fills missing registry languages with
    // English before constructing a CustomMessage.
    if (!localized.german)
      localized.german = localized.english;
    if (!localized.french)
      localized.french = localized.english;
    return &localized;
  }
  return nullptr;
}
static const char stone[] =
    "You found the \x05\x44Stone of Agony\x05\x40!\x01It reacts to hidden "
    "secrets.\x02";
static const char magic[] =
    "You received the Magic Meter!\x04Use magic items with it.\x02";
static const char doubleMagic[] =
    "Your Magic Meter is enhanced!\x04You can use twice as much magic.\x02";
static const char compass[] =
    "You got the Compass!\x01Now you can see hidden things.\x02";
static const char dungeonMap[] =
    "You got the Dungeon Map!\x01" "Blue rooms are places you have visited.\x02";
static const char smallKey[] =
    "You got a Small Key!\x01This key will open a locked door in this dungeon.\x02";
static const char chestGameKey[] =
    "You got a Key!\x01It opens the next door in the Treasure Chest Game.\x02";
static const char bossKey[] =
    "You got the Boss Key!\x01Now you can get inside the chamber where the Boss lurks.\x02";
static const char minuet[] =
    "You have learned the Minuet of Forest!\x01" "A melody that will take you to the forest.\x02";
MessageTableEntry nativeTable[] = {
    {0x68, 0, stone, sizeof(stone) - 1},
    {0xE4, 0, magic, sizeof(magic) - 1},
    {0xE8, 0, doubleMagic, sizeof(doubleMagic) - 1},
    {0x67, 0, compass, sizeof(compass) - 1},
    {0x66, 0, dungeonMap, sizeof(dungeonMap) - 1},
    {0x60, 0, smallKey, sizeof(smallKey) - 1},
    {0xF3, 0, chestGameKey, sizeof(chestGameKey) - 1},
    {0xC7, 0, bossKey, sizeof(bossKey) - 1},
    {0x73, 0, minuet, sizeof(minuet) - 1},
    {0xFFFF, 0, nullptr, 0}};
MessageTableEntry *sNesMessageEntryTablePtr = nativeTable;
constexpr int MF_RAW = 0;
static std::string loadedMessage;
struct CustomMessage {
  std::string english;
  CwItemReceiptPresentation receiptPresentation{};
  void operator+=(const std::string& value) { english += value; }
  CustomMessage() = default;
  CustomMessage(std::string en, int = 0) : english(std::move(en)) {}
  CustomMessage(const Text& text, int = 0) : english(text.GetEnglish()) {}
  CustomMessage(std::string en, std::string, std::string, int = 0)
      : english(std::move(en)) {}
  void Replace(const char *key, const CustomMessage &value) {
    size_t pos = 0;
    while ((pos = english.find(key, pos)) != std::string::npos) {
      english.replace(pos, std::strlen(key), value.english);
      pos += value.english.size();
    }
  }
  void Replace(const char *key, const std::string &value) {
    Replace(key, CustomMessage(value));
  }
  void Replace(const char *key, const char *value) {
    Replace(key, std::string(value));
  }
  void Replace(std::string key, std::string value) {
    Replace(key.c_str(), value);
  }
  static std::string ITEM_OBTAINED(uint8_t icon) {
    return std::string("\x13") + static_cast<char>(icon);
  }
  static std::string NEWLINE() { return "\x01"; }
  static std::string WAIT_FOR_INPUT() { return "\x04"; }
  static std::string PLAYER_NAME() { return "\x0F"; }
  static std::string MESSAGE_END() { return "\x02"; }
  static std::string COLOR(std::string c) { return "\x05" + c; }
  std::vector<std::string> colors;
  void FormatString(std::string& str) const;
  void AutoFormatString(std::string& str) const;
  void ReplaceSpecialCharacters(std::string& str) const;
  void ReplaceColors(std::string& str) const;
  void ReplaceAltarIcons(std::string& str) const;
  void EncodeColors(std::string& str) const;
  size_t FindNEWLINE(std::string& str, size_t start) const;
  bool AddBreakString(std::string& str, size_t pos, std::string br) const;
  void Format() { FormatString(english); }
  void AutoFormat() { AutoFormatString(english); }
  void AutoFormat(int icon) {
    english.insert(0, ITEM_OBTAINED(icon));
    AutoFormat();
    Replace(WAIT_FOR_INPUT(), WAIT_FOR_INPUT() + ITEM_OBTAINED(icon));
  }
  std::string GetEnglish(int) const { return english; }
  void LoadIntoFont() const { loadedMessage = english; }
};
/* ACTUAL_FORMATTER */
constexpr int RHT_DUNGEON_ORDINARY = 0, RHT_DUNGEON_MASTERFUL = 1;
namespace Rando::StaticData {
struct ReceiptHint {
  CustomMessage message;
  CustomMessage GetHintMessage() const { return message; }
};
ReceiptHint hintTextTable[] = {{CustomMessage("&It's %gordinary%w.")},
                               {CustomMessage("&It's %rmasterful%w!")}};
} // namespace Rando::StaticData
void BuildQuarterHeartMessage(CustomMessage &msg) {
  msg.english = "A Quarter Heart!\x01You gained quarter of a heart.\x02";
}
void BuildDefenseUpgradeMessage(CustomMessage &msg) {
  msg.english = "Defense Upgrade!\x01Max defense reached!\x02";
}
void BuildSpeedUpgradeMessage(CustomMessage &msg) {
  msg.english = "Speed Upgrade!\x01Max speed reached!\x02";
}
void BuildPowerUpgradeMessage(CustomMessage &msg) {
  msg.english = "Power Upgrade!\x01Raises your double damage chance!\x02";
}
void BuildMagicStatUpgradeMessage(CustomMessage &msg) {
  msg.english =
      std::string("Magic Meter!\x05\x00", 14) + "more to reach max stat.\x02";
}
void BuildCrawlSpeedUpgradeMessage(CustomMessage &msg) {
  msg.english = "Crawl Speed Upgrade!\x02";
}
void BuildClimbSpeedUpgradeMessage(CustomMessage &msg) {
  msg.english = "Climb Speed Upgrade!\x02";
}
void BuildPushSpeedUpgradeMessage(CustomMessage &msg) {
  msg.english = "Push Speed Upgrade!\x02";
}
constexpr int MOD_RANDOMIZER = 1;
struct Player {
  struct {
    int comboForeignCheck = RC_RECEIPT_FIXTURE, getItemId = RG_NONE,
        modIndex = MOD_RANDOMIZER, itemId = 0;
  } getItemEntry;
} receiptPlayer;
struct PlayState {
  int sceneNum = SCENE_DEKU_TREE;
} play;
PlayState *gPlayState = &play;
#define GET_PLAYER(play) (&receiptPlayer)
namespace ComboRando {
constexpr int GAME_MM = 1, GAME_OOT = 0;
std::string StripGameSuffix(const std::string& name) {
  if (name.ends_with(" (MM)")) return name.substr(0, name.size() - 5);
  if (name.ends_with(" (OOT)")) return name.substr(0, name.size() - 6);
  return name;
}
struct ForeignItem {
  int itemGame = GAME_MM;
  bool trap = false;
  std::string itemName = "Goht's Remains", displayName = "Goht's Remains (MM)",
              fakeTrickName;
  bool HasDisguise() const { return !fakeTrickName.empty(); }
};
std::string ShownForeignName(const ForeignItem &, const char *name) {
  return std::string(name) + " (MM)";
}
} // namespace ComboRando
static ComboRando::ForeignItem foreign;
static ComboRando::ForeignItem rewardForeign;
static const char *resolvedForeign = nullptr;
static int foreignLatches = 0;
const ComboRando::ForeignItem *OOT_LookupForeign(int, const std::string &) {
  return &foreign;
}
const ComboRando::ForeignItem *OOT_LookupForeignByCheck(RandomizerCheck check) {
  assert(check == RC_PHANTOM_GANON);
  return &rewardForeign;
}
static bool mmRewardAvailable = false;
int32_t FixtureMmReward(int32_t dungeon, char *buffer, uint32_t capacity) {
  assert(dungeon == 1);
  const std::string reward = "Progressive Hookshot (OOT)";
  assert(capacity > reward.size());
  std::memcpy(buffer, reward.c_str(), reward.size() + 1);
  return reward.size();
}
extern "C" int32_t OOT_GetSeedItemIconInfo(const char* name, CwItemIconInfo* out) {
  *out = {};
  out->path = std::strcmp(name, "Fire Medallion") == 0
      ? "__OTR__textures/icon_item_24_static/gQuestIconMedallionFireTex"
      : "__OTR__textures/icon_item_custom/gItemIconDekuLeafTex";
  out->width = out->height = std::strcmp(name, "Fire Medallion") == 0 ? 24 : 32;
  return 1;
}
static int32_t FixtureSeedIcon(const char* name, CwItemIconInfo* out) {
  assert(std::string(name) == "Progressive Goron Lullaby");
  *out = {};
  out->path = "__OTR__icon_item_static_yar/gItemIconSongNoteTex";
  out->width = 16; out->height = 24; out->isIA8 = 1;
  return 1;
}
void *Combo_ResolveSym(const char *module, const char *symbol) {
  assert(std::string(module) == "2ship");
  if (std::string(symbol) == "MM_GetDungeonRewardName")
    return mmRewardAvailable ? reinterpret_cast<void *>(FixtureMmReward) : nullptr;
  if (std::string(symbol) == "MM_GetSeedItemIconInfo")
    return reinterpret_cast<void *>(FixtureSeedIcon);
  assert(std::string(symbol) == "MM_GetDungeonRewardIconInfo");
  return nullptr;
}
void Randomizer_LatchComboForeign(RandomizerCheck) { ++foreignLatches; }
const char *Randomizer_ComboForeignLatchedName(int32_t) {
  return resolvedForeign;
}
namespace Rando::StaticData {
struct Location {
  std::string GetName() const { return "fixture"; }
} receiptLocation;
Location *GetLocation(RandomizerCheck) { return &receiptLocation; }
} // namespace Rando::StaticData
namespace Rando::Traps {
void BuildIceTrapMessageNamed(CustomMessage &msg, const std::string &) {
  msg.english = "Trap receipt";
}
} // namespace Rando::Traps
#ifndef COMBO_EXPORT
#define COMBO_EXPORT
#endif
/* CONTEXT_BUILDERS */
/* FOREIGN_BUILDER */
/* DONOR_EXPORT */

// Test-only entry point for the receiver integration. OoT is dormant while MM
// owns the active save; its generated seed still supplies this saved option.
extern "C" COMBO_EXPORT void FixtureConfigureForestCompassReceipt(int information) {
  for (const auto &fixture : traditionalReceipts)
    Rando::StaticData::itemNameToEnum[fixture.name] = fixture.item;
  receiptContext.information = information;
  receiptContext.generated = true;
  receiptContext.spoiler = false;
  receiptContext.bossShuffle = RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF;
  receiptContext.mqMode = 2;
  receiptContext.mqCount = 6;
  receiptContext.dungeon.mq = true;
  receiptContext.placements[RC_PHANTOM_GANON] = {RG_DEKU_LEAF, "Deku Leaf"};
  randoActive = false;
  gPlayState = nullptr;
}

static void CheckForestCompassReceiptInformation() {
  const auto contextBefore = receiptContext;
  auto *playBefore = gPlayState;
  const bool activeBefore = randoActive;
  const auto saveBefore = gSaveContext;
  const int resolutionsBefore = liveResolutionCalls;
  const int latchesBefore = foreignLatches;
  for (int information : {0, 1}) {
    FixtureConfigureForestCompassReceipt(information);
    char buffer[1269];
    const int32_t size = OOT_GetItemReceiptText("Forest Temple Compass", buffer, sizeof(buffer));
    assert(size > 0);
    const std::string text(buffer, size);
    CwItemReceiptPresentation presentation{};
    assert(OOT_GetDungeonItemReceiptPresentation("Forest Temple Compass", &presentation) == information);
    if (information) {
      assert(ComboReceipt_HasIcon(&presentation));
      assert(std::string(presentation.iconPath) == "__OTR__@oot:textures/icon_item_custom/gItemIconDekuLeafTex");
      assert(presentation.rewardLine == 2 && presentation.iconWidth == 32 && presentation.iconHeight == 32);
    }
    assert(text.find("Forest Temple Compass") != std::string::npos);
    if (information) {
      assert(text.find("You got the Compass!") == std::string::npos && "On must replace the native tutorial");
      assert(text.find("Now you can see hidden things.") == std::string::npos);
      assert(text.find("masterful") == std::string::npos && "Compass information has exactly the title, boss, and reward lines");
      assert(text.find("It points to") != std::string::npos);
      assert(text.find("Phantom Ganon") != std::string::npos);
      assert(text.find("Defeating the boss grants") != std::string::npos);
      assert(text.find("Deku Leaf") != std::string::npos);
      assert(text.find('\x10') == std::string::npos && "Compass information is one textbox");
      assert(std::count(text.begin(), text.end(), '\x11') == 2);
    } else {
      assert(text.find("You got the Compass!") != std::string::npos);
      assert(text.find("Now you can see hidden things.") != std::string::npos);
      assert(text.find("masterful") != std::string::npos);
      assert(text.find("Phantom Ganon") == std::string::npos);
      assert(text.find("Defeating the boss grants") == std::string::npos);
    }
  }
  assert(!std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
  assert(liveResolutionCalls == resolutionsBefore && foreignLatches == latchesBefore);
  receiptContext = contextBefore;
  gPlayState = playBefore;
  randoActive = activeBefore;
  std::cout << "Forest Temple Compass: saved Off reproduces tutorial/title; saved On replaces the tutorial with three boss/reward lines, dormant seed preserved passed\n";
}

static void CheckApprovedCompassExample() {
  const auto contextBefore = receiptContext;
  const auto overridesBefore = std::to_array(entranceOverrides);
  receiptContext.information = 1;
  receiptContext.bossShuffle = 1;
  receiptContext.placements[RC_QUEEN_GOHMA] = {RG_KOKIRI_EMERALD, "Kokiri Emerald"};
  receiptContext.placements[RC_VOLVAGIA] = {RG_FIRE_MEDALLION, "Fire Medallion"};
  entranceOverrides[0] = {ENTR_DEKU_TREE_BOSS_ENTRANCE, 1, ENTR_FIRE_TEMPLE_BOSS_ENTRANCE, 1};
  char buffer[1269];
  const int size = OOT_GetItemReceiptText("Great Deku Tree Compass", buffer, sizeof(buffer));
  assert(size > 0);
  std::string plain;
  for (int i = 0; i < size; ++i) {
    const uint8_t c = buffer[i];
    if (c == 0x11) plain += '\n';
    else if (c >= 0x20) plain += c;
  }
  assert(plain == "You found the Deku Tree Compass!\nIt points to Volvagia!\nDefeating the boss grants the Fire Medallion!");
  CwItemReceiptPresentation presentation{};
  assert(OOT_GetDungeonItemReceiptPresentation("Great Deku Tree Compass", &presentation) == 1);
  assert(std::string(presentation.iconPath) == "__OTR__@oot:textures/icon_item_24_static/gQuestIconMedallionFireTex");
  assert(presentation.iconWidth == 24 && presentation.iconHeight == 24);
  assert(liveResolutionCalls == 0);
  receiptContext = contextBefore;
  std::copy(overridesBefore.begin(), overridesBefore.end(), entranceOverrides);
}

int main() {
  // Item names and unresolved native text IDs come from the real OoT catalog.
  // Message-table ownership is the seam; unique bodies reveal wrong IDs.
  std::vector<std::string> songBodies;
  songBodies.reserve(std::size(traditionalReceipts));
  std::vector<MessageTableEntry> nativeMessages(nativeTable, nativeTable + std::size(nativeTable) - 1);
  for (const auto& fixture : traditionalReceipts) {
    Rando::StaticData::itemNameToEnum[fixture.name] = fixture.item;
    if (fixture.item < RG_ZELDAS_LULLABY || fixture.item > RG_PRELUDE_OF_LIGHT || fixture.expectedText == 0x73)
      continue;
    songBodies.emplace_back(std::string("Native song tutorial: ") + fixture.name + "\x02");
    const auto& body = songBodies.back();
    nativeMessages.push_back({fixture.expectedText, 0, body.c_str(), static_cast<uint32_t>(body.size())});
  }
  nativeMessages.push_back({0xFFFF, 0, nullptr, 0});
  sNesMessageEntryTablePtr = nativeMessages.data();
  CheckForestCompassReceiptInformation();
  CheckApprovedCompassExample();
  entranceOverrides[0] = {ENTR_DEKU_TREE_BOSS_ENTRANCE, 1,
                          ENTR_FOREST_TEMPLE_BOSS_ENTRANCE, 1};
  char buffer[1269];
  auto read = [&](const char *name) {
    const int32_t size = OOT_GetItemReceiptText(name, buffer, sizeof(buffer));
    if (size <= 0)
      std::cerr << "Missing donor receipt: " << name << '\n';
    assert(size > 0);
    return std::string(buffer, size);
  };
  const auto cane = read("Cane of Somaria"), roc = read("Progressive Roc"),
             agony = read("Stone of Agony");
  assert(cane.find("Statue") != std::string::npos &&
         roc.find("high jump") != std::string::npos);
  stateAdvanced =
      true; // grant happened; recollection still refers to the frozen identity
  assert(read("Cane of Somaria") == cane &&
         "repeat receipt advanced to the next Cane skill");
  assert(read("Progressive Roc") == roc &&
         "repeat receipt advanced to Roc's Cape");
  assert(read("Stone of Agony") == agony &&
         "repeat receipt advanced to Quartz");
  assert(liveResolutionCalls == 0);
  assert(read("Magic Meter").find("Use magic") != std::string::npos);
  assert(read("Enhanced Magic Meter").find("twice") != std::string::npos);
  const auto leaf = read("Deku Leaf");
  assert(leaf.find("glide") != std::string::npos &&
         leaf.find('\0') != std::string::npos);
  assert(read("Power Upgrade").find("double damage") != std::string::npos);
  assert(read("Magic Stat Upgrade").find("reach max stat") !=
         std::string::npos);
  assert(read("Goht's Remains").find("\x01Goht") != std::string::npos);
  assert(read("Goron Lullaby").find("\x01Goron") != std::string::npos);
  assert(read("New Wave Bossa Nova").find("\x03New Wave") != std::string::npos);
  assert(read("Song of Healing").find("\x06Song") != std::string::npos);
  assert(read("Song of Storms (MM)").find("rain") != std::string::npos);
  // Export keeps the native tutorial body even for randomizer keys whose GI
  // entry points at the generic custom textbox. Extra seed info follows it.
  assert(read("Forest Temple Small Key").find("locked door") != std::string::npos);
  assert(read("Forest Temple Boss Key").find("Boss lurks") != std::string::npos);
  assert(read("Ganon's Castle Boss Key").find("Boss lurks") != std::string::npos);
  assert(read("Minuet of Forest").find("take you to the forest") != std::string::npos);
  receiptContext.information = 0;
  for (const auto& fixture : traditionalReceipts) {
    const auto found = std::find_if(nativeMessages.begin(), nativeMessages.end(), [&](const auto& message) {
      return message.textId == fixture.expectedText;
    });
    assert(found != nativeMessages.end());
    std::string expected;
    assert(ComboItemReceiptText::FromOotMessage(std::string_view(found->segment, found->msgSize), expected));
    const auto receipt = read(fixture.name);
    if (receipt.compare(0, expected.size(), expected) != 0)
      std::cerr << "Replaced traditional receipt: " << fixture.name << '\n';
    assert(receipt.compare(0, expected.size(), expected) == 0);
  }
  assert(liveResolutionCalls == 0);
  for (int enabled : {0, 1}) {
    receiptContext.information = enabled;
    assert((read("Great Deku Tree Compass").find("see hidden things") != std::string::npos) == !enabled);
    assert((read("Great Deku Tree Map").find("Blue rooms") != std::string::npos) == !enabled);
  }
  const std::array<std::pair<const char *, const char *>, 4> mmMaps{
      {{"Woodfall Map", "Woodfall"},
       {"Snowhead Map", "Snowhead"},
       {"Great Bay Map", "Zora Cape's turtle"},
       {"Stone Tower Map", "Stone Tower"}}};
  for (int enabled : {0, 1}) {
    receiptContext.information = enabled;
    for (const auto &[map, entrance] : mmMaps) {
      const auto text = read(map);
      assert((text.find("Its entrance is at") != std::string::npos) ==
             bool(enabled));
      if (enabled)
        assert(text.find(entrance) != std::string::npos);
    }
  }
  const auto shuffledCompass = read("Great Deku Tree Compass");
  assert(shuffledCompass.find("Phantom Ganon") != std::string::npos);
  assert(shuffledCompass.find("ordinary") == std::string::npos);
  receiptContext.placements[RC_QUEEN_GOHMA] = {RG_KOKIRI_EMERALD,
                                               "Kokiri Emerald"};
  receiptContext.placements[RC_PHANTOM_GANON] = {RG_DEKU_LEAF, "Deku Leaf"};
  auto rewardCompass = read("Great Deku Tree Compass");
  assert(rewardCompass.find("Deku Leaf") != std::string::npos &&
         rewardCompass.find("Kokiri Emerald") == std::string::npos);
  receiptContext.placements[RC_PHANTOM_GANON] = {RG_COMBO_FOREIGN,
                                                 "Combo Foreign Item"};
  rewardForeign.itemName = "Progressive Goron Lullaby";
  rewardForeign.displayName = "Progressive Goron Lullaby (MM)";
  assert(
      read("Great Deku Tree Compass").find("Progressive Goron Lullaby (MM)") !=
      std::string::npos);
  assert(liveResolutionCalls == 0 && foreignLatches == 0);
  receiptContext.dungeonShuffle = 1;
  entranceOverrides[1] = {ENTR_DODONGOS_CAVERN_ENTRANCE, 1,
                          ENTR_DEKU_TREE_ENTRANCE, 1};
  entranceOverrides[2] = {ENTR_DEKU_TREE_ENTRANCE, 1,
                          ENTR_WATER_TEMPLE_ENTRANCE, 1};
  EntranceTracker::data[ENTR_DODONGOS_CAVERN_ENTRANCE] = {
      "Dodongo's Cavern"};
  EntranceTracker::data[ENTR_DEKU_TREE_ENTRANCE] = {"Kokiri Forest"};
  assert(read("Great Deku Tree Map").find("Dodongo's Cavern") !=
         std::string::npos);
  assert(read("Great Deku Tree Map").find("Kokiri Forest") ==
         std::string::npos);
  entranceOverrides[2].override = ENTR_FOREST_TEMPLE_ENTRANCE;
  const auto forestMap = read("Forest Temple Map");
  assert(forestMap.find("Deku Tree") != std::string::npos && forestMap.find("Kokiri Forest") == std::string::npos);
  assert(forestMap.find('\x10') == std::string::npos && forestMap.find("vanilla") != std::string::npos);
  entranceOverrides[2].override = ENTR_WATER_TEMPLE_ENTRANCE;
  // Ownership alone makes the pause view available, as for Start With. The
  // acquisition latch and item grant history have never been set in this save.
  assert(Randomizer_GetDungeonItemInfoTextId(ITEM_COMPASS) == 0);
  assert(Randomizer_GetDungeonItemInfoTextId(ITEM_DUNGEON_MAP) == 0);
  gSaveContext.inventory.dungeonItems[0] =
      (1 << DUNGEON_MAP) | (1 << DUNGEON_COMPASS);
  const auto saveBeforeInfo = gSaveContext;
  for (const auto &[item, id] : std::array<std::pair<int, uint16_t>, 2>{
           {{ITEM_COMPASS, TEXT_DESC_DUNGEON_COMPASS_INFO},
            {ITEM_DUNGEON_MAP, TEXT_DESC_DUNGEON_MAP_INFO}}}) {
    assert(Randomizer_GetDungeonItemInfoTextId(item) == id);
    uint16_t text = id;
    bool fromTable = true;
    BuildDungeonPauseInfoMessage(&text, &fromTable);
    assert(!fromTable && loadedMessage.find("You found") == std::string::npos);
    assert(loadedMessage.find(item == ITEM_COMPASS
                                  ? "Progressive Goron Lullaby (MM)"
                                  : "Dodongo's Cavern") !=
           std::string::npos);
  }
  assert(std::memcmp(&saveBeforeInfo, &gSaveContext, sizeof(gSaveContext)) ==
         0);
  play.sceneNum = SCENE_HYRULE_FIELD;
  assert(Randomizer_GetDungeonItemInfoTextId(ITEM_COMPASS) ==
         0); // overworld map has no dungeon selection
  play.sceneNum = SCENE_DEKU_TREE;
  // A dormant OoT donor still has a generated/restored seed in MM-first runs.
  randoActive = false;
  gPlayState = nullptr;
  receiptContext.dungeon.mq = true;
  masterQuest = true; // runtime helper deliberately ignores this while dormant
  assert(OOT_MapCompassInfoEnabled() == 1);
  assert(read("Great Deku Tree Map").find("Master Quest") != std::string::npos);
  assert(read("Great Deku Tree Compass").find("masterful") ==
         std::string::npos);
  assert(
      read("Great Deku Tree Compass").find("Progressive Goron Lullaby (MM)") !=
      std::string::npos);
  assert(read("Great Deku Tree Map").find("Dodongo's Cavern") !=
         std::string::npos);
  receiptContext.generated = false;
  assert(OOT_MapCompassInfoEnabled() == 0);
  receiptContext.spoiler = true;
  assert(OOT_MapCompassInfoEnabled() == 1);
  receiptContext.information = 0;
  assert(OOT_MapCompassInfoEnabled() == 0);
  receiptContext.generated = true;
  receiptContext.spoiler = false;
  randoActive = true;
  masterQuest = false;
  gPlayState = &play;
  receiptContext.information = 0;
  assert(Randomizer_GetDungeonItemInfoTextId(ITEM_COMPASS) == 0);
  assert(read("Great Deku Tree Compass").find("Phantom Ganon") ==
         std::string::npos);
  assert(read("Great Deku Tree Compass").find("Lullaby") == std::string::npos);
  assert(read("Great Deku Tree Map").find("Dodongo's Cavern") ==
         std::string::npos);
  assert(read("Great Deku Tree Map").find("ordinary") != std::string::npos);
  receiptContext.information = 1;
  // Unshuffled/native pickup entries still identify their current dungeon.
  receiptPlayer.getItemEntry.modIndex = 0;
  receiptPlayer.getItemEntry.itemId = ITEM_DUNGEON_MAP;
  uint16_t nativeText = 0x66;
  bool nativeTable = true;
  loadedMessage.clear();
  BuildMapMessage(&nativeText, &nativeTable);
  assert(!nativeTable &&
         loadedMessage.find("Dodongo's Cavern") != std::string::npos);
  receiptPlayer.getItemEntry.itemId = ITEM_COMPASS;
  nativeTable = true;
  BuildMapMessage(&nativeText, &nativeTable);
  assert(!nativeTable && loadedMessage.find("Progressive Goron Lullaby (MM)") !=
                             std::string::npos);
  receiptPlayer.getItemEntry.modIndex = MOD_RANDOMIZER;
  masterQuest = true;
  assert(read("Great Deku Tree Compass").find("masterful") ==
         std::string::npos);
  assert(read("Great Deku Tree Map").find("Master Quest") != std::string::npos);
  assert(read("Snowhead Compass").find("Goht") != std::string::npos);
  mmRewardAvailable = true;
  assert(read("Snowhead Compass").find("Progressive Hookshot (OOT)") !=
         std::string::npos);
  assert(read("Ice Cavern Compass").find("Phantom Ganon") == std::string::npos);
  assert(read("Ice Cavern Compass").find("reward") == std::string::npos);
  receiptContext.mqMode = RO_MQ_DUNGEONS_NONE;
  assert(read("Bottom of the Well Compass").find("reward") ==
         std::string::npos);
  receiptContext.mqMode = RO_MQ_DUNGEONS_NONE;
  assert(read("Great Deku Tree Compass").find("masterful") ==
         std::string::npos);
  receiptContext.bossShuffle = RO_BOSS_ROOM_ENTRANCE_SHUFFLE_OFF;
  assert(read("Great Deku Tree Compass").find("Gohma") != std::string::npos);
  receiptContext.bossShuffle = 1;
  entranceOverrides[0].override =
      ENTR_DEKU_TREE_ENTRANCE; // mixed pool: this door no longer reaches a boss
  assert(read("Great Deku Tree Compass").find("Gohma") == std::string::npos);
  assert(read("Great Deku Tree Compass").find("reward") == std::string::npos);
  gSaveContext.inventory.gsTokens = 42;
  assert(read("Gold Skulltula Token").find("43") != std::string::npos);
  receiptNeiSave.comboObtained[FC_MM_SKULLS_SWAMP] = 7;
  receiptNeiSave.comboObtained[FC_MM_SKULLS_OCEAN] = 19;
  assert(read("Swamp Gold Skulltula Token").find("8") != std::string::npos);
  assert(read("Ocean Gold Skulltula Token").find("20") != std::string::npos);
  assert(gSaveContext.inventory.gsTokens == 42);
  assert(receiptNeiSave.comboObtained[FC_MM_SKULLS_SWAMP] == 7);
  CustomMessage foreignMessage;
  BuildComboForeignMessage(&receiptPlayer, foreignMessage);
  std::string foreignBody;
  assert(ComboItemReceiptText::FromOotMessage(foreignMessage.GetEnglish(MF_RAW),
                                              foreignBody));
  assert(foreignBody.find("\x01Goht") != std::string::npos &&
         foreignBody.find("mechanical") != std::string::npos);
  foreign.itemName = "Progressive Goron Lullaby";
  resolvedForeign = "Goron Lullaby Intro";
  BuildComboForeignMessage(&receiptPlayer, foreignMessage);
  assert(foreignMessage.english.find("opening bars") != std::string::npos);
  const int latchesBeforeTrap = foreignLatches;
  foreign.trap = true;
  BuildComboForeignMessage(&receiptPlayer, foreignMessage);
  assert(foreignMessage.english == "Trap receipt" &&
         foreignLatches == latchesBeforeTrap);
  foreign.trap = false;
  foreign.itemName = "Unknown MM Item";
  foreign.displayName = "Unknown MM Item (MM)";
  resolvedForeign = nullptr;
  BuildComboForeignMessage(&receiptPlayer, foreignMessage);
  assert(foreignMessage.english.find("Unknown MM Item") != std::string::npos);
  // The actual receive route for MM Maps found in OoT uses this same info,
  // independently of the donor export and the MM-native catalog fixture.
  for (int enabled : {0, 1}) {
    receiptContext.information = enabled;
    for (const auto &[map, entrance] : mmMaps) {
      foreign.itemName = map;
      foreign.displayName = std::string(map) + " (MM)";
      BuildComboForeignMessage(&receiptPlayer, foreignMessage);
      assert((foreignMessage.english.find("Its entrance is at") !=
              std::string::npos) == bool(enabled));
      if (enabled)
        assert(foreignMessage.english.find(entrance) != std::string::npos);
    }
  }
  assert(OOT_GetItemReceiptText("Deku Leaf", buffer, 5) == 0);
  assert(OOT_GetItemReceiptText("unknown", buffer, sizeof(buffer)) == 0);
  assert(OOT_GetItemReceiptText("Ice Trap", buffer, sizeof(buffer)) == 0);
  assert(OOT_GetItemReceiptText("Green Rupee", buffer, sizeof(buffer)) == 0);
  assert(OOT_GetItemReceiptText(nullptr, buffer, sizeof(buffer)) == 0);
  assert(OOT_GetItemReceiptText("Deku Leaf", nullptr, sizeof(buffer)) == 0);
  globals.gRandoContext = nullptr;
  assert(OOT_MapCompassInfoEnabled() == 0);
  assert(OOT_GetItemReceiptText("Deku Leaf", buffer, sizeof(buffer)) == 0);
  globals.gRandoContext = &receiptContext;
  assert(read("Deku Leaf") == leaf);
  throwOnRead = true;
  assert(OOT_GetItemReceiptText("Magic Meter", buffer, sizeof(buffer)) == 0);
  std::cout << "actual OoT export: full text, fixed tiers, binary lengths, "
               "guards and exceptions passed\n";
  return 0;
}
