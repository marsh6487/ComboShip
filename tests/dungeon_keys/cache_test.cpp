#include "ComboItemDrawABI.h"
#include <cstdio>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
extern "C" {
#include "fixture_api.h"
}
#define COMBO_EXPORT
enum RandoItemId {
  RI_UNKNOWN = 0,
  RI_WOODFALL_SMALL_KEY,
  RI_WOODFALL_BOSS_KEY,
  RI_WOODFALL_MAP,
  RI_WOODFALL_COMPASS,
  RI_SNOWHEAD_SMALL_KEY,
  RI_SNOWHEAD_BOSS_KEY,
  RI_SNOWHEAD_MAP,
  RI_SNOWHEAD_COMPASS,
  RI_GREAT_BAY_SMALL_KEY,
  RI_GREAT_BAY_BOSS_KEY,
  RI_GREAT_BAY_MAP,
  RI_GREAT_BAY_COMPASS,
  RI_STONE_TOWER_SMALL_KEY,
  RI_STONE_TOWER_BOSS_KEY,
  RI_STONE_TOWER_MAP,
  RI_STONE_TOWER_COMPASS,
  RI_PROGRESSIVE_SWORD,
  RI_PROGRESSIVE_BOW,
  RI_PROGRESSIVE_BOMB_BAG,
  RI_PROGRESSIVE_WALLET,
  RI_PROGRESSIVE_MAGIC,
  RI_PROGRESSIVE_LULLABY,
  RI_TIME_PROGRESSIVE,
  RI_JUNK,
  RI_TRAP,
  RI_TRIFORCE_PIECE,
  RI_TRIFORCE_PIECE_PREVIOUS,
  RI_TEST_SWORD,
  RI_TEST_COMPASS
};
/* PRODUCTION_OWNER */
RandoItemId progression = RI_TEST_SWORD;
namespace Rando {
RandoItemId ConvertItem(RandoItemId) { return progression; }
RandoItemId CurrentJunkItem() { return RI_UNKNOWN; }
RandoItemId CurrentTrapItem() { return RI_UNKNOWN; }
namespace StaticData {
struct Item {
  int drawId;
  const char *name;
};
std::map<RandoItemId, Item> Items;
RandoItemId GetItemIdFromDisplayName(const char *name) {
  for (const auto &[id, item] : Items)
    if (!strcmp(name, item.name))
      return id;
  return RI_UNKNOWN;
}
RandoItemId GetItemIdFromName(const char *name) {
  return GetItemIdFromDisplayName(name);
}
struct Location {
  std::string GetName() { return "fixture check"; }
};
Location *GetLocation(int) {
  static Location loc;
  return &loc;
}
} // namespace StaticData
} // namespace Rando
namespace ItemGrantAudit {
struct Scope {
  Scope(const char *, int, int, bool) {}
};
} // namespace ItemGrantAudit
/* PRODUCTION_OPS */
/* PRODUCTION_CROSS */
int32_t MM_FillSongDrawInfo(RandoItemId, CwItemDrawInfo *) { return 0; }
int32_t MM_FillOpsDrawInfo(RandoItemId, CwItemDrawInfo *) { return 0; }
int32_t MM_FillSimpleDrawInfo(RandoItemId, CwItemDrawInfo *) { return 0; }
int32_t MM_FillEnemySoulDrawInfo(RandoItemId, CwItemDrawInfo *) { return 0; }
int32_t MM_FillGidAliasDrawInfo(RandoItemId, CwItemDrawInfo *) { return 0; }
bool MM_HasAnimDraw(RandoItemId) { return false; }
const char *gGiMoonsTearItemDL = "__OTR__unrelated/tear";
const char *gGiMoonsTearTexAnim = "__OTR__unrelated/tearTex";
const char *gGiFairyBottleEmptyDL = "__OTR__unrelated/bottle";
const char *gGiFairyBottleTexAnim = "__OTR__unrelated/bottleTex";
/* PRODUCTION_PRODUCER */
using RandomizerCheck = int;
constexpr int RC_UNKNOWN_CHECK = -1;
namespace ComboRando {
constexpr int GAME_MM = 1;
struct ForeignItem {
  int itemGame = GAME_MM;
  std::string itemName, fakeItemName;
  bool HasDisguise() const { return !fakeItemName.empty(); }
};
} // namespace ComboRando
ComboRando::ForeignItem foreign;
bool ready = true;
const ComboRando::ForeignItem *OOT_LookupForeign(int, const std::string &) {
  return ready ? &foreign : nullptr;
}
uint64_t generation = 1;
uint64_t OOT_ForeignMapGen() { return generation; }
struct {
  int fileNum = 0;
} gSaveContext;
void *Combo_ResolveSym(const char *, const char *symbol) {
  return !strcmp(symbol, "MM_GetItemDrawInfo") ? (void *)MM_GetItemDrawInfo
                                               : nullptr;
}
/* PRODUCTION_CACHE */
/* PRODUCTION_HOST */

