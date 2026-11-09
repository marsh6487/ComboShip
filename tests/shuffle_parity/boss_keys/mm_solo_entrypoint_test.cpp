// Native SaveContext types with complete production entrypoint. Generation and
// graph services are boundaries; only lifecycle/transaction/schema are claimed.
#include "Rando/Logic/Logic.h"
#include "Rando/Logic/SeedGeneration.h"
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#define COMBO_EXPORT
extern "C" {
SaveContext gSaveContext{};
PlayState *gPlayState = nullptr;
int gComboGameMode = COMBO_GAME_MM;
int gFcCombo_SuppressRecord = 0;
}
struct OTRGlobals {
  static void *Instance;
};
void *OTRGlobals::Instance = nullptr;
uint64_t sMMComboRandoSeed = 77;
bool sMMComboRandoSeedSet = true, sMM_OracleActive = false;
SaveContext previousSave{};
std::unordered_map<uint32_t, uint16_t> sMMComboCheckPrices;
int resets = 0, restores = 0, generations = 0;
bool failGeneration = false;
namespace Rando {
bool gComboDormantGive = false;
void PrepareNewRandomizedSave() {
  assert(gPlayState == nullptr && gComboDormantGive &&
         gFcCombo_SuppressRecord == 9);
}
std::string GenerateSeedInSaveContext(std::string s) {
  ++generations;
  sMMComboCheckPrices[RC_CURIOSITY_SHOP_SPECIAL_ITEM] = 199;
  sMMComboCheckPrices[RC_BOMB_SHOP_ITEM_03] = 88;
  if (failGeneration)
    throw std::runtime_error("native failure boundary");
  gSaveContext.save.shipSaveInfo.rando.finalSeed = 1234;
  auto &paid = RANDO_SAVE_CHECKS[RC_CURIOSITY_SHOP_SPECIAL_ITEM];
  paid.randoItemId = RI_MASK_ALL_NIGHT;
  paid.shuffled = true;
  paid.price = 17;
  auto &free = RANDO_SAVE_CHECKS[RC_BOMB_SHOP_ITEM_03];
  free.randoItemId = RI_PROGRESSIVE_BOMB_BAG;
  free.shuffled = true;
  free.price = 0;
  return s;
}
namespace StaticData {
std::map<RandoCheckId, RandoStaticCheck> Checks;
const std::string &GetCheckDisplayName(RandoCheckId id) {
  static const std::string paid = "native paid check",
                           free = "native free check";
  return id == RC_CURIOSITY_SHOP_SPECIAL_ITEM ? paid : free;
}
const std::string &GetItemDisplayName(RandoItemId) {
  static const std::string s = "native item";
  return s;
}
} // namespace StaticData
} // namespace Rando
extern "C" int32_t CVarGetInteger(const char *, int32_t f) { return f; }
extern "C" void Sram_InitNewSave() {
  gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
}
void MM_SetComboRandoSeed(uint64_t s) {
  sMMComboRandoSeed = s;
  sMMComboRandoSeedSet = true;
}
void Combo_MM_Rando_Reset() {
  ++resets;
  previousSave = gSaveContext;
  memset(&gSaveContext, 0, sizeof(gSaveContext));
  sMM_OracleActive = true;
}
void Combo_MM_Rando_Restore() {
  ++restores;
  gSaveContext = previousSave;
  sMM_OracleActive = false;
}
const char *MM_DumpRandoSettings() { return "{}"; }
#include "mm/2s2h/Rando/Logic/SoloRandoHeadless.inc"
int main() {
  using nlohmann::json;
  Rando::StaticData::Checks[RC_CURIOSITY_SHOP_SPECIAL_ITEM] = {
      RC_CURIOSITY_SHOP_SPECIAL_ITEM,
      "RC_CURIOSITY_SHOP_SPECIAL_ITEM",
      RCTYPE_SHOP,
      SCENE_AYASHIISHOP,
      FLAG_NONE,
      0,
      RI_MASK_ALL_NIGHT};
  Rando::StaticData::Checks[RC_BOMB_SHOP_ITEM_03] = {RC_BOMB_SHOP_ITEM_03,
                                                     "RC_BOMB_SHOP_ITEM_03",
                                                     RCTYPE_SHOP,
                                                     SCENE_BOMYA,
                                                     FLAG_NONE,
                                                     0,
                                                     RI_PROGRESSIVE_BOMB_BAG};
  sMMComboCheckPrices = {{RC_CURIOSITY_SHOP_SPECIAL_ITEM, 41},
                         {RC_BOMB_SHOP_ITEM_03, 99}};
  const auto originalPrices = sMMComboCheckPrices;
  gComboGameMode = COMBO_GAME_BOTH;
  auto r = json::parse(MM_FillSoloRandoSeed(2027));
  assert(!r["success"].get<bool>() && resets == 0);
  gComboGameMode = COMBO_GAME_MM;
  OTRGlobals::Instance = &resets;
  r = json::parse(MM_FillSoloRandoSeed(2027));
  assert(!r["success"].get<bool>() && resets == 0);
  OTRGlobals::Instance = nullptr;
  sMM_OracleActive = true;
  r = json::parse(MM_FillSoloRandoSeed(2027));
  assert(!r["success"].get<bool>() && resets == 0);
  sMM_OracleActive = false;
  memset(&gSaveContext, 0, sizeof(gSaveContext));
  gSaveContext.save.day = 3;
  auto before = gSaveContext;
  auto *play = reinterpret_cast<PlayState *>(uintptr_t(0x1234));
  gPlayState = play;
  gFcCombo_SuppressRecord = 9;
  r = json::parse(MM_FillSoloRandoSeed(2027));
  assert(r["success"].get<bool>() && r["finalSeed"] == 1234 &&
         generations == 1 && resets == 1 && restores == 1);
  assert(r["prices"]["native paid check"] == 17 &&
         r["prices"]["native free check"] == 0);
  assert(sMMComboCheckPrices == originalPrices);
  assert(!memcmp(&before, &gSaveContext, sizeof(gSaveContext)) &&
         gPlayState == play && gFcCombo_SuppressRecord == 9 &&
         !Rando::gComboDormantGive && sMMComboRandoSeed == 77 &&
         sMMComboRandoSeedSet && !sMM_OracleActive);
  failGeneration = true;
  r = json::parse(MM_FillSoloRandoSeed(2028));
  assert(!r["success"].get<bool>() && generations == 2 && restores == 2);
  assert(sMMComboCheckPrices == originalPrices);
  assert(!memcmp(&before, &gSaveContext, sizeof(gSaveContext)) &&
         gPlayState == play && gFcCombo_SuppressRecord == 9 &&
         !Rando::gComboDormantGive && sMMComboRandoSeed == 77);
}
