// Uses real MM item IDs, the production static/FC catalogs and receipt builder.
// The font/message table and donor service are the runtime boundaries.
#include "2s2h/FleetShipCombo/FleetComboItems.h"
#include "2s2h/FleetShipCombo/FleetComboItemsGlue.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include "2s2h/Rando/StaticData/StaticData.h"
#include "ComboExport.h"
#include "ComboItemReceiptText.h"
extern "C" {
#include "message_data_static.h"
#include "mods/extended_inventory.h"
#include "soh/Enhancements/randomizer/randomizerTypes.h"
}
#include "receipt_catalogs.inc"
#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>

extern "C" {
PlayState *gPlayState = nullptr;
float sNESFontWidths[160];
const NeiItem *Nei_FindByItem(int32_t item) {
  for (const auto &nei : sReceiptNeiItems)
    if (nei.item != NEI_NO_ITEM && nei.item == item)
      return &nei;
  return nullptr;
}
const NeiItem *Nei_FindByRg(int16_t id) {
  for (const auto &nei : sReceiptNeiItems)
    if (nei.rg != NEI_NO_RG && nei.rg == id)
      return &nei;
  return nullptr;
}
static NeiSaveData neiSave{};
NeiSaveData *Nei_Save() { return &neiSave; }
SaveContext gSaveContext{};
u8 gItemSlots[77]{};
u8 Nei_BulletBagLevel() { return std::min<int>(3, neiSave.ootUpgrades & 7); }
}

static std::string requested;
static bool donorReady = true;
static int donorReads = 0;
static uint64_t generation = 0;
namespace Rando::MiscBehavior {
uint64_t ComboRandoGen() { return generation; }
} // namespace Rando::MiscBehavior
extern "C" COMBO_EXPORT int32_t OOT_GetItemReceiptText(const char *name,
                                                       char *buffer,
                                                       uint32_t capacity) {
  requested = name;
  if (!donorReady || requested == "Progressive Hookshot")
    return 0;
  ++donorReads;
  const std::string body =
      requested == "Deku Leaf"
          ? ComboItemReceiptText::FromNeiMarkup(kDekuLeafMessage)
          : "Full description of " + requested + " #" +
                std::to_string(donorReads);
  assert(body.size() <= capacity);
  std::memcpy(buffer, body.data(), body.size());
  return body.size();
}

