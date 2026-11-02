// Actual launcher read/write/create callbacks plus the actual MM native
// creator. ROM initialization and game serialization are service boundaries;
// file writes use the production atomic writer against a real temporary
// directory.
#include "combo/vanilla/ComboVanillaFile.h"
#include "combo/vanilla/ComboVanillaSave.h"
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
using nlohmann::json;
#define COMBO_RELEASE_VERSION "9.3.0"
namespace ComboRando {
enum { GAME_OOT = 0, GAME_MM = 1 };
}
static std::filesystem::path directory;
static std::mutex g_containerMutex;
static std::map<int, json> g_containerCache;
static std::map<int, ComboVanilla::Creation> g_vanillaCreations;
static bool ComboIsValidSlot(int slot) { return slot >= 0 && slot < 3; }
static auto ComboContainerPath(int slot) {
  return directory / (std::to_string(slot) + ".combosav");
}
static json &LoadOrCreateContainer(int slot) { return g_containerCache[slot]; }
static int g_MmSaveInMemorySlot = -1;
static int completionSlot = -1, hintsSlot = -1;
static void LoadComboCompletion(int slot) { completionSlot = slot; }
static void Combo_PushHintTrackerData(int slot) { hintsSlot = slot; }
static void PlayerName(unsigned char *name) {
  std::memcpy(name, "\x17\x0A\x16\x0E\x01\x02\x03\x04", 8);
}
static void (*SOH_GetCurrentPlayerName)(unsigned char *) = PlayerName;
static int (*MM_InitVanillaSaveFile)(int, const unsigned char *);
static void Discard();
void (*MM_DiscardVanillaSaveCreation)() = Discard;
/* HOST_FUNCTIONS */

static bool combined = true;
static int Combo_IsCombinedGame() { return combined; }
static int gComboVanillaMode = 0, gComboOwlBlobSlot = 42;
static int checkpoint = 8;
static void Combo_MM_LoadCrossDungeonCheckpoint(int value) {
  checkpoint = value;
}
constexpr int SAVETYPE_VANILLA = 0, CURRENT_SAVE_VERSION = 9;
struct NativeSave {
  bool hasTatl = false, isFirstCycle = false;
  int playerForm = 4;
  struct {
    int saveType = 0;
  } shipSaveInfo;
  struct {
    struct {
      unsigned char playerName[8]{};
      bool isMagicAcquired = false;
      int threeDayResetCount = 0;
    } playerData;
  } saveInfo;
};
static void to_json(json &j, const NativeSave &save) {
  j = {{"shipSaveInfo", {{"saveType", save.shipSaveInfo.saveType}}},
       {"hasTatl", save.hasTatl},
       {"isFirstCycle", save.isFirstCycle},
       {"playerForm", save.playerForm},
       {"saveInfo",
        {{"playerData",
          {{"playerName", save.saveInfo.playerData.playerName},
           {"isMagicAcquired", save.saveInfo.playerData.isMagicAcquired},
           {"threeDayResetCount",
            save.saveInfo.playerData.threeDayResetCount}}}}}};
}
struct {
  int fileNum = -1;
  NativeSave save;
} gSaveContext;
static void Sram_InitNewSave() { gSaveContext.save = NativeSave{}; }
static auto SaveManager_GetFileName(int oneBasedSlot) {
  return std::to_string(oneBasedSlot);
}
static void SaveManager_WriteSaveFile(const std::string &path,
                                      const json &document) {
  const int slot = std::stoi(path) - 1;
  Combo_WriteGameSave(1, slot, document.dump().c_str());
}
static auto gComboWriteGameSave = Combo_WriteGameSave;
/* MM_CREATOR */
#define COMBO_BUILD
#define SPDLOG_ERROR(...) ((void)0)
namespace ItemGrantAudit { struct Scope { Scope(const char*){} }; }
static int fileMode=1;
static int Combo_QueryVanillaMode(int) { return fileMode; }
/* MM_INVALIDATION */
static void Discard() { SaveManager_DiscardVanillaCreation(); }

