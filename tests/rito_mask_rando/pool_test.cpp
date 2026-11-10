#include "FleetShipCombo/FleetShipCombo.h"
#include "Rando/Logic/Logic.h"
#include "tests/test_require.h"
#include <cstring>
#include <memory>
extern "C" {
int gComboGameMode = COMBO_GAME_MM;
int gMMComboGoalHunt = 0, gMMComboGoalRequired = 0, gMMComboGoalPieces = -1;
int requestRito=0,previewRito=0;
int requestMirror = 0;
int previewMirror = 0;
bool startWithMelody = false;
s32 Ship_Random(s32 min, s32) { return min; }
int FleetCombo_UnifiedPoolActive() { return 0; }
int32_t CVarGetInteger(const char *key, int32_t fallback) {
  if (!strcmp(key,"gCombo.Rando.ShuffleRitoMask")) return previewRito;
  if (!strcmp(key,"gRando.Generation.ShuffleRitoMask")) return requestRito;
  if (strcmp(key, "gCombo.Rando.ShuffleMagicMirror") == 0)
    return previewMirror;
  return strcmp(key, "gRando.Generation.ShuffleMagicMirror") == 0
             ? requestMirror
             : fallback;
}
}
namespace Rando {
std::vector<RandoItemId> GetStartingItemsFromSave(RandoSaveInfo &) {
  return startWithMelody ? std::vector<RandoItemId>{RI_OOT_SONG_COMMAND_MELODY} : std::vector<RandoItemId>{};
}
std::vector<RandoItemId> GetComputedStartingItems(RandoSaveInfo &) {
  return {};
}
std::vector<RandoCheckId> GetExcludedChecksFromConfig() { return {}; }
namespace StaticData {
std::map<RandoCheckId, RandoStaticCheck> Checks;
std::map<RandoItemId, RandoStaticItem> Items;
} // namespace StaticData
namespace Logic {
std::map<RandoRegionId, RandoRegion> Regions;
}
} // namespace Rando
int main() {
 using namespace Rando;
 StaticData::Items[RI_OOT_NEI_RITO_MASK].randoItemType=RITYPE_MAJOR;
 for(int mode:{COMBO_GAME_MM,COMBO_GAME_BOTH}) for(int nei:{0,1}) for(int plentiful:{0,1}) for(int native:{0,1}) for(int shared:{0,1}) for(bool preview:{false,true}) {
  gComboGameMode=mode; requestRito=shared;previewRito=!shared;
  auto save=std::make_unique<RandoSaveInfo>();save->randoSaveOptions[RO_STARTING_HEALTH]=3;
  save->randoSaveOptions[RO_SHUFFLE_NEI_ITEMS]=nei;save->randoSaveOptions[RO_PLENTIFUL_ITEMS]=plentiful;
  save->randoSaveOptions[RO_SHUFFLE_RITO_MASK]=native;
  std::vector<RandoCheckId> checks;std::vector<RandoItemId> items;
  Logic::GeneratePools(*save,checks,items,preview);
  const bool enabled=mode==COMBO_GAME_BOTH ? (preview?previewRito:requestRito) : native;
  REQUIRE(std::count(items.begin(),items.end(),RI_OOT_NEI_RITO_MASK)==enabled);
 }
 puts("PASS complete MM GeneratePools: Rito OFF/ON, solo/Combo snapshot/preview, NEI independent and Plentiful single copy");
}