int main() {
  for (float &w : sNESFontWidths)
    w = 8;
  PlayState play{};
  gPlayState = &play;
  // Header is deliberately a story entry with a different icon and next ID.
  const char bow[] = "\x00\x00\x20\x00\x03\xFF\xFF\xFF\xFF\xFF\xFF"
                     "You got the Hero's Bow!\x10Press \xB2 to aim.\x19\xBF";
  const uint16_t bowText = Player_GetItemReceiptTextId(GI_QUIVER_30, ITEM_BOW);
  assert(bowText != 0);
  const char hook[] =
      "\x00\x00\x20\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
      "You got the Hookshot!\x11Latch onto distant targets.\x19\xBF";
  const uint16_t hookText =
      Player_GetItemReceiptTextId(GI_HOOKSHOT, ITEM_HOOKSHOT);
  assert(hookText != 0);
  MessageTableEntry table[] = {{bowText, 0, bow, sizeof(bow) - 1},
                               {hookText, 0, hook, sizeof(hook) - 1},
                               {0xFFFF, 0, nullptr, 0}};
  play.msgCtx.messageTableNES = table;
  CustomMessage::Entry entry;
  entry.icon = 0xF5;
  assert(Rando::ApplyItemReceiptText(RI_BOW, entry));
  assert(entry.msg.find("aim") != std::string::npos);
  assert(entry.msg.find('\x19') == std::string::npos &&
         entry.msg.find('\xBF') == std::string::npos);
  assert(entry.icon == 0xF5 && entry.nextMessageID == 0xFFFF &&
         !entry.autoFormat);
  Rando::AppendReceiptSource(entry, " (Bank reward)");
  assert(entry.msg.back() == '\xBF' &&
         entry.msg.find("Bank reward") != std::string::npos);

  // Canonical GI/Item namespace validation protects frog/model aliases.
  assert(Player_GetItemReceiptTextId(GI_MASK_DON_GERO, ITEM_NONE) == 0);
  assert(Player_GetItemReceiptTextId(GI_MASK_DON_GERO, ITEM_BOW) == 0);
  assert(Player_GetItemReceiptTextId(GI_NONE, ITEM_BOW) == 0);
  assert(Player_GetItemReceiptTextId(GI_MAX, ITEM_BOW) == 0);
  assert(Player_GetItemReceiptTextId(-1, ITEM_BOW) == 0);
  assert(Player_GetItemReceiptTextId(GI_SHIP, ITEM_SHIP) == 0);

  entry = {};
  entry.icon = 0xF5;
  assert(Rando::ApplyItemReceiptText(RI_OOT_NEI_DEKU_LEAF, entry));
  assert(entry.msg.find("glide") != std::string::npos &&
         entry.msg.find("gust") != std::string::npos);
  assert(entry.msg.find('%') == std::string::npos &&
         entry.msg.find('^') == std::string::npos);
  assert(entry.msg.find('\0') !=
         std::string::npos); // default color is binary NUL
  Rando::AppendReceiptSource(entry, "");
  assert(entry.msg.back() == '\xBF');
  donorReady = false;
  entry.msg = "fallback";
  assert(Rando::ApplyItemReceiptText(
      RI_OOT_NEI_DEKU_LEAF, entry)); // local registry works without donor
  assert(entry.msg.find("glide") != std::string::npos);
  assert(!Rando::ApplyItemReceiptText(RI_SINGLE_MAGIC, entry));
  entry = {};
  entry.msg = "generic Clawshot";
  assert(!Rando::ApplyItemReceiptText(RI_CLAWSHOT, entry) &&
         entry.msg == "generic Clawshot");
  neiSave.ootHookshotLevel = 1;
  assert(!Rando::ApplyItemReceiptText(
      RI_HOOKSHOT, entry)); // no incorrect vanilla Hookshot for Longshot
  neiSave.ootHookshotLevel = 0;
  assert(Rando::ApplyItemReceiptText(RI_HOOKSHOT, entry) &&
         entry.msg.find("Latch") != std::string::npos);
  // Actual MM decoder stores a line offset for every newline and at page end.
  // A four-line Roc's Cape page previously indexed beyond its three entries.
  assert(Rando::ApplyItemReceiptText(RI_OOT_NEI_ROCS_CAPE, entry));
  const auto lineCapacity =
      sizeof(play.msgCtx.unk11F1A) / sizeof(play.msgCtx.unk11F1A[0]);
  size_t lineIndex = 0;
  for (unsigned char c : entry.msg) {
    if (c == 0x10 || c == 0x12)
      lineIndex = 0;
    else if (c == 0x11)
      ++lineIndex;
    assert(lineIndex < lineCapacity &&
           "receipt exceeds the MM decoder line array");
  }
  donorReady = true;
  assert(Rando::ApplyItemReceiptText(RI_OOT_NEI_DEKU_LEAF,
                                     entry)); // no negative cache
  const std::pair<RandoItemId, const char *> concrete[] = {
      {RI_SINGLE_MAGIC, "Magic Meter"},
      {RI_DOUBLE_MAGIC, "Enhanced Magic Meter"},
      {RI_CLAWSHOT, "Clawshot"},
      {RI_OOT_MASTER_SWORD, "Master Sword"},
      {RI_OOT_TRUE_MASTER_SWORD, "True Master Sword"},
      {RI_OOT_HAMMER, "Megaton Hammer"},
      {RI_OOT_IRON_KNUCKLE_AXE, "Iron Knuckle's Axe"},
      {RI_OOT_BIGGORON_SWORD, "Biggoron's Sword"},
      {RI_OOT_GORONS_BRACELET, "Goron's Bracelet"},
      {RI_OOT_SILVER_GAUNTLETS, "Silver Gauntlets"},
      {RI_OOT_GOLDEN_GAUNTLETS, "Golden Gauntlets"},
      {RI_OOT_NEI_ROCS_FEATHER, "Progressive Roc"},
      {RI_OOT_NEI_ROCS_CAPE, "Roc's Cape"},
      {RI_OOT_STONE_OF_AGONY, "Stone of Agony"},
      {RI_OOT_QUARTZ_OF_MOTION, "Quartz of Motion"},
      {RI_OOT_NEI_CANE_PACCI_FLIP, "Cane of Pacci"},
      {RI_OOT_NEI_CANE_PACCI_STONE, "Pacci Stone Skill"},
      {RI_OOT_NEI_CANE_PACCI_ULTRAHAND, "Ultrahand"},
      {RI_OOT_NEI_CANE_SOMARIA_BLOCK, "Somaria Block Skill"},
      {RI_OOT_NEI_CANE_SOMARIA_PLATFORM, "Somaria Platform Skill"}};
  for (const auto &[id, name] : concrete) {
    requested.clear();
    assert(Rando::ApplyItemReceiptText(id, entry));
    assert(requested == name &&
           "concrete receipt went back through progressive donor state");
  }
  for (int owned = 0; owned <= 3; ++owned) {
    neiSave.ootHookshotLevel = owned;
    assert(Rando::ApplyItemReceiptText(RI_HOOKSHOT, entry));
    assert(requested == (owned == 0   ? "Hookshot"
                         : owned == 1 ? "Longshot"
                                      : "Ultrashot"));
  }
  neiSave.ootHookshotLevel = 0;
  INV_CONTENT(ITEM_HOOKSHOT) = ITEM_HOOKSHOT;
  assert(Rando::ApplyItemReceiptText(RI_HOOKSHOT, entry) &&
         requested == "Longshot");
  neiSave.ootQuestItems = 1u << OOT_QUEST_STONE_OF_AGONY;
  assert(Rando::ApplyItemReceiptText(RI_OOT_STONE_OF_AGONY, entry) &&
         requested == "Quartz of Motion");
  neiSave.ootQuestItems = 0;
  neiSave.slingshotOwned = 0;
  assert(Rando::ApplyItemReceiptText(RI_FAIRY_SLINGSHOT, entry) &&
         requested == "Fairy Slingshot");
  neiSave.slingshotOwned = 1;
  for (int level = 0; level <= 3; ++level) {
    neiSave.ootUpgrades = level;
    assert(Rando::ApplyItemReceiptText(RI_FAIRY_SLINGSHOT, entry));
    assert(requested == (level == 0   ? "Fairy Slingshot"
                         : level == 1 ? "Big Deku Seed Bullet Bag"
                                      : "Biggest Deku Seed Bullet Bag"));
  }
  const char *sticks[] = {"Deku Stick Bag", "Deku Stick Capacity (20)",
                          "Deku Stick Capacity (30)"};
  const char *nuts[] = {"Deku Nut Bag", "Deku Nut Capacity (30)",
                        "Deku Nut Capacity (40)"};
  for (int level = 0; level <= 3; ++level) {
    gSaveContext.save.saveInfo.inventory.upgrades =
        (level << gUpgradeShifts[UPG_DEKU_STICKS]) |
        (level << gUpgradeShifts[UPG_DEKU_NUTS]);
    assert(
        Rando::ApplyItemReceiptText(RI_OOT_PROGRESSIVE_STICK_CAPACITY, entry));
    assert(requested == sticks[std::min(level, 2)]);
    assert(Rando::ApplyItemReceiptText(RI_OOT_PROGRESSIVE_NUT_CAPACITY, entry));
    assert(requested == nuts[std::min(level, 2)]);
  }
  assert(Rando::ApplyForeignItemReceiptText("Magic Meter", entry,
                                            RC_CLOCK_TOWER_ROOF_OCARINA));
  const auto firstReceipt = entry.msg;
  const int readsBeforeRepeat = donorReads;
  assert(Rando::ApplyForeignItemReceiptText("Magic Meter", entry,
                                            RC_CLOCK_TOWER_ROOF_OCARINA));
  assert(entry.msg == firstReceipt &&
         donorReads ==
             readsBeforeRepeat); // fixed body, including stat counters
  ++generation;
  assert(Rando::ApplyForeignItemReceiptText("Magic Meter", entry,
                                            RC_CLOCK_TOWER_ROOF_OCARINA));
  assert(entry.msg != firstReceipt);
  const int readsBeforeSlot = donorReads;
  ++gSaveContext.fileNum;
  assert(Rando::ApplyForeignItemReceiptText("Magic Meter", entry,
                                            RC_CLOCK_TOWER_ROOF_OCARINA));
  assert(donorReads == readsBeforeSlot + 1);
  entry.msg = "brief";
  assert(!Rando::ApplyItemReceiptText(RI_RUPEE_GREEN, entry));
  assert(!Rando::ApplyItemReceiptText(RI_TRAP, entry));
  assert(!Rando::ApplyItemReceiptText(RI_MAX, entry));
  assert(entry.msg == "brief");
  std::cout << "real native/FC item catalogs, donor descriptions, binary "
               "length, safe aliases and cold fallback passed\n";
}
