#include "combo/menu/ItemGrantAuditBridge.h"
#include "combo/rando/RpgStats.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>

#define COMBO_EXPORT
#define COMBO_BUILD
#include "combo/ComboGameMode.h"
#include "combo/vanilla/ComboVanillaMode.h"

extern "C" { int gComboGameMode = COMBO_GAME_BOTH; int gComboVanillaMode = 0; }
// This fixture exercises seeded native magic. Vanilla's exact-profile pulls
// have their own adapter tests and must not enter the legacy relay boundary.
void FleetShared_RequestPullFromPeer() { assert(false && "unexpected Vanilla pull in seeded magic fixture"); }

#define SPDLOG_INFO(...) ((void)0)
#define SPDLOG_WARN(...) ((void)0)
#define MAGIC_NORMAL_METER 48

enum { MAGIC_STATE_IDLE, MAGIC_STATE_STEP_CAPACITY, MAGIC_STATE_FILL };

/* NATIVE_IDS */

struct PlayerData {
  bool isMagicAcquired = false, isDoubleMagicAcquired = false;
  int magic = 0, magicLevel = 0;
};
struct {
  int fileNum = 0, magicState = MAGIC_STATE_IDLE, magicFillTarget = 0;
  bool isMagicAcquired = false, isDoubleMagicAcquired = false;
  int magic = 0, magicLevel = 0;
  struct {
    struct {
      struct {
        struct {
          uint8_t comboNativeMagicLevel = 0;
        } randomizer;
      } data;
    } quest;
  } ship;
  struct {
    struct {
      PlayerData playerData;
    } saveInfo;
  } save;
} gSaveContext;

struct Vitals {
  struct Magic {
    int amount;
    template <class T> T get() const { return static_cast<T>(amount); }
  };
  std::optional<Magic> magic;
  const Magic* find(const char* key) const {
    assert(std::strcmp(key, "magic") == 0);
    return magic ? &*magic : nullptr;
  }
  const Magic* end() const { return nullptr; }
};

/* Oot Magic Carryover */
#define MM_PD gSaveContext.save.saveInfo.playerData
/* Mm Magic Carryover */
#undef MM_PD

static void TestMagicCarryover() {
  auto& mm = gSaveContext.save.saveInfo.playerData;
  gSaveContext.magicState = MAGIC_STATE_IDLE;
  gSaveContext.isMagicAcquired = true;
  gSaveContext.magicLevel = 0;
  gSaveContext.magicFillTarget = 0;
  mm.magic = 100;
  ApplyOotMagic({Vitals::Magic{ExtractMmMagic()}});
  assert(gSaveContext.magic == 100 && gSaveContext.magicFillTarget == 100);
  gSaveContext.magic = 0;
  assert(ExtractOotMagic() == 100); // Staged amount survives even before the first HUD tick.
  ApplyOotMagic({Vitals::Magic{96}});
  gSaveContext.magic = 0; // The OoT meter is still building as the next switch starts.
  gSaveContext.magicLevel = 2;
  gSaveContext.magicState = MAGIC_STATE_STEP_CAPACITY;
  assert(ExtractOotMagic() == 96);

  mm.magic = 40;
  gSaveContext.magicState = MAGIC_STATE_FILL;
  ApplyMmMagic({Vitals::Magic{ExtractOotMagic()}});
  assert(mm.magic == 96 && gSaveContext.magicFillTarget == 96);
  ApplyMmMagic({Vitals::Magic{0}});
  assert(mm.magic == 0 && ExtractMmMagic() == 0);
  ApplyOotMagic({Vitals::Magic{ExtractMmMagic()}});
  assert(gSaveContext.magic == 0 && gSaveContext.magicFillTarget == 0);

  gSaveContext.magicLevel = 2;
  gSaveContext.magicState = MAGIC_STATE_IDLE;
  ApplyOotMagic({Vitals::Magic{37}});
  ApplyOotMagic({});
  assert(ExtractOotMagic() == 37 && gSaveContext.magicFillTarget == 0);
  ApplyMmMagic({Vitals::Magic{ExtractOotMagic()}});
  ApplyMmMagic({});
  assert(ExtractMmMagic() == 37 && gSaveContext.magicFillTarget == 0);
}

