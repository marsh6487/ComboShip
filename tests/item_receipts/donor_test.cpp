// Production OoT export; only runtime item/message ownership is replaced.
#include "combo/menu/ComboItemReceiptText.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x)                                                      \
  }                                                                            \
  ;
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
constexpr int ITEM_CATEGORY_JUNK = 0, ITEM_CATEGORY_MAJOR = 1,
              TEXT_RANDOMIZER_CUSTOM_ITEM = 0x9000;
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
    {"Ice Trap", RG_ICE_TRAP},
    {"Green Rupee", RG_GREEN_RUPEE},
    {"None", RG_NONE}};
struct Item {
  RandomizerGet id;
  int GetCategory() {
    return id == RG_GREEN_RUPEE ? ITEM_CATEGORY_JUNK : ITEM_CATEGORY_MAJOR;
  }
  std::shared_ptr<GetItemEntry> GetGIEntryUnresolved() const {
    if (throwOnRead)
      throw 1;
    uint16_t text = id == RG_STONE_OF_AGONY ? 0x68
                    : id == RG_MAGIC_SINGLE ? 0xE4
                    : id == RG_MAGIC_DOUBLE ? 0xE8
                                            : TEXT_RANDOMIZER_CUSTOM_ITEM;
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
  void *gRandoContext = reinterpret_cast<void *>(1);
  void *gRandomizer = reinterpret_cast<void *>(1);
};
OTRGlobals globals;
OTRGlobals *OTRGlobals::Instance = &globals;
/* DONOR_MESSAGES */
const CustomItemMessageEntry *GetCustomItemMessage(int rg) {
  for (const auto &msg : receiptMessages)
    if (msg.rgId == rg)
      return &msg;
  return nullptr;
}
static const char stone[] =
    "You found the \x05\x44Stone of Agony\x05\x40!\x01It reacts to hidden "
    "secrets.\x02";
static const char magic[] =
    "You received the Magic Meter!\x04Use magic items with it.\x02";
static const char doubleMagic[] =
    "Your Magic Meter is enhanced!\x04You can use twice as much magic.\x02";
MessageTableEntry nativeTable[] = {
    {0x68, 0, stone, sizeof(stone) - 1},
    {0xE4, 0, magic, sizeof(magic) - 1},
    {0xE8, 0, doubleMagic, sizeof(doubleMagic) - 1},
    {0xFFFF, 0, nullptr, 0}};
MessageTableEntry *sNesMessageEntryTablePtr = nativeTable;
constexpr int MF_RAW = 0;
struct CustomMessage {
  std::string english;
  std::string GetEnglish(int) const { return english; }
};
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
#define COMBO_EXPORT
/* DONOR_EXPORT */

int main() {
  char buffer[1269];
  auto read = [&](const char *name) {
    const int32_t size = OOT_GetItemReceiptText(name, buffer, sizeof(buffer));
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
  assert(OOT_GetItemReceiptText("Deku Leaf", buffer, 5) == 0);
  assert(OOT_GetItemReceiptText("unknown", buffer, sizeof(buffer)) == 0);
  assert(OOT_GetItemReceiptText("Ice Trap", buffer, sizeof(buffer)) == 0);
  assert(OOT_GetItemReceiptText("Green Rupee", buffer, sizeof(buffer)) == 0);
  assert(OOT_GetItemReceiptText(nullptr, buffer, sizeof(buffer)) == 0);
  assert(OOT_GetItemReceiptText("Deku Leaf", nullptr, sizeof(buffer)) == 0);
  globals.gRandoContext = nullptr;
  assert(OOT_GetItemReceiptText("Deku Leaf", buffer, sizeof(buffer)) == 0);
  globals.gRandoContext = reinterpret_cast<void *>(1);
  assert(read("Deku Leaf") == leaf);
  throwOnRead = true;
  assert(OOT_GetItemReceiptText("Magic Meter", buffer, sizeof(buffer)) == 0);
  std::cout << "actual OoT export: full text, fixed tiers, binary lengths, "
               "guards and exceptions passed\n";
}
