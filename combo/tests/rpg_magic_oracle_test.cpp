#include "rando/CrossForeign.h"
#include "mods/nei_save.h"
#include "mods/combo_rpg.h"
#include <iostream>
#include <unordered_map>

// The harness extracts the production owned-item adapter, native magic grants,
// and logical magic gates. Other inventory actions are outside this test.
enum RandoItemId { RI_SINGLE_MAGIC, RI_DOUBLE_MAGIC, RI_OOT_EXT_MAGIC_CAPE };
enum { RANDO_INF_INFINITE_MAGIC };
enum { ITEM_BOW, ITEM_ARROW_FIRE, ITEM_ARROW_ICE, ITEM_ARROW_LIGHT };
static struct {
    struct {
        struct {
            struct {
                bool isMagicAcquired = false, isDoubleMagicAcquired = false;
                int16_t magic = 0;
            } playerData;
        } saveInfo;
        struct {
            NeiSaveData nei{};
        } shipSaveInfo;
    } save;
    int16_t magicCapacity = 0;
} gSaveContext;
static bool infiniteOwned;
static bool RandoCapacity_InfiniteFlag(int) {
    return infiniteOwned;
}
extern "C" NeiSaveData* Nei_Save() {
    return &gSaveContext.save.shipSaveInfo.nei;
}
#define MAGIC_NORMAL_METER 48
#define MAGIC_DOUBLE_METER 96
#define SET_WEEKEVENTREG(...) ((void)0)
#define HAS_ITEM(item) true
#define COMBO_EXPORT
#define CAN_BE_DEKU true
#define CAN_BE_GORON true

static const std::unordered_map<std::string, RandoItemId>& Combo_MM_SpoilerNameToItemId() {
    static const std::unordered_map<std::string, RandoItemId> ids = { { "Power of Magic", RI_SINGLE_MAGIC },
                                                                      { "Magic Upgrade", RI_DOUBLE_MAGIC },
                                                                      { "Magic Cape", RI_OOT_EXT_MAGIC_CAPE } };
    return ids;
}
#include "rpg_magic_oracle_production.inc"

static int failures;
static void Check(bool ok, const char* message) {
    if (!ok) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}
static void Reset() {
    gSaveContext = {};
    infiniteOwned = false;
}
static std::string Token(int required, int total, int level) {
    return "__combo_rpg_magic:" +
           nlohmann::json({ { "required", required }, { "total", total }, { "level", level } }).dump();
}
static void Apply(const nlohmann::json& owned) {
    Combo_MM_Rando_SetOwnedItems(owned.dump().c_str());
}

int main() {
    for (int capacity : { 0, 1, 2, 3, 4, 7, 8, 12, 48, 96, 100 }) {
        Reset();
        Apply(nlohmann::json::array({ Token(100, 100, capacity) }));
        Check(gSaveContext.save.saveInfo.playerData.magic == capacity,
              "oracle imports exact partial RPG capacity instead of a native tier");
        Check(bool(HAS_MAGIC) == (capacity >= 1), "lens and continuous magic need at least one unit");
        Check(bool(CAN_USE_DEKU_BUBBLE) == (capacity >= 2), "Deku bubble needs two units");
        Check(bool(CAN_GORON_SPIKE_ROLL) == (capacity >= 2), "Goron spike roll needs two units to start");
        Check(bool(CAN_USE_MAGIC_ARROW(FIRE)) == (capacity >= 4), "fire arrow uses its four-unit cost");
        Check(bool(CAN_USE_MAGIC_ARROW(ICE)) == (capacity >= 4), "ice arrow uses its four-unit cost");
        Check(bool(CAN_USE_MAGIC_ARROW(LIGHT)) == (capacity >= 8), "light arrow uses its eight-unit cost");
        // A reachable refill can restore capacity; current spent magic must not
        // remove logical capabilities from a live tracker or oracle query.
        gSaveContext.save.saveInfo.playerData.magic = 0;
        Check(bool(CAN_USE_MAGIC_ARROW(LIGHT)) == (capacity >= 8), "logic uses refillable capacity, not current magic");
        Apply(nlohmann::json::array({ "Magic Cape" }));
        Check(bool(CAN_USE_MAGIC_ARROW(FIRE)) == (capacity >= 2) && bool(CAN_USE_MAGIC_ARROW(ICE)) == (capacity >= 2) &&
                  bool(CAN_USE_MAGIC_ARROW(LIGHT)) == (capacity >= 4),
              "owned Magic Cape preserves the native half-cost arrow gates");
        Check(bool(CAN_USE_DEKU_BUBBLE) == (capacity >= 2) && bool(CAN_GORON_SPIKE_ROLL) == (capacity >= 2),
              "Magic Cape does not lower independent bubble and Goron startup gates");
        infiniteOwned = true;
        Check(bool(CAN_USE_MAGIC_ARROW(LIGHT)) == (capacity > 0) && bool(CAN_USE_DEKU_BUBBLE) == (capacity > 0),
              "Infinite Magic permits expensive actions after any magic capacity is acquired");
    }
    Reset();
    Apply(nlohmann::json::array({ Token(8, 96, 1) }));
    Check(gSaveContext.save.saveInfo.playerData.magic == 12 && CAN_USE_MAGIC_ARROW(LIGHT),
          "first default RPG pickup already permits MM light arrows before native 48");
    for (const char* native : { "Power of Magic", "Magic Upgrade" }) {
        const int expected = std::string(native) == "Power of Magic" ? 48 : 96;
        for (bool tokenFirst : { false, true }) {
            Reset();
            Apply(tokenFirst ? nlohmann::json::array({ Token(8, 96, 1), native })
                             : nlohmann::json::array({ native, Token(8, 96, 1) }));
            Check(gSaveContext.save.saveInfo.playerData.magic == expected && CAN_USE_MAGIC_ARROW(LIGHT),
                  "native fixed/starting capacity survives RPG oracle import in either order");
        }
        Reset();
        Apply(nlohmann::json::array({ native }));
        Check(gSaveContext.save.saveInfo.playerData.magic == expected && CAN_USE_MAGIC_ARROW(LIGHT),
              "ordinary native magic requires no RPG metadata");
    }
    if (!failures)
        std::cout << "PASS: real MM oracle import and logical costs at fractional and native capacities\n";
    return failures != 0;
}