bool rando = true;
#define IS_RANDO rando
struct {
  ComboRpgState comboRpg{};
} nei;
auto Nei_Save() { return &nei; }
ComboRpgState ootRpg{};
namespace FleetRpg {
ComboRpgState Read() {
  auto state = ootRpg;
  state.nativeMagicLevel =
      gSaveContext.ship.quest.data.randomizer.comboNativeMagicLevel;
  return state;
}
} // namespace FleetRpg
int receiveDepth = 0, persisted = 0, echoes = 0;
struct FleetSharedReceiveGuard {
  FleetSharedReceiveGuard() { ++receiveDepth; }
  ~FleetSharedReceiveGuard() { --receiveDepth; }
};
struct SaveManager {
  static SaveManager *Instance;
  void SaveFile(int) { ++persisted; }
};
SaveManager manager;
SaveManager *SaveManager::Instance = &manager;
void SaveManager_SaveCurrentForCombo() { ++persisted; }
int ComboRpg_IsEnabled(int stat) {
  return ComboRpgState_Enabled(&nei.comboRpg, stat);
}
int16_t ComboRpg_MagicCapacity(int16_t native) {
  return ComboRpgState_MagicCapacity(&nei.comboRpg, native);
}
void ComboRpg_RecordNativeMagic(uint8_t tier) {
  nei.comboRpg.nativeMagicLevel = std::max(nei.comboRpg.nativeMagicLevel, tier);
}

/* MAGIC_EXPORTS */
/* NATIVE_TIER_GETTERS */

void SharedLegacy(const char *) { ++echoes; }
void *Combo_ResolveSym(const char *module, const char *symbol) {
  if (std::string(module) == "soh" &&
      std::string(symbol) == "SOH_ApplySharedMagicFloor")
    return reinterpret_cast<void *>(SOH_ApplySharedMagicFloor);
  if (std::string(module) == "2ship" &&
      std::string(symbol) == "MM_ApplySharedMagicFloor")
    return reinterpret_cast<void *>(MM_ApplySharedMagicFloor);
  return reinterpret_cast<void *>(SharedLegacy);
}
constexpr int FCI_NO_ITEM = -1, FC_COMBO_ITEM_COUNT = 1024;
int FcCombo_ItemForNative(int) { return 0; }
const char *FcCombo_PeerNameForItem(int) { return "peer"; }
int FleetCombo_ChainAliasFor(int) { return 0; }
struct {
  const char *ootName = "peer";
} gFcComboItems[1];

namespace OotRelay {
int sReceiveDepth = 0;
// This fixture exercises scalar magic, not a soul-component receipt.
bool SOH_ShareSoulReceipt(int) { return false; }
/* OOT_RELAY */
} // namespace OotRelay
namespace MmRelay {
int sReceiveDepth = 0;
bool MM_ShareSoulReceipt(int) { return false; }
/* MM_RELAY */
} // namespace MmRelay

