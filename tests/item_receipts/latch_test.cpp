#include "mm/2s2h/Rando/Types.h"
#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include "combo/menu/ComboItemDrawABI.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
struct {
  int fileNum = 0;
} gSaveContext;
static uint64_t generation = 0;
namespace Rando::MiscBehavior {
uint64_t ComboRandoGen() { return generation; }
} // namespace Rando::MiscBehavior
// Native draw-cache dependencies: this receipt-identity fixture has no loaded
// resource owner or optional soul presentation. Keep executing the real cache.
namespace Ship {
struct ResourceManager {
  bool IsAltAssetsEnabled() const { return false; }
};
struct CrossRMRegistry {
  static ResourceManager* Get(const char*) { return nullptr; }
};
} // namespace Ship
namespace ComboSoulModels {
unsigned PreferenceMask() { return 0; }
} // namespace ComboSoulModels
struct ComboForeignDrawInfoOOT {
  bool ok = true, stateDependent = true, appearanceDependent = true,
       animOk = false;
  int neiShimmer = 0;
  int drawKind = CW_DRAW_KIND_SIMPLE;
  CwItemAnimDrawInfo anim{};
  std::string resolvedName = "Magic Meter";
};
enum class ComboForeignResolveOOT { Ok, NotReady, Unknown };
static ComboForeignDrawInfoOOT pending;
static bool ready = true;
ComboForeignResolveOOT
ComboFillForeignDrawInfoOOT(RandoCheckId, ComboForeignDrawInfoOOT &out,
                           const char *namedItem = nullptr) {
  if (!ready)
    return ComboForeignResolveOOT::NotReady;
  out = pending;
  if (namedItem)
    out.resolvedName = namedItem;
  return ComboForeignResolveOOT::Ok;
}
/* FOREIGN_CACHE */
int main() {
  const auto check = RC_CLOCK_TOWER_ROOF_OCARINA;
  assert(!ComboForeignLatchedNameOOT(check));
  ComboLatchForeignDrawOOT(check);
  assert(ComboForeignLatchedNameOOT(check) &&
         "live cosmetics hid the frozen receipt tier");
  assert(std::string(ComboForeignLatchedNameOOT(check)) == "Magic Meter");
  pending.resolvedName =
      "Enhanced Magic Meter"; // grant advanced the donor's save
  assert(ComboResolveForeignDrawInfoOOT(check)->resolvedName ==
         "Enhanced Magic Meter");
  assert(std::string(ComboForeignLatchedNameOOT(check)) ==
         "Magic Meter"); // cycle recollection
  ready = false;
  ComboLatchForeignDrawOOT(check);
  assert(std::string(ComboForeignLatchedNameOOT(check)) == "Magic Meter");
  ++generation;
  assert(!ComboForeignLatchedNameOOT(check));
  ready = true;
  ComboLatchForeignDrawOOT(check);
  assert(std::string(ComboForeignLatchedNameOOT(check)) ==
         "Enhanced Magic Meter");
  ++gSaveContext.fileNum;
  assert(!ComboForeignLatchedNameOOT(check));
  pending.stateDependent = pending.appearanceDependent = false;
  pending.resolvedName = "Cane of Somaria";
  ComboLatchForeignDrawOOT(check);
  assert(std::string(ComboForeignLatchedNameOOT(check)) == "Cane of Somaria");
  ComboLatchForeignDrawOOT(RC_UNKNOWN);
  assert(!ComboForeignLatchedNameOOT(RC_UNKNOWN));
  std::cout << "actual foreign latch: frozen receipt tiers, live appearance "
               "and slot/generation reset passed\n";
}
