// Complete production GeneratePools, native placement helpers and real save
// types. The check catalogue, starting inventory, configuration and RNG are
// boundaries.
#include "FleetShipCombo/FleetShipCombo.h"
#include "Rando/Logic/Logic.h"
#include <cstdio>
#include <memory>

static int activeGame = -1;
static int randomCalls;
extern "C" {
#ifdef COMBO_BUILD
int gComboGameMode = COMBO_GAME_MM;
#endif
int gMMComboGoalHunt = 0, gMMComboGoalRequired = 0, gMMComboGoalPieces = -1;
int FleetShipCombo_GetActiveGame() { return activeGame; }
int32_t CVarGetInteger(const char *, int32_t fallback) { return fallback; }
s32 Ship_Random(s32 min, s32) {
  ++randomCalls;
  return min;
}
}
#include "unified_pool.inc"

static std::vector<RandoItemId> startingItems;
namespace Rando {
std::vector<RandoItemId> GetStartingItemsFromSave(RandoSaveInfo &) {
  return startingItems;
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

static int failures;
static void expect(bool ok, const char *message) {
  if (!ok && failures++ < 12)
    std::fprintf(stderr, "FAIL: %s\n", message);
}

int main() {
  using namespace Rando;
  auto addCheck = [](RandoCheckId id, RandoCheckType type, RandoItemId item) {
    StaticData::Checks[id] = {id,        "fixture", type, SCENE_20SICHITAI,
                              FLAG_NONE, 0,         item};
    Logic::Regions[(RandoRegionId)0].checks[id] = {[] { return true; }, "true"};
  };
  addCheck(RC_CLOCK_TOWN_SOUTH_CHEST_LOWER, RCTYPE_CHEST, RI_JUNK);
  const std::pair<RandoCheckId, RandoItemId> songs[] = {
      {RC_ANCIENT_CASTLE_OF_IKANA_BOSS, RI_SONG_ELEGY},
      {RC_BENEATH_THE_GRAVEYARD_SONG_OF_STORMS, RI_SONG_STORMS},
      {RC_CLOCK_TOWER_ROOF_SONG_OF_TIME, RI_SONG_TIME},
      {RC_DEKU_KINGS_CHAMBER_MONKEY, RI_SONG_SONATA},
      {RC_GIANTS_CHAMBER_OATH_TO_ORDER, RI_SONG_OATH},
      {RC_GORON_SHRINE_FULL_LULLABY, RI_PROGRESSIVE_LULLABY},
      {RC_GREAT_BAY_COAST_NEW_WAVE_BOSSA_NOVA, RI_SONG_NOVA},
      {RC_PATH_TO_GORON_VILLAGE_LULLABY_INTRO, RI_PROGRESSIVE_LULLABY},
      {RC_ROMANI_RANCH_EPONAS_SONG, RI_SONG_EPONA},
      {RC_SOUTHERN_SWAMP_SONG_OF_SOARING, RI_SONG_SOARING},
      {RC_STARTING_ITEM_SONG_OF_HEALING, RI_SONG_HEALING},
  };
  for (auto [id, item] : songs)
    addCheck(id, RCTYPE_SONG, item);
  for (int item = 0; item < RI_MAX; ++item)
    StaticData::Items[(RandoItemId)item].randoItemType = RITYPE_MAJOR;
  StaticData::Items[RI_JUNK].randoItemType = RITYPE_JUNK;
  int configurations = 0;
  for (bool paired : {false, true}) {
#ifdef COMBO_BUILD
    gComboGameMode = paired ? COMBO_GAME_BOTH : COMBO_GAME_MM;
    activeGame =
        0; // A legacy transport is not sufficient in modern single-game mode.
#else
    activeGame = paired ? 0 : -1;
#endif
    expect(bool(Combo_IsCombinedGame() || FleetCombo_UnifiedPoolActive()) ==
               paired,
           "actual modern/legacy OoT availability predicate");
    for (bool quest : {false, true})
      for (bool gear : {false, true})
        for (bool nei : {false, true})
          for (bool plentiful : {false, true})
            for (int songMode :
                 {RO_SONG_SHUFFLE_VANILLA, RO_SONG_SHUFFLE_ANYWHERE,
                  RO_SONG_SHUFFLE_SONG_LOCATIONS})
              for (bool startWith : {false, true}) {
                auto save = std::make_unique<RandoSaveInfo>();
                save->randoSaveOptions[RO_STARTING_HEALTH] = 3;
                save->randoSaveOptions[RO_SHUFFLE_OOT_QUEST] = quest;
                save->randoSaveOptions[RO_SHUFFLE_OOT_GEAR] = gear;
                save->randoSaveOptions[RO_SHUFFLE_NEI_ITEMS] = nei;
                save->randoSaveOptions[RO_PLENTIFUL_ITEMS] = plentiful;
                save->randoSaveOptions[RO_SHUFFLE_SONGS] = songMode;
                startingItems =
                    startWith
                        ? std::vector<
                              RandoItemId>{RI_OOT_SONG_COMMAND_MELODY,
                                           RI_OOT_SONG_FUGUE_OF_HOME,
                                           RI_OOT_SONG_BALLAD_OF_THE_HERO,
                                           RI_OOT_ABILITY_CHESTS,
                                           RI_OOT_NEI_CANE_SOMARIA_PLATFORM}
                        : std::vector<RandoItemId>{};
                std::vector<RandoCheckId> checks;
                std::vector<RandoItemId> items;
                randomCalls = 0;
                Logic::GeneratePools(*save, checks, items);
                auto count = [&](RandoItemId id) {
                  return std::count(items.begin(), items.end(), id);
                };
                const bool unified = FleetCombo_UnifiedPoolActive();
                const int density = plentiful ? 2 : 1;
                expect(count(RI_OOT_SONG_FUGUE_OF_HOME) == 0 &&
                           count(RI_OOT_SONG_BALLAD_OF_THE_HERO) == 0,
                       "unfinished songs never enter a new pool");
                expect(
                    count(RI_OOT_NEI_CANE_SOMARIA_PLATFORM) ==
                        (paired && nei ? 1 : 0),
                    "Trirod only enters pools with OoT gameplay, not MM-only");
                const int chestSupply = paired ? (unified ? 2 : int(gear)) : 0;
                expect(count(RI_OOT_ABILITY_CHESTS) ==
                           std::max(0, chestSupply - int(startWith)) * density,
                       "Open Chests supply requires OoT and respects starting "
                       "inventory/density");
                expect(count(RI_OOT_SONG_COMMAND_MELODY) == !startWith,
                       "Command Melody retains one source or its requested "
                       "starting copy");
                const int nativeSongCount =
                    songMode == RO_SONG_SHUFFLE_VANILLA
                        ? 0
                        : (plentiful &&
                                   songMode != RO_SONG_SHUFFLE_SONG_LOCATIONS
                               ? 2
                               : 1);
                expect(count(RI_SONG_EPONA) == nativeSongCount &&
                           count(RI_SONG_STORMS) == nativeSongCount,
                       "native Epona and Storms supply remains when unfinished "
                       "swaps are removed");
                for (auto item :
                     {RI_OOT_NEI_CANE_OF_SOMARIA, RI_OOT_NEI_CANE_SOMARIA_BLOCK,
                      RI_OOT_NEI_CANE_PACCI_FLIP, RI_OOT_NEI_CANE_PACCI_STONE,
                      RI_OOT_NEI_CANE_PACCI_ULTRAHAND,
                      RI_OOT_NEI_SLATE_RUNE_BOMB,
                      RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE,
                      RI_OOT_NEI_SLATE_RUNE_STASIS,
                      RI_OOT_NEI_SLATE_RUNE_CRYONIS, RI_OOT_NEI_LIGHT_ROD})
                  expect(count(item) == int(nei),
                         "functional NEI skills/runes retained and absent when "
                         "disabled");
                expect(count(RI_OOT_SONG_ZELDAS_LULLABY) ==
                           (unified || quest ? density : 0),
                       "functional OoT quest supply retains its existing "
                       "option/mode policy");
                expect(randomCalls ==
                           (songMode == RO_SONG_SHUFFLE_VANILLA
                                ? 1
                                : (songMode == RO_SONG_SHUFFLE_SONG_LOCATIONS
                                       ? 23
                                       : 12)),
                       "availability changes consume no extra RNG draws");
                ++configurations;
              }
  }
  std::printf("%s native item availability: %d option/mode configurations, %d "
              "failures\n",
              failures ? "FAIL" : "PASS", configurations, failures);
  return failures != 0;
}
