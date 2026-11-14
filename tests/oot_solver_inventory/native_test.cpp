#include "z64.h"

// macros.h uses GraphicsContext; load its native definition before the macros.
#include "macros.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#define private public
#include "soh/Enhancements/randomizer/logic.h"
#undef private
#include "NeiGracePolicy.h"
#include "mods/extended_equipment.h"
#include "mods/extended_inventory.h"
#include "mods/items/custom_items.h"
#include "rando/ComboPlaythrough.h"
#include "soh/Enhancements/debugger/performanceTimer.h"
#include "soh/Enhancements/randomizer/RustyDoorKeys.h"
#include "soh/Enhancements/randomizer/SoulShuffle.h"
#include "soh/Enhancements/randomizer/bean_patches.h"
#include "soh/Enhancements/randomizer/dungeon.h"
#include "soh/Enhancements/randomizer/entrance.h"
#include "soh/Enhancements/randomizer/location_access.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/randostatupgrade.h"
extern "C" {
SaveContext gSaveContext{};
#include "arrays.inc"
}
// Runtime-only external boundary: no game loops or disk services are needed.
struct OTRGlobals {
  static OTRGlobals *Instance;
  bool HasOriginal() const { return true; }
};
static OTRGlobals globals;
OTRGlobals *OTRGlobals::Instance = &globals;
static NeiSaveData gNeiSave{};
static NeiSaveData &live = gNeiSave;
extern "C" NeiSaveData *Nei_Save() { return &gNeiSave; }
extern "C" uint8_t Wand_RandoMode() {
  return Rando::Context::GetInstance()
      ->GetOption(RSK_ELEMENTAL_WAND_SHUFFLE)
      .Get();
}
extern "C" uint8_t BombArrows_RandoMode() {
  return Rando::Context::GetInstance()
      ->GetOption(RSK_SHUFFLE_BOMB_ARROWS)
      .Get();
}
extern "C" u8 Picto_IsOwned() { std::abort(); }
extern "C" u8 PowerKeg_IsOwned() { std::abort(); }
extern "C" u8 TwilightUpgrade_HasBombArrows() { return 0; }
extern "C" u8 RitoMask_IsOwned() { return (live.ritoMaskFlags & 1) != 0; }
#ifdef COMBO_BUILD
extern "C" void ItemGrantAudit_Begin(const char *, int, int, int) {}
extern "C" void ItemGrantAudit_End() {}
#endif
extern "C" {
#include "runtime.inc"
}
static size_t globalInventoryWrites;
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t value) {
  ++globalInventoryWrites;
  FixtureRawNei_SetOwnedItem(slot, value);
}
#include "text.inc"
extern "C" unsigned char TradeAdult_IsOwnedIndex(int index) {
  return (live.tradeAdultOwned >> index) & 1;
}
// These branches are outside the synthetic Goron City world. Abort if reached;
// never answer an unmodeled capability permissively.
s8 *Randomizer::SilverFieldFromSaveContext(SaveContext *, RandomizerGet) {
  std::abort();
}
s8 Randomizer::SilverTotal(RandomizerGet) { std::abort(); }
void StartPerformanceTimer(TimerID) {}
void StopPerformanceTimer(TimerID) {}
namespace Rando {
int8_t Logic::GetSmallKeyCount(SceneID) { std::abort(); }
bool Logic::IsFireLoopLocked() { return true; }
DungeonInfo *Context::GetDungeon(size_t) const { std::abort(); }
bool DungeonInfo::IsVanilla() const { std::abort(); }
Context::Context() {}
Hint::Hint() {}
ItemLocation::ItemLocation() {}
static std::shared_ptr<Context> fixtureContext;
std::shared_ptr<Context> Context::GetInstance() { return fixtureContext; }
OptionValue &Context::GetOption(RandomizerSettingKey key) {
  return mOptions[key];
}
OptionValue &Context::GetTrickOption(RandomizerTrick key) {
  return mTrickOptions[key];
}
#ifdef COMBO_BUILD
std::shared_ptr<GetItemEntry> Item::GetGIEntry(RandomizerGet *) const {
  return giEntry;
}
#else
std::shared_ptr<GetItemEntry> Item::GetGIEntry() const { return giEntry; }
#endif
} // namespace Rando
#include "production.inc"
Region::Region() {}
Region::~Region() {}
Region *RegionTable(RandomizerRegion) {
  static Region region;
  region.scene = SCENE_GORON_CITY;
  return &region;
}
namespace Rando {
std::shared_ptr<Logic> Context::GetLogic() { return mLogic; }
} // namespace Rando
#undef LOCATION
#define LOCATION(check, ...)                                                   \
  static bool Maze(Rando::Logic *logic, Rando::Context *ctx) {                 \
    return (__VA_ARGS__);                                                      \
  }
