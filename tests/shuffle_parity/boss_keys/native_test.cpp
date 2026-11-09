#include "FleetShipCombo/FleetShipCombo.h"
#include "Rando/Logic/Logic.h"
#include "Rando/KeyRings.h"
#include "Rando/StaticData/StaticData.h"
#include "rando/CrossWorldRando.h"
#include "overlays/actors/ovl_En_Sth/z_en_sth.h"
#include <cstring>
#include <memory>
#include <random>
#include <stdexcept>

#define REQUIRE(x) do { if (!(x)) throw std::runtime_error("FAIL: " #x); } while (0)
using J = nlohmann::json;
static std::map<std::string, int> cvars;
static std::vector<RandoCheckId> excluded;
static std::vector<RandoItemId> configuredStart;
static std::mt19937 rng;
static bool throwAtDraw = false, throwAtHint = false;
static int confinedDraws = 0;
extern "C" {
SaveContext gSaveContext{};
int gComboGameMode = COMBO_GAME_BOTH;
int gMMComboGoalHunt = 0, gMMComboGoalRequired = 0, gMMComboGoalPieces = -1;
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    auto it = cvars.find(key); return it == cvars.end() ? fallback : it->second;
}
void CVarSetInteger(const char* key, int32_t value) { cvars[key] = value; }
void CVarSetString(const char*, const char*) {}
void Ship_Random_Seed(uint32_t seed) { rng.seed(seed); }
s32 Ship_Random(s32 min, s32 max) {
    if (throwAtDraw && RANDO_SAVE_OPTIONS[RO_PLACEMENT_BOSS_KEYS] == RO_DUNGEON_ITEM_OWN_DUNGEON &&
        ++confinedDraws > 1) throw std::runtime_error("injected confinement failure");
    return max > min ? min + rng() % (max - min) : min;
}
int FleetShipCombo_GetActiveGame() { return -1; }
}
#include "unified.inc"
namespace Rando {
std::vector<RandoCheckId> GetExcludedChecksFromConfig() { return excluded; }
void SetExcludedChecksInConfig(std::vector<RandoCheckId>& items) { excluded = items; }
std::vector<RandoItemId> GetStartingItemsFromConfig() { return configuredStart; }
void SetStartingItemsInConfig(std::vector<RandoItemId>& items) { configuredStart = items; }
std::vector<RandoItemId> ComboJunkPool() { return {RI_RUPEE_BLUE}; }
#include "starting.inc"
namespace StaticData {
#include "catalog.inc"
// Naming/hint rendering are boundaries unrelated to placement. Native reversible
// item names above are retained; check tokens give exact physical catalog IDs.
const std::string& GetCheckDisplayName(RandoCheckId id) {
    static std::map<RandoCheckId, std::string> names;
    return names.try_emplace(id, Checks.at(id).name).first->second;
}
RandoCheckId GetCheckIdFromName(const char* name) {
    for (auto& [id, check] : Checks) if (!strcmp(name, check.name)) return id;
    return RC_UNKNOWN;
}
std::vector<std::string> GetTrickNames(RandoItemId) { return {}; }
std::string GetLocationNameForHint(RandoCheckId, bool) {
    if (throwAtHint) throw std::runtime_error("injected hint failure");
    return "fixture";
}
}
namespace Logic {
std::map<RandoRegionId, RandoRegion> Regions;
int DungeonItemToDungeon(RandoItemId);
int SceneIdToDungeon(SceneId);
}
}
static bool sMMComboRandoSeedSet = true;
static unsigned sMMComboRandoSeed = 1;
static std::unordered_map<uint32_t, uint16_t> sMMComboCheckPrices;
std::unordered_map<RandoItemId, u32> riToWeight;
std::unordered_map<RandoItemType, u32> itemTypeToWeight;
#define COMBO_EXPORT
#include "adapter.inc"

