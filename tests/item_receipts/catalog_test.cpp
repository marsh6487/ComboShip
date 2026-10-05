// Uses real MM item IDs, the production static/FC catalogs and receipt builder.
// The font/message table and donor service are the runtime boundaries.
#include "2s2h/FleetShipCombo/FleetComboItems.h"
#include "2s2h/FleetShipCombo/FleetComboItemsGlue.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include "2s2h/Rando/StaticData/StaticData.h"
#include "rando/CrossForeign.h"
#include "ComboExport.h"
#include "ComboItemReceiptText.h"
#include "ComboSongDrawMM.h"
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
TexturePtr gItemIcons[131]{};
void Message_StageCustomItemIcon(void*, s16) {}
static u8 stagedColor[3];
static s16 stagedWidth, stagedHeight;
static u8 stagedIA8;
void Message_StageCustomItemIconTint(void*, s16 width, s16 height, u8 ia8, u8 r, u8 g, u8 b) {
    stagedWidth = width;
    stagedHeight = height;
    stagedIA8 = ia8;
    stagedColor[0] = r;
    stagedColor[1] = g;
    stagedColor[2] = b;
}
u8 gItemSlots[77]{};
u32 gBitFlags[32] = {1, 2, 4};
u8 Nei_BulletBagLevel() { return std::min<int>(3, neiSave.ootUpgrades & 7); }
}

namespace Rando::StaticData {
const char* GetIconTexturePath(RandoItemId) { return nullptr; }
const std::string& GetCheckDisplayName(RandoCheckId id) {
  static const std::map<RandoCheckId, std::string> names = {
    {RC_WOODFALL_TEMPLE_BOSS_WARP, "Woodfall Temple Boss Warp"},
    {RC_SNOWHEAD_TEMPLE_BOSS_WARP, "Snowhead Temple Boss Warp"},
    {RC_GREAT_BAY_TEMPLE_BOSS_WARP, "Great Bay Temple Boss Warp"},
    {RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_WARP, "Stone Tower Temple Inverted Boss Warp"}
  };
  return names.at(id);
}
}

static std::string requested;
static bool donorReady = true;
static int donorReads = 0;
static uint64_t generation = 0;
static bool mapCompassInfo = false;
static RandoCheckId foreignRewardCheck = RC_UNKNOWN;
static ComboRando::ForeignItem foreignReward;
namespace Rando::MiscBehavior {
uint64_t ComboRandoGen() { return generation; }
const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId check) {
  return check == foreignRewardCheck ? &foreignReward : nullptr;
}
} // namespace Rando::MiscBehavior
#ifndef COMPASS_DONOR_INTEGRATION
extern "C" COMBO_EXPORT int32_t OOT_MapCompassInfoEnabled() { return mapCompassInfo; }
#endif
#include "receipt_map_pause.inc"

static std::string SmallKeyDonorReceipt(const std::string& name) {
  for (int id = RI_OOT_SMALL_KEY_BOTTOM_OF_THE_WELL; id <= RI_OOT_SMALL_KEY_WATER_TEMPLE; ++id) {
    const int fc = FcCombo_ItemForNative(id);
    if (name != gFcComboItems[fc].ootName)
      continue;
    // The donor boundary supplies a complete converted native tutorial.
    // Chest Game owns a different native body (0xF3 rather than 0x60).
    const std::string native = id == RI_OOT_SMALL_KEY_TREASURE_GAME
        ? "You got a Key!\x01It opens the next door in the Treasure Chest Game.\x02"
        : "You got a Small Key!\x01This key will open a locked door in this dungeon.\x02";
    std::string body;
    assert(ComboItemReceiptText::FromOotMessage(native, body));
    return body;
  }
  return {};
}

#ifndef COMPASS_DONOR_INTEGRATION
extern "C" COMBO_EXPORT int32_t OOT_GetItemReceiptText(const char *name,
                                                       char *buffer,
                                                       uint32_t capacity) {
  requested = name;
  if (!donorReady || requested == "Progressive Hookshot")
    return 0;
  ++donorReads;
  const std::string key = SmallKeyDonorReceipt(requested);
  const std::string body = !key.empty() ? key :
      requested == "Deku Leaf"
          ? ComboItemReceiptText::FromNeiMarkup(kDekuLeafMessage)
          : "Full description of " + requested + " #" +
                std::to_string(donorReads);
  assert(body.size() <= capacity);
  std::memcpy(buffer, body.data(), body.size());
  return body.size();
}
#endif

