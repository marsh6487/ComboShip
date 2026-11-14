#include <algorithm>
#include <cstring>
#include <nlohmann/json.hpp>
extern "C" {
// clang-format off
#include "z64.h"
#include "macros.h"
#include "mods/nei_save.h"
#include "soh/Enhancements/randomizer/randomizerEnums.h"
#include "soh/FleetShipCombo/FleetComboItems.h"
// clang-format on
SaveContext gSaveContext;
// TABLES
// UPGRADE
}
#define SPDLOG_INFO(...) ((void)0)
static NeiSaveData nei;
extern "C" NeiSaveData *Nei_Save(void) { return &nei; }
// Frozen seed-option and flag reads are engine boundaries. Capacity arithmetic
// and native progression storage below are the production helper unchanged.
static int bombchuOption = RO_BOMBCHU_BAG_PROGRESSIVE;
static bool infiniteBombchus = false;
struct SeedOption {
  bool Is(int value) const { return bombchuOption == value; }
};
#define RAND_GET_OPTION(option)                                                \
  SeedOption {}
static bool Flags_GetRandomizerInf(RandomizerInf flag) {
  return flag == RAND_INF_HAS_INFINITE_BOMBCHUS && infiniteBombchus;
}

namespace oot {
static int replayGrants;
static bool sApplyingFc;
static PlayState *gPlayState;
static int pendingNative;
static bool Combo_IsVanilla() { return false; }
namespace ComboTrade {
static bool AcknowledgeConsumedGrant(NeiSaveData &, int) { return false; }
} // namespace ComboTrade
static int FcCombo_NativeForItem(int id) {
  if (id == FCI_STICK_CAPACITY)
    return RG_PROGRESSIVE_STICK_UPGRADE;
  if (id == FCI_NUT_CAPACITY)
    return RG_PROGRESSIVE_NUT_UPGRADE;
  return id == FCI_BOMBCHU_BAG ? RG_PROGRESSIVE_BOMBCHU_BAG : FCI_NO_ITEM;
}
namespace Rando::StaticData {
struct Row {
  GetItemEntry GetGIEntry_Copy() {
    GetItemEntry entry{};
    entry.modIndex = MOD_RANDOMIZER;
    return entry;
  }
};
static Row RetrieveItem(RandomizerGet item) {
  pendingNative = item;
  return {};
}
} // namespace Rando::StaticData
static void Randomizer_Item_Give(PlayState *, GetItemEntry) {
  if (pendingNative == RG_PROGRESSIVE_STICK_UPGRADE ||
      pendingNative == RG_PROGRESSIVE_NUT_UPGRADE) {
    const bool stick = pendingNative == RG_PROGRESSIVE_STICK_UPGRADE;
    const int upgrade = stick ? UPG_STICKS : UPG_NUTS;
    const int ammoItem = stick ? ITEM_STICK : ITEM_NUT;
    Inventory_ChangeUpgrade(upgrade,
                            std::min<unsigned>(3, CUR_UPG_VALUE(upgrade) + 1));
    AMMO(ammoItem) = CUR_CAPACITY(upgrade);
    return;
  }
  ++replayGrants;
  INV_CONTENT(ITEM_BOMBCHU) = ITEM_BOMBCHU;
  AMMO(ITEM_BOMBCHU) = 20;
}
// PRODUCTION
} // namespace oot

static const int items[] = {ITEM_STICK, ITEM_NUT, ITEM_BOMBCHU};
void OotReset() {
  std::memset(&gSaveContext, 0, sizeof gSaveContext);
  std::memset(gSaveContext.inventory.items, ITEM_NONE,
              sizeof gSaveContext.inventory.items);
  nei = {};
  bombchuOption = RO_BOMBCHU_BAG_PROGRESSIVE;
  infiniteBombchus = false;
  oot::replayGrants = 0;
  gSaveContext.ship.quest.id = QUEST_RANDOMIZER;
  gSaveContext.ship.quest.data.randomizer.bombchuUpgradeLevel = 1;
  Inventory_ChangeUpgrade(UPG_STICKS, 1);
  Inventory_ChangeUpgrade(UPG_NUTS, 1);
}
void OotBombchuBag(int level, bool progressive, bool infinite) {
  bombchuOption =
      progressive ? RO_BOMBCHU_BAG_PROGRESSIVE : RO_BOMBCHU_BAG_SINGLE;
  gSaveContext.ship.quest.data.randomizer.bombchuUpgradeLevel = level;
  infiniteBombchus = infinite;
}
void OotCapacitySnapshot(nlohmann::json &sh) {
  oot::RecordBombchuCapacity();
  sh["comboObtained"] = nei.comboObtained;
}
void OotApplyCapacity(const nlohmann::json &sh) { oot::ApplyBombchuCapacity(sh); }
void OotLegacyReceipt() { nei.comboObtainedFc[FCI_BOMBCHU_BAG] = 1; }
void OotReplayReceipt() {
  nei.comboObtainedFc[FCI_BOMBCHU_BAG] = 1;
  oot::ApplyFcRegistryToNatives();
}
int OotReplayGrants() { return oot::replayGrants; }
void OotReplayCapacity(int which) {
  const int id = which == 0 ? FCI_STICK_CAPACITY : FCI_NUT_CAPACITY;
  nei.comboObtainedFc[id] = 1;
  oot::ApplyFcRegistryToNatives();
}
int OotTier(int which) {
  return CUR_UPG_VALUE(which == 0 ? UPG_STICKS : UPG_NUTS);
}
void OotSetTier(int which, int tier) {
  Inventory_ChangeUpgrade(which == 0 ? UPG_STICKS : UPG_NUTS, tier);
}
void OotBag(int which, bool owned) {
  Inventory_ChangeUpgrade(which == 0 ? UPG_STICKS : UPG_NUTS, owned ? 1 : 0);
}
void OotSet(int which, int ammo, bool owned) {
  INV_CONTENT(items[which]) = owned ? items[which] : ITEM_NONE;
  AMMO(items[which]) = ammo;
}
int OotAmmo(int which) { return AMMO(items[which]); }
void OotExtract(nlohmann::json &inv, int which) {
  const char *names[] = {"stick", "nut", "bombchu"};
  oot::PutInvItem(inv, names[which], SLOT(items[which]), true);
}
void OotApply(const nlohmann::json &inv, int which) {
  const char *names[] = {"stick", "nut", "bombchu"};
  oot::ApplyInvItem(inv, names[which], SLOT(items[which]), items[which], true);
}