static const RandoItemId keys[] = {RI_WOODFALL_BOSS_KEY, RI_SNOWHEAD_BOSS_KEY,
                                  RI_GREAT_BAY_BOSS_KEY, RI_STONE_TOWER_BOSS_KEY};
static const char* ownCvar = "gRando.Options.RO_PLACEMENT_BOSS_KEYS";
static std::unique_ptr<RandoSaveInfo> Snapshot() {
    auto copy = std::make_unique<RandoSaveInfo>();
    memcpy(copy.get(), &gSaveContext.save.shipSaveInfo.rando, sizeof(*copy));
    return copy;
}
static bool Unchanged(const RandoSaveInfo& copy) {
    return !memcmp(&copy, &gSaveContext.save.shipSaveInfo.rando, sizeof(copy));
}
static int KeyCount(const J& rows, const char* field) {
    int count = 0;
    for (const auto& row : rows) for (auto key : keys)
        count += row.at(field) == Rando::StaticData::GetItemDisplayName(key);
    return count;
}
static bool OwnPhysicalDungeon(RandoItemId key, SceneId scene) {
    switch (key) {
        case RI_WOODFALL_BOSS_KEY: return scene == SCENE_MITURIN || scene == SCENE_MITURIN_BS;
        case RI_SNOWHEAD_BOSS_KEY: return scene == SCENE_HAKUGIN || scene == SCENE_HAKUGIN_BS;
        case RI_GREAT_BAY_BOSS_KEY: return scene == SCENE_SEA || scene == SCENE_SEA_BS;
        case RI_STONE_TOWER_BOSS_KEY:
            return scene == SCENE_INISIE_N || scene == SCENE_INISIE_R || scene == SCENE_INISIE_BS;
        default: return false;
    }
}
static int VerifyFixed(const J& dump) {
    int count = 0;
    for (const auto& row : dump.at("fixed")) {
        auto id = Rando::StaticData::GetItemIdFromDisplayName(row.at("item").get<std::string>().c_str());
        if (std::find(std::begin(keys), std::end(keys), id) == std::end(keys)) continue;
        auto rc = Rando::StaticData::GetCheckIdFromName(row.at("check").get<std::string>().c_str());
        REQUIRE(std::find(excluded.begin(), excluded.end(), rc) == excluded.end());
        REQUIRE(OwnPhysicalDungeon(id, Rando::StaticData::Checks.at(rc).sceneId));
        ++count;
    }
    return count;
}
static std::string ootReach, mmReach;
static std::map<std::string, std::string> blocked;
static std::set<std::string> ownedMm;
static void Reset() {}
static void Place(const char*, const char*) {}
static void OotOwned(const char*) {}
static void MmOwned(const char* raw) {
    ownedMm.clear(); for (auto& item : J::parse(raw)) ownedMm.insert(item.get<std::string>());
}
static const char* OotReach() { return ootReach.c_str(); }
static const char* MmReach() {
    static std::string result;
    auto reachable = J::parse(mmReach);
    for (const auto& [check, item] : blocked)
        if (ownedMm.count(item)) reachable.push_back(check);
    result = reachable.dump(); return result.c_str();
}
static uint8_t Portal() { return 1; }
static J OotDump() {
    J d = {{"checks", J::array()}, {"pool", J::array()}, {"fixed", J::array()}};
    J reach = J::array({"Ganon"});
    for (int i = 0; i < 32; ++i) {
        std::string name = "OOT fixture " + std::to_string(i);
        d["checks"].push_back({{"name", name}}); reach.push_back(name);
        d["pool"].push_back({{"name", "Blue Rupee"}, {"advancement", false}, {"category", "junk"}});
    }
    // Models an already native-confined OoT key; the MM regression must not move it.
    d["fixed"].push_back({{"check", "OOT Forest Temple"}, {"item", "Forest Temple Boss Key"},
                           {"advancement", true}});
    reach.push_back("OOT Forest Temple"); ootReach = reach.dump(); return d;
}
static void SetReach(const J& dump, bool omitConfined = false) {
    blocked.clear();
    J reach = J::array({"Moon Majora Pot 01"});
    for (const auto& c : dump["checks"]) reach.push_back(c["name"]);
    for (const auto& f : dump["fixed"])
        if (!omitConfined || !f.value("validate", false)) reach.push_back(f["check"]);
        else blocked.emplace(f["check"].get<std::string>(), f["item"].get<std::string>());
    mmReach = reach.dump();
}
static void CheckExhaustion(const RandoSaveInfo& before, const J& od,
                            const ComboRando::OracleFns& oo, const ComboRando::OracleFns& mo) {
    excluded.clear();
    for (const auto& [id, check] : Rando::StaticData::Checks)
        if (Rando::Logic::SceneIdToDungeon(check.sceneId) >= 0) excluded.push_back(id);
    // GUI and headless callers receive a nonempty, explicit failure dump. The
    // actual common parser must reject it before an empty pool can look valid.
    auto failure = J::parse(MM_DumpRandoStaticData());
    REQUIRE(failure.contains("error")); REQUIRE(failure["error"].is_string());
    REQUIRE(Unchanged(before));
    auto fill = ComboRando::CrossWorldCombinedFill(od.dump(), failure.dump(), 130, oo, mo);
    REQUIRE(!fill.success);
    REQUIRE(fill.error.find("confined item") != std::string::npos);
    // Missing oracle exports must not turn a failed native pool into an empty,
    // valid-looking no-logic spoiler. Both source games use this same boundary.
    for (bool failOot : {false, true}) {
        bool rejected = false;
        try {
            ComboRando::CrossWorldGenerateSpoiler(failOot ? failure.dump() : od.dump(),
                                                   failOot ? od.dump() : failure.dump(), 130);
        } catch (const std::exception& e) {
            rejected = std::string(e.what()).find("confined item") != std::string::npos;
        }
        REQUIRE(rejected);
    }
    auto fallback = J::parse(ComboRando::CrossWorldGenerateSpoiler(od.dump(), od.dump(), 130));
    REQUIRE(fallback["ootCount"] == 32 && fallback["mmCount"] == 32);
    excluded.clear();
}

