// Engine construction and graph traversal are controlled boundaries. The
// provider, snapshot/restore routine, native storage types and foreign seed
// loader are real.
#include "ComboGameMode.h"
#include "MMHintBridge.h"
#include "rando/CrossForeign.h"
#include "soh/Enhancements/randomizer/SoulShuffle.h"
#include "soh/Enhancements/randomizer/entrance.h"
#include "soh/Enhancements/randomizer/item.h"
#include "soh/Enhancements/randomizer/location.h"
#include "soh/Enhancements/randomizer/location_access.h"
#include <cassert>
#include <functional>
#include <stdexcept>

Text::Text() = default;
Text::Text(std::string value) : english(std::move(value)) {}
const std::string &Text::GetEnglish() const { return english; }
SaveContext gSaveContext;
int gComboGameMode = COMBO_GAME_BOTH;
static std::shared_ptr<Rando::Context> fixtureContext;
std::shared_ptr<Rando::Logic> logic;
static std::array<Region, RR_MAX> regions;
static std::array<Rando::Location, RC_MAX> definitions;
static std::array<Rando::Item, RG_MAX> items;
static std::map<const Rando::Location *, std::string> names;
class OTRGlobals {
public:
  static OTRGlobals *Instance;
  std::shared_ptr<Rando::Context> gRandoContext;
};
OTRGlobals fixtureGlobals;
OTRGlobals *OTRGlobals::Instance = &fixtureGlobals;
namespace Rando {
Context::Context() { mLogic = std::make_shared<Logic>(); }
std::shared_ptr<Context> Context::GetInstance() { return fixtureContext; }
ItemLocation *Context::GetItemLocation(RandomizerCheck check) {
  return &itemLocationTable[check];
}
std::shared_ptr<Logic> Context::GetLogic() { return mLogic; }
OptionValue &Context::GetOption(RandomizerSettingKey key) {
  return mOptions[key];
}
Hint::Hint() {}
Logic::Logic() { std::fill(std::begin(inLogic), std::end(inLogic), false); }
Item::Item() : price(0) {}
Item::Item(RandomizerGet id, Text title, ItemType, int16_t, bool, LogicVal,
           RandomizerHintTextKey, GetItemCategory, Text, std::string, uint16_t)
    : name(std::move(title)), randomizerGet(id), price(0) {}
Region *unused;
Location *StaticData::GetLocation(RandomizerCheck check) {
  return &definitions[check];
}
Item &StaticData::RetrieveItem(RandomizerGet item) { return items[item]; }
const std::string &Location::GetName() const {
  static const std::string empty;
  const auto found = names.find(this);
  return found == names.end() ? empty : found->second;
}
// Status hook dispatch is an engine boundary; storage remains the native field.
void ItemLocation::SetCheckStatus(RandomizerCheckStatus value) {
  status = value;
}
} // namespace Rando
Region::Region() {}
Region::~Region() {}
Region *RegionTable(RandomizerRegion key) { return &regions[key]; }
static bool failTraversal;
static bool targetReachable = true;
static std::vector<RandomizerCheck>
ReachabilitySearch(const std::vector<RandomizerCheck> &checks, RandomizerGet,
                   bool liveInventory) {
  assert(liveInventory);
  assert(logic->mSaveContext != &gSaveContext);
  assert(logic->mSaveContext->health == gSaveContext.health);
  assert(fixtureContext->GetOption(RSK_NO_LOGIC).Get() == RO_GENERIC_OFF);
  logic->mSaveContext->health = 1;
  logic->CalculatingAvailableChecks = true;
  regions[RR_ROOT].adultDay = !regions[RR_ROOT].adultDay;
  fixtureContext->GetItemLocation(checks.front())->AddToPool();
  if (failTraversal)
    throw std::runtime_error("fixture traversal failure");
  return targetReachable ? checks : std::vector<RandomizerCheck>{};
}
#include "provider.inc"
#include "provider_native.inc"
int main() {
  fixtureContext = std::make_shared<Rando::Context>();
  fixtureGlobals.gRandoContext = fixtureContext;
  logic = fixtureContext->GetLogic();
  logic->mSaveContext = &gSaveContext;
  gSaveContext.health = 80;
  const auto check = RC_KF_KOKIRI_SWORD_CHEST;
  names[&definitions[check]] = "Kokiri Sword Chest";
  fixtureContext->allLocations = {check};
  auto location = fixtureContext->GetItemLocation(check);
  location->SetPlacedItem(RG_COMBO_FOREIGN);
  ComboRando::Combo_SetForeignJson(
      R"({"foreign":[{"checkGame":"oot","checkName":"Kokiri Sword Chest","itemGame":"mm","itemName":"Progressive Magic"}]})");
  auto candidates = nlohmann::json::parse(OOT_GetMMHintCandidates(0, 0));
  assert(candidates.size() == 1 &&
         candidates[0]["checkName"] == "Kokiri Sword Chest");
  assert(candidates[0]["itemName"] == "Progressive Magic" &&
         candidates[0]["itemGame"] == "mm");
  fixtureContext->GetOption(RSK_NO_LOGIC).Set(RO_GENERIC_ON);
  regions[RR_ROOT].regionName = "Root";
  regions[RR_ROOT].adultDay = true;
  candidates = nlohmann::json::parse(OOT_GetMMHintCandidates(0, 1));
  assert(candidates.size() == 1);
  assert(gSaveContext.health == 80 && logic->mSaveContext == &gSaveContext);
  assert(!logic->CalculatingAvailableChecks && regions[RR_ROOT].adultDay);
  assert(!location->IsAddedToPool());
  assert(fixtureContext->GetOption(RSK_NO_LOGIC).Get() == RO_GENERIC_ON);
  targetReachable = false;
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 1)).empty());
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 0)).size() == 1);
  targetReachable = true;
  failTraversal = true;
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 1)).empty());
  assert(gSaveContext.health == 80 && logic->mSaveContext == &gSaveContext);
  assert(!logic->CalculatingAvailableChecks && regions[RR_ROOT].adultDay);
  assert(!location->IsAddedToPool());
  assert(fixtureContext->GetOption(RSK_NO_LOGIC).Get() == RO_GENERIC_ON);
  failTraversal = false;
  location->SetCheckStatus(RCSHOW_COLLECTED);
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 0)).empty());
  location->SetCheckStatus(RCSHOW_SAVED);
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 0)).empty());
  location->SetCheckStatus(RCSHOW_UNCHECKED);
  location->SetExcludedOption(RO_LOCATION_EXCLUDE);
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 0)).empty());
  location->SetExcludedOption(RO_LOCATION_INCLUDE);
  ComboRando::Combo_SetForeignJson(
      R"({"foreign":[{"checkGame":"oot","checkName":"Kokiri Sword Chest","itemGame":"mm","itemName":"Progressive Magic","trap":true}]})");
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 0)).empty());
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(1, 0)).empty());
  location->SetPlacedItem(RG_PROGRESSIVE_MAGIC_METER);
  ComboRando::Combo_SetForeignJson(R"({"sharedItems":["magic"]})");
  candidates = nlohmann::json::parse(OOT_GetMMHintCandidates(0, 1));
  assert(candidates.size() == 1 && candidates[0]["itemGame"] == "mm");
  assert(candidates[0]["itemName"] == "Progressive Magic" &&
         candidates[0]["checkName"] == "Kokiri Sword Chest");
  ComboRando::Combo_SetForeignJson(R"({"sharedItems":[]})");
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 0)).empty());
  const RandomizerGet upgrades[] = {
      RG_MAGIC_STAT_UPGRADE, RG_DEFENSE_UPGRADE,     RG_SPEED_UPGRADE,
      RG_POWER_UPGRADE,      RG_CRAWL_SPEED_UPGRADE, RG_CLIMB_SPEED_UPGRADE,
      RG_PUSH_SPEED_UPGRADE};
  const char *upgradeNames[] = {"Magic Stat Upgrade",  "Defense Upgrade",
                                "Speed Upgrade",       "Power Upgrade",
                                "Crawl Speed Upgrade", "Climb Speed Upgrade",
                                "Push Speed Upgrade"};
  for (size_t i = 0; i < std::size(upgrades); ++i) {
    items[upgrades[i]] =
        Rando::Item(upgrades[i], Text(upgradeNames[i]), ITEMTYPE_ITEM, 0, true,
                    LOGIC_NONE, RHT_NONE, ITEM_CATEGORY_MAJOR);
    location->SetPlacedItem(upgrades[i]);
    candidates = nlohmann::json::parse(OOT_GetMMHintCandidates(0, 1));
    assert(candidates.size() == 1 &&
           candidates[0]["itemName"] == upgradeNames[i]);
    assert(candidates[0]["itemGame"] == "oot" &&
           candidates[0]["checkName"] == "Kokiri Sword Chest");
  }
  const RandomizerGet sharedSouls[] = {RG_SOUL_ENEMY_OCTOROK, RG_SOUL_NPC_DAMPE,
                                       RG_SOUL_ANIMAL_DOG, RG_SOUL_MISC_GS};
  const char *sharedKeys[] = {"sharedSoulsEnemy", "sharedSoulsNpc",
                              "sharedSoulsAnimal", "sharedSoulsMisc"};
  const char *counterpartNames[] = {"Soul of Octoroks", "Dampe NPC Soul",
                                    "Dog Animal Soul",
                                    "Gold Skulltula Misc Soul"};
  for (size_t i = 0; i < std::size(sharedSouls); ++i) {
    const auto *soul = Rando::SoulShuffle::Find(sharedSouls[i]);
    assert(soul);
    location->SetPlacedItem(sharedSouls[i]);
    ComboRando::Combo_SetForeignJson(
        nlohmann::json{{"sharedItems", {sharedKeys[i]}}}.dump().c_str());
    fixtureContext->GetOption(soul->option).Set(0);
    assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 1)).empty());
    fixtureContext->GetOption(soul->option).Set(1);
    candidates = nlohmann::json::parse(OOT_GetMMHintCandidates(0, 1));
    assert(candidates.size() == 1 && candidates[0]["itemGame"] == "mm");
    assert(candidates[0]["itemName"] == counterpartNames[i]);
    assert(candidates[0]["checkName"] == "Kokiri Sword Chest");
    ComboRando::Combo_SetForeignJson(R"({"sharedItems":[]})");
    assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 0)).empty());
  }
  location->SetPlacedItem(RG_PUSH_SPEED_UPGRADE);
  fixtureContext->allLocations.clear();
  // Saved native placements remain answerable without the generation pool.
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 0)).size() == 1);
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 1)).size() == 1);
  regions[RR_ROOT].regionName.clear();
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 1)).empty());
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 0)).size() == 1);
  location->SetPlacedItem(RG_NONE);
  assert(nlohmann::json::parse(OOT_GetMMHintCandidates(0, 0)).empty());
  puts("PASS native OoT cross hint provider, seed identities, filters and "
       "exception-safe graph restoration");
}