void reset(bool rpg) {
  gSaveContext = {};
  nei = {};
  ootRpg = {};
  rando = true;
  persisted = echoes = receiveDepth = OotRelay::sReceiveDepth =
      MmRelay::sReceiveDepth = 0;
  for (auto *state : {&ootRpg, &nei.comboRpg}) {
    state->knownMask = (1 << COMBO_RPG_COUNT) - 1;
    state->enabledMask = rpg ? 1 << COMBO_RPG_MAGIC : 0;
    state->required[COMBO_RPG_MAGIC] = 8;
    state->magicTotal = 96;
  }
}
int main() {
  TestMagicCarryover();
  for (bool rpg : {false, true}) {
    reset(rpg);
    // Foreign MM magic collected in OoT: the receiver can be dormant, and
    // the OoT meter must be ready before any game transition or shared tick.
    MmRelay::FleetShared_OnNativeObtained(RI_SINGLE_MAGIC);
    assert(gSaveContext.isMagicAcquired && gSaveContext.magic == 48);
    assert(gSaveContext.ship.quest.data.randomizer.comboNativeMagicLevel == 1);
    assert(!gSaveContext.isDoubleMagicAcquired && persisted == 1 &&
           echoes == 0);
    gSaveContext.magic = 9;
    MmRelay::FleetShared_OnNativeObtained(RI_SINGLE_MAGIC);
    assert(gSaveContext.magic == 9 && persisted == 1); // no extra tier/refill
    MmRelay::FleetShared_OnNativeObtained(RI_DOUBLE_MAGIC);
    assert(gSaveContext.isDoubleMagicAcquired && gSaveContext.magic == 96 &&
           persisted == 2);
    MmRelay::FleetShared_OnNativeObtained(RI_SINGLE_MAGIC);
    assert(gSaveContext.magic == 96 && persisted == 2); // no downgrade

    reset(rpg);
    OotRelay::FleetShared_OnNativeObtained(RG_MAGIC_SINGLE);
    assert(gSaveContext.save.saveInfo.playerData.magic == 48 &&
           nei.comboRpg.nativeMagicLevel == 1);
    gSaveContext.save.saveInfo.playerData.magic = 7;
    OotRelay::FleetShared_OnNativeObtained(RG_MAGIC_SINGLE);
    assert(gSaveContext.save.saveInfo.playerData.magic == 7 && persisted == 1);
    OotRelay::FleetShared_OnNativeObtained(RG_MAGIC_DOUBLE);
    assert(gSaveContext.save.saveInfo.playerData.magic == 96 &&
           nei.comboRpg.nativeMagicLevel == 2);
    assert(persisted == 2 && receiveDepth == 0 && echoes == 0);
  }
  reset(true);
  ootRpg.level[COMBO_RPG_MAGIC] = nei.comboRpg.level[COMBO_RPG_MAGIC] =
      6; // 72-unit RPG bar
  gSaveContext.isMagicAcquired = gSaveContext.isDoubleMagicAcquired = true;
  gSaveContext.save.saveInfo.playerData.isMagicAcquired = true;
  gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired = true;
  assert(OotNativeTier() == 0 &&
         MmNativeTier() ==
             0); // fractional HUD flags do not imply two native pickups
  SOH_ApplySharedMagicFloor(1);
  MM_ApplySharedMagicFloor(1);
  assert(gSaveContext.magic == 72 &&
         gSaveContext.save.saveInfo.playerData.magic == 72);
  assert(ootRpg.level[COMBO_RPG_MAGIC] == 6 &&
         nei.comboRpg.level[COMBO_RPG_MAGIC] == 6);
  assert(gSaveContext.ship.quest.data.randomizer.comboNativeMagicLevel == 1 &&
         nei.comboRpg.nativeMagicLevel == 1);
  assert(OotNativeTier() == 1 && MmNativeTier() == 1);
  reset(true);
  OotRelay::sReceiveDepth = MmRelay::sReceiveDepth = 1;
  OotRelay::FleetShared_OnNativeObtained(RG_MAGIC_SINGLE);
  MmRelay::FleetShared_OnNativeObtained(RI_SINGLE_MAGIC);
  assert(persisted == 0); // receive guard prevents a loop
  for (int invalid : {-1, 0, 3, 255}) {
    SOH_ApplySharedMagicFloor(invalid);
    MM_ApplySharedMagicFloor(invalid);
  }
  assert(persisted == 0);
  gSaveContext.fileNum = 0xFF;
  SOH_ApplySharedMagicFloor(1);
  MM_ApplySharedMagicFloor(1);
  assert(persisted == 0);
  gSaveContext.fileNum = 0;
  rando = false;
  SOH_ApplySharedMagicFloor(1);
  MM_ApplySharedMagicFloor(1);
  assert(persisted == 0);

  reset(true);
  const int slot = 1;
  gSaveContext.fileNum = slot;
  nei.comboRpg = {}; // Fresh MM save with no snapshot from a previous game switch.
  nei.comboRpg.level[COMBO_RPG_SPEED] = 3;
  ComboRpgState progress{};
  assert(!SOH_GetRpgMagicProgress(slot, &progress)); // No write for an uncollected stat.
  ootRpg.level[COMBO_RPG_MAGIC] = 1;
  ootRpg.level[COMBO_RPG_SPEED] = 1;
  assert(!SOH_GetRpgMagicProgress(0, &progress));
  assert(SOH_GetRpgMagicProgress(slot, &progress));
  auto& mm = gSaveContext.save.saveInfo.playerData;
  MM_ApplyRpgMagicProgress(0, &progress);
  assert(!mm.isMagicAcquired && persisted == 0);

  mm.magicLevel = 1;
  MM_ApplyRpgMagicProgress(slot, &progress);
  assert(mm.isMagicAcquired && !mm.isDoubleMagicAcquired && mm.magic == 12);
  assert(mm.magicLevel == 0 && gSaveContext.magicFillTarget == 12 && persisted == 1);
  assert(nei.comboRpg.level[COMBO_RPG_MAGIC] == 1 && nei.comboRpg.level[COMBO_RPG_SPEED] == 3);
  mm.magic = 0; // A repeated tick must preserve spent magic.
  mm.magicLevel = 1;
  MM_ApplyRpgMagicProgress(slot, &progress);
  assert(mm.magic == 0 && mm.magicLevel == 1 && persisted == 1);

  ootRpg.level[COMBO_RPG_MAGIC] = 2;
  assert(SOH_GetRpgMagicProgress(slot, &progress));
  MM_ApplyRpgMagicProgress(slot, &progress);
  assert(mm.magic == 24 && persisted == 2);
  progress.level[COMBO_RPG_MAGIC] = 1;
  mm.magic = 5;
  MM_ApplyRpgMagicProgress(slot, &progress);
  assert(mm.magic == 5 && nei.comboRpg.level[COMBO_RPG_MAGIC] == 2 && persisted == 2);

  // Existing MM files that have the stat count but no initialized meter still recover.
  mm.isMagicAcquired = false;
  MM_ApplyRpgMagicProgress(slot, &progress);
  assert(mm.isMagicAcquired && mm.magic == 24 && persisted == 3);

  reset(true);
  gSaveContext.fileNum = slot;
  MM_ApplySharedMagicFloor(1);
  ootRpg.level[COMBO_RPG_MAGIC] = 1;
  assert(SOH_GetRpgMagicProgress(slot, &progress));
  MM_ApplyRpgMagicProgress(slot, &progress);
  assert(mm.magic == 48 && nei.comboRpg.nativeMagicLevel == 1 && persisted == 2);

  reset(true);
  gSaveContext.fileNum = slot;
  ootRpg.level[COMBO_RPG_MAGIC] = 3;
  ootRpg.required[COMBO_RPG_MAGIC] = 6;
  ootRpg.magicTotal = 100;
  assert(SOH_GetRpgMagicProgress(slot, &progress));
  MM_ApplyRpgMagicProgress(slot, &progress);
  assert(mm.isDoubleMagicAcquired && mm.magic == 50 && persisted == 1);
  assert(nei.comboRpg.required[COMBO_RPG_MAGIC] == 6 && nei.comboRpg.magicTotal == 100);

  rando = false;
  assert(!SOH_GetRpgMagicProgress(slot, &progress));
  progress.level[COMBO_RPG_MAGIC] = 6;
  MM_ApplyRpgMagicProgress(slot, &progress);
  assert(mm.magic == 50 && persisted == 1);
  std::cout << "PASS immediate native and RPG magic, save recovery, duplicate/echo "
               "suppression and invalid slots\n";
}
