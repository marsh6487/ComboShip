// Complete production entrypoint lifecycle/schema. Fill/replay are explicit
// service boundaries here, not a replacement world graph or native seed proof.
#include "ComboGameMode.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#define COMBO_EXPORT
bool gComboHeadlessRando = true;
extern "C" {
int gComboGameMode = COMBO_GAME_OOT;
}
uint64_t sComboRandoSeed = 0;
bool sComboRandoSeedSet = false, sComboContextPrepared = false;
using RandomizerCheck = int;
constexpr int RG_NONE = 0;
int fills = 0, replays = 0, resets = 0, preps = 0, stamps = 0, fillResult = 1;
namespace Rando::RustyDoorKeys {
void PrepareNewGeneration() { ++stamps; }
} // namespace Rando::RustyDoorKeys
bool replayResult = true;
struct LogicBoundary {
  void Reset() { ++resets; }
};
struct ItemLocationBoundary {
  int GetPlacedRandomizerGet() const { return 1; }
  bool IsExcluded() const { return false; }
};
struct ContextBoundary {
  std::vector<int> allLocations = {1};
  LogicBoundary logic;
  ItemLocationBoundary location;
  void SetSeed(uint32_t) {}
  void SetSeedString(std::string) {}
  void SetSpoilerLoaded(bool) {}
  void ClearItemLocations() {}
  void HintReset() {}
  LogicBoundary *GetLogic() { return &logic; }
  ItemLocationBoundary *GetItemLocation(int) { return &location; }
};
struct OTRGlobals {
  static OTRGlobals *Instance;
  std::shared_ptr<ContextBoundary> gRandoContext =
      std::make_shared<ContextBoundary>();
} globals;
OTRGlobals *OTRGlobals::Instance = &globals;
namespace Rando::StaticData {
struct LocationBoundary {
  std::string GetName() const { return "Native check"; }
  int GetScene() const { return 7; }
} location;
struct ItemBoundary {
  ItemBoundary &GetName() { return *this; }
  std::string GetEnglish() const { return "Native item"; }
} item;
LocationBoundary *GetLocation(int) { return &location; }
ItemBoundary &RetrieveItem(int) { return item; }
} // namespace Rando::StaticData
void ClearProgress() {}
void SOH_PrepRandoContext() {
  assert(stamps == preps + 1);
  ++preps;
  sComboContextPrepared = true;
}
int Fill() {
  ++fills;
  return fillResult;
}
bool CheckBeatable() {
  ++replays;
  assert(resets == replays);
  return replayResult;
}
const char *SOH_DumpRandoSettings() { return "{\"nativeSetting\":1}"; }
const char *SOH_DumpDungeonLayouts() { return "{\"masterQuestDungeons\":[]}"; }
#include "soh/soh/FleetShipCombo/SoloRandoHeadless.inc"
int main() {
  using nlohmann::json;
  gComboGameMode = COMBO_GAME_BOTH;
  auto r = json::parse(SOH_FillSoloRandoSeed(17));
  assert(!r["success"].get<bool>() && fills == 0 && preps == 0);
  gComboGameMode = COMBO_GAME_OOT;
  gComboHeadlessRando = false;
  r = json::parse(SOH_FillSoloRandoSeed(18));
  assert(!r["success"].get<bool>() && fills == 0);
  gComboHeadlessRando = true;
  r = json::parse(SOH_FillSoloRandoSeed(19));
  assert(r["success"].get<bool>() && fills == 1 && replays == 1);
  assert(r["placements"]["Native check"] == "Native item" &&
         r["checks"]["Native check"]["scene"] == 7 && r["seed"] == 19 &&
         sComboRandoSeed == 19 && sComboRandoSeedSet);
  fillResult = -1;
  r = json::parse(SOH_FillSoloRandoSeed(20));
  assert(!r["success"].get<bool>() && fills == 2 && replays == 1);
  fillResult = 1;
  replayResult = false;
  r = json::parse(SOH_FillSoloRandoSeed(21));
  assert(!r["success"].get<bool>() && fills == 3 && replays == 2);
}
