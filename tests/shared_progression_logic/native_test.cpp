// Exercise the production fill and forward replay with small possession-gated worlds.
// The oracles model only the stated edges; they do not grant every check or inventory item.
#include "rando/ComboPlaythrough.h"
#include "rando/NativeRewardTopology.h"
#include <array>
#include <iostream>
#include <map>
#include <set>

using namespace ComboRando;
using json = nlohmann::json;

namespace {
constexpr const char* kKey = "Stone Tower Boss Key";
constexpr const char* kKeyCheck = "Stone Tower guarded check";
std::array<std::vector<std::string>, 2> owned, freeChecks;
std::array<std::map<std::string, std::vector<std::string>>, 2> gatedChecks;
std::array<std::string, 2> buffers;
std::vector<std::string> ootGoalItems;
std::string ootPortalItem;
std::string mmGoalItem;
int failures = 0;

bool Has(int game, const std::string& name) {
    return std::find(owned[game].begin(), owned[game].end(), name) != owned[game].end();
}

void ResetOot() {
    owned[GAME_OOT].clear();
}
void ResetMm() {
    owned[GAME_MM].clear();
}
void SetOot(const char* value) {
    owned[GAME_OOT] = json::parse(value).get<std::vector<std::string>>();
}
void SetMm(const char* value) {
    owned[GAME_MM] = json::parse(value).get<std::vector<std::string>>();
}
void Place(const char*, const char*) {
}
uint8_t Portal() {
    return ootPortalItem.empty() || Has(GAME_OOT, ootPortalItem);
}

const char* Reach(int game) {
    json result = freeChecks[game];
    for (const auto& [check, items] : gatedChecks[game]) {
        if (std::all_of(items.begin(), items.end(), [&](const std::string& item) { return Has(game, item); })) {
            result.push_back(check);
        }
    }
    if (game == GAME_OOT) {
        if (std::all_of(ootGoalItems.begin(), ootGoalItems.end(),
                        [](const std::string& item) { return Has(GAME_OOT, item); })) {
            result.push_back("Ganon");
        }
    } else if (mmGoalItem.empty() || Has(GAME_MM, mmGoalItem)) {
        result.push_back("Moon Majora Pot 01");
    }
    buffers[game] = result.dump();
    return buffers[game].c_str();
}

const char* ReachOot() {
    return Reach(GAME_OOT);
}
const char* ReachMm() {
    return Reach(GAME_MM);
}
const OracleFns ootOracle{ ResetOot, SetOot, ReachOot, Place, Portal };
const OracleFns mmOracle{ ResetMm, SetMm, ReachMm, Place };

void ResetWorld() {
    owned = {};
    freeChecks = {};
    gatedChecks = {};
    ootGoalItems.clear();
    ootPortalItem.clear();
    mmGoalItem.clear();
}

void Check(bool value, const std::string& message) {
    if (!value) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

json EmptyDump() {
    return {
        { "pool", json::array() }, { "checks", json::array() }, { "items", json::array() }, { "fixed", json::array() }
    };
}

void Add(json& dump, const std::string& item, const std::string& check, bool advancement = true) {
    dump["items"].push_back({ { "name", item }, { "token", "RI_" + item } });
    dump["pool"].push_back(
        { { "name", item }, { "advancement", advancement }, { "category", advancement ? "major" : "junk" } });
    dump["checks"].push_back({ { "name", check } });
}

CombinedFillResult Fill(const json& oot, const json& mm, OotAccess access, GameId start = GAME_OOT, uint32_t mask = 0,
                        const std::string& pairs = "", const std::string& forced = "") {
    return CrossWorldCombinedFill(oot.dump(), mm.dump(), 20261110, ootOracle, mmOracle, nullptr, forced, access, {},
                                  start, pairs, mask);
}

void SharedSourceOpensConfinedKey(bool nativeFamily, GameId start) {
    ResetWorld();
    const std::string source = nativeFamily ? "Progressive Bow" : "Whip";
    const std::string peer = nativeFamily ? "Progressive Bow" : "Whip (MM)";
    const uint32_t mask = nativeFamily ? (1u << SF_BOW) : 0;
    const std::string pairs = nativeFamily ? "" : json::array({ { { "oot", source }, { "mm", peer } } }).dump();
    const std::string label =
        std::string(nativeFamily ? "native bow" : "NEI alias") + (start == GAME_OOT ? ", OoT start" : ", MM start");
    json oot = EmptyDump(), mm = EmptyDump();
    Add(oot, source, "Unreachable OoT check 1");
    Add(oot, "OoT junk", "Unreachable OoT check 2", false);
    Add(mm, peer, "MM free check 1");
    Add(mm, "MM junk", "MM free check 2", false);
    mm["fixed"].push_back({ { "check", kKeyCheck },
                            { "item", kKey },
                            { "advancement", true },
                            { "validate", true },
                            { "allowedChecks", json::array({ kKeyCheck }) } });
    // The only legal key location requires the shared capability, but never the key itself.
    // All free pickups are in MM, forcing the retained OoT item through foreign-item serialization.
    freeChecks[GAME_MM] = { "MM free check 1", "MM free check 2" };
    gatedChecks[GAME_MM][kKeyCheck] = { peer };
    mmGoalItem = kKey;
    const auto result = Fill(oot, mm, OotAccess::NO_LOGIC, start, mask, pairs);
    Check(result.success,
          label + ": shared progression must participate before the confined key (" + result.error + ")");
    if (!result.success) {
        return;
    }
    const auto placements = ParseSpoilerPlacements(result.spoilerJson, oot.dump(), mm.dump());
    int sourceCopies = 0, peerCopies = 0, keys = 0;
    std::set<std::pair<GameId, std::string>> checks;
    for (const auto& p : placements) {
        Check(checks.emplace(p.checkGame, p.check).second, label + ": each check is filled once");
        if (p.itemGame == GAME_OOT && p.item == source) {
            ++sourceCopies;
            Check(p.checkGame == GAME_MM, label + ": retained source uses a reachable MM check");
        }
        peerCopies += p.itemGame == GAME_MM && p.item == peer;
        if (p.item == kKey) {
            ++keys;
            Check(p.itemGame == GAME_MM && p.checkGame == GAME_MM && p.check == kKeyCheck,
                  label + ": native key identity and confinement remain intact");
        }
    }
    Check(placements.size() == 5 && sourceCopies == 1 && peerCopies == 0 && keys == 1,
          label + ": deduplication, item ownership and total check count remain intact");
    // Forward replay begins with no assumed items and has to collect the shared source, then the key.
    const auto replay = RunPlaythrough(result.spoilerJson, ootOracle, mmOracle, label, nullptr, nullptr, oot.dump(),
                                       mm.dump(), false, false, {}, start == GAME_MM, mask, pairs);
    Check(replay.beatable && replay.majoraReachable && replay.unreachableMm == 0,
          label + ": forward replay reaches MM progression from empty inventory");
}

void OotOnlyNoLogicRemainsPermissive() {
    ResetWorld();
    json oot = EmptyDump(), mm = EmptyDump();
    Add(oot, "OoT-only advancement", "Unreachable OoT check");
    const auto noLogic = Fill(oot, mm, OotAccess::NO_LOGIC);
    Check(noLogic.success, "No Logic still permits unreachable advancement that serves only OoT");
    if (noLogic.success) {
        const auto placements = ParseSpoilerPlacements(noLogic.spoilerJson, oot.dump(), mm.dump());
        Check(placements.size() == 1 && placements[0].itemGame == GAME_OOT &&
                  placements[0].item == "OoT-only advancement" && placements[0].check == "Unreachable OoT check",
              "OoT-only No Logic allowance preserves the item rather than discarding it");
    }
    Check(!Fill(oot, mm, OotAccess::ALL_REACHABLE).success,
          "the same unreachable OoT-only item remains invalid in All Reachable mode");
}

void BeatableOnlyCannotStrandSharedSource() {
    ResetWorld();
    json oot = EmptyDump(), mm = EmptyDump();
    Add(oot, "Progressive Bow", "Unreachable OoT bow check");
    Add(mm, "Progressive Bow", "Unreachable MM bow check");
    Add(mm, "MM junk", "Unreachable MM junk check", false);
    // Both victory markers are available, but no item check is reachable. This is acceptable only
    // for genuinely OoT-only optional advancement, not a source replacing MM's advancement copy.
    Check(!Fill(oot, mm, OotAccess::BEATABLE_ONLY, GAME_OOT, 1u << SF_BOW).success,
          "Beatability-only cannot relax a shared MM progression source into unreachable junk fill");
}

void FinalValidationChecksSharedFixedSource() {
    ResetWorld();
    json oot = EmptyDump(), mm = EmptyDump();
    Add(oot, "OoT junk", "Unreachable OoT junk check", false);
    oot["fixed"].push_back({ { "check", "Unreachable fixed source" },
                             { "item", "Progressive Bow" },
                             { "advancement", true },
                             { "validate", true } });
    Add(mm, "Progressive Bow", "Unreachable MM bow check");
    Add(mm, "MM junk", "Unreachable MM junk check", false);
    // This source bypasses placement entirely. Its native adapter explicitly requires validation,
    // so the final empty-inventory check must still enforce the MM progression obligation.
    const auto result = Fill(oot, mm, OotAccess::NO_LOGIC, GAME_OOT, 1u << SF_BOW);
    Check(!result.success, "final validation rejects an unreachable validated fixed source serving MM");
    if (!result.success) {
        Check(result.error.find("validation failed") != std::string::npos,
              "the fixed-source negative reaches final validation rather than failing pool construction");
    }
}

void DungeonRewardsFollowFinalEntrances() {
    ResetWorld();
    json oot = EmptyDump(), mm = EmptyDump();
    Add(oot, "OoT junk", "OoT free chest", false);
    // A foreign check deliberately shares a boss name. Names alone cannot define a native domain.
    Add(mm, "MM junk", "Queen Gohma", false);
    oot["fixed"] = json::array({ { { "check", "Queen Gohma" },
                                   { "item", "Kokiri Emerald" },
                                   { "advancement", true },
                                   { "major", true },
                                   { "hintable", true } },
                                 { { "check", "Phantom Ganon" },
                                   { "item", "Forest Medallion" },
                                   { "advancement", true },
                                   { "major", true },
                                   { "hintable", true } } });
    for (const auto* reward : { "Kokiri Emerald", "Forest Medallion", "Light Medallion" }) {
        oot["items"].push_back({ { "name", reward }, { "token", std::string("RI_") + reward } });
    }
    // The native-selected Pocket reward is absent from both the movable pool and boss domain.
    const std::string forced = json({ { "Link's Pocket", { { "item", "Light Medallion" } } } }).dump();
    freeChecks[GAME_OOT] = { "Queen Gohma", "Phantom Ganon", "OoT free chest", "Link's Pocket" };
    freeChecks[GAME_MM] = { "Queen Gohma" };
    ootGoalItems = { "Kokiri Emerald", "Forest Medallion" };
    const auto original = Fill(oot, mm, OotAccess::ALL_REACHABLE, GAME_OOT, 0, "", forced);
    Check(original.success, "the native reward preplacement works before the entrance topology changes");
    if (!original.success) {
        return;
    }
    json originalSteps = json::array();
    Check(RunPlaythrough(original.spoilerJson, ootOracle, mmOracle, "original reward topology", nullptr, &originalSteps,
                         oot.dump(), mm.dump(), false)
              .beatable,
          "the original reward ordering wins by forward collection");

    // The final entrance mapping now makes Gohma require its own preplaced Emerald. The other boss
    // is still reachable, so swapping native rewards can repair the cycle without opening a route.
    freeChecks[GAME_OOT] = { "Phantom Ganon", "OoT free chest", "Link's Pocket" };
    gatedChecks[GAME_OOT]["Queen Gohma"] = { "Kokiri Emerald" };
    json staleSteps = json::array();
    Check(!RunPlaythrough(original.spoilerJson, ootOracle, mmOracle, "stale reward topology", nullptr, &staleSteps,
                          oot.dump(), mm.dump(), false)
               .beatable,
          "the same preplacement is self-locked under the final entrance topology");
    for (auto& reward : oot["fixed"]) {
        reward["validate"] = true;
    }
    const auto locked = Fill(oot, mm, OotAccess::ALL_REACHABLE, GAME_OOT, 0, "", forced);
    Check(!locked.success && locked.error.find("validation failed") != std::string::npos,
          "validating stale fixed rewards detects the cycle but cannot repair their assignment");

    for (auto& reward : oot["fixed"]) {
        reward["allowedChecks"] = json::array({ "Queen Gohma", "Phantom Ganon" });
    }
    const auto repaired = Fill(oot, mm, OotAccess::ALL_REACHABLE, GAME_OOT, 0, "", forced);
    Check(repaired.success, "native reward-domain fill repairs the final entrance cycle (" + repaired.error + ")");
    if (!repaired.success) {
        return;
    }
    const auto placements = ParseSpoilerPlacements(repaired.spoilerJson, oot.dump(), mm.dump());
    std::map<std::pair<GameId, std::string>, int> actualItems;
    std::set<std::pair<GameId, std::string>> checks;
    for (const auto& p : placements) {
        ++actualItems[{ p.itemGame, p.item }];
        Check(checks.emplace(p.checkGame, p.check).second, "reward relocation fills each check exactly once");
        if (p.item == "Kokiri Emerald" || p.item == "Forest Medallion") {
            Check(p.itemGame == GAME_OOT && p.checkGame == GAME_OOT &&
                      (p.check == "Queen Gohma" || p.check == "Phantom Ganon"),
                  "movable rewards retain their native identity and stay at native boss checks");
        }
        if (p.item == "Light Medallion" || p.check == "Link's Pocket") {
            Check(p.itemGame == GAME_OOT && p.checkGame == GAME_OOT && p.item == "Light Medallion" &&
                      p.check == "Link's Pocket",
                  "the forced Pocket reward stays outside reward relocation");
        }
    }
    const std::map<std::pair<GameId, std::string>, int> expectedItems = { { { GAME_OOT, "Kokiri Emerald" }, 1 },
                                                                          { { GAME_OOT, "Forest Medallion" }, 1 },
                                                                          { { GAME_OOT, "Light Medallion" }, 1 },
                                                                          { { GAME_OOT, "OoT junk" }, 1 },
                                                                          { { GAME_MM, "MM junk" }, 1 } };
    Check(placements.size() == 5 && actualItems == expectedItems,
          "reward relocation preserves every native item and multiplicity, including the forced reward");
    json repairedSteps = json::array();
    const auto replay = RunPlaythrough(repaired.spoilerJson, ootOracle, mmOracle, "repaired reward topology", nullptr,
                                       &repairedSteps, oot.dump(), mm.dump(), false);
    Check(replay.beatable && replay.unreachableOot == 0 && replay.unreachableMm == 0,
          "the repaired native reward ordering wins from empty inventory");
    std::map<std::string, int> rewardSpheres;
    for (const auto& sphere : repairedSteps) {
        for (const auto& step : sphere.at("steps")) {
            if (step.at("game") == "oot") {
                rewardSpheres[step.at("item").get<std::string>()] = sphere.at("sphere").get<int>();
            }
        }
    }
    Check(rewardSpheres.count("Kokiri Emerald") && rewardSpheres.count("Forest Medallion") &&
              rewardSpheres.at("Kokiri Emerald") < rewardSpheres.at("Forest Medallion"),
          "forward replay collects the moved Emerald before opening the gated boss reward");

    // Neither a reachable overworld chest nor a same-named MM boss may rescue a blocked native domain.
    freeChecks[GAME_OOT] = { "OoT free chest", "Link's Pocket" };
    gatedChecks[GAME_OOT]["Phantom Ganon"] = { "Kokiri Emerald" };
    const auto blocked = Fill(oot, mm, OotAccess::ALL_REACHABLE, GAME_OOT, 0, "", forced);
    Check(!blocked.success && blocked.error.find("dead end placing") != std::string::npos,
          "a blocked reward domain must fail rather than escape to an overworld or foreign check");

    for (auto& reward : oot["fixed"]) {
        reward["allowedChecks"].push_back("Link's Pocket");
    }
    const auto invalidPocket = Fill(oot, mm, OotAccess::ALL_REACHABLE, GAME_OOT, 0, "", forced);
    Check(!invalidPocket.success && invalidPocket.error.find("not an eligible check") != std::string::npos,
          "the forced Pocket check cannot accidentally join the native boss reward domain");
}

void RewardOrderingPreservesScarceBossChecks() {
    ResetWorld();
    json oot = EmptyDump(), mm = EmptyDump();
    const std::array<std::pair<const char*, const char*>, 3> rewards = { { { "Queen Gohma", "Kokiri Emerald" },
                                                                           { "King Dodongo", "Goron Ruby" },
                                                                           { "Phantom Ganon", "Forest Medallion" } } };
    for (const auto& [check, item] : rewards) {
        oot["items"].push_back({ { "name", item }, { "token", std::string("RI_") + item } });
        oot["fixed"].push_back(
            { { "check", check },
              { "item", item },
              { "advancement", true },
              { "major", true },
              { "hintable", true },
              { "validate", true },
              { "allowedChecks", json::array({ "Queen Gohma", "King Dodongo", "Phantom Ganon" }) } });
    }
    freeChecks[GAME_OOT] = { "Queen Gohma", "King Dodongo" };
    gatedChecks[GAME_OOT]["Phantom Ganon"] = { "Kokiri Emerald", "Goron Ruby" };
    ootGoalItems = { "Kokiri Emerald", "Goron Ruby", "Forest Medallion" };
    // Without a stone, only the two early bosses are legal. The medallion can use any boss.
    // Seed 9 wastes an early boss on the medallion in all three original random-order passes.
    const auto result = CrossWorldCombinedFill(oot.dump(), mm.dump(), 9, ootOracle, mmOracle);
    Check(result.success, "reward ordering preserves scarce early boss checks (" + result.error + ")");
    if (!result.success) {
        return;
    }
    const auto placements = ParseSpoilerPlacements(result.spoilerJson, oot.dump(), mm.dump());
    std::set<std::string> items, checks;
    for (const auto& p : placements) {
        items.insert(p.item);
        checks.insert(p.check);
        Check(p.itemGame == GAME_OOT && p.checkGame == GAME_OOT,
              "scarce-domain ordering preserves native reward and check ownership");
        Check(p.item == "Forest Medallion" ? p.check == "Phantom Ganon"
                                           : (p.check == "Queen Gohma" || p.check == "King Dodongo"),
              "both stones retain early capacity while the medallion uses the late boss");
    }
    Check(placements.size() == 3 &&
              items == std::set<std::string>{ "Kokiri Emerald", "Goron Ruby", "Forest Medallion" } &&
              checks == std::set<std::string>{ "Queen Gohma", "King Dodongo", "Phantom Ganon" },
          "scarce-domain ordering preserves every reward and fills each native boss once");
    json steps = json::array();
    const auto replay = RunPlaythrough(result.spoilerJson, ootOracle, mmOracle, "scarce boss reward domain", nullptr,
                                       &steps, oot.dump(), mm.dump(), false);
    Check(replay.beatable && replay.unreachableOot == 0,
          "the scarce-domain placement wins by collecting both stones before the late boss reward");
}

void PortalPrerequisitesRespectNativeDomains() {
    ResetWorld();
    json oot = EmptyDump(), mm = EmptyDump();
    Add(oot, "OoT junk", "OoT portal-side chest", false);
    Add(mm, "MM portal prize", "MM free check");
    oot["items"].push_back({ { "name", "Kokiri Emerald" }, { "token", "RI_KOKIRI_EMERALD" } });
    oot["fixed"].push_back({ { "check", "Queen Gohma" },
                             { "item", "Kokiri Emerald" },
                             { "advancement", true },
                             { "validate", true },
                             { "allowedChecks", json::array({ "Queen Gohma" }) } });
    ootPortalItem = "Kokiri Emerald";
    ootGoalItems = { "Kokiri Emerald" };
    mmGoalItem = "MM portal prize";
    freeChecks[GAME_OOT] = { "Queen Gohma", "OoT portal-side chest" };
    freeChecks[GAME_MM] = { "MM free check" };
    const auto valid = Fill(oot, mm, OotAccess::ALL_REACHABLE);
    Check(valid.success, "a reachable native boss can supply the portal prerequisite (" + valid.error + ")");
    if (valid.success) {
        const auto placements = ParseSpoilerPlacements(valid.spoilerJson, oot.dump(), mm.dump());
        int emeralds = 0;
        for (const auto& p : placements) {
            if (p.item == "Kokiri Emerald") {
                ++emeralds;
                Check(p.itemGame == GAME_OOT && p.checkGame == GAME_OOT && p.check == "Queen Gohma",
                      "portal prerequisite placement must preserve the reward's native boss domain");
            }
        }
        Check(placements.size() == 3 && emeralds == 1,
              "portal prerequisite placement preserves the native reward and total item count");
        json steps = json::array();
        Check(RunPlaythrough(valid.spoilerJson, ootOracle, mmOracle, "confined portal prerequisite", nullptr, &steps,
                             oot.dump(), mm.dump())
                  .beatable,
              "forward replay earns the confined reward before crossing the closed portal");
    }

    // There is an ordinary reachable OoT check, but the only legal reward boss is behind the reward.
    // Opening the portal by using that ordinary check would silently violate End-of-Dungeon settings.
    freeChecks[GAME_OOT] = { "OoT portal-side chest" };
    gatedChecks[GAME_OOT]["Queen Gohma"] = { "Kokiri Emerald" };
    const auto blocked = Fill(oot, mm, OotAccess::ALL_REACHABLE);
    Check(!blocked.success && blocked.error.find("portal's prerequisites") != std::string::npos,
          "an impossible confined portal prerequisite cannot escape to a reachable ordinary OoT check");

    json unconstrained = EmptyDump();
    Add(unconstrained, "Kokiri Emerald", "OoT portal-side chest");
    const auto ordinary = Fill(unconstrained, mm, OotAccess::ALL_REACHABLE);
    Check(ordinary.success,
          "an unconstrained portal prerequisite still uses an ordinary OoT check (" + ordinary.error + ")");
}

bool RewardTopologyPermits(const std::vector<std::string>& rewards, const std::vector<std::string>& domain,
                           const std::vector<std::string>& pocket = {}, size_t* callbackCalls = nullptr) {
    return HasNativeRewardPlacement(rewards.size(), [&](uint16_t collected) {
        if (callbackCalls) {
            ++*callbackCalls;
        }
        std::vector<std::string> inventory = pocket;
        for (size_t i = 0; i < rewards.size(); ++i) {
            if (collected & (1u << i)) {
                inventory.push_back(rewards[i]);
            }
        }
        const auto reachable = QueryReachable(ootOracle, inventory);
        return static_cast<size_t>(std::count_if(domain.begin(), domain.end(),
                                                 [&](const std::string& check) { return reachable.count(check); }));
    });
}

void NativeRewardTopologyRejectsUnavoidableCycles() {
    ResetWorld();
    const std::vector<std::string> stones = { "Kokiri Emerald", "Goron Ruby", "Zora Sapphire" };
    const std::vector<std::string> rewards = { "Kokiri Emerald", "Goron Ruby", "Zora Sapphire", "Forest Medallion" };
    const std::vector<std::string> bosses = { "Queen Gohma", "King Dodongo", "Barinade", "Phantom Ganon" };
    freeChecks[GAME_OOT] = { "Queen Gohma" };
    for (const auto* boss : { "King Dodongo", "Barinade", "Phantom Ganon" }) {
        gatedChecks[GAME_OOT][boss] = stones;
    }
    Check(!RewardTopologyPermits(rewards, bosses),
          "one early boss cannot supply the three stones required to reach the other bosses");

    freeChecks[GAME_OOT] = { "Queen Gohma", "King Dodongo", "Barinade" };
    gatedChecks[GAME_OOT].clear();
    gatedChecks[GAME_OOT]["Phantom Ganon"] = stones;
    Check(RewardTopologyPermits(rewards, bosses),
          "three early bosses can supply all three stones before the medallion boss");

    freeChecks[GAME_OOT] = { "Queen Gohma", "King Dodongo" };
    Check(RewardTopologyPermits({ "Goron Ruby", "Zora Sapphire", "Forest Medallion" },
                                { "Queen Gohma", "King Dodongo", "Phantom Ganon" }, { "Kokiri Emerald" }),
          "a stone already in Pocket leaves only two stones needing early boss slots");

    ResetWorld();
    gatedChecks[GAME_OOT]["Queen Gohma"] = { "Goron Ruby" };
    gatedChecks[GAME_OOT]["King Dodongo"] = { "Zora Sapphire" };
    gatedChecks[GAME_OOT]["Barinade"] = { "Kokiri Emerald" };
    Check(!RewardTopologyPermits(stones, { "Queen Gohma", "King Dodongo", "Barinade" }),
          "an all-rewards-reachable graph still fails when its reward cycle has no first pickup");

    ResetWorld();
    freeChecks[GAME_OOT] = { "Queen Gohma", "King Dodongo", "Barinade" };
    gatedChecks[GAME_OOT]["Phantom Ganon"] = stones;
    gatedChecks[GAME_OOT]["Volvagia"] = stones;
    Check(RewardTopologyPermits(rewards, bosses), "the third eligible early boss supplies the required capacity");
    Check(!RewardTopologyPermits(rewards, { "Queen Gohma", "King Dodongo", "Phantom Ganon", "Volvagia" }),
          "a reachable but excluded early boss cannot count as movable reward capacity");

    ResetWorld();
    const std::vector<std::string> allRewards = { "Kokiri Emerald",   "Goron Ruby",      "Zora Sapphire",
                                                  "Forest Medallion", "Fire Medallion",  "Water Medallion",
                                                  "Spirit Medallion", "Shadow Medallion" };
    const std::vector<std::string> allBosses = { "Queen Gohma", "King Dodongo", "Barinade", "Phantom Ganon",
                                                 "Volvagia",    "Morpha",       "Twinrova", "Bongo Bongo" };
    freeChecks[GAME_OOT] = { "Queen Gohma", "King Dodongo", "Barinade", "Phantom Ganon",
                             "Volvagia",    "Morpha",       "Twinrova" };
    gatedChecks[GAME_OOT]["Bongo Bongo"] = allRewards;
    size_t callbackCalls = 0;
    Check(!RewardTopologyPermits(allRewards, allBosses, {}, &callbackCalls),
          "seven early bosses cannot supply the eighth reward locked behind all eight");
    Check(callbackCalls <= 256, "eight-reward feasibility uses at most 256 native reachability queries");
    freeChecks[GAME_OOT].push_back("Bongo Bongo");
    gatedChecks[GAME_OOT].clear();
    Check(RewardTopologyPermits(allRewards, allBosses), "eight reachable native bosses can hold all eight rewards");

    ResetWorld();
    Check(RewardTopologyPermits({}, {}), "an empty reward domain is already feasible");
    Check(!HasNativeRewardPlacement(9, [](uint16_t) { return size_t{ 9 }; }),
          "an unsupported reward count is rejected instead of exceeding the eight-reward bound");
}
} // namespace

int main() {
    for (bool nativeFamily : { true, false }) {
        for (GameId start : { GAME_OOT, GAME_MM }) {
            SharedSourceOpensConfinedKey(nativeFamily, start);
        }
    }
    OotOnlyNoLogicRemainsPermissive();
    BeatableOnlyCannotStrandSharedSource();
    FinalValidationChecksSharedFixedSource();
    DungeonRewardsFollowFinalEntrances();
    RewardOrderingPreservesScarceBossChecks();
    PortalPrerequisitesRespectNativeDomains();
    NativeRewardTopologyRejectsUnavoidableCycles();
    std::cout << (failures ? "FAIL" : "PASS") << ": shared progression and native reward-domain regressions ("
              << failures << " failed assertions)\n";
    return failures ? 1 : 0;
}
