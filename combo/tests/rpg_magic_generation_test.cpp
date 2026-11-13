// Real combined fill with bounded native dump fixtures. This tests pool policy,
// not the reachability of a complete game seed.
#include "rando/CrossWorldRando.h"
#include "rando/ComboPlaythrough.h"
#include <iostream>
#include <stdexcept>

using Json = nlohmann::json;
static std::string ootReach, mmReach;
static bool magicGate;
static int oracleMagic;
static void Reset() {
}
static void Owned(const char*) {
}
static void Place(const char*, const char*) {
}
static const char* OotReach() {
    return ootReach.c_str();
}
static void MmOwned(const char* owned) {
    oracleMagic = 0;
    ComboRpgState state{};
    for (const auto& item : Json::parse(owned))
        ComboRando::ParseRpgMagicOracleItem(item.get<std::string>(), state);
    oracleMagic = ComboRpgState_MagicCapacity(&state, 0);
}
static const char* MmReach() {
    if (!magicGate)
        return mmReach.c_str();
    static std::string gated;
    Json reachable = Json::array({ "Free MM" });
    if (oracleMagic >= 8) {
        reachable.push_back("MM gate");
        reachable.push_back("Moon Majora Pot 01");
    }
    gated = reachable.dump();
    return gated.c_str();
}
static uint8_t Portal() {
    return 1;
}
static int failures;
static void Check(bool ok, const char* message) {
    if (!ok) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

static Json Dump(const char* prefix, const char* item, int count) {
    Json dump = { { "checks", Json::array() },
                  { "pool", Json::array() },
                  { "items", Json::array() },
                  { "fixed", Json::array() },
                  { "junkPool", Json::array({ "Blue Rupee" }) } };
    for (int i = 0; i < 20; ++i) {
        dump["checks"].push_back({ { "name", std::string(prefix) + std::to_string(i) } });
        dump["pool"].push_back({ { "name", i < count ? item : "Blue Rupee" },
                                 { "category", i < count ? "major" : "junk" },
                                 { "advancement", i < count } });
    }
    return dump;
}

static int Count(const Json& spoiler, const char* name) {
    int count = 0;
    for (const char* game : { "oot", "mm" })
        for (const auto& value : spoiler[game])
            count += value == name;
    return count;
}

static void TestOwnedViews() {
    struct Case {
        int required, total, count, capacity;
    };
    for (const auto test : { Case{ 8, 96, 0, 0 }, Case{ 8, 96, 1, 12 }, Case{ 8, 96, 9, 96 }, Case{ 25, 100, 1, 4 },
                             Case{ 25, 100, 12, 48 }, Case{ 100, 100, 1, 1 }, Case{ 100, 100, 8, 8 } }) {
        const auto rules = ComboRando::ParseRpgMagicRules(
            Json({ { "rpgMagic", { { "enabled", true }, { "required", test.required }, { "total", test.total } } } })
                .dump());
        const std::vector<std::string> ootOwned(test.count, "Magic Stat Upgrade"), mmOwned{ "Bow" };
        std::vector<std::string> ootSeen, mmSeen, firstView;
        for (int repeat = 0; repeat < 3; ++repeat) {
            ComboRando::SharedOwnedViews({}, ootOwned, mmOwned, ootSeen, mmSeen, rules);
            ComboRpgState state{};
            int metadata = 0;
            for (const auto& item : mmSeen)
                metadata += ComboRando::ParseRpgMagicOracleItem(item, state);
            Check(metadata == 1 && ComboRpgState_MagicCapacity(&state, 0) == test.capacity,
                  "owned view uses frozen rules and exact bounded RPG count even with no native shared groups");
            Check(ootSeen == ootOwned && mmOwned == std::vector<std::string>{ "Bow" },
                  "oracle metadata never becomes a real pickup or feeds back into OoT");
            if (repeat == 0)
                firstView = mmSeen;
            Check(mmSeen == firstView, "repeated spheres rebuild the same view without additive magic");
        }
        ComboRando::SharedOwnedViews({}, ootOwned, mmOwned, ootSeen, mmSeen);
        Check(mmSeen == mmOwned, "old and RPG-disabled callers retain native ownership only");
    }
}

static void TestReplay(const ComboRando::OracleFns& oot, const ComboRando::OracleFns& mm) {
    magicGate = true;
    struct Case {
        int required, total, copies;
        bool win;
    };
    for (const auto test : { Case{ 8, 96, 1, true }, Case{ 100, 100, 1, false }, Case{ 100, 100, 7, false },
                             Case{ 100, 100, 8, true } }) {
        for (bool foreign : { false, true }) {
            Json od = Dump("RPG ", "Magic Stat Upgrade", test.copies), md = Dump("MM ", "Blue Rupee", 0);
            od["rpgMagic"] = { { "enabled", true }, { "required", test.required }, { "total", test.total } };
            Json spoiler = { { "oot", Json::object() }, { "mm", Json::object() }, { "foreign", Json::array() } };
            Json available = Json::array({ "Ganon" });
            for (int i = 0; i < test.copies; ++i) {
                const bool inMm = foreign && i == 0;
                const std::string check = inMm ? "Free MM" : "RPG " + std::to_string(i);
                spoiler[inMm ? "mm" : "oot"][check] = "Magic Stat Upgrade";
                if (inMm)
                    spoiler["foreign"].push_back({ { "checkGame", "mm" },
                                                   { "checkName", check },
                                                   { "itemGame", "oot" },
                                                   { "itemName", "Magic Stat Upgrade" } });
                else
                    available.push_back(check);
            }
            spoiler["mm"]["MM gate"] = "Blue Rupee";
            ootReach = available.dump();
            Json spheres;
            const auto replay = ComboRando::RunPlaythrough(spoiler.dump(), oot, mm, "rpg-magic-fixture", nullptr,
                                                           &spheres, od.dump(), md.dump(), false);
            Check(replay.beatable == test.win && replay.majoraReachable == test.win,
                  "GUI/headless shared replay uses partial RPG capacity and frozen thresholds");
            Check(spoiler.dump().find(ComboRando::kRpgMagicOraclePrefix) == std::string::npos &&
                      spheres.dump().find(ComboRando::kRpgMagicOraclePrefix) == std::string::npos,
                  "private oracle metadata never appears in placements or playthrough steps");
            if (test.win) {
                const auto required =
                    ComboRando::PareDownPlaythrough(spoiler.dump(), oot, mm, nullptr, od.dump(), md.dump(), {}, {},
                                                    ComboRando::DefaultGanonMajoraGoal, false);
                Check(required.candidateCount == test.copies, "requiredness reaches the winning RPG sphere");
                for (int i = 0; i < test.copies; ++i) {
                    const std::string check = foreign && i == 0 ? "mm:Free MM" : "oot:RPG " + std::to_string(i);
                    Check(required.requiredByCheck.at(check),
                          "removing any necessary RPG pickup blocks the MM magic gate");
                }
            }
            const auto hint =
                ComboRando::FindSariaMagicSphere(spoiler.dump(), oot, mm, od.dump(), md.dump(), false, false, 0, "");
            Check(!hint.empty() && hint.at(0).at("sphere") == 0,
                  "dedicated magic hint search shares the same frozen-rule traversal");
        }
    }
    magicGate = false;
}

int main() {
    const auto pairs =
        Json::array({ { { "oot", "Progressive Magic Meter" }, { "mm", "RI_PROGRESSIVE_MAGIC" } } }).dump();
    const ComboRando::OracleFns oot{ Reset, Owned, OotReach, Place, Portal }, mm{ Reset, MmOwned, MmReach, Place };
    TestOwnedViews();
    TestReplay(oot, mm);
    for (int mode : { 0, 1, 2 }) { // absent (old DLL), explicitly off, enabled
        for (bool requestShared : { false, true }) {
            for (bool fixedNative : { false, true }) {
                auto od = Dump("OOT check ", "Magic Stat Upgrade", 8);
                auto md = Dump("MM check ", "Progressive Magic", 2);
                md["items"].push_back({ { "token", "RI_PROGRESSIVE_MAGIC" }, { "name", "Progressive Magic" } });
                if (mode != 0)
                    od["rpgMagic"] = { { "enabled", mode == 2 }, { "required", 8 }, { "total", 96 } };
                if (fixedNative)
                    md["fixed"].push_back(
                        { { "check", "Fixed Fairy" }, { "item", "Power of Magic" }, { "advancement", true } });
                Json a = Json::array(), b = Json::array();
                for (const auto& check : od["checks"])
                    a.push_back(check["name"]);
                for (const auto& check : md["checks"])
                    b.push_back(check["name"]);
                if (fixedNative)
                    b.push_back("Fixed Fairy");
                a.push_back("Ganon");
                b.push_back("Moon Majora Pot 01");
                ootReach = a.dump();
                mmReach = b.dump();
                const auto fill = ComboRando::CrossWorldCombinedFill(
                    od.dump(), md.dump(), 1, oot, mm, nullptr, "", ComboRando::OotAccess::ALL_REACHABLE, {},
                    ComboRando::GAME_OOT, pairs, requestShared ? (1u << ComboRando::SF_MAGIC) : 0);
                Check(fill.success, "RPG pool policy still fills every check");
                if (!fill.success)
                    continue;
                const auto spoiler = Json::parse(fill.spoilerJson);
                Check(Count(spoiler, "Magic Stat Upgrade") == 8, "RPG pool copies survive native suppression");
                Check(Count(spoiler, "Progressive Magic") == (mode == 2 ? 0 : 2),
                      "RPG mode suppresses MM native free magic, with shared magic both off and on");
                Check(Count(spoiler, "Power of Magic") == (fixedNative ? 1 : 0),
                      "fixed native magic remains an earned native reward");
                Check(spoiler["oot"].size() + spoiler["mm"].size() == (fixedNative ? 41 : 40),
                      "native magic replacement preserves check balance");
            }
        }
    }
    if (!failures)
        std::cout << "PASS: RPG magic pool suppression, native controls, fixed supply and check balance\n";
    return failures != 0;
}