void CheckRendered(const ComboForeignDrawInfo *info, int owner, int kind,
                   bool custom) {
  assert(info && info->ok && info->appearanceDependent && info->stateDependent);
  assert(info->drawKind == CW_DRAW_KIND_OPS);
  PlayState play{};
  TestReset();
  OOT_DrawForeignOps(&play, info);
  assert(testDepth == 0 && !testGray[0] && !testGray[1]);
  if (custom) {
    assert(testCount == 2 && testScales == 1 &&
           strstr(testPaths[0], "@mm:alt/objects/cor_mm_keys_poc2/"));
    assert(testColorAtDraw[0].r == (testColors && testChanged ? 10 + owner
                                    : kind == 1               ? 233
                                                              : 213));
    const int defaults[] = {236, 129, 99, 201};
    assert(testColorAtDraw[1].r ==
           (testEmblemChanged ? 210 + owner : defaults[owner]));
  } else {
    assert(testCount == (kind == 1 || kind == 3 ? 2 : 1) && testScales == 0);
    assert(strstr(testPaths[0], "@mm:objects/") &&
           !strstr(testPaths[0], "cor_mm_keys_poc2"));
    assert(testTint[0] == (bool)testColors);
    if (testColors)
      assert(testColorAtDraw[0].r == 10 + owner &&
             testColorAtDraw[0].a == (kind == 2   ? 96
                                      : kind == 3 ? 176
                                                  : 192));
    if (kind == 1 || kind == 3)
      assert(testTint[1] == (kind == 1 && testEmblemChanged));
    if (kind == 1 && testEmblemChanged)
      assert(testColorAtDraw[1].r == 210 + owner);
    if (kind == 3)
      assert(testSetup5 == 1 && !testTint[1]);
  }
}

int main() {
  const RandoItemId itemIds[4][4] = {
      {RI_WOODFALL_SMALL_KEY, RI_WOODFALL_BOSS_KEY, RI_WOODFALL_MAP,
       RI_WOODFALL_COMPASS},
      {RI_SNOWHEAD_SMALL_KEY, RI_SNOWHEAD_BOSS_KEY, RI_SNOWHEAD_MAP,
       RI_SNOWHEAD_COMPASS},
      {RI_GREAT_BAY_SMALL_KEY, RI_GREAT_BAY_BOSS_KEY, RI_GREAT_BAY_MAP,
       RI_GREAT_BAY_COMPASS},
      {RI_STONE_TOWER_SMALL_KEY, RI_STONE_TOWER_BOSS_KEY, RI_STONE_TOWER_MAP,
       RI_STONE_TOWER_COMPASS}};
  const char *names[4][4] = {{"WF key", "WF boss", "WF map", "WF compass"},
                             {"SH key", "SH boss", "SH map", "SH compass"},
                             {"GB key", "GB boss", "GB map", "GB compass"},
                             {"ST key", "ST boss", "ST map", "ST compass"}};
  for (int d = 0; d < 4; d++)
    for (int k = 0; k < 4; k++)
      Rando::StaticData::Items[itemIds[d][k]] = {k, names[d][k]};
  for (int d = 0; d < 4; d++)
    for (int k = 0; k < 4; k++) {
      const int rc = d * 4 + k;
      foreign.itemName = names[d][k];
      generation++;
      testAlt = false;
      testMissing = 0;
      testColors = 1;
      testChanged = 1;
      testEmblemChanged = 1;
      CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, false);
      ComboLatchForeignDraw(rc);
      assert(ComboForeignDrawCacheGet().map.at(rc).appearanceDependent);
      // Editing or Tab switching after the GI latch must immediately
      // re-resolve.
      testAlt = true;
      CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, k < 2);
      if (k < 2) {
        testChanged = 0;
        testEmblemChanged = 0;
        CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, true);
        testChanged = 1;
        testEmblemChanged = 1;
      }
      testAlt = false;
      testColors = 0;
      testEmblemChanged = 0;
      const auto *plain = ComboResolveForeignDrawInfo(rc);
      assert(plain && plain->drawKind == CW_DRAW_KIND_SIMPLE &&
             plain->appearanceDependent);
      assert(!strstr(plain->dls[0], "cor_mm_keys_poc2"));
      ComboLatchForeignDraw(rc);
      testColors = 1;
      testEmblemChanged = 1;
      CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, false);
      testAlt = true;
      for (int missing : {1, 2}) {
        testMissing = missing;
        CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, false);
      }
      testMissing = 0;
      CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, k < 2);
      if (k == 1) {
        testAlt = false;
        testColors = 0;
        testEmblemChanged = 1;
        CheckRendered(ComboResolveForeignDrawInfo(rc), d, k, false);
      }
    }
  // The original progressive latch must remain frozen across grant-state
  // changes.
  Rando::StaticData::Items[RI_PROGRESSIVE_SWORD] = {0, "Progressive sword"};
  Rando::StaticData::Items[RI_TEST_SWORD] = {0, "First sword"};
  Rando::StaticData::Items[RI_TEST_COMPASS] = {3, "Next tier"};
  foreign.itemName = "Progressive sword";
  generation++;
  progression = RI_TEST_SWORD;
  const int rc = 99;
  const auto *before = ComboResolveForeignDrawInfo(rc);
  assert(before && before->stateDependent && !before->appearanceDependent &&
         before->resolvedName == "First sword");
  ComboLatchForeignDraw(rc);
  progression = RI_TEST_COMPASS;
  const auto *frozen = ComboResolveForeignDrawInfo(rc);
  assert(frozen && frozen->count == 1 && !frozen->stateDependent &&
         frozen->resolvedName == "First sword");
  assert(!strcmp(ComboForeignLatchedName(rc), "First sword"));
  const char *retained = frozen->dls[0];
  for (int i = 0; i < 1000; i++)
    ComboInternRoutedPath(std::string("__OTR__@mm:") + std::to_string(i));
  assert(strstr(retained, "@mm:objects/object_gi_key/"));
  gSaveContext.fileNum = 1;
  assert(ComboResolveForeignDrawInfo(rc)->resolvedName == "Next tier");
  generation++;
  progression = RI_TEST_SWORD;
  assert(ComboResolveForeignDrawInfo(rc)->resolvedName == "First sword");
  puts("PASS actual MM export -> OoT resolver/cache/latch -> OPS replay: all "
       "16 dungeon GIs, independent palettes without assets, live edits/Alt "
       "after grant, retained progressive tiers and cache sweeps.");
}
