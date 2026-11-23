#include "FleetShipCombo/FleetShipCombo.h"
#include "Rando/Logic/Logic.h"
#include "tests/test_require.h"
#include <cstring>
#include <memory>
extern "C" {
int gComboGameMode = COMBO_GAME_MM;
int gMMComboGoalHunt = 0, gMMComboGoalRequired = 0, gMMComboGoalPieces = -1;
int requestMirror = 0;
int previewMirror = 0;
bool startWithMelody = false;
s32 Ship_Random(s32 min, s32) { return min; }
int FleetCombo_UnifiedPoolActive() { return 0; }
int32_t CVarGetInteger(const char *key, int32_t fallback) {
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
  StaticData::Checks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP] = {
      RC_LOST_WOODS_MAGIC_MIRROR_STUMP,
      "RC_LOST_WOODS_MAGIC_MIRROR_STUMP",
      RCTYPE_NPC,
      SCENE_LOST_WOODS,
      FLAG_NONE,
      0,
      RI_OOT_NEI_MAGIC_MIRROR};
  StaticData::Items[RI_OOT_NEI_MAGIC_MIRROR].randoItemType = RITYPE_MAJOR;
  StaticData::Items[RI_OOT_SONG_COMMAND_MELODY].randoItemType = RITYPE_MAJOR;
  // The native MM route still supports the standalone shuffle.
  Logic::Regions[RR_WOODS_OF_MYSTERY]
      .checks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP] = {[] { return true; }, "true"};
  for (int mode : {COMBO_GAME_MM, COMBO_GAME_BOTH}) {
    gComboGameMode = mode;
    for (int nei : {0, 1})
      for (int quest : {0, 1})
      for (int plentiful : {0, 1})
      for (bool startingSong : {false, true}) {
        startWithMelody = startingSong;
        requestMirror = 0;
        auto control = std::make_unique<RandoSaveInfo>();
        control->randoSaveOptions[RO_STARTING_HEALTH] = 3;
        control->randoSaveOptions[RO_SHUFFLE_NEI_ITEMS] = nei;
        control->randoSaveOptions[RO_SHUFFLE_OOT_QUEST] = quest;
        control->randoSaveOptions[RO_PLENTIFUL_ITEMS] = plentiful;
        std::vector<RandoCheckId> offChecks;
        std::vector<RandoItemId> offItems;
        Logic::GeneratePools(*control, offChecks, offItems);
        REQUIRE(std::count(offItems.begin(), offItems.end(), RI_OOT_SONG_COMMAND_MELODY) == !startingSong);
        REQUIRE(std::count(offChecks.begin(), offChecks.end(),
                           RC_LOST_WOODS_MAGIC_MIRROR_STUMP) == 0);
        REQUIRE(std::count(offItems.begin(), offItems.end(),
                           RI_OOT_NEI_MAGIC_MIRROR) == 0);
        for (int option : {0, 1})
          for (int request : {0, 1}) {
            requestMirror = request;
            auto save = std::make_unique<RandoSaveInfo>(*control);
            save->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] = option;
            std::vector<RandoCheckId> checks;
            std::vector<RandoItemId> items;
            Logic::GeneratePools(*save, checks, items);
            REQUIRE(std::count(items.begin(), items.end(), RI_OOT_SONG_COMMAND_MELODY) == !startingSong);
            const bool enabled = mode == COMBO_GAME_BOTH ? request : option;
            const bool fillable = enabled && mode == COMBO_GAME_MM;
            REQUIRE(save->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] == enabled);
            REQUIRE(std::count(checks.begin(), checks.end(),
                               RC_LOST_WOODS_MAGIC_MIRROR_STUMP) == fillable);
            REQUIRE(std::count(items.begin(), items.end(),
                               RI_OOT_NEI_MAGIC_MIRROR) == enabled);
            REQUIRE(checks.size() == offChecks.size() + fillable);
            REQUIRE(items.size() == offItems.size() + enabled);
            if (enabled && mode == COMBO_GAME_BOTH) {
              const auto& stump = save->randoSaveChecks[RC_LOST_WOODS_MAGIC_MIRROR_STUMP];
              REQUIRE(stump.shuffled && stump.skipped && stump.randoItemId == RI_JUNK);
            }
            // Live preview is authoritative even when the frozen generation
            // value and native solo preference disagree with it.
            previewMirror = !request;
            auto preview = std::make_unique<RandoSaveInfo>(*control);
            preview->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] = option;
            std::vector<RandoCheckId> previewChecks;
            std::vector<RandoItemId> previewItems;
            Logic::GeneratePools(*preview, previewChecks, previewItems, true);
            const bool previewEnabled =
                mode == COMBO_GAME_BOTH ? previewMirror : option;
            REQUIRE(preview->randoSaveOptions[RO_SHUFFLE_MAGIC_MIRROR] ==
                    previewEnabled);
            REQUIRE(std::count(previewChecks.begin(), previewChecks.end(),
                               RC_LOST_WOODS_MAGIC_MIRROR_STUMP) ==
                    (previewEnabled && mode == COMBO_GAME_MM));
            REQUIRE(std::count(previewItems.begin(), previewItems.end(),
                               RI_OOT_NEI_MAGIC_MIRROR) == previewEnabled);
            REQUIRE(requestMirror == request);
          }
      }
  }
  puts("PASS native MM generator: one shuffled Mirror, optional Combo stump, "
       "solo shuffle, NEI independence, Plentiful and shared live preview");
}