int main(int argc, char** argv) try {
    bool observe = argc > 1 && !strcmp(argv[1], "--observe");
    // Full production catalog membership. Reachability conditions are deliberately
    // outside this pool/confinement fixture, rather than a replacement solver.
    auto& region = Rando::Logic::Regions[(RandoRegionId)0];
    for (const auto& [id, check] : Rando::StaticData::Checks)
        if (id != RC_UNKNOWN) region.checks[id] = {[] { return true; }, "fixture catalog membership"};
    // Restore and re-dump the actual persisted snapshot; stale live save is Anywhere.
    auto settings = J::parse(MM_DumpRandoSettings());
    settings[ownCvar] = RO_DUNGEON_ITEM_OWN_DUNGEON;
    settings["gRando.Options.RO_HYLIAS_GRACE"] = RO_GRACE_OFF;
    MM_RestoreRandoSettings(settings.dump().c_str());
    REQUIRE(J::parse(MM_DumpRandoSettings()).at(ownCvar) == RO_DUNGEON_ITEM_OWN_DUNGEON);
    auto before = Snapshot();
    J dump = J::parse(MM_DumpRandoStaticData());
    std::cerr << "cold-live-save: boss keys fixed=" << KeyCount(dump["fixed"], "item")
              << " free=" << KeyCount(dump["pool"], "name") << " live restored=" << Unchanged(*before) << '\n';
    if (!observe) { REQUIRE(VerifyFixed(dump) == 4); REQUIRE(KeyCount(dump["pool"], "name") == 0); REQUIRE(Unchanged(*before)); }

    ComboRando::OracleFns oo{Reset, OotOwned, OotReach, Place, Portal};
    ComboRando::OracleFns mo{Reset, MmOwned, MmReach, Place};
    auto od = OotDump();
    if (argc > 1 && !strcmp(argv[1], "--failure-path-only")) {
        CheckExhaustion(*before, od, oo, mo);
        std::cerr << "PASS explicit exhaustion error, common GUI/headless fill rejection, state restoration\n";
        return 0;
    }
    int foreignKeys = 0, foreignStoneTower = 0, fills = 0, inverted = 0, normal = 0;
    for (unsigned seed = 1; seed <= 64; ++seed) {
        sMMComboRandoSeed = seed;
        dump = J::parse(MM_DumpRandoStaticData());
        if (!observe) { REQUIRE(VerifyFixed(dump) == 4); REQUIRE(Unchanged(*before)); }
        for (const auto& row : dump["fixed"]) if (row["item"] == Rando::StaticData::GetItemDisplayName(RI_STONE_TOWER_BOSS_KEY)) {
            auto rc = Rando::StaticData::GetCheckIdFromName(row["check"].get<std::string>().c_str());
            // Chest IDs retain SCENE_INISIE_N for shared chest flags even when
            // logically inverted; use the physical check's native token.
            bool isInverted = std::string(Rando::StaticData::Checks.at(rc).name).find("_INVERTED_") != std::string::npos;
            inverted += isInverted; normal += !isInverted;
        }
        SetReach(dump);
        auto fill = ComboRando::CrossWorldCombinedFill(od.dump(), dump.dump(), seed, oo, mo);
        REQUIRE(fill.success); ++fills;
        auto placed = J::parse(fill.spoilerJson);
        REQUIRE(placed["oot"]["OOT Forest Temple"] == "Forest Temple Boss Key");
        for (const auto& [check, item] : placed["oot"].items()) for (auto key : keys) {
            bool isKey = item == Rando::StaticData::GetItemDisplayName(key);
            foreignKeys += isKey;
            if (isKey && key == RI_STONE_TOWER_BOSS_KEY) {
                ++foreignStoneTower;
                if (observe) std::cerr << "Stone Tower Boss Key outside dungeon: seed=" << seed << " check=" << check << '\n';
            }
        }
        if (!observe) for (const auto& row : dump["fixed"])
            REQUIRE(placed["mm"][row["check"].get<std::string>()] == row["item"]);
    }
    std::cerr << "seeded full-catalog pool/combined fills=" << fills << " MM boss keys in OoT=" << foreignKeys
              << " Stone Tower normal=" << normal << " inverted=" << inverted << '\n';
    if (observe) { REQUIRE(foreignKeys > 0); REQUIRE(foreignStoneTower > 0); return 0; }
    REQUIRE(foreignKeys == 0); REQUIRE(normal > 0); REQUIRE(inverted > 0);
    REQUIRE(Rando::Logic::SceneIdToDungeon(SCENE_INISIE_N) == DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE);
    REQUIRE(Rando::Logic::SceneIdToDungeon(SCENE_INISIE_R) == DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE);
    REQUIRE(Rando::Logic::SceneIdToDungeon(SCENE_INISIE_BS) == DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE);

    // Imported exclusions cover every boss-key vanilla chest and many ordinary
    // checks; an excluded check cannot receive any required key.
    for (const auto& [id, check] : Rando::StaticData::Checks)
        if (check.randoItemId == RI_STONE_TOWER_BOSS_KEY || check.randoItemId == RI_WOODFALL_BOSS_KEY ||
            (id % 4 == 0 && Rando::Logic::SceneIdToDungeon(check.sceneId) >= 0)) excluded.push_back(id);
    settings = J::parse(MM_DumpRandoSettings());
    MM_RestoreRandoSettings(settings.dump().c_str());
    REQUIRE(J::parse(MM_DumpRandoSettings())["gRando.ExcludedChecks"] == settings["gRando.ExcludedChecks"]);
    for (unsigned seed = 65; seed <= 128; ++seed) {
        sMMComboRandoSeed = seed; dump = J::parse(MM_DumpRandoStaticData()); REQUIRE(VerifyFixed(dump) == 4);
    }
    // Placement flag distinguishes randomized confined checks from vanilla ones.
    SetReach(dump, true);
    auto impossible = ComboRando::CrossWorldCombinedFill(od.dump(), dump.dump(), 129, oo, mo);
    REQUIRE(!impossible.success);
    // Legacy fixed placements retain their existing validation semantics.
    auto legacy = dump;
    for (auto& row : legacy["fixed"]) row.erase("validate");
    auto unvalidated = ComboRando::CrossWorldCombinedFill(od.dump(), legacy.dump(), 129, oo, mo);
    REQUIRE(unvalidated.success);
    // Failure after confinement must also restore the exact live save bytes.
    throwAtHint = true; bool threw = false;
    try { MM_DumpRandoStaticData(); } catch (const std::runtime_error&) { threw = true; }
    throwAtHint = false; REQUIRE(threw); REQUIRE(Unchanged(*before));
    throwAtDraw = true; confinedDraws = 0; threw = false;
    try { MM_DumpRandoStaticData(); } catch (const std::runtime_error&) { threw = true; }
    throwAtDraw = false; REQUIRE(threw); REQUIRE(confinedDraws == 2); REQUIRE(Unchanged(*before));
    // With no legal physical dungeon checks, reject rather than export a free
    // boss key. The transaction restores the stale live save on this path too.
    CheckExhaustion(*before, od, oo, mo);
    for (int mode : {RO_DUNGEON_ITEM_ANYWHERE, RO_DUNGEON_ITEM_START_WITH, RO_DUNGEON_ITEM_VANILLA}) {
        settings[ownCvar] = mode; settings["gRando.ExcludedChecks"] = J::array();
        MM_RestoreRandoSettings(settings.dump().c_str()); dump = J::parse(MM_DumpRandoStaticData());
        REQUIRE(KeyCount(dump["pool"], "name") == (mode == RO_DUNGEON_ITEM_ANYWHERE ? 4 : 0));
        REQUIRE(KeyCount(dump["fixed"], "item") == (mode == RO_DUNGEON_ITEM_VANILLA ? 4 : 0));
        REQUIRE(Unchanged(*before));
        if (mode == RO_DUNGEON_ITEM_ANYWHERE) {
            int crossed = 0;
            SetReach(dump);
            for (unsigned seed = 1; seed <= 16; ++seed) {
                auto fill = ComboRando::CrossWorldCombinedFill(od.dump(), dump.dump(), seed, oo, mo);
                REQUIRE(fill.success);
                auto spoiler = J::parse(fill.spoilerJson);
                for (const auto& item : spoiler["oot"])
                    for (auto key : keys) crossed += item == Rando::StaticData::GetItemDisplayName(key);
            }
            REQUIRE(crossed > 0);
            std::cerr << "Anywhere negative control: 16 combined fills, foreign boss keys=" << crossed << '\n';
        }
    }
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_PLACEMENT_BOSS_KEYS] = RO_DUNGEON_ITEM_OWN_DUNGEON;
    before = Snapshot();
    settings[ownCvar] = RO_DUNGEON_ITEM_ANYWHERE;
    MM_RestoreRandoSettings(settings.dump().c_str()); dump = J::parse(MM_DumpRandoStaticData());
    REQUIRE(KeyCount(dump["pool"], "name") == 4); REQUIRE(KeyCount(dump["fixed"], "item") == 0);
    REQUIRE(Unchanged(*before));
    std::cerr << "PASS exclusions=64 seeds, self-lock rejected, exception restoration, Anywhere/Start With/Vanilla preserved\n";
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
