#include "combo/rando/ComboPlaythrough.h"
#include <cassert>
#include <functional>
#include <string>

using namespace ComboRando;
static int queries = 0;
static void Reset() { ++queries; }
static void Owned(const char*) { ++queries; }
static const char* Reachable() { ++queries; return "[]"; }
static void Rejected(const std::function<void()>& action) {
    bool rejected = false;
    try { action(); } catch (const std::exception&) { rejected = true; }
    assert(rejected);
}

int main() {
    const std::string good = R"({"pool":[],"fixed":[],"items":[]})";
    const std::string failed = R"({"error":"native pool failed","checks":[],"pool":[],"fixed":[]})";
    const std::string spoiler = R"({"oot":{},"mm":{},"foreign":[]})";
    OracleFns oracle{Reset, Owned, Reachable, nullptr, nullptr};
    for (const auto& bad : {failed, std::string("{broken"), std::string("[]")}) {
        Rejected([&] { ParseSpoilerPlacements(spoiler, bad, good); });
        Rejected([&] { ParseSpoilerPlacements(spoiler, good, bad); });
        Rejected([&] { ParseItemMeta(bad); });
        Rejected([&] { bool present; ParseIceTrapModels(bad, present); });
        Rejected([&] { ResolveSharedPairs("", bad); });
        Rejected([&] { OotAccessFromDump(bad); });
        Rejected([&] { BuildSharedItemGroups({}, 0, good, bad); });
        Rejected([&] {
            auto a = nlohmann::json::object(), b = a, foreign = nlohmann::json::array();
            SuffixCrossGameItems(a, b, foreign, good, bad);
        });
        Rejected([&] { RunPlaythrough(spoiler, oracle, oracle, "invalid-native-dump", nullptr, nullptr, bad, good); });
        Rejected([&] { RunPlaythrough(spoiler, oracle, oracle, "invalid-native-dump", nullptr, nullptr, good, bad); });
        assert(queries == 0); // Reject before touching either native oracle.
    }
    // Optional/older metadata remains supported; an explicit diagnostic is never optional.
    assert(ParseSpoilerPlacements(spoiler).empty());
    assert(ParseSpoilerPlacements(spoiler, good, good).empty());
    assert(ParseItemMeta("{}").empty());
    assert(OotAccessFromDump("{}") == OotAccess::ALL_REACHABLE);
    assert(OotAccessFromDump(R"({"accessibility":{"noLogic":true}})") == OotAccess::NO_LOGIC);
}
