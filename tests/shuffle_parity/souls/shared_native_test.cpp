#include <cassert>
#include <nlohmann/json.hpp>
#include <set>
#ifdef TEST_MM
#include "Rando/SoulShuffleRuntime.cpp"
#include "Rando/SoulShuffleShared.cpp"
#define PREFIX(name) MM_##name
#define PEER(name) SOH_##name
#define MASK gMMComboSharedMask
#define SOUL_ID(category, oot, mm) RI_SOUL_##category##_##mm
#define OWN_OPTIONS RANDO_SAVE_OPTIONS
#define OPTION(category) RO_SHUFFLE_##category##_SOULS
#else
#include "soh/Enhancements/randomizer/SoulShuffleRuntime.cpp"
#include "soh/Enhancements/randomizer/SoulShuffleShared.cpp"
#define PREFIX(name) SOH_##name
#define PEER(name) MM_##name
#define MASK gComboSharedMask
#define SOUL_ID(category, oot, mm) RG_SOUL_##category##_##oot
#define OWN_OPTIONS optionValues
#define OPTION(category) RSK_SHUFFLE_##category##_SOULS
#endif
extern "C" {
SaveContext gSaveContext{};
#ifdef TEST_MM
s32 Flags_GetRandoInf(s32 f) {
  return (gSaveContext.save.shipSaveInfo.rando.randoInf[f / 16] >> (f % 16)) &
         1;
}
#else
u8 optionValues[RSK_MAX]{};
u8 Randomizer_GetSettingValue(RandomizerSettingKey k) {
  return optionValues[k];
}
s32 Flags_GetRandomizerInf(RandomizerInf f) {
  return (gSaveContext.ship.randomizerInf[f / 16] >> (f % 16)) & 1;
}
#endif
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_BOTH;
int gComboVanillaMode = 0;
int MASK = 0;
#endif
}
#ifdef COMBO_BUILD
static int localGrants = 0, peerGrants = 0;
static int peerSlot = 1;
static uint32_t peerMask = ComboRando::SS_ALL;
static std::set<std::string> peerOwned;
static void Own(int id) {
#ifdef TEST_MM
  auto *s = Rando::SoulShuffle::Find(static_cast<RandoItemId>(id));
  int flag = s ? s->flag : SOUL_RI_TO_RANDO_INF(id);
  gSaveContext.save.shipSaveInfo.rando.randoInf[flag / 16] |= 1u << (flag % 16);
#else
  auto *s = Rando::SoulShuffle::Find(static_cast<RandomizerGet>(id));
  assert(s);
  gSaveContext.ship.randomizerInf[s->flag / 16] |= 1u << (s->flag % 16);
#endif
}
extern "C" void PREFIX(GrantSharedItem)(const char *name) {
  for (const auto &item : NativeSharedAdapter::Items())
    if (std::string(name) == item.name) {
      Own(item.id);
      ++localGrants;
      return;
    }
  assert(false);
}
struct PeerAdapter {
  static ComboRando::GameId Game() {
    return NativeSharedAdapter::Game() == ComboRando::GAME_OOT
               ? ComboRando::GAME_MM
               : ComboRando::GAME_OOT;
  }
  static int Slot() { return peerSlot; }
  static uint32_t Mask() { return peerMask; }
  static bool Owned(const std::string &name) {
    return peerOwned.contains(name);
  }
  static void GrantNative(const char *name) {
    if (peerOwned.insert(name).second)
      ++peerGrants;
  }
};
using PeerRuntime = ComboRando::SharedSoulRuntime<PeerAdapter>;
extern "C" COMBO_EXPORT void PEER(GrantSharedSoulForSlot)(const char *name,
                                                          int sourceSlot,
                                                          uint32_t sourceMask) {
  PeerRuntime::Receive(name, sourceSlot, sourceMask);
}
static int sReceiveDepth = 0, legacyCalls = 0;
static constexpr int FCI_NO_ITEM = 0;
static int FcCombo_ItemForNative(int item) {
#ifdef TEST_MM
  return item >= RI_SOUL_ENEMY_ALIEN && item <= RI_SOUL_ENEMY_WOLFOS
             ? 1
             : FCI_NO_ITEM;
#else
  return 1;
#endif
}
static void LegacyGrant(const char *) { ++legacyCalls; }
using FnGrantSharedItem = void (*)(const char *);
static FnGrantSharedItem ResolvePeerGrant() { return LegacyGrant; }
static void ShareNativeMagicFloor(int) {}
static void FleetShared_RequestPullFromPeer() {}
#define SPDLOG_WARN(...) ((void)0)
#define SPDLOG_INFO(...) ((void)0)
#ifdef TEST_MM
static constexpr int FC_COMBO_ITEM_COUNT = 2;
static int ChainAliasFor(int) { return 0; }
static bool ComboRpg_IsEnabled(int) { return false; }
struct FcItem {
  const char *ootName;
};
static FcItem gFcComboItems[] = {{""}, {"Soul of Wallmasters"}};
#include "shared_receipt_mm.inc"
#else
static int FleetCombo_ChainAliasFor(int) { return 0; }
static const char *FcCombo_PeerNameForItem(int) {
  return "RI_SOUL_ENEMY_WALLMASTER";
}
#include "shared_receipt.inc"
#endif
static void Reset() {
  gSaveContext = {};
  gSaveContext.fileNum = 1;
#ifdef TEST_MM
  gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
#else
  gSaveContext.ship.quest.id = QUEST_RANDOMIZER;
  std::fill(std::begin(optionValues), std::end(optionValues), 0);
#endif
  MASK = ComboRando::SS_ALL;
  gComboGameMode = COMBO_GAME_BOTH;
  gComboVanillaMode = 0;
  localGrants = peerGrants = legacyCalls = 0;
  sReceiveDepth = 0;
  peerSlot = 1;
  peerMask = ComboRando::SS_ALL;
  peerOwned.clear();
}
#endif
int main() {
#ifdef COMBO_BUILD
  struct Pair {
    uint32_t bit;
    int id;
    int option;
    const char *name;
    const char *peer;
  };
  Pair pairs[] = {
#ifdef TEST_MM
#define SHARED_SOUL(category, oot, mm, ootName, mmName)                        \
  {ComboRando::SS_##category, SOUL_ID(category, oot, mm), OPTION(category),    \
   mmName, ootName},
#else
#define SHARED_SOUL(category, oot, mm, ootName, mmName)                        \
  {ComboRando::SS_##category, SOUL_ID(category, oot, mm), OPTION(category),    \
   ootName, mmName},
#endif
#include "rando/SharedSoulCatalog.inc"
#undef SHARED_SOUL
  };
  for (const auto &p : pairs) {
#ifdef TEST_MM
    assert(std::string(Rando::SoulShuffle::SharedName(
               static_cast<RandoItemId>(p.id))) == p.name);
#else
    assert(std::string(Rando::SoulShuffle::SharedName(
               static_cast<RandomizerGet>(p.id))) == p.name);
#endif

    Reset();
    PREFIX(ShareSoulReceipt)(p.id);
    assert(localGrants == 0 && peerGrants == 0);
    OWN_OPTIONS[p.option] = 1;
    MASK = 0;
    PREFIX(ShareSoulReceipt)(p.id);
    assert(peerGrants == 0);
    MASK = p.bit;
    gComboVanillaMode = 1;
    PREFIX(ShareSoulReceipt)(p.id);
    assert(peerGrants == 0);
    gComboVanillaMode = 0;
    gComboGameMode = COMBO_GAME_OOT;
    PREFIX(ShareSoulReceipt)(p.id);
    assert(peerGrants == 0);
    gComboGameMode = COMBO_GAME_BOTH;
    peerSlot = 2; // A failed dormant load left another valid slot resident.
    PREFIX(ShareSoulReceipt)(p.id);
    assert(peerGrants == 0);
#ifdef TEST_MM
    FleetShared_OnNativeObtained(
        p.id); // Whole native callback must not take the old peer grant next.
    assert(peerGrants == 0 && legacyCalls == 0);
#endif
    peerSlot = 1;
    peerMask = ComboRando::SS_ALL & ~p.bit;
    PREFIX(ShareSoulReceipt)(p.id);
    assert(peerGrants == 0);
#ifdef TEST_MM
    FleetShared_OnNativeObtained(p.id);
    assert(legacyCalls == 0);
#endif
    peerMask = p.bit;
    assert(PREFIX(ShareSoulReceipt)(p.id));
    assert(peerOwned.contains(p.peer));
    // Receipt callback may precede native ownership; it must not grant its
    // source.
    assert(!NativeSharedAdapter::Owned(p.name));
    Own(p.id);
    int grants = localGrants;
    PREFIX(GrantSharedSoulForSlot)(p.name, 1, p.bit);
    assert(localGrants == grants);
    PREFIX(ShareSoulReceipt)(p.id);
    assert(localGrants == grants);
    auto state = nlohmann::json::parse(PREFIX(ExtractSharedSoulState)());
    assert(state["slot"] == 1 && state["mask"] == p.bit);
    Reset();
    OWN_OPTIONS[p.option] = 1;
    MASK = p.bit;
    nlohmann::json incoming = {{"slot", 1},
                               {"mask", p.bit},
                               {"owned", nlohmann::json::array({p.peer})}};
    auto text = incoming.dump();
    PREFIX(ApplySharedSoulState)(text.c_str());
    assert(NativeSharedAdapter::Owned(p.name));
    grants = localGrants;
    PREFIX(ApplySharedSoulState)(text.c_str());
    assert(localGrants == grants);
    Reset();
    OWN_OPTIONS[p.option] = 1;
    MASK = p.bit;
    incoming["slot"] = 2;
    text = incoming.dump();
    PREFIX(ApplySharedSoulState)(text.c_str());
    assert(localGrants == 0);
    PREFIX(ApplySharedSoulState)("{}");
    PREFIX(ApplySharedSoulState)("bad json");
    assert(localGrants == 0);
    incoming["slot"] = 1;
    incoming["mask"] = uint32_t(0);
    text = incoming.dump();
    PREFIX(ApplySharedSoulState)(text.c_str());
    assert(localGrants == 0);
    incoming["mask"] = p.bit;
    incoming["owned"] = {"unknown"};
    text = incoming.dump();
    PREFIX(ApplySharedSoulState)(text.c_str());
    assert(localGrants == 0);
    // Execute the actual native receiver export against a real native save.
    Reset();
    OWN_OPTIONS[p.option] = 1;
    MASK = p.bit;
    PREFIX(GrantSharedSoulForSlot)(p.name, 2, p.bit);
    assert(localGrants == 0);
    PREFIX(GrantSharedSoulForSlot)(p.name, 1, ComboRando::SS_ALL & ~p.bit);
    assert(localGrants == 0);
    PREFIX(GrantSharedSoulForSlot)(p.name, 1, p.bit);
    assert(localGrants == 1);
    PREFIX(GrantSharedSoulForSlot)(p.name, 1, p.bit);
    assert(localGrants == 1);
    // A rejected stale-slot peer is recovered from the persisted source flag
    // on a later matching-slot pull, without a volatile cross-slot item queue.
    Reset();
    OWN_OPTIONS[p.option] = 1;
    MASK = p.bit;
    peerSlot = 2;
    PREFIX(ShareSoulReceipt)(p.id);
    assert(peerGrants == 0);
    Own(p.id);
    auto pending = nlohmann::json::parse(PREFIX(ExtractSharedSoulState)());
    PeerRuntime::Apply(pending);
    assert(peerGrants == 0);
    peerSlot = 1;
    PeerRuntime::Apply(pending);
    assert(peerOwned.contains(p.peer));
    int delivered = peerGrants;
    PeerRuntime::Apply(pending);
    assert(peerGrants == delivered);
  }
#ifdef TEST_MM
  Reset();
  OWN_OPTIONS[OPTION(ENEMY)] = 1;
  MASK = ComboRando::SS_ENEMY;
  FleetShared_OnNativeObtained(RI_SOUL_ENEMY_WALLMASTER);
  assert(peerGrants == 2 && legacyCalls == 0);
  sReceiveDepth = 1;
  FleetShared_OnNativeObtained(RI_SOUL_ENEMY_WALLMASTER);
  assert(peerGrants == 2 && legacyCalls == 0);
  sReceiveDepth = 0;
  MASK = 0;
  FleetShared_OnNativeObtained(RI_SOUL_ENEMY_WALLMASTER);
  assert(legacyCalls == 1); // Preserve the disabled-category legacy path.
#else
  Reset();
  OWN_OPTIONS[OPTION(ENEMY)] = 1;
  FleetShared_OnNativeObtained(RG_MM_SOUL_WALLMASTER);
  assert(localGrants == 2 && peerGrants == 1 && legacyCalls == 0);
  assert(NativeSharedAdapter::Owned("Wallmaster Enemy Soul"));
  assert(NativeSharedAdapter::Owned("Floormaster Enemy Soul"));
  assert(PREFIX(ShareSoulReceipt)(RG_MM_SOUL_WALLMASTER));
  assert(localGrants == 2 && peerGrants == 1);
  sReceiveDepth = 1;
  FleetShared_OnNativeObtained(RG_MM_SOUL_WALLMASTER);
  assert(legacyCalls == 0);
  sReceiveDepth = 0;
  MASK = 0;
  FleetShared_OnNativeObtained(RG_MM_SOUL_WALLMASTER);
  assert(legacyCalls == 1);
#endif
#else
  assert(!PREFIX(ShareSoulReceipt)(0));
  PREFIX(GrantSharedSoulForSlot)("unknown", 1, 0);
  assert(std::string(PREFIX(ExtractSharedSoulState)()) == "{}");
  PREFIX(ApplySharedSoulState)("{}");
#endif
  puts("PASS complete native shared-soul TU: saved flags/options, all "
       "categories, receipts, reconciliation and legacy guards");
}