static std::string FlattenReceiptLines(const std::string& body) {
  std::string flat;
  for (char c : body) {
    if (c == '\x11')
      c = ' ';
    if (c != ' ' || flat.empty() || flat.back() != ' ')
      flat += c;
  }
  return flat;
}

#ifdef COMPASS_DONOR_INTEGRATION
extern "C" void FixtureConfigureForestCompassReceipt(int information);

static void CheckForestCompassReceiveRoutes() {
  gSaveContext.fileNum = 0;
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  const auto saveBefore = gSaveContext;
  const auto neiBefore = neiSave;
  // The second Off is another loaded seed, so an On receipt cannot survive in
  // the foreign-check cache after its seed generation changes.
  for (int information : {0, 1, 0}) {
    FixtureConfigureForestCompassReceipt(information);
    ++generation;
    CustomMessage::Entry native, foreign;
    native.icon = foreign.icon = 0xF5;
    assert(Rando::ApplyItemReceiptText(RI_OOT_COMPASS_FOREST_TEMPLE, native));
    assert(Rando::ApplyForeignItemReceiptText("Forest Temple Compass", foreign, RC_CLOCK_TOWER_ROOF_OCARINA));
    assert(!native.autoFormat && !foreign.autoFormat);
    assert(native.icon == 0xF5 && foreign.icon == 0xF5);
    assert(native.msg == foreign.msg);
    const auto text = FlattenReceiptLines(native.msg);
    assert(text.find("You got the Compass!") != std::string::npos);
    assert(text.find("Now you can see hidden things.") != std::string::npos);
    assert(text.find("Forest Temple Compass") != std::string::npos);
    assert(text.find("masterful") != std::string::npos);
    assert((text.find("Phantom Ganon") != std::string::npos) == bool(information));
    assert((text.find("The reward is") != std::string::npos) == bool(information));
    assert((text.find("Deku Leaf") != std::string::npos) == bool(information));
    const auto body = native.msg;
    Rando::AppendReceiptSource(native, " (Bank reward)");
    assert(native.msg.compare(0, body.size(), body) == 0);
    assert(native.msg.substr(body.size()) == "\x10 (Bank reward)\xBF");
    assert(!std::memcmp(&saveBefore, &gSaveContext, sizeof(saveBefore)));
    assert(!std::memcmp(&neiBefore, &neiSave, sizeof(neiBefore)));
  }
  std::cout << "Real OoT donor -> MM Forest Temple Compass native/foreign routes: saved Off/On, full tutorial/MQ/reward, cache reset and source append passed\n";
}
#endif

static void CheckConcreteSmallKeyReceipts() {
  const auto previousSave = gSaveContext;
  const auto previousNei = neiSave;
  gSaveContext.fileNum = 0;
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  donorReady = true;
  for (int enabled : {0, 1}) {
    mapCompassInfo = enabled;
    assert(Rando::MapCompassInfoEnabled() == bool(enabled));
    for (int id = RI_OOT_SMALL_KEY_BOTTOM_OF_THE_WELL; id <= RI_OOT_SMALL_KEY_WATER_TEMPLE; ++id) {
      const auto key = static_cast<RandoItemId>(id);
      const int fc = FcCombo_ItemForNative(key);
      assert(fc >= 0 && gFcComboItems[fc].chainLen > 1);
      const char* name = gFcComboItems[fc].ootName;
      const std::string expected = FlattenReceiptLines(SmallKeyDonorReceipt(name));
      for (int count : {0, int(gFcComboItems[fc].chainLen)}) {
        neiSave.comboObtainedFc[fc] = count;
        const auto saveBefore = gSaveContext;
        const auto neiBefore = neiSave;
        CustomMessage::Entry concrete;
        concrete.icon = 0xF5;
        concrete.msg = "generic name-only receipt";
        requested.clear();
        const bool accepted = Rando::ApplyItemReceiptText(key, concrete);
        if (!accepted)
          std::cerr << "Missing concrete MM key tutorial: " << name << '\n';
        assert(accepted && "count-chain key bypassed the concrete receipt donor");
        assert(requested == name && !concrete.autoFormat);
        assert(FlattenReceiptLines(concrete.msg) == expected);
        // Direct foreign-sentinel composition must agree with the local item.
        CustomMessage::Entry foreign;
        foreign.icon = 0xF5;
        assert(Rando::ApplyForeignItemReceiptText(name, foreign));
        assert(foreign.msg == concrete.msg && !foreign.autoFormat);
        assert(concrete.msg.find("The reward is") == std::string::npos);
        assert(concrete.msg.find("Its entrance is") == std::string::npos);
        const std::string body = concrete.msg;
        Rando::AppendReceiptSource(concrete, " (Bank reward)");
        assert(concrete.msg.compare(0, body.size(), body) == 0);
        assert(concrete.msg.substr(body.size()) == "\x10 (Bank reward)\xBF");
        assert(std::memcmp(&gSaveContext, &saveBefore, sizeof(saveBefore)) == 0);
        assert(std::memcmp(&neiSave, &neiBefore, sizeof(neiBefore)) == 0);
      }
    }
  }
  gSaveContext = previousSave;
  neiSave = previousNei;
  mapCompassInfo = false;
  std::cout << "All ten concrete OoT small keys preserve native tutorials in MM; foreign, count, append and info guards passed\n";
}