#include "maze.inc"
#undef LOCATION
#include "starting.inc"
static int failures;
static void Check(bool value, const std::string &message) {
  std::cout << (value ? "PASS: " : "FAIL: ") << message << '\n';
  failures += !value;
}
static Rando::Item MakeItem(RandomizerGet rg, uint16_t id = ITEM_NONE,
                            ItemType type = ITEMTYPE_ITEM) {
  Rando::Item item;
  item.randomizerGet = rg;
  item.type = type;
  item.logicVal = LOGIC_NONE;
  item.giEntry = std::make_shared<GetItemEntry>();
  item.giEntry->itemId = id;
  item.giEntry->objectId = OBJECT_GAMEPLAY_KEEP;
  return item;
}
static std::map<RandomizerGet, Rando::Item> items;
namespace Rando {
Item &StaticData::RetrieveItem(RandomizerGet rg) { return items.at(rg); }
} // namespace Rando
static void Effect(Rando::Logic &logic, RandomizerGet rg, bool grant = true,
                   uint16_t id = ITEM_NONE, ItemType type = ITEMTYPE_ITEM) {
  auto item = MakeItem(rg, id, type);
  logic.ApplyItemEffect(item, grant);
}
static std::shared_ptr<Rando::Logic> Fresh() {
  auto logic = std::make_shared<Rando::Logic>();
  logic->SetContext(Rando::fixtureContext);
  logic->NewSaveContext();
  logic->SetRandoInf(RAND_INF_CAN_OPEN_CHEST, true);
  logic->SetRandoInf(RAND_INF_CAN_GRAB, true);
  logic->IsAdult = true;
  logic->CurrentRegionKey = RR_GORON_CITY;
  return logic;
}
static void InventoryMatrix() {
  auto logic = Fresh();
  const NeiSaveData before = live;
  const auto writesBefore = globalInventoryWrites;
  Check(!Maze(logic.get(), Rando::fixtureContext.get()),
        "actual GC maze-left predicate rejects dirty-live mask");
  Effect(*logic, RG_MM_MASK_GORON, true, ITEM_MM_MASK_GORON);
  Check(Maze(logic.get(), Rando::fixtureContext.get()),
        "actual GC maze-left predicate accepts earned mask");
  logic->NewSaveContext();
  logic->SetRandoInf(RAND_INF_CAN_OPEN_CHEST, true);
  logic->IsAdult = true;
  Check(!Maze(logic.get(), Rando::fixtureContext.get()),
        "actual maze empty -> mask -> empty is isolated");
  auto other = Fresh();
  Effect(*logic, RG_MM_MASK_GORON, true, ITEM_MM_MASK_GORON);
  Check(!other->HasItem(RG_MM_MASK_GORON),
        "separate Logic instances do not inherit mask");
  logic->Reset(false);
  Check(logic->HasItem(RG_MM_MASK_GORON),
        "Reset(false) retains selected isolated inventory");
  auto snapshot = *logic;
  Effect(*logic, RG_MM_MASK_GORON, false, ITEM_MM_MASK_GORON);
  Check(snapshot.HasItem(RG_MM_MASK_GORON) && !logic->HasItem(RG_MM_MASK_GORON),
        "copied Logic snapshots retain private custom inventory");
  *logic = snapshot;
  Check(logic->HasItem(RG_MM_MASK_GORON),
        "restored Logic snapshot retains private custom inventory");
  Effect(*other, RG_MM_MASK_GORON, true, ITEM_MM_MASK_GORON);
  Effect(*other, RG_MM_MASK_GORON, true, ITEM_MM_MASK_GORON);
  Effect(*other, RG_MM_MASK_GORON, false, ITEM_MM_MASK_GORON);
  Check(other->HasItem(RG_MM_MASK_GORON),
        "duplicate mask survives first removal");
  Effect(*other, RG_MM_MASK_GORON, false, ITEM_MM_MASK_GORON);
  Check(!other->HasItem(RG_MM_MASK_GORON),
        "last duplicate mask removal removes mask");
  for (const auto rg :
       {RG_EXT_CANE_OF_BYRNA, RG_EXT_FOUR_SWORD, RG_EXT_DIVINE_SHIELD,
        RG_EXT_SHEIKAH_SHIELD, RG_EXT_SHIELD_OF_IKANA,
        RG_EXT_SPIRIT_BREASTPLATE, RG_EXT_CHAMPIONS_TUNIC,
        RG_EXT_PEGASUS_ANKLET, RG_EXT_TRIDENT, RG_EXT_CLIMB_BOOTS,
        RG_EXT_ROC_BOOTS, RG_EXT_WATER_DRAGON_SCALE, RG_EXT_MAGIC_CAPE,
        RG_HYLIAS_GRACE, RG_DESIRE_SENSOR, RG_RITO_MASK, RG_MAGIC_MIRROR,
        RG_BOMB_ARROWS, RG_MM_PENDANT_OF_MEMORIES}) {
    Check(!other->HasItem(rg),
          "dirty live custom flag cannot supply private item " +
              std::to_string(rg));
    Effect(*other, rg);
    Effect(*other, rg);
    Check(other->HasItem(rg),
          "actual ApplyItemEffect grants custom ownership " +
              std::to_string(rg));
    Effect(*other, rg, false);
    Check(other->HasItem(rg), "duplicate custom grant survives first removal " +
                                  std::to_string(rg));
    Effect(*other, rg, false);
    Effect(*other, rg, false);
    Check(!other->HasItem(rg),
          "last removal and extra removal leave custom item absent " +
              std::to_string(rg));
  }
  Rando::fixtureContext->GetOption(RSK_EXT_EQUIPMENT).Set(0);
  Effect(*other, RG_MM_PENDANT_OF_MEMORIES);
  Check(other->HasItem(RG_EXT_PENDANT_OF_MEMORIES),
        "MM Pendant grants extended alias with equipment option off");
  Effect(*other, RG_EXT_PENDANT_OF_MEMORIES);
  Effect(*other, RG_MM_PENDANT_OF_MEMORIES, false);
  Check(other->HasItem(RG_MM_PENDANT_OF_MEMORIES),
        "Pendant alias grants share removal count");
  Effect(*other, RG_EXT_PENDANT_OF_MEMORIES, false);
  Check(!other->HasItem(RG_MM_PENDANT_OF_MEMORIES),
        "Pendant aliases fully remove");
  Rando::fixtureContext->GetOption(RSK_EXT_EQUIPMENT).Set(1);
  other->SetInventory(EXT_ITEM_MAGIC_MIRROR, EXT_ITEM_MAGIC_MIRROR);
  Check(other->CurrentInventory(EXT_ITEM_MAGIC_MIRROR) == EXT_ITEM_MAGIC_MIRROR,
        "extended inventory preserves 16-bit item IDs");
#ifndef LEGACY_SOLVER_INVENTORY
  for (uint32_t invalid : {56u, 0xffffu, 0x10000u, 0xffffffffu}) {
    other->SetInventory(invalid, ITEM_MM_MASK_GORON);
    Check(other->CurrentInventory(invalid) == ITEM_NONE &&
              !other->CheckInventory(invalid, false),
          "invalid inventory reads empty and rejects writes " +
              std::to_string(invalid));
  }
#endif
  for (auto rg : {RG_SHOVEL, RG_DEMISE_DESTRUCTION}) {
    auto shovel = Fresh();
    Effect(*shovel, rg, true,
           rg == RG_SHOVEL ? ITEM_SHOVEL : ITEM_DEMISE_DESTRUCTION);
    Check(shovel->HasItem(rg) && !shovel->HasBottle(),
          "Shovel/Demise ownership never creates bottle " + std::to_string(rg));
    delete shovel->mSaveContext;
  }
  Effect(*other, RG_PROGRESSIVE_ROCS);
  Effect(*other, RG_PROGRESSIVE_ROCS);
  Check(other->CurrentInventory(ITEM_ROCS_FEATHER_SKIJER) == ITEM_ROCS_CAPE,
        "two progressive Rocs grants earn Cape");
  Effect(*other, RG_PROGRESSIVE_ROCS, false);
  Check(other->CurrentInventory(ITEM_ROCS_FEATHER_SKIJER) ==
            ITEM_ROCS_FEATHER_SKIJER,
        "Rocs removal restores Feather");
  Effect(*other, RG_ROCS_CAPE);
  Effect(*other, RG_PROGRESSIVE_ROCS, false);
  Check(other->CurrentInventory(ITEM_ROCS_FEATHER_SKIJER) == ITEM_ROCS_CAPE,
        "direct Cape survives progressive Rocs removal");
  Effect(*other, RG_ROCS_CAPE, false);
  Check(!other->CheckInventory(ITEM_ROCS_FEATHER_SKIJER, false),
        "last Rocs ownership removal empties cell");
  auto &ctx = *Rando::fixtureContext;
  ctx.GetOption(RSK_ELEMENTAL_WAND_SHUFFLE).Set(RO_WAND_ELEMENTAL_SHUFFLE);
  Effect(*other, RG_WAND_SAND_ROD);
  Check(other->HasItem(RG_WAND_SAND_ROD) && !other->HasItem(RG_WAND_WATER_ROD),
        "elemental wand grants only earned mode");
  ctx.GetOption(RSK_ELEMENTAL_WAND_SHUFFLE).Set(RO_WAND_SINGLE_ITEM);
  Check(other->HasItem(RG_WAND_WATER_ROD) &&
            other->HasItem(RG_WAND_SHADOW_SCEPTER),
        "single-item wand grants all modes from private ownership");
  ctx.GetOption(RSK_ELEMENTAL_WAND_SHUFFLE).Set(RO_WAND_MEDALLIONS);
  Check(!other->HasItem(RG_WAND_SAND_ROD),
        "live medallions cannot unlock private wand mode");
  Effect(*other, RG_SPIRIT_MEDALLION, true, ITEM_NONE, ITEMTYPE_DUNGEONREWARD);
  Check(other->HasItem(RG_WAND_SAND_ROD) && !other->HasItem(RG_WAND_WATER_ROD),
        "private medallion unlocks corresponding wand mode only");
  ctx.GetOption(RSK_SHUFFLE_BOMB_ARROWS).Set(RO_BOMB_ARROWS_BOMB_BAG);
  Check(!other->HasItem(RG_BOMB_ARROWS),
        "live Bomb Bag cannot auto-grant private Bomb Arrows");
  Effect(*other, RG_PROGRESSIVE_BOMB_BAG);
  Check(other->HasItem(RG_BOMB_ARROWS),
        "private Bomb Bag auto-grants Bomb Arrows");
  Check(
      std::memcmp(&before, &live, sizeof live) == 0,
      "all isolated queries/grants/removals preserve complete live NEI bytes");
  Check(globalInventoryWrites == writesBefore,
        "isolated solver never calls global NEI setter, including idempotent "
        "writes");
  const SaveContext nativeBefore = gSaveContext;
  Rando::Logic bound;
  bound.SetContext(Rando::fixtureContext);
  bound.SetSaveContext(&gSaveContext);
  Check(bound.HasItem(RG_MM_MASK_GORON) && bound.HasItem(RG_EXT_FOUR_SWORD),
        "SetSaveContext(global) reads live custom view");
  SaveContext copied = gSaveContext;
  bound.SetSaveContext(&copied);
  Check(bound.HasItem(RG_MM_MASK_GORON) && bound.HasItem(RG_EXT_FOUR_SWORD),
        "SetSaveContext(copied save) still reads live custom view");
  bound.Reset(false);
  Check(bound.HasItem(RG_MM_MASK_GORON), "Reset(false) retains live binding");
#ifndef LEGACY_SOLVER_INVENTORY
  bound.NewSaveContext();
  Check(!bound.HasItem(RG_MM_MASK_GORON),
        "NewSaveContext after borrowed copied-save binding isolates without "
        "deleting caller save");
#else
  bound.mSaveContext = nullptr;
#endif
  Check(std::memcmp(&nativeBefore, &gSaveContext, sizeof gSaveContext) == 0,
        "solver work preserves complete live native save bytes");
  auto starting = Fresh();
  Rando::fixtureContext->mLogic = starting;
  items[RG_MM_MASK_GORON] = MakeItem(RG_MM_MASK_GORON, ITEM_MM_MASK_GORON);
  StartingInventory = {RG_MM_MASK_GORON};
  ApplyStartingInventory();
  Check(Maze(starting.get(), Rando::fixtureContext.get()),
        "production ApplyStartingInventory supplies legitimate early mask");
  delete starting->mSaveContext;
  delete bound.mSaveContext;
  delete logic->mSaveContext;
  delete other->mSaveContext;
}
static void CombinedGuards();
int main() {
  std::cout << std::unitbuf;
  Rando::fixtureContext = std::make_shared<Rando::Context>();
  auto &ctx = *Rando::fixtureContext;
  for (auto option : {RSK_MM_MASKS_ALL, RSK_MM_MASKS_TRANSFORM,
                      RSK_SKIJER_CUSTOM_ITEMS, RSK_EXT_EQUIPMENT})
    ctx.GetOption(option).Set(1);
  ctx.GetOption(RSK_HYLIAS_GRACE).Set(NEI_GRACE_ON);
  std::fill(std::begin(live.ownedItems), std::end(live.ownedItems), ITEM_NONE);
  Nei_SetOwnedItem(SLOT_MM_MASK_GORON, ITEM_MM_MASK_GORON);
  live.extEquipOwnedBits = 0xffffffff;
  live.capeOwned = 1;
  live.hyliasGraceOwned = 1;
  live.slateRunesOwned = 0xff;
  live.ritoMaskFlags = 1;
  live.magicMirrorOwned = 1;
  live.bombArrowsOwned = 1;
  live.tradeAdultOwned = 0xffffffff;
  live.wandRodsOwned = 0xff;
  gSaveContext.inventory.questItems = 0x3f;
  InventoryMatrix();
  CombinedGuards();
  std::cout << (failures ? "FAIL: " : "PASS: ")
            << "native solver inventory isolation (" << failures
            << " failed assertions)\n";
  return failures ? 1 : 0;
}
namespace {
using namespace ComboRando;
using json = nlohmann::json;
constexpr auto kMaskName = "Goron Mask (MM)";
constexpr auto kMazeName = "GC Maze Left Chest";
constexpr auto kFreeName = "Synthetic early chest";
constexpr auto kCycleName = "Synthetic strength-gated chest";
std::shared_ptr<Rando::Logic> oracleLogic;
std::string reachableBuffer;
bool freeEarly = false;
bool requirePrize = false, prizeOwned = false;
void OracleReset() {
  prizeOwned = false;
  if (oracleLogic)
    delete oracleLogic->mSaveContext;
  oracleLogic = Fresh();
}
void OracleSet(const char *value) {
  for (const auto &name : json::parse(value)) {
    const auto n = name.get<std::string>();
    if (n == kMaskName)
      Effect(*oracleLogic, RG_MM_MASK_GORON, true, ITEM_MM_MASK_GORON);
    else if (n == "Progressive Strength Upgrade")
      Effect(*oracleLogic, RG_PROGRESSIVE_STRENGTH);
    else if (n == "Maze prize")
      prizeOwned = true;
  }
}
const char *OracleReach() {
  json checks = json::array();
  if (freeEarly)
    checks.push_back(kFreeName);
  if (Maze(oracleLogic.get(), Rando::fixtureContext.get()))
    checks.push_back(kMazeName);
  if (oracleLogic->HasItem(RG_GORONS_BRACELET))
    checks.push_back(kCycleName);
  // Victory requires crossing the actual authored maze edge in this small
  // world.
  if (Maze(oracleLogic.get(), Rando::fixtureContext.get()) &&
      (!requirePrize || prizeOwned))
    checks.push_back("Ganon");
  reachableBuffer = checks.dump();
  return reachableBuffer.c_str();
}
void NoopReset() {}
void NoopSet(const char *) {}
void NoopPlace(const char *, const char *) {}
const char *MmReach() { return "[\"Moon Majora Pot 01\"]"; }
uint8_t PortalOpen() { return 1; }
const OracleFns ootOracle{OracleReset, OracleSet, OracleReach, NoopPlace,
                          PortalOpen};
const OracleFns mmOracle{NoopReset, NoopSet, MmReach, NoopPlace};
json EmptyDump() {
  return {{"pool", json::array()},
          {"checks", json::array()},
          {"items", json::array()},
          {"fixed", json::array()}};
}
json FixedDump(const json &placements) {
  json dump = EmptyDump();
  for (auto it = placements.begin(); it != placements.end(); ++it) {
    dump["items"].push_back({{"name", it.value()},
                             {"token", "RI_" + it.value().get<std::string>()}});
    dump["fixed"].push_back({{"check", it.key()},
                             {"item", it.value()},
                             {"advancement", true},
                             {"validate", true}});
  }
  return dump;
}
void Prime() {
  const auto checks =
      QueryReachable(ootOracle, {kMaskName, "Progressive Strength Upgrade",
                                 "Progressive Strength Upgrade"});
  Check(checks.count(kMazeName), "full mask query primes real maze capability");
}
} // namespace
static void CombinedGuards() {
  using namespace ComboRando;
  using json = nlohmann::json;
  for (int scenario = 0; scenario < 3; ++scenario) {
    freeEarly = scenario == 2;
    requirePrize = scenario == 2;
    const bool valid = scenario == 2;
    json placements;
    if (scenario == 0)
      placements = {{kMazeName, kMaskName}};
    if (scenario == 1)
      placements = {{kMazeName, "Progressive Strength Upgrade"},
                    {kCycleName, kMaskName}};
    if (scenario == 2)
      placements = {{kFreeName, kMaskName}, {kMazeName, "Maze prize"}};
    const auto oot = FixedDump(placements);
    const auto mm = EmptyDump();
    const auto spoiler =
        json({{"oot", placements}, {"mm", json::object()}}).dump();
    const std::string label = scenario == 0   ? "mask behind itself"
                              : scenario == 1 ? "two-item cycle"
                                              : "free early mask";
    Prime();
    json spheres = json::array();
    const auto replay =
        RunPlaythrough(spoiler, ootOracle, mmOracle, label, nullptr, &spheres,
                       oot.dump(), mm.dump(), false);
    Check(replay.beatable == valid, "production forward replay " + label +
                                        (valid ? " passes" : " rejects"));
    if (valid) {
      int maskSphere = -1, mazeSphere = -1;
      for (const auto &sphere : spheres)
        for (const auto &step : sphere.at("steps")) {
          if (step.at("check") == kFreeName)
            maskSphere = sphere.at("sphere");
          if (step.at("check") == kMazeName)
            mazeSphere = sphere.at("sphere");
        }
      Check(maskSphere >= 0 && mazeSphere > maskSphere,
            "valid replay earns mask before a later maze sphere");
    }
    Prime();
    const auto fill = CrossWorldCombinedFill(oot.dump(), mm.dump(), 20261114,
                                             ootOracle, mmOracle);
    Check(fill.success == valid, "production combined fill final validation " +
                                     label + (valid ? " passes" : " rejects") +
                                     " (" + fill.error + ")");
    if (!valid)
      Check(fill.error.find("validation failed") != std::string::npos,
            "negative fill reaches final validation: " + label + " (" +
                fill.error + ")");
  }
  if (oracleLogic) {
    delete oracleLogic->mSaveContext;
    oracleLogic.reset();
  }
}
