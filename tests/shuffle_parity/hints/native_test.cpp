#include "Rando/Logic/Logic.h"
#include "Rando/MagicHints.h"
#include "Rando/MiscBehavior/ClockShuffle.h"
#include "mods/combo_rpg.h"
#include "mods/extended_inventory.h"
#include "mods/nei_save.h"
#include "rando_capacity.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>

#define RI(id, article, name, type, itemId, getItemId, drawId)                 \
  {                                                                            \
    id, { id, #id, article, name, type, itemId, getItemId, drawId }            \
  }
namespace Rando::StaticData {
#include "items.inc"
std::map<RandoCheckId, RandoStaticCheck> Checks;
} // namespace Rando::StaticData
#undef RI

SaveContext gSaveContext;
static std::map<std::string, int32_t> configuredUpgrades;
extern "C" int32_t CVarGetInteger(const char *name, int32_t fallback) {
  auto found = configuredUpgrades.find(name);
  return found == configuredUpgrades.end() ? fallback : found->second;
}
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
static ComboRando::ForeignItem foreign;
const ComboRando::ForeignItem *
Rando::MiscBehavior::MM_LookupForeign(RandoCheckId) {
  return &foreign;
}
#endif
static nlohmann::json crossCandidates = nlohmann::json::array();
static bool crossReachable = true;
#ifdef COMBO_BUILD
static const char *QueryCrossCandidates(int, int reachableOnly) {
  static std::string json;
  json = (reachableOnly && !crossReachable ? nlohmann::json::array()
                                           : crossCandidates)
             .dump();
  return json.c_str();
}
static void *Combo_ResolveSym(const char *, const char *) {
  return reinterpret_cast<void *>(&QueryCrossCandidates);
}
#endif
// Full production ConvertItem executes against native inventory. Other engine
// chains and obtainability are declared boundaries of this hint fixture.
extern "C" NeiSaveData *Nei_Save() {
  return &gSaveContext.save.shipSaveInfo.nei;
}
#include "native_magic.inc"
extern "C" uint16_t Nei_GetOwnedItem(uint8_t) { return ITEM_NONE; }
extern "C" uint8_t Nei_StrengthLevel() { return 0; }
extern "C" s32 Inventory_HasEmptyBottle() { return false; }
extern "C" s32 Flags_GetRandoInf(s32) { return false; }
u16 gEquipMasks[4] = {};
u8 gEquipShifts[4] = {};
u32 gBitFlags[32] = {};
u32 gUpgradeMasks[8] = {};
u8 gUpgradeShifts[8] = {};
namespace Rando::ClockItems {
int GetHalfDayIndexFromClockItem(RandoItemId) { return -1; }
} // namespace Rando::ClockItems
static RandoItemId ResolveCanePresentation() {
  return RI_OOT_NEI_CANE_OF_SOMARIA;
}
bool Rando::IsItemObtainable(RandoItemId item, RandoCheckId) {
  const auto &player = gSaveContext.save.saveInfo.playerData;
  int level = ComboRpg_NativeMagicTier(player.isMagicAcquired +
                                       player.isDoubleMagicAcquired);
  if (item == RI_PROGRESSIVE_MAGIC || item == RI_DOUBLE_MAGIC)
    return level < 2;
  if (item == RI_SINGLE_MAGIC)
    return level == 0;
  if (Rando::IsSoulHintItem(item))
    return Rando::SoulHintUseful(item);
  return true;
}
static void SetTier(RandoItemId next) {
  if (next != RI_SINGLE_MAGIC && next != RI_DOUBLE_MAGIC && next != RI_JUNK)
    return;
  auto &player = gSaveContext.save.saveInfo.playerData;
  player.isMagicAcquired = next != RI_SINGLE_MAGIC;
  player.isDoubleMagicAcquired = next == RI_JUNK;
  Nei_Save()->comboRpg.nativeMagicLevel =
      player.isMagicAcquired + player.isDoubleMagicAcquired;
}
#include "convert.inc"
bool Ship_IsCStringEmpty(const char *text) { return !text || !*text; }
RandoItemId Rando::CurrentJunkItem(RandoCheckId) { return RI_RUPEE_GREEN; }
RandoItemId Rando::CurrentTrapItem(RandoCheckId) { return RI_RUPEE_GREEN; }
#ifdef COMBO_BUILD
const char *Rando::ComboForeignLiveName(RandoCheckId) { return nullptr; }
#endif
namespace Rando::StaticData {
static std::map<RandoItemId, std::vector<std::string>> fakeItemNames;
}
// Native location rendering is a scene/message boundary; cross-world names
// come directly from the provider's actual seed check identity.
std::string Rando::GetHintLocationText(RandoItemId, RandoCheckId check, bool) {
  assert(check == RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST);
  return "at Astral Observatory Passage Chest";
}
static std::string sSensorHintText;
#include "placement.inc"
#include "saria.inc"
#include "sensor_menu.inc"

static bool reachable = true;
namespace Rando::Logic {
std::map<RandoRegionId, RandoRegion> Regions;
std::unordered_map<RandoRegionId, RegionTimeState>
InitializeRegionTimeStates(RandoRegionId) {
  return {};
}
void FindReachableRegions(
    RandoRegionId, std::set<RandoRegionId> &regions,
    std::unordered_map<RandoRegionId, RegionTimeState> &) {
  if (reachable)
    regions.insert(static_cast<RandoRegionId>(0));
}
RandoRegionId GetRegionIdFromEntrance(s32) {
  return static_cast<RandoRegionId>(0);
}
} // namespace Rando::Logic
extern "C" s32 Ship_Random(s32 first, s32) { return first; }
#include "selection.inc"

static RandoItemId Named(const char *name) {
  for (const auto &[id, item] : Rando::StaticData::Items)
    if (std::strcmp(item.name, name) == 0)
      return id;
  return RI_UNKNOWN;
}

int main() {
  using namespace Rando;
#include "../legacy_ids.inc"
  const char *names[] = {"Progressive Magic",   "Magic Stat Upgrade",
                         "Defense Upgrade",     "Speed Upgrade",
                         "Power Upgrade",       "Crawl Speed Upgrade",
                         "Climb Speed Upgrade", "Push Speed Upgrade"};
  const auto saria = GetSariaPriorityItemCandidates();
  const auto sensor = SensorItemChoices();
  assert(std::find(saria.begin(), saria.end(), RI_SOUL_ENEMY_OCTOROK) !=
         saria.end());
  std::vector<RandoItemId> wishes;
  for (const char *name : names) {
    auto id = Named(name);
    if (id == RI_UNKNOWN) {
      std::fprintf(stderr, "FAIL missing real MM hint catalogue entry: %s\n",
                   name);
      return 1;
    }
    assert(std::find(saria.begin(), saria.end(), id) != saria.end());
    assert(std::find(sensor.begin(), sensor.end(),
                     std::pair<int32_t, std::string>{id, name}) !=
           sensor.end());
    assert(SensorDesireName(id) == name);
    wishes.push_back(id);
  }

  // A progressive-family wish must remain selectable when the configured pool
  // contains concrete native tiers rather than the progressive parent.
  assert(MagicHintAvailable(RI_PROGRESSIVE_MAGIC, {RI_SINGLE_MAGIC}));
  assert(MagicHintAvailable(RI_PROGRESSIVE_MAGIC, {RI_DOUBLE_MAGIC}));
  assert(!MagicHintAvailable(RI_DOUBLE_MAGIC, {RI_SINGLE_MAGIC}));

  nlohmann::json spoiler;
  SetSariaPriorityItemsInSpoiler(spoiler, wishes);
  assert(GetSariaPriorityItemsFromSpoiler(spoiler) == wishes);
  auto save = std::make_unique<RandoSaveInfo>();
  SetSariaPriorityItemsInSave(*save, wishes);
  assert(GetSariaPriorityItemsFromSave(*save) == wishes);
  wishes.clear();
  SetSariaPriorityItemsInSave(*save, wishes);
  assert(GetSariaPriorityItemsFromSave(*save).empty());

  // RPG magic capacity must not impersonate already-owned native meter tiers.
  auto &nativeMagic = Nei_Save()->comboRpg;
  nativeMagic.knownMask = nativeMagic.enabledMask = 1u << COMBO_RPG_MAGIC;
  gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
  gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = true;
  nativeMagic.nativeMagicLevel = 0;
  assert(ConvertItem(RI_PROGRESSIVE_MAGIC) == RI_SINGLE_MAGIC);
  nativeMagic.nativeMagicLevel = 1;
  assert(ConvertItem(RI_PROGRESSIVE_MAGIC) == RI_DOUBLE_MAGIC);
  nativeMagic.nativeMagicLevel = 2;
  assert(ConvertItem(RI_PROGRESSIVE_MAGIC) == RI_JUNK);
  nativeMagic.knownMask = nativeMagic.enabledMask = 0;
  SetTier(RI_SINGLE_MAGIC);

  const auto check = RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST;
  StaticData::Checks[check] = {};
  RANDO_SAVE_CHECKS[check].shuffled = true;
  Logic::Regions[static_cast<RandoRegionId>(0)].checks[check] = {
      [] { return true; }, "fixture access"};
  // Native Enemy/NPC/Animal/Misc identities use their actual flags/options.
  const RandoItemId soulItems[] = {RI_SOUL_ENEMY_OCTOROK, RI_SOUL_NPC_AROMA,
                                   RI_SOUL_ANIMAL_DOG, RI_SOUL_MISC_GS};
  for (auto soul : soulItems) {
    const auto definition = SoulShuffle::Find(soul);
    const int flag = definition ? definition->flag
                                : soul - RI_SOUL_BOSS_GOHT +
                                      RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT;
    const auto option =
        definition ? definition->option : RO_SHUFFLE_ENEMY_SOULS;
    RANDO_SAVE_OPTIONS[option] = 1;
    RANDO_SAVE_CHECKS[check].randoItemId = soul;
    std::vector<RandoItemId> soulWish{soul};
    SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando, soulWish);
    auto nativeSoul = GetSariaHintTarget();
    assert(nativeSoul.check == RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST &&
           nativeSoul.item == soul);
    configuredUpgrades[std::string(CVAR_SENSOR_DESIRE_PREFIX) + "0"] = soul;
    assert(SensorRune_PrepareHint());
    assert(sSensorHintText.find(StaticData::Items[soul].name) !=
           std::string::npos);
    assert(sSensorHintText.find("Astral Observatory Passage Chest") !=
           std::string::npos);
    auto &flags = gSaveContext.save.shipSaveInfo.rando.randoInf;
    flags[flag >> 4] |= 1u << (flag & 0xf);
    assert(!GetSariaHintTarget().IsValid());
    assert(GetDesiredItemHintTarget(soul).IsValid()); // explicit raw-copy wish
    flags[flag >> 4] &= ~(1u << (flag & 0xf));
    RANDO_SAVE_OPTIONS[option] = 0;
    assert(!GetSariaHintTarget().IsValid());
    RANDO_SAVE_OPTIONS[option] = 1;
  }
  configuredUpgrades.clear();
  std::vector<RandoItemId> magicWish{RI_PROGRESSIVE_MAGIC};
  SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando, magicWish);
  RANDO_SAVE_CHECKS[check].randoItemId = RI_PROGRESSIVE_MAGIC;
  assert(FindMultiItemPlacement(RI_PROGRESSIVE_MAGIC) ==
         std::vector<RandoCheckId>{check});
  assert(FindMultiItemPlacement(RI_SINGLE_MAGIC) ==
         std::vector<RandoCheckId>{check});
  assert(GetProgressiveCheckInLogic() == check);
  assert(SensorFindOutstandingCheck(RI_PROGRESSIVE_MAGIC) == check);
  // A removed/excluded placement is not a valid actionable hint.
  RANDO_SAVE_CHECKS[check].skipped = true;
  assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
  assert(SensorFindOutstandingCheck(RI_PROGRESSIVE_MAGIC) == RC_UNKNOWN);
  RANDO_SAVE_CHECKS[check].skipped = false;
  RANDO_SAVE_CHECKS[check].cycleObtained = true;
  assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
  RANDO_SAVE_CHECKS[check].cycleObtained = false;
  SetTier(RI_DOUBLE_MAGIC);
  assert(FindMultiItemPlacement(RI_DOUBLE_MAGIC) ==
         std::vector<RandoCheckId>{check});
  configuredUpgrades[std::string(CVAR_SENSOR_DESIRE_PREFIX) + "0"] =
      RI_PROGRESSIVE_MAGIC;
  assert(SensorRune_PrepareHint() == 1);
  assert(sSensorHintText == "The slate senses %ga Magic Upgrade%w %yat Astral "
                            "Observatory Passage Chest%w...");
  configuredUpgrades.clear();
  SetTier(RI_JUNK);
  assert(FindMultiItemPlacement(RI_DOUBLE_MAGIC).empty());
  assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);