static void CheckMapCompassInformation() {
  const auto saved = gSaveContext;
  gSaveContext.fileNum = 0;
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
  mapCompassInfo = true;
  const RandoCheckId checks[] = { RC_WOODFALL_TEMPLE_BOSS_WARP, RC_SNOWHEAD_TEMPLE_BOSS_WARP,
      RC_GREAT_BAY_TEMPLE_BOSS_WARP, RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_WARP };
  const RandoItemId rewards[] = { RI_PROGRESSIVE_SWORD, RI_SONG_NOVA, RI_COMBO_FOREIGN, RI_PROGRESSIVE_MAGIC };
  const RandoItemId compasses[] = { RI_WOODFALL_COMPASS, RI_SNOWHEAD_COMPASS, RI_GREAT_BAY_COMPASS, RI_STONE_TOWER_COMPASS };
  const RandoItemId maps[] = { RI_WOODFALL_MAP, RI_SNOWHEAD_MAP, RI_GREAT_BAY_MAP, RI_STONE_TOWER_MAP };
  const char* bosses[] = { "Odolwa", "Goht", "Gyorg", "Twinmold" };
  const char* entrances[] = { "Woodfall", "Snowhead", "Zora Cape's turtle", "Stone Tower" };
  foreignRewardCheck = checks[2];
  foreignReward.itemGame = ComboRando::GAME_OOT;
  foreignReward.itemName = "Progressive Hookshot";
  foreignReward.fakeItemName = "Light Arrows";
  for (int d = 0; d < 4; ++d) {
    auto& check = gSaveContext.save.shipSaveInfo.rando.randoSaveChecks[checks[d]];
    check = {};
    check.randoItemId = rewards[d];
    gSaveContext.save.saveInfo.inventory.dungeonItems[d] = 0;
    assert(!PauseItemDesc_GetMapInfo(d, ITEM_COMPASS));
    assert(!PauseItemDesc_GetMapInfo(d, ITEM_DUNGEON_MAP));
    // Start With ownership: no obtain history, visited flags, or receipt latch.
    gSaveContext.save.saveInfo.inventory.dungeonItems[d] = (1 << DUNGEON_COMPASS) | (1 << DUNGEON_MAP);
    const std::string expected = d == 2 ? "Progressive Hookshot (OOT)" : Rando::StaticData::Items.at(rewards[d]).name;
    const auto before = gSaveContext;
    const int reads = donorReads;
    char name[128];
    assert(MM_GetDungeonRewardName(d, name, sizeof(name)) == static_cast<int>(expected.size()));
    assert(name == expected);
    const auto info = Rando::GetDungeonMapCompassInfo(d, true);
    assert(info.find(bosses[d]) != std::string::npos && info.find(expected) != std::string::npos);
    assert(info.find("Light Arrows") == std::string::npos);
    assert(PauseItemDesc_GetMapInfo(d, ITEM_COMPASS) == info);
    const auto map = Rando::GetDungeonMapCompassInfo(d, false);
    assert(map.find(entrances[d]) != std::string::npos && map.find("entrance") != std::string::npos);
    assert(PauseItemDesc_GetMapInfo(d, ITEM_DUNGEON_MAP) == map);
    CustomMessage::Entry receipt;
    assert(Rando::ApplyItemReceiptText(compasses[d], receipt));
    assert(receipt.msg.find(bosses[d]) != std::string::npos && !receipt.autoFormat);
    assert(Rando::ApplyItemReceiptText(maps[d], receipt));
    assert(receipt.msg.find("entrance") != std::string::npos);
    assert(donorReads == reads && !std::memcmp(&before, &gSaveContext, sizeof(before)));
  }
  // Raw placed names remain stable across live equipment tiers and acquired checks.
  gSaveContext.save.saveInfo.equips.equipment = 0xFFFF;
  gSaveContext.save.shipSaveInfo.rando.randoSaveChecks[checks[0]].obtained = true;
  assert(Rando::GetDungeonMapCompassInfo(0, true).find("Progressive Sword") != std::string::npos);
  char shortName[2] = {'x', 'x'};
  assert(MM_GetDungeonRewardName(0, shortName, sizeof(shortName)) == 0 && !shortName[0]);
  assert(MM_GetDungeonRewardName(-1, shortName, sizeof(shortName)) == 0);
  assert(MM_GetDungeonRewardName(4, shortName, sizeof(shortName)) == 0);
  assert(MM_GetDungeonRewardName(0, nullptr, 0) == 0);
  assert(!PauseItemDesc_GetMapInfo(4, ITEM_COMPASS));
  assert(!PauseItemDesc_GetMapInfo(0, ITEM_KEY_BOSS));
  foreignRewardCheck = RC_UNKNOWN;
  assert(Rando::GetDungeonMapCompassInfo(2, true).empty()); // Missing foreign data never names the sentinel.
  mapCompassInfo = false;
  for (int d = 0; d < 4; ++d) {
    assert(Rando::GetDungeonMapCompassInfo(d, true).empty());
    assert(Rando::GetDungeonMapCompassInfo(d, false).empty());
    assert(!PauseItemDesc_GetMapInfo(d, ITEM_COMPASS));
  }
  mapCompassInfo = true;
  gSaveContext.fileNum = 0xFF;
  assert(!Rando::MapCompassInfoEnabled() && MM_GetDungeonRewardName(0, shortName, sizeof(shortName)) == 0);
  nlohmann::json seed = {
    {"mm", {{"placements", {{"Woodfall Temple Boss Warp", "Deku Leaf"}}}}},
    {"foreign", nlohmann::json::array({{{"checkGame", "mm"}, {"checkName", "Woodfall Temple Boss Warp"},
                                      {"itemGame", "oot"}, {"itemName", "Deku Leaf"}}})}
  };
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());
  ++generation;
  char dormantName[128];
  assert(MM_GetDungeonRewardName(0, dormantName, sizeof(dormantName)) > 0);
  assert(!strcmp(dormantName, "Deku Leaf (OOT)"));
  seed["foreign"][0]["itemName"] = "Poké Ball";
  seed["foreign"][0]["shared"] = true;
  seed["foreign"][0]["fakeItemName"] = "Light Arrows";
  ComboRando::Combo_SetForeignJson(seed.dump().c_str());
  ++generation;
  assert(MM_GetDungeonRewardName(0, dormantName, sizeof(dormantName)) == std::strlen("Poké Ball"));
  assert(!strcmp(dormantName, "Poké Ball")); // UTF-8 bytes, actual name and shared ownership survive cache refresh.
  assert(MM_GetDungeonRewardName(1, dormantName, sizeof(dormantName)) == 0 && !dormantName[0]);
  ComboRando::Combo_SetForeignJson("{malformed");
  ++generation;
  assert(MM_GetDungeonRewardName(0, dormantName, sizeof(dormantName)) == 0 && !dormantName[0]);
  ComboRando::Combo_SetForeignJson(nullptr);
  ++generation;
  gSaveContext = saved;
  mapCompassInfo = false;
  std::cout << "MM map/compass information: four placed rewards, foreign owner names, native entrances, owned pause/Start With, gates and no save writes passed\n";
}

