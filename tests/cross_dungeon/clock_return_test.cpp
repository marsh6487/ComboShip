#include "combo/rando/CrossWorldRando.h"
#include <cassert>
#include <iostream>
using namespace ComboRando;
using Json = nlohmann::json;
static std::vector<std::string> owned[2];
static Json accessState[2];
static bool daytime = false, early = false, route = false, worldReturn = false;
static bool HasClock() {
    return std::find(owned[0].begin(), owned[0].end(), "Clock") != owned[0].end();
}
static void Access(int game, const char* raw) { accessState[game] = Json::parse(raw); }
static bool Arrival(int game, int index) {
    return std::find(accessState[game]["arrivals"].begin(), accessState[game]["arrivals"].end(), index) !=
           accessState[game]["arrivals"].end();
}
static const char* Checks(int game) {
    static std::string raw[2];
    Json checks = Json::array();
    if (game == 0 && (early || HasClock() || Arrival(0, 20)))
        checks.push_back("OOT chest");
    if (game == 0 && Arrival(0, 20))
        checks.push_back("Fixed Clock");
    if (game == 1 && accessState[1].value("overworld", true))
        checks.push_back("MM chest");
    if (game == 1 && Arrival(1, 10))
        checks.push_back("Room key");
    raw[game] = checks.dump();
    return raw[game].c_str();
}
static uint8_t Portal() { return daytime || HasClock(); }
static const char* OotExits() { return route ? "[{\"game\":1,\"index\":10}]" : "[]"; }
static const char* MmExits() {
    return ((route && Arrival(1, 10)) || (worldReturn && accessState[1].value("overworld", true))) ? "[{\"game\":0,\"index\":20}]" : "[]";
}
static OracleFns Oot() {
    return {[] {}, [](const char* raw) { owned[0] = Json::parse(raw).get<std::vector<std::string>>(); },
            [] { return Checks(0); }, [](const char*, const char*) {}, Portal, [](const char* raw) { Access(0, raw); }, OotExits};
}
static OracleFns Mm() {
    return {[] {}, [](const char* raw) { owned[1] = Json::parse(raw).get<std::vector<std::string>>(); },
            [] { return Checks(1); }, [](const char*, const char*) {}, nullptr, [](const char* raw) { Access(1, raw); }, MmExits};
}
static Json Dump(int game) {
    const char* item = game == 0 ? "Clock" : "Blue Rupee";
    return {{"checks", Json::array({{{"name", game == 0 ? "OOT chest" : "MM chest"}}})},
            {"pool", Json::array({{{"name", item}, {"advancement", game == 0},
                                    {"category", game == 0 ? "major" : "junk"}}})},
            {"items", Json::array({{{"name", item}}})}, {"fixed", Json::array()}};
}
static CombinedFillResult Fill(GameId start = GAME_MM, OotAccess logic = OotAccess::ALL_REACHABLE) {
    accessState[0] = accessState[1] = {{"overworld", true}, {"arrivals", Json::array()}};
    owned[0].clear(); owned[1].clear();
    return CrossWorldCombinedFill(Dump(0).dump(), Dump(1).dump(), 51, Oot(), Mm(), nullptr, "", logic, {}, start);
}
static CombinedFillResult SharedRoomKey() {
    auto oot = Dump(0), mm = Dump(1);
    oot["pool"] = Json::array({{{"name", "OOT Rupee"}, {"advancement", false}, {"category", "junk"}}});
    mm["items"].push_back({{"name", "MM Clock"}});
    mm["fixed"] = Json::array({{{"check", "Room key"}, {"item", "MM Clock"},
                                  {"advancement", true}, {"validate", true}}});
    // Only the entered room is available before Clock; MM overworld is shut.
    const auto pairs = Json::array({{{"oot", "Clock"}, {"mm", "MM Clock"}}}).dump();
    route = true;
    auto result = CrossWorldCombinedFill(oot.dump(), mm.dump(), 53, Oot(), Mm(), nullptr, "",
                                        OotAccess::ALL_REACHABLE, {}, GAME_MM, pairs);
    route = false;
    return result;
}
static CombinedFillResult FixedBehindMm() {
    auto oot = Dump(0), mm = Dump(1);
    oot["pool"] = Json::array();
    oot["checks"] = Json::array();
    oot["fixed"] = Json::array({{{"check", "Fixed Clock"}, {"item", "Clock"},
                                  {"advancement", true}, {"validate", true}}});
    worldReturn = true;
    auto result = CrossWorldCombinedFill(oot.dump(), mm.dump(), 52, Oot(), Mm(), nullptr, "",
                                        OotAccess::ALL_REACHABLE, {}, GAME_MM);
    worldReturn = false;
    return result;
}
static void Clean() {
    for (const auto& a : accessState)
        assert(a.at("overworld") == true && a.at("arrivals").empty() && a.at("logicalClears") == 0);
}
int main(int argc, char**) {
    if (argc > 1) {
        accessState[0] = accessState[1] = {{"overworld", true}, {"arrivals", Json::array()}};
        assert(!FixedBehindMm().success && "fixed Clock behind MM must fail the final return guard");
        return 0;
    }
    const auto trapped = Fill();
    assert(!trapped.success && "MM-start Clock behind MM must not certify an itemless OOT return");
    Clean();
    const auto fixed = FixedBehindMm();
    assert(!fixed.success && fixed.error.find("not re-openable") != std::string::npos);
    Clean();
    assert(SharedRoomKey().success); Clean();
    auto missing = Oot(); missing.GetPortalOpen = nullptr;
    assert(!CrossWorldCombinedFill(Dump(0).dump(), Dump(1).dump(), 51, missing, Mm(), nullptr, "",
                                  OotAccess::ALL_REACHABLE, {}, GAME_MM).success);
    early = true;
    const auto clockEarly = Fill();
    assert(clockEarly.success);
    assert(Json::parse(clockEarly.spoilerJson).at("oot").at("OOT chest") == "Clock");
    Clean();
    early = false; daytime = true;
    assert(Fill().success); Clean();
    daytime = false; route = true;
    // A real foreign dungeon and chained OOT arrival may bootstrap Clock;
    // they must not grant all MM checks while the portal remains shut.
    assert(Fill().success); Clean();
    route = false;
    assert(Fill(GAME_OOT, OotAccess::NO_LOGIC).success); Clean();
    assert(Fill(GAME_MM, OotAccess::NO_LOGIC).success); Clean();
    route = true;
    assert(Fill(GAME_OOT).success); Clean();
    std::cout << "Clock return: MM-start closed-world guard, early/daytime Clock, native cross routes, OOT start, NoLogic and access cleanup PASS\n";
}
