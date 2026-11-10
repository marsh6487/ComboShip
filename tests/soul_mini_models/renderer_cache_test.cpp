#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include "ComboItemDrawABI.h"
static unsigned preferences;
namespace ComboSoulModels { unsigned PreferenceMask() { return preferences; } }
namespace Ship {
struct ResourceManager { bool alt = false; bool IsAltAssetsEnabled() { return alt; } };
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const char*) { static auto rm = std::make_shared<ResourceManager>(); return rm; }
};
}
static struct { int fileNum = 1; } gSaveContext;
uint64_t OOT_ForeignMapGen() { return 1; }
namespace Rando::MiscBehavior { uint64_t ComboRandoGen() { return 1; } }
using RandomizerCheck = int;
using RandoCheckId = int;
struct ComboForeignDrawInfo {
    bool ok = false, stateDependent = false, animOk = false;
    int drawKind = CW_DRAW_KIND_SIMPLE;
    CwItemAnimDrawInfo anim{};
};
using ComboForeignDrawInfoOOT = ComboForeignDrawInfo;
enum class ComboForeignResolve { Ok, Unknown, NotReady };
enum class ComboForeignResolveOOT { Ok, Unknown, NotReady };
static bool unavailable;
static int fills;
static void fill(ComboForeignDrawInfo& info) {
    ++fills;
    info.ok = !unavailable;
    info.drawKind = preferences ? CW_DRAW_KIND_SHUFFLED_SOUL : CW_DRAW_KIND_SIMPLE;
    info.animOk = !preferences;
    info.anim.fallbackTokenPath = "__OTR__token";
}
ComboForeignResolve ComboFillForeignDrawInfo(int, int, ComboForeignDrawInfo& info) {
    fill(info); return unavailable ? ComboForeignResolve::Unknown : ComboForeignResolve::Ok;
}
ComboForeignResolveOOT ComboFillForeignDrawInfoOOT(int, ComboForeignDrawInfoOOT& info, const char*) {
    fill(info); return unavailable ? ComboForeignResolveOOT::Unknown : ComboForeignResolveOOT::Ok;
}
#include "renderer_cache_production.inc"
int main() {
    for (int receivingHost = 0; receivingHost < 2; ++receivingHost) {
        auto resolve = [&]() -> const ComboForeignDrawInfo* {
            return receivingHost ? ComboResolveForeignDrawInfoOOT(7) : ComboResolveForeignDrawInfo(7);
        };
        auto& map = receivingHost ? ComboForeignDrawCacheOOTGet().map : ComboForeignDrawCacheGet().map;
        map.clear(); preferences = 0; unavailable = false; fills = 0;
        assert(resolve()->animOk); assert(resolve()->animOk && fills == 1);
        // A held, unrelated progressive tier remains latched across soul choices.
        map[100].ok = true; map[100].drawKind = CW_DRAW_KIND_SIMPLE;
        preferences = 1; assert(!resolve()->animOk && fills == 2);
        preferences = 2; assert(!resolve()->animOk && fills == 3);
        preferences = 0; assert(resolve()->animOk && fills == 4);
        assert(map.contains(100));
        Ship::CrossRMRegistry::Get("owner")->alt = true;
        assert(resolve()->animOk && fills == 5);
        assert(map.contains(100));
        map.clear(); unavailable = true; assert(!resolve()); assert(!resolve());
        int before = fills;
        unavailable = false; preferences = 1;
        assert(resolve() && fills == before + 1);
        assert(!resolve()->animOk);
        preferences = 0; assert(resolve()->animOk);
        Ship::CrossRMRegistry::Get("owner")->alt = false;
    }
    std::cout << "Both foreign consumer caches: positive/negative immediate preference switches, owner Alt selection and unrelated held tier preservation passed\n";
}
