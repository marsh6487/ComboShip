#include "combo/menu/ComboItemDrawABI.h"
#include "soh/assets/textures/icon_item_static/icon_item_static.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
using s16 = int16_t;
using u8 = uint8_t;
using TexturePtr = void *;
enum RandomizerGet {
  RG_NONE,
  RG_CUSTOM,
  RG_PROGRESSIVE,
  RG_TIER,
  RG_NATIVE,
  RG_BAD,
  RG_BOTTLE_WITH_RED_POTION,
  RG_BOTTLE_WITH_GREEN_POTION,
  RG_BOTTLE_WITH_BLUE_POTION,
  RG_BOTTLE_WITH_FAIRY,
  RG_BOTTLE_WITH_FISH,
  RG_BOTTLE_WITH_BLUE_FIRE,
  RG_BOTTLE_WITH_BUGS,
  RG_BOTTLE_WITH_POE,
  RG_BOTTLE_WITH_BIG_POE,
  RG_COUNT
};
constexpr int ICON_SIZE_24 = 24, ICON_SIZE_32 = 32, ITEM_MEDALLION_FOREST = 102,
              ITEM_HEART_PIECE_2 = 122, ITEM_ROCS_FEATHER_SKIJER = 158;
void *gItemIcons[158] = {};
const char *customPath = "__OTR__textures/icon_item_custom/fire";
int customSize = 32, nativeId = 0, grants = 0;
std::map<RandomizerGet, const char *> bottleIcons;
struct GetItemEntry {
  int itemId;
};
namespace Rando::StaticData {
struct Item {
  RandomizerGet id;
  std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet *actual) {
    if (id == RG_PROGRESSIVE)
      *actual = RG_TIER;
    return std::make_shared<GetItemEntry>(GetItemEntry{nativeId});
  }
  Item CustomIcon(const char *path) {
    bottleIcons[id] = path;
    return *this;
  }
  bool HasCustomIcon() {
    return bottleIcons.contains(id) || id == RG_CUSTOM || id == RG_TIER;
  }
  const char *GetCustomIcon() {
    return bottleIcons.contains(id) ? bottleIcons.at(id)
           : id == RG_TIER
               ? (grants ? "__OTR__next-tier" : "__OTR__awarded-tier")
               : customPath;
  }
  int GetCustomIconSize() { return bottleIcons.contains(id) ? 32 : customSize; }
};
Item RetrieveItem(RandomizerGet id) { return {id}; }
} // namespace Rando::StaticData
/* OWNER_ICON */
static TexturePtr sMsgCustomIconTex = nullptr;
static s16 sMsgCustomIconWidth = 32, sMsgCustomIconHeight = 32;
static u8 sMsgCustomIconIA8 = 0;
/* STAGE_ICON */
namespace ComboRando {
constexpr int GAME_OOT = 0;
struct ForeignItem {
  int itemGame = 0;
  bool trap = false;
  std::string itemName = "progressive";
};
} // namespace ComboRando
using RandoCheckId = int;
ComboRando::ForeignItem foreign;
bool found = true, provider = true;
namespace Rando {
uint8_t ComboForeignMessageIcon(RandoCheckId);
namespace MiscBehavior {
const ComboRando::ForeignItem *MM_LookupForeign(RandoCheckId) {
  return found ? &foreign : nullptr;
}
} // namespace MiscBehavior
} // namespace Rando
int32_t DescribeIcon(const char *name, CwItemIconInfo *out) {
  return OOT_FillItemIconInfo(
      std::string(name) == "progressive"   ? RG_PROGRESSIVE
      : std::string(name) == "blue-potion" ? RG_BOTTLE_WITH_BLUE_POTION
                                           : RG_CUSTOM,
      out);
}
void *Combo_ResolveSym(const char *, const char *) {
  return provider ? (void *)DescribeIcon : nullptr;
}
const char *ComboInternRoutedPathOOT(const std::string &path) {
  static std::set<std::string> paths;
  return paths.insert(path).first->c_str();
}
/* CONSUMER_ICON */
int main() {
  Rando::StaticData::Item itemTable[RG_COUNT];
  for (int i = 0; i < RG_COUNT; i++)
    itemTable[i].id = static_cast<RandomizerGet>(i);
  /* BOTTLE_ICON_BINDINGS */
  const std::pair<RandomizerGet, const char *> bottles[] = {
      {RG_BOTTLE_WITH_RED_POTION, gItemIconBottlePotionRedTex},
      {RG_BOTTLE_WITH_GREEN_POTION, gItemIconBottlePotionGreenTex},
      {RG_BOTTLE_WITH_BLUE_POTION, gItemIconBottlePotionBlueTex},
      {RG_BOTTLE_WITH_FAIRY, gItemIconBottleFairyTex},
      {RG_BOTTLE_WITH_FISH, gItemIconBottleFishTex},
      {RG_BOTTLE_WITH_BLUE_FIRE, gItemIconBottleBlueFireTex},
      {RG_BOTTLE_WITH_BUGS, gItemIconBottleBugTex},
      {RG_BOTTLE_WITH_POE, gItemIconBottlePoeTex},
      {RG_BOTTLE_WITH_BIG_POE, gItemIconBottleBigPoeTex},
  };
  // A small randomizer itemId can collide with an unrelated native inventory
  // icon.
  nativeId = 12;
  gItemIcons[nativeId] = (void *)gItemIconArrowIceTex;
  CwItemIconInfo info{};
  for (auto [id, path] : bottles) {
    assert(OOT_FillItemIconInfo(id, &info) == 1 &&
           std::string(info.path) == path);
    assert(info.width == 32 && info.height == 32 && !info.isIA8);
  }
  provider = false;
  assert(Rando::ComboForeignMessageIcon(1) == 0xFE);
  provider = true;
  foreign.itemName = "blue-potion";
  assert(Rando::ComboForeignMessageIcon(1) == 0xF5);
  assert(std::string((char *)sMsgCustomIconTex) ==
         "__OTR__@oot:textures/icon_item_static/gItemIconBottlePotionBlueTex");
  foreign.itemName = "progressive";
  assert(OOT_FillItemIconInfo(RG_CUSTOM, &info) == 1 && info.width == 32 &&
         !info.isIA8);
  customSize = 24;
  info = {};
  assert(OOT_FillItemIconInfo(RG_CUSTOM, &info) == 1 && info.width == 24);
  customPath = "__OTR__textures/icon_item_static/gSongNoteTex";
  info = {};
  assert(OOT_FillItemIconInfo(RG_CUSTOM, &info) == 1 && info.width == 16 &&
         info.height == 24 && info.isIA8);
  customPath = "__OTR__textures/parameter_static/gOcarinaBtnIconATex";
  info = {};
  assert(OOT_FillItemIconInfo(RG_CUSTOM, &info) == 1 && info.width == 16 &&
         info.height == 16 && info.isIA8);
  customPath = "__OTR__textures/icon_item_static/gHeartPieceIcon2Tex";
  info = {};
  assert(OOT_FillItemIconInfo(RG_CUSTOM, &info) == 1 && info.width == 48 &&
         info.height == 48 && info.isIA8);
  gItemIcons[102] = (void *)"__OTR__textures/icon_item_24_static/medallion";
  nativeId = 102;
  info = {};
  assert(OOT_FillItemIconInfo(RG_NATIVE, &info) == 1 && info.width == 24 &&
         !info.isIA8);
  for (int scale : {0x53, 0x54}) {
    const char *path =
        scale == 0x53
            ? "__OTR__textures/icon_item_static/gItemIconScaleSilverTex"
            : "__OTR__textures/icon_item_static/gItemIconScaleGoldenTex";
    gItemIcons[scale] = (void *)path;
    nativeId = scale;
    info = {};
    assert(OOT_FillItemIconInfo(RG_NATIVE, &info) == 1 && info.width == 32 &&
           info.height == 32 && !info.isIA8);
    assert(info.path == path);
  }
  for (int bad : {-1, 158, 50000}) {
    nativeId = bad;
    info = {};
    assert(OOT_FillItemIconInfo(RG_BAD, &info) == 0);
  }
  assert(Rando::ComboForeignMessageIcon(1) == 0xF5);
  assert(std::string((char *)sMsgCustomIconTex) == "__OTR__@oot:awarded-tier");
  ++grants; // grant changes owner's inventory after staging
  assert(std::string((char *)sMsgCustomIconTex) == "__OTR__@oot:awarded-tier");
  const char *frozen = (char *)sMsgCustomIconTex;
  for (int i = 0; i < 1000; i++)
    ComboInternRoutedPathOOT("__OTR__@oot:" + std::to_string(i));
  assert(std::string(frozen) == "__OTR__@oot:awarded-tier");
  foreign.trap = true;
  assert(Rando::ComboForeignMessageIcon(1) == 0xFE);
  foreign.trap = false;
  found = false;
  assert(Rando::ComboForeignMessageIcon(1) == 0xFE);
  found = true;
  customPath = "missing-marker";
  foreign.itemName = "custom";
  assert(Rando::ComboForeignMessageIcon(1) == 0xFE);
  customPath = "__OTR__textures/icon_item_static/gSongNoteTex";
  foreign.itemName = "custom";
  assert(Rando::ComboForeignMessageIcon(1) == 0xF5 &&
         sMsgCustomIconWidth == 16 && sMsgCustomIconHeight == 24 &&
         sMsgCustomIconIA8);
  Message_StageCustomItemIcon((void *)"native", 32);
  assert(sMsgCustomIconWidth == 32 && sMsgCustomIconHeight == 32 &&
         !sMsgCustomIconIA8);
  Message_StageCustomItemIconEx((void *)"bad", 0, 24, 0);
  assert(!sMsgCustomIconTex);
  Message_StageCustomItemIconEx((void *)"bad", 24, 65, 0);
  assert(!sMsgCustomIconTex);
  Message_StageCustomItemIconEx((void *)"bad", 24, 24, 2);
  assert(!sMsgCustomIconTex);
  std::cout << "PASS bottle icon bindings, actual icon export, archive "
               "routing, pre-grant freeze, pointer lifetime, formats, "
               "dimensions and staging bounds\n";
}