static bool mmFails = false, mmThrows = false;
static int CreateMm(int slot, const unsigned char *name) {
  if (mmThrows) throw std::runtime_error("injected native creation failure");
  return mmFails ? -1 : SaveManager_InitVanillaForCombo(slot, name);
}
static json ReadDisk(int slot) {
  json document;
  std::ifstream(ComboContainerPath(slot)) >> document;
  return document;
}
static json Old() {
  return {{"comboVersion", 1},
          {"comboRelease", COMBO_RELEASE_VERSION},
          {"slot", 0},
          {"combo", {{"rando", {{"masterSeed", 11}}}}},
          {"oot", {{"previous", true}}},
          {"mm", {{"previous", true}}}};
}
static json NativeOot() {
    return { { "fileType", 0 }, { "version", 1 },
             { "sections", { { "base", { { "version", 4 }, { "data", {
                 { "playerName", json::array({23,10,22,14,1,2,3,4}) }, { "healthCapacity", 48 }
             } } } } } } };
}

int main(int argc, char **argv) {
  assert(argc == 2);
  directory = argv[1];
  MM_InitVanillaSaveFile = CreateMm;
  g_containerCache[0] = Old();
  assert(FlushContainer(0));
  const auto previous = ReadDisk(0);
  assert(Combo_QueryFileMode(0) == 0);
  assert(!Combo_VanillaCreation(-1, 0) && !Combo_VanillaCreation(3, 0));
  assert(Combo_VanillaCreation(0, 0));
  assert(Combo_QueryFileMode(0) == 1);
  assert(!Combo_VanillaCreation(0, 0));
  Combo_WriteGameSave(0, 0, NativeOot().dump().c_str());
  assert(ReadDisk(0) == previous && g_containerCache[0] == Old());
  assert(std::string(Combo_ReadGameSave(1, 0))
             .empty()); // Old MM never leaks into a fresh pair.
  assert(Combo_VanillaCreation(0, 1));
  const auto created = ReadDisk(0);
  assert(ComboVanilla::FileMode(created) == ComboVanilla::Mode::Vanilla);
  assert(completionSlot == 0 && hintsSlot == 0 && g_MmSaveInMemorySlot == 0);
  const auto &mm = created["mm"]["newCycleSave"]["save"];
  assert(mm["hasTatl"] == false && mm["isFirstCycle"] == false &&
         mm["playerForm"] == 4);
  assert(mm["saveInfo"]["playerData"]["isMagicAcquired"] == false);
  assert(mm["saveInfo"]["playerData"]["threeDayResetCount"] == 0);
  assert(mm["saveInfo"]["playerData"]["playerName"] ==
         json::array({23, 10, 22, 14, 1, 2, 3, 4}));
  assert(checkpoint == -1 && gComboOwlBlobSlot == -1);

  assert(SaveManager_LoadFailedForCombo(-77)==-77 && gSaveContext.fileNum==0xFF);
  fileMode=-1; gSaveContext.fileNum=2;gSaveContext.save.shipSaveInfo.saveType=1;
  assert(LoadIdentityForTests(2)==-6);
  assert(gSaveContext.fileNum==0xFF&&gSaveContext.save.shipSaveInfo.saveType==0);
  fileMode=1;
  for (int failure = 0; failure < 4; ++failure) {
    g_containerCache[0] = Old();
    assert(FlushContainer(0));
    const auto before = ReadDisk(0);
    assert(Combo_VanillaCreation(0, 0));
    if (failure != 0)
      Combo_WriteGameSave(0, 0, NativeOot().dump().c_str());
    mmFails = failure == 1;
    mmThrows = failure == 3;
    if (failure == 2)
      std::filesystem::create_directory(ComboContainerPath(0).string() +
                                        ".temp");
    assert(!Combo_VanillaCreation(0, 1));
    assert(ReadDisk(0) == before && g_containerCache[0] == Old());
    assert(g_MmSaveInMemorySlot == -1 && !g_vanillaCreations[0].Active());
    assert(gSaveContext.fileNum==0xFF&&gComboVanillaMode==0);
    if (failure == 2)
      std::filesystem::remove(ComboContainerPath(0).string() + ".temp");
  }
  mmFails = false;
  std::cout << "PASS actual launcher/MM creation callbacks: native first-cycle "
               "state, names, staged IO, complete pair and disk rollback\n";
}