#ifdef COMBO_BUILD
  RANDO_SAVE_CHECKS[check].randoItemId = RI_COMBO_FOREIGN;
  foreign.itemGame = ComboRando::GAME_OOT;
  const int statIds[] = {-1,
                         COMBO_RPG_MAGIC,
                         COMBO_RPG_DEFENSE,
                         COMBO_RPG_SPEED,
                         COMBO_RPG_POWER,
                         COMBO_RPG_CRAWL,
                         COMBO_RPG_CLIMB,
                         COMBO_RPG_PUSH};
  const char *settingNames[] = {"",
                                "gRandoSettings.MagicStatUpgrade",
                                "gRandoSettings.DefenseUpgrade",
                                "gRandoSettings.SpeedUpgrade",
                                "gRandoSettings.PowerUpgrade",
                                "gRandoSettings.CrawlSpeedUpgrade",
                                "gRandoSettings.ClimbSpeedUpgrade",
                                "gRandoSettings.PushSpeedUpgrade"};
  auto &rpg = gSaveContext.save.shipSaveInfo.nei.comboRpg;
  for (size_t caseIndex = 1; caseIndex < ARRAY_COUNT(names); ++caseIndex) {
    const char *name = names[caseIndex];
    auto id = Named(name);
    foreign.itemName = name;
    foreign.trap = false;
    assert(MagicHintIdentity(RI_COMBO_FOREIGN, check) == id);
    assert(FindMultiItemPlacement(id) == std::vector<RandoCheckId>{check});
    configuredUpgrades.clear();
    assert(!MagicHintAvailable(id, {}));
    configuredUpgrades[settingNames[caseIndex]] = 1;
    assert(MagicHintAvailable(id, {}));
    std::vector<RandoItemId> rpgWish{id};
    SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando, rpgWish);
    SetTier(RI_COMBO_FOREIGN);
    assert(GetProgressiveCheckInLogic() == check);
    const auto nativeTarget = GetSariaHintTarget();
    assert(nativeTarget.check == RC_ASTRAL_OBSERVATORY_PASSAGE_CHEST &&
           nativeTarget.item == id);
    assert(SensorFindOutstandingCheck(id) == check);
    const int stat = statIds[caseIndex];
    rpg.knownMask = rpg.enabledMask = static_cast<uint8_t>(1u << stat);
    rpg.required[stat] = 5;
    rpg.level[stat] = 4;
    assert(GetProgressiveCheckInLogic() == check);
    rpg.level[stat] = 5;
    assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
    const auto fallback = RC_BENEATH_THE_GRAVEYARD_CHEST;
    StaticData::Checks[fallback] = {};
    RANDO_SAVE_CHECKS[fallback].shuffled = true;
    RANDO_SAVE_CHECKS[fallback].randoItemId = RI_HOOKSHOT;
    Logic::Regions[static_cast<RandoRegionId>(0)].checks[fallback] = {
        [] { return true; }, "fixture access"};
    assert(GetProgressiveCheckInLogic() == fallback);
    // Plentiful copies beyond a configured cap must also yield to a useful
    // item.
    rpg.level[stat] = 7;
    assert(GetProgressiveCheckInLogic() == fallback);
    rpg.required[stat] = 9;
    assert(GetProgressiveCheckInLogic() == check);
    rpg.required[stat] = 5;
    // Desired Items retains OoT's explicit wish for uncollected raw copies.
    assert(SensorFindOutstandingCheck(id) == check);
    rpg.level[stat] = 0;
    rpg.enabledMask = 0;
    assert(GetProgressiveCheckInLogic() == fallback);
    RANDO_SAVE_CHECKS[fallback].shuffled = false;
    assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
    // Older paired saves without mirrored RPG rules keep prior guidance.
    rpg.knownMask = 0;
    assert(GetProgressiveCheckInLogic() == check);
    RANDO_SAVE_CHECKS[check].obtained = true;
    assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
    assert(SensorFindOutstandingCheck(id) == RC_UNKNOWN);
    RANDO_SAVE_CHECKS[check].obtained = false;
    reachable = false;
    assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
    reachable = true;
    foreign.trap = true;
    assert(FindMultiItemPlacement(id).empty());
    SetTier(RI_JUNK);
    assert(GetProgressiveCheckInLogic() == RC_UNKNOWN);
    foreign.trap = false;
    foreign.itemGame = ComboRando::GAME_MM;
    assert(FindMultiItemPlacement(id).empty());
    foreign.itemGame = ComboRando::GAME_OOT;
  }
  const char *sharedOotNames[] = {"Octorok Enemy Soul", "Dampe NPC Soul",
                                  "Dog Animal Soul",
                                  "Gold Skulltula Misc Soul"};
  const RandoItemId sharedMmIds[] = {RI_SOUL_ENEMY_OCTOROK, RI_SOUL_NPC_DAMPE,
                                     RI_SOUL_ANIMAL_DOG, RI_SOUL_MISC_GS};
  for (size_t i = 0; i < std::size(sharedMmIds); ++i) {
    const auto soul = sharedMmIds[i];
    const auto definition = SoulShuffle::Find(soul);
    const int flag = definition ? definition->flag
                                : soul - RI_SOUL_BOSS_GOHT +
                                      RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT;
    const auto option =
        definition ? definition->option : RO_SHUFFLE_ENEMY_SOULS;
    RANDO_SAVE_OPTIONS[option] = 1;
    foreign.itemGame = ComboRando::GAME_OOT;
    foreign.itemName = sharedOotNames[i];
    foreign.shared = true;
    std::vector<RandoItemId> soulWish{soul};
    SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando, soulWish);
    assert(MagicHintIdentity(RI_COMBO_FOREIGN, check) == soul);
    auto target = GetSariaHintTarget();
    assert(target.check == check && target.item == soul);
    auto &flags = gSaveContext.save.shipSaveInfo.rando.randoInf;
    flags[flag >> 4] |= 1u << (flag & 0xf);
    assert(!GetSariaHintTarget().IsValid());
    assert(GetDesiredItemHintTarget(soul).IsValid());
    flags[flag >> 4] &= ~(1u << (flag & 0xf));
    RANDO_SAVE_OPTIONS[option] = 0;
    assert(!GetSariaHintTarget().IsValid());
    RANDO_SAVE_OPTIONS[option] = 1;
    foreign.shared = false;
    assert(MagicHintIdentity(RI_COMBO_FOREIGN, check) == RI_COMBO_FOREIGN);
    assert(!GetSariaHintTarget().IsValid());
  }
  // A genuine shared OoT magic meter at an MM check grants the native MM tier.
  foreign.itemGame = ComboRando::GAME_OOT;
  foreign.itemName = "Progressive Magic Meter";
  foreign.shared = true;
  rpg.knownMask = rpg.enabledMask = 0;
  std::vector<RandoItemId> sharedMagicWish{RI_PROGRESSIVE_MAGIC};
  SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando,
                              sharedMagicWish);
  SetTier(RI_DOUBLE_MAGIC);
  assert(MagicHintIdentity(RI_COMBO_FOREIGN, check) == RI_PROGRESSIVE_MAGIC);
  assert(FindMultiItemPlacement(RI_DOUBLE_MAGIC) ==
         std::vector<RandoCheckId>{check});
  auto sharedTarget = GetSariaHintTarget();
  assert(sharedTarget.check == check && sharedTarget.item == RI_DOUBLE_MAGIC);
  assert(GetDesiredItemHintTarget(RI_PROGRESSIVE_MAGIC).item ==
         RI_DOUBLE_MAGIC);
  SetTier(RI_JUNK);
  assert(!GetSariaHintTarget().IsValid());
  assert(GetDesiredItemHintTarget(RI_PROGRESSIVE_MAGIC).item ==
         RI_PROGRESSIVE_MAGIC);
  foreign.shared = false;
  SetTier(RI_DOUBLE_MAGIC);
  assert(MagicHintIdentity(RI_COMBO_FOREIGN, check) == RI_COMBO_FOREIGN);
  assert(!GetSariaHintTarget().IsValid());

  // Real source-game/check identities are retained, not synthetic MM check IDs.
  RANDO_SAVE_CHECKS[check].shuffled = false;
  std::vector<RandoItemId> crossWish{RI_PROGRESSIVE_MAGIC};
  SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando, crossWish);
  crossCandidates = {{{"itemName", "Progressive Magic"},
                      {"itemGame", "mm"},
                      {"checkName", "Kokiri Sword Chest"}}};
  SetTier(RI_DOUBLE_MAGIC);
  auto target = GetSariaHintTarget();
  assert(target.IsValid() && target.check == RC_UNKNOWN &&
         target.item == RI_DOUBLE_MAGIC);
  assert(target.locationText == "at Kokiri Sword Chest (OOT)");
  configuredUpgrades[std::string(CVAR_SENSOR_DESIRE_PREFIX) + "0"] =
      RI_PROGRESSIVE_MAGIC;
  assert(SensorRune_PrepareHint() == 1);
  assert(sSensorHintText == "The slate senses %ga Magic Upgrade%w %yat Kokiri "
                            "Sword Chest (OOT)%w...");

  target = GetDesiredItemHintTarget(RI_DOUBLE_MAGIC);
  assert(target.IsValid() && target.item == RI_DOUBLE_MAGIC);
  assert(target.locationText == "at Kokiri Sword Chest (OOT)");
  crossReachable = false;
  assert(!GetSariaHintTarget().IsValid());
  // The explicit Desired Items wish may name an inaccessible outstanding check.
  assert(GetDesiredItemHintTarget(RI_PROGRESSIVE_MAGIC).locationText ==
         "at Kokiri Sword Chest (OOT)");
  crossReachable = true;
  SetTier(RI_JUNK);
  assert(!GetSariaHintTarget().IsValid());
  for (const auto soul : sharedMmIds) {
    const auto definition = SoulShuffle::Find(soul);
    const int flag = definition ? definition->flag
                                : soul - RI_SOUL_BOSS_GOHT +
                                      RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT;
    const auto option =
        definition ? definition->option : RO_SHUFFLE_ENEMY_SOULS;
    RANDO_SAVE_OPTIONS[option] = 1;
    crossCandidates = {{{"itemName", StaticData::Items[soul].name},
                        {"itemGame", "mm"},
                        {"checkName", "KF Kokiri Sword Chest"}}};
    std::vector<RandoItemId> soulWish{soul};
    SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando, soulWish);
    target = GetSariaHintTarget();
    assert(target.item == soul &&
           target.locationText == "at KF Kokiri Sword Chest (OOT)");
    auto &flags = gSaveContext.save.shipSaveInfo.rando.randoInf;
    flags[flag >> 4] |= 1u << (flag & 0xf);
    assert(!GetSariaHintTarget().IsValid());
    assert(GetDesiredItemHintTarget(soul).locationText ==
           "at KF Kokiri Sword Chest (OOT)");
    flags[flag >> 4] &= ~(1u << (flag & 0xf));
    RANDO_SAVE_OPTIONS[option] = 0;
    assert(!GetSariaHintTarget().IsValid());
    RANDO_SAVE_OPTIONS[option] = 1;
  }
  for (size_t i = 1; i < ARRAY_COUNT(names); ++i) {
    auto id = Named(names[i]);
    crossCandidates = {{{"itemName", names[i]},
                        {"itemGame", "oot"},
                        {"checkName", "Hyrule Field Open Grotto Chest"}}};
    crossWish = {id};
    SetSariaPriorityItemsInSave(gSaveContext.save.shipSaveInfo.rando,
                                crossWish);
    rpg.knownMask = rpg.enabledMask = 1u << statIds[i];
    rpg.required[statIds[i]] = 3;
    rpg.level[statIds[i]] = 2;
    target = GetSariaHintTarget();
    assert(target.item == id &&
           target.locationText == "at Hyrule Field Open Grotto Chest (OOT)");
    assert(GetDesiredItemHintTarget(id).locationText == target.locationText);
    rpg.level[statIds[i]] = 3;
    assert(!GetSariaHintTarget().IsValid());
    assert(GetDesiredItemHintTarget(id).IsValid());
    rpg.enabledMask = 0;
    assert(!GetSariaHintTarget().IsValid());
  }
  gComboGameMode = COMBO_GAME_MM;
  for (const char *name : names)
    if (std::strcmp(name, "Progressive Magic") != 0)
      assert(!MagicHintAvailable(Named(name), {}));
#else
  for (const char *name : names)
    if (std::strcmp(name, "Progressive Magic") != 0)
      assert(!MagicHintAvailable(Named(name), {}));
#endif
  std::puts("PASS actual item catalogue, both menu lists, progressive tiers, "
            "saved/spoiler priorities and foreign hint identities");
}