int main() {
  for (float &w : sNESFontWidths)
    w = 8;
  PlayState play{};
  gPlayState = &play;
#ifdef COMPASS_DONOR_INTEGRATION
  CheckForestCompassReceiveRoutes();
  return 0;
#else
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
  CheckConcreteSmallKeyReceipts();
  CheckMapCompassInformation();
  // GI_NONE song rows must still take the engine's 16x24 IA8 note branch.
  const std::pair<RandoItemId, int> songIcons[] = {
      { RI_SONG_SONATA, ITEM_SONG_SONATA }, { RI_SONG_LULLABY, ITEM_SONG_LULLABY },
      { RI_SONG_LULLABY_INTRO, ITEM_SONG_LULLABY }, { RI_SONG_NOVA, ITEM_SONG_NOVA },
      { RI_SONG_ELEGY, ITEM_SONG_ELEGY }, { RI_SONG_OATH, ITEM_SONG_OATH },
      { RI_SONG_SARIA, ITEM_SONG_SARIA }, { RI_SONG_TIME, ITEM_SONG_TIME },
      { RI_SONG_DOUBLE_TIME, ITEM_SONG_TIME }, { RI_SONG_INVERTED_TIME, ITEM_SONG_TIME },
      { RI_SONG_HEALING, ITEM_SONG_HEALING }, { RI_SONG_EPONA, ITEM_SONG_EPONA },
      { RI_SONG_SOARING, ITEM_SONG_SOARING }, { RI_SONG_STORMS, ITEM_SONG_STORMS },
      { RI_SONG_SUN, ITEM_SONG_SUN },
  };
  for (auto [song, icon] : songIcons) {
    auto byte = Rando::StaticData::GetIconForZMessage(song);
    (void)icon;
    assert(byte == 0xF5 && stagedWidth == 16 && stagedHeight == 24 && stagedIA8);
  }
  Rando::StaticData::GetIconForZMessage(RI_SONG_DOUBLE_TIME);
  assert(stagedColor[0] == 128 && stagedColor[1] == 216 && stagedColor[2] == 240);
  Rando::StaticData::GetIconForZMessage(RI_SONG_INVERTED_TIME);
  assert(stagedColor[0] == 74 && stagedColor[1] == 112 && stagedColor[2] == 202);
  Rando::StaticData::GetIconForZMessage(RI_SONG_LULLABY_INTRO);
  assert(stagedColor[0] == 255 && stagedColor[1] == 100 && stagedColor[2] == 100);
  const std::pair<RandoItemId, uint32_t> receiptColors[] = {
      {RI_SONG_DOUBLE_TIME, 0x80D8F0}, {RI_SONG_ELEGY, 0xFF6200},         {RI_SONG_EPONA, 0xD96E30},
      {RI_SONG_HEALING, 0xFF96E6},     {RI_SONG_INVERTED_TIME, 0x4A70CA}, {RI_SONG_LULLABY_INTRO, 0xFF6464},
      {RI_SONG_LULLABY, 0xFF1414},     {RI_SONG_NOVA, 0x1414FF},          {RI_SONG_OATH, 0x620062},
      {RI_SONG_SARIA, 0x6ACB62},       {RI_SONG_SOARING, 0xC8A0FF},       {RI_SONG_SONATA, 0x62FF62},
      {RI_SONG_STORMS, 0x929292},      {RI_SONG_SUN, 0xEDE73E},           {RI_SONG_TIME, 0x62B1D3}};
  for (auto [song, rgb] : receiptColors) {
      assert(Rando::StaticData::GetIconForZMessage(song) == 0xF5);
      assert((uint32_t(stagedColor[0]) << 16 | uint32_t(stagedColor[1]) << 8 | stagedColor[2]) == rgb);
  }
  const int healing = ITEM_SONG_HEALING - ITEM_SONG_SONATA;
  assert(D_801CFE04[healing] == 255 && D_801CFE1C[healing] == 150 && D_801CFE34[healing] == 230);
  const int time = ITEM_SONG_TIME - ITEM_SONG_SONATA;
  assert(D_801CFE04[time] == 98 && D_801CFE1C[time] == 177 && D_801CFE34[time] == 211);
  // Receipt is composed before GiveItem. The token's identity, never the
  // current scene, chooses the spider-house counter.
  play.sceneId = SCENE_CLOCKTOWER;
  gSaveContext.save.saveInfo.skullTokenCount = (7u << 16) | 19u;
  assert(Rando::ApplyItemReceiptText(RI_GS_TOKEN_SWAMP, entry));
  assert(entry.msg.find("Swamp") != std::string::npos &&
         entry.msg.find("8") != std::string::npos &&
         entry.msg.find("20") == std::string::npos);
  assert(Rando::ApplyItemReceiptText(RI_GS_TOKEN_OCEAN, entry));
  assert(entry.msg.find("Ocean") != std::string::npos &&
         entry.msg.find("20") != std::string::npos);
  neiSave.ootGsCount = 42;
  assert(Rando::ApplyItemReceiptText(RI_OOT_GS_TOKEN, entry));
  assert(entry.msg.find("43") != std::string::npos);
  assert(gSaveContext.save.saveInfo.skullTokenCount == ((7u << 16) | 19u));
  assert(neiSave.ootGsCount == 42); // descriptions cannot grant items
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
  // Missing donor access must not send songs to story/teaching text, or leave
  // a one-line identity receipt. Both warm and cold runs use the MM meanings.
  const std::pair<RandoItemId, const char *> descriptions[] = {
    {RI_SONG_SONATA, "slumber"}, {RI_SONG_LULLABY, "sleep"},
    {RI_SONG_LULLABY_INTRO, "opening bars"}, {RI_SONG_NOVA, "new life"},
    {RI_SONG_ELEGY, "hollow"}, {RI_SONG_OATH, "giants"},
    {RI_SONG_HEALING, "masks"}, {RI_SONG_SOARING, "statue"},
    {RI_SONG_TIME, "Termina"}, {RI_SONG_STORMS, "thunder"},
    {RI_SONG_SUN, "night"}, {RI_SONG_EPONA, "horse"},
    {RI_SONG_SARIA, "forest"}, {RI_SONG_DOUBLE_TIME, "dusk"},
    {RI_SONG_INVERTED_TIME, "slows"}
  };
  for (bool ready : {false, true}) {
    donorReady = ready;
    gSaveContext.options.language = LANGUAGE_ENG;
    for (auto [id, description] : descriptions) {
      entry = {};
      assert(Rando::ApplyItemReceiptText(id, entry) && "MM song has no descriptive receipt");
      assert(entry.msg.find(description) != std::string::npos && !entry.autoFormat);
    }
    gSaveContext.options.language = LANGUAGE_GER;
    assert(Rando::ApplyItemReceiptText(RI_SONG_HEALING, entry));
    assert(entry.msg.find("Masken") != std::string::npos);
    gSaveContext.options.language = LANGUAGE_FRE;
    assert(Rando::ApplyItemReceiptText(RI_SONG_HEALING, entry));
    assert(entry.msg.find("masques") != std::string::npos);
  }
  gSaveContext.options.language = LANGUAGE_ENG;
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
      {RI_SONG_SONATA, "Sonata of Awakening"},
      {RI_SONG_LULLABY, "Goron Lullaby"},
      {RI_SONG_LULLABY_INTRO, "Goron Lullaby Intro"},
      {RI_SONG_NOVA, "New Wave Bossa Nova"},
      {RI_SONG_ELEGY, "Elegy of Emptiness"},
      {RI_SONG_OATH, "Oath to Order"},
      {RI_SONG_HEALING, "Song of Healing"},
      {RI_SONG_SOARING, "Song of Soaring"},
      {RI_SONG_TIME, "Song of Time (MM)"},
      {RI_SONG_STORMS, "Song of Storms (MM)"},
      {RI_SONG_SUN, "Sun's Song (MM)"},
      {RI_SONG_EPONA, "Epona's Song (MM)"},
      {RI_SONG_SARIA, "Saria's Song (MM)"},
      {RI_SONG_DOUBLE_TIME, "Song of Double Time"},
      {RI_SONG_INVERTED_TIME, "Inverted Song of Time"},
      {RI_OOT_COMPASS_DEKU_TREE, "Great Deku Tree Compass"},
      {RI_OOT_MAP_DEKU_TREE, "Great Deku Tree Map"},
      {RI_SNOWHEAD_COMPASS, "Snowhead Compass"},
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
    assert((id >= RI_SONG_DOUBLE_TIME && id <= RI_SONG_TIME) || requested == name &&
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
#endif
}
