#pragma once

#include "CrossDungeonCoordinator.h"
#include "CrossWorldRando.h"

namespace ComboRando::CrossDungeon {

inline constexpr size_t kStructuralLayoutAttempts = 128;
class StructureExhausted : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

inline void CheckStructuralCancellation(const ComboGenProgress* progress) {
    if (GenCancelled(progress))
        throw std::runtime_error("generation cancelled during entrance validation");
}

// Preserve counts: two Hookshots or several dungeon keys are not the same inventory as one.
inline std::vector<std::string> StructuralOwned(const nlohmann::json& dump) {
    std::vector<std::string> owned;
    for (const auto& item : dump.at("pool"))
        owned.push_back(item.at("name").get<std::string>());
    return owned;
}

// Forced checks are granted at save creation. Reserve copies already represented by the
// free pool or movable native placements; native Pocket rewards can instead be omitted
// from both sources. A category-only force draws from that same inventory during fill.
inline void AddStructuralForcedItems(const nlohmann::json& dump, const std::string& forcedJson,
                                     std::vector<std::string>& owned) {
    if (forcedJson.empty())
        return;
    const auto forced = nlohmann::json::parse(forcedJson);
    if (!forced.is_object())
        throw std::invalid_argument("invalid structural forced-placement object");
    std::unordered_map<std::string, size_t> available;
    std::unordered_set<std::string> known;
    for (const auto& name : owned) {
        ++available[name];
        known.insert(name);
    }
    for (const auto& item : dump.value("items", nlohmann::json::array()))
        known.insert(item.at("name").get<std::string>());
    for (const auto& item : dump.value("fixed", nlohmann::json::array()))
        known.insert(item.at("item").get<std::string>());
    for (const auto& [check, spec] : forced.items()) {
        if (!spec.is_object())
            throw std::invalid_argument("invalid structural forced placement at " + check);
        if (!spec.contains("item"))
            continue;
        const auto name = spec.at("item").get<std::string>();
        if (name.empty() || !known.count(name))
            throw std::invalid_argument("unknown native structural forced item at " + check + ": " + name);
        if (available[name])
            --available[name];
        else
            owned.push_back(name);
    }
}

// startKnown stores check identities, not item values. Resolve those identities from
// the recorded placements, including foreign ownership; never roll another Pocket.
// Older consolidated spoilers omit startKnown but retain the native Pocket placement.
inline nlohmann::json RecordedForcedOot(const nlohmann::json& spoiler) {
    auto forced = nlohmann::json::object();
    const auto oot = spoiler.value("oot", nlohmann::json::object());
    const auto placements = oot.contains("placements") ? oot.at("placements") : oot;
    if (!placements.is_object())
        throw std::invalid_argument("invalid recorded OOT placement object");
    const auto foreign = spoiler.value("foreign", nlohmann::json::array());
    if (!foreign.is_array())
        throw std::invalid_argument("invalid recorded foreign-placement array");
    const auto add = [&](const std::string& game, const std::string& check) {
        if (game != "oot")
            throw std::invalid_argument("unsupported recorded forced check game: " + game);
        if (check.empty() || !placements.contains(check) || !placements.at(check).is_string())
            throw std::invalid_argument("missing recorded forced placement at " + check);
        auto item = placements.at(check).get<std::string>();
        std::string owner = "oot";
        bool foreignFound = false;
        for (const auto& entry : foreign) {
            if (entry.value("checkGame", std::string()) != game || entry.value("checkName", std::string()) != check)
                continue;
            if (foreignFound)
                throw std::invalid_argument("duplicate recorded foreign placement at " + check);
            foreignFound = true;
            owner = entry.at("itemGame").get<std::string>();
            item = entry.at("itemName").get<std::string>();
        }
        if (owner != "oot" || (!foreignFound && item.size() >= 5 && item.substr(item.size() - 5) == " (MM)"))
            throw std::invalid_argument("recorded forced item is not native OOT at " + check);
        item = StripGameSuffix(std::move(item));
        if (item.empty())
            throw std::invalid_argument("empty recorded forced item at " + check);
        forced[check] = { { "item", item } };
    };
    if (spoiler.contains("startKnown")) {
        if (!spoiler.at("startKnown").is_array())
            throw std::invalid_argument("invalid recorded startKnown array");
        for (const auto& entry : spoiler.at("startKnown"))
            add(entry.at("checkGame").get<std::string>(), entry.at("checkName").get<std::string>());
    } else if (placements.contains("Link's Pocket")) {
        add("oot", "Link's Pocket");
    }
    return forced;
}

// Regenerating the native dump can remove its chosen Pocket from the pool. If that
// concrete choice differs from the recorded start, the dump no longer describes the
// saved seed's supply. Reject instead of silently adding one reward and losing another.
inline void ValidateRecordedForcedOot(const std::string& nativeJson, const nlohmann::json& recorded,
                                      const std::string& ootDump) {
    const auto native = nativeJson.empty() ? nlohmann::json::object() : nlohmann::json::parse(nativeJson);
    if (!native.is_object() || !recorded.is_object() || native.size() != recorded.size())
        throw std::invalid_argument("recorded forced placements differ from the prepared native pool");
    const auto dump = nlohmann::json::parse(ootDump);
    std::vector<std::pair<std::string, bool>> sources;
    for (const auto& item : dump.at("pool"))
        sources.emplace_back(item.at("name").get<std::string>(), item.value("advancement", true));
    for (const auto& item : dump.value("fixed", nlohmann::json::array()))
        if (item.contains("allowedChecks"))
            sources.emplace_back(item.at("item").get<std::string>(), item.value("advancement", true));
    for (const auto& [check, spec] : native.items()) {
        if (!spec.is_object() || !recorded.contains(check) || !recorded.at(check).contains("item") ||
            (spec.contains("item") && spec.at("item") != recorded.at(check).at("item")))
            throw std::invalid_argument("recorded forced placement differs from the prepared native pool at " + check);
        const auto item = recorded.at(check).at("item").get<std::string>();
        const auto category = spec.value("category", std::string("any"));
        const auto source = std::find_if(sources.begin(), sources.end(), [&](const auto& source) {
            return source.first == item && (spec.contains("item") || category == "any" || source.second);
        });
        if (!spec.contains("item") && ((category != "any" && category != "advancement") || source == sources.end()))
            throw std::invalid_argument("recorded forced category item is absent from the native pool at " + check);
        if (source != sources.end())
            sources.erase(source);
    }
}

inline std::unordered_set<std::string> StructuralQuery(const OracleFns& oracle, const std::vector<std::string>& owned) {
    if (!oracle.Reset || !oracle.SetOwnedItems || !oracle.GetReachableChecks)
        throw std::runtime_error("cross-dungeon structural oracle exports missing");
    oracle.Reset();
    oracle.SetOwnedItems(nlohmann::json(owned).dump().c_str());
    const auto reachable = nlohmann::json::parse(oracle.GetReachableChecks());
    if (!reachable.is_array())
        throw std::invalid_argument("invalid structural oracle check array");
    std::unordered_set<std::string> result;
    for (const auto& name : reachable)
        result.insert(name.get<std::string>());
    return result;
}

struct StructureProbe {
    struct Fixed {
        std::string check, item;
    };
    std::vector<std::string> ootOwned, mmOwned;
    std::vector<Fixed> ootFixed, mmFixed;
    std::unordered_set<std::string> ootRequired, mmRequired;
    bool mmInitiallyOpen = false;
    const ComboGenProgress* progress = nullptr;

    StructureProbe(const std::string& ootDump, const std::string& mmDump, const OracleFns& oot, const OracleFns& mm,
                   bool mmStart, const ComboGenProgress* progress = nullptr, const std::string& forcedOotJson = "")
        : progress(progress) {
        const auto od = nlohmann::json::parse(ootDump), md = nlohmann::json::parse(mmDump);
        ootOwned = StructuralOwned(od);
        mmOwned = StructuralOwned(md);
        // These native randomized items have not been placed yet. Structural
        // connectivity assumes them like pool items; the later item fill must
        // earn them at a reachable member of their native allowed-check set.
        for (auto [dump, owned] : { std::pair{ &od, &ootOwned }, std::pair{ &md, &mmOwned } })
            for (const auto& entry : dump->value("fixed", nlohmann::json::array()))
                if (entry.contains("allowedChecks"))
                    owned->push_back(entry.at("item").get<std::string>());
        AddStructuralForcedItems(od, forcedOotJson, ootOwned);
        auto fixed = [](const nlohmann::json& dump) {
            std::vector<Fixed> placements;
            for (const auto& entry : dump.value("fixed", nlohmann::json::array()))
                if (!entry.contains("allowedChecks"))
                    placements.push_back({ entry.at("check").get<std::string>(), entry.at("item").get<std::string>() });
            return placements;
        };
        ootFixed = fixed(od);
        mmFixed = fixed(md);
        mmInitiallyOpen = mmStart || OotAccessFromDump(ootDump) == OotAccess::NO_LOGIC;
        auto required = [](const nlohmann::json& dump, const auto& reach) {
            std::unordered_set<std::string> checks;
            for (const auto& check : dump.at("checks")) {
                const auto name = check.at("name").template get<std::string>();
                if (reach.count(name))
                    checks.insert(name);
            }
            for (const auto& fixed : dump.value("fixed", nlohmann::json::array())) {
                const auto name = fixed.at("check").template get<std::string>();
                if (fixed.value("advancement", true) && reach.count(name))
                    checks.insert(name);
            }
            return checks;
        };
        // Native logic intentionally omits some goal-gated or under-modelled checks even with every
        // item. Compare with its actual baseline, rather than demanding impossible named checks.
        const auto native = Closure(oot, mm, false);
        ootRequired = required(od, native.oot);
        mmRequired = required(md, native.mm);
    }

    JointReach Closure(const OracleFns& oot, const OracleFns& mm, bool joint) const {
        auto oo = ootOwned, mo = mmOwned;
        std::vector<bool> ootCredited(ootFixed.size(), false), mmCredited(mmFixed.size(), false);
        const size_t maxQueries = ootFixed.size() + mmFixed.size() + 1;
        const auto query = [&](const OracleFns& oracle, const std::vector<std::string>& owned) {
            CheckStructuralCancellation(progress);
            auto reached = StructuralQuery(oracle, owned);
            CheckStructuralCancellation(progress);
            return reached;
        };
        for (size_t pass = 0; pass < maxQueries; ++pass) {
            auto reach = joint ? QueryJoint(oot, mm, oo, mo, mmInitiallyOpen, query)
                               : JointReach{ query(oot, oo), query(mm, mo) };
            auto collect = [](const auto& fixed, auto& credited, const auto& reached, auto& owned) {
                bool changed = false;
                for (size_t i = 0; i < fixed.size(); ++i)
                    if (!credited[i] && reached.count(fixed[i].check)) {
                        owned.push_back(fixed[i].item);
                        credited[i] = true;
                        changed = true;
                    }
                return changed;
            };
            // A confined key/song/reward is still earned at its real check. Granting every fixed
            // item up front would certify a dungeon that locks its own required song behind it.
            const bool ootChanged = collect(ootFixed, ootCredited, reach.oot, oo);
            const bool mmChanged = collect(mmFixed, mmCredited, reach.mm, mo);
            if (!ootChanged && !mmChanged)
                return reach;
        }
        throw std::runtime_error("cross-dungeon fixed-item structural closure did not converge");
    }

    std::string Missing(const OracleFns& oot, const OracleFns& mm) const {
        const auto joint = Closure(oot, mm, true);
        auto missing = [](const auto& expected, const auto& reached, const char* game) {
            std::vector<std::string> lost;
            for (const auto& check : expected)
                if (!reached.count(check))
                    lost.push_back(check);
            std::sort(lost.begin(), lost.end());
            return lost.empty() ? std::string()
                                : std::string(game) + " loses " + std::to_string(lost.size()) +
                                      " native-reachable checks (e.g. " + lost.front() + ")";
        };
        const auto ootLost = missing(ootRequired, joint.oot, "OOT");
        const auto mmLost = missing(mmRequired, joint.mm, "MM");
        return ootLost + (ootLost.empty() || mmLost.empty() ? "" : "; ") + mmLost;
    }
};

inline void ClearStructuralAccess(const Coordinator& coordinator) {
    // The all-owned fixed point must not leave arrival roots in the next item-fill query (whose
    // portal prerequisite derivation begins with an OoT-only search before its first joint query).
    for (const auto& host : coordinator.hosts)
        if (host.SetAccess)
            host.SetAccess("{\"overworld\":true,\"arrivals\":[]}");
}

// Sparse valid layouts become expensive to find by replacing every pool at once. Keep
// native reachability after each accepted cycle, then split a rejected cycle into swaps.
// Every candidate is still a full, bijective, pool-isolated map checked by the real oracle.
template <typename IsValid>
std::optional<nlohmann::json> GenerateConstrainedSeed(const std::vector<Dungeon>& dungeons, uint64_t masterSeed,
                                                      IsValid isValid) {
    const auto desired = GenerateSeed(dungeons, masterSeed).at("permutation").get<std::vector<size_t>>();
    std::vector<size_t> current(dungeons.size());
    std::iota(current.begin(), current.end(), 0);
    if (!isValid(Encode(dungeons, current)))
        return std::nullopt;
    std::vector<bool> visited(dungeons.size(), false);
    bool changed = false;
    for (size_t first = 0; first < desired.size(); ++first) {
        if (visited[first])
            continue;
        std::vector<size_t> cycle;
        for (size_t at = first; !visited[at]; at = desired[at]) {
            visited[at] = true;
            cycle.push_back(at);
        }
        if (cycle.size() < 2)
            continue;
        auto candidate = current;
        for (size_t member : cycle)
            candidate[member] = desired[member];
        if (isValid(Encode(dungeons, candidate))) {
            current = std::move(candidate);
            changed = true;
            continue;
        }
        // Adjacent pairs let a constrained member stay fixed while its neighbors shuffle.
        for (size_t pair = 1; pair < cycle.size(); ++pair) {
            candidate = current;
            std::swap(candidate[cycle[pair - 1]], candidate[cycle[pair]]);
            if (isValid(Encode(dungeons, candidate))) {
                current = std::move(candidate);
                changed = true;
            }
        }
    }
    if (!changed)
        return std::nullopt;
    auto result = Encode(dungeons, current);
    // The last rejected candidate may still be installed in the temporary oracle.
    if (!isValid(result))
        throw std::runtime_error("constrained cross-entrance validation was not deterministic");
    return result;
}

// Native entrances are already shuffled. Only the cross-map's isolated stream is retried; no native
// RNG, item-fill RNG, settings or gameplay routing is changed by this layout validation.
inline nlohmann::json GenerateValidatedSeed(Coordinator& coordinator, bool enabled, uint64_t masterSeed,
                                            const std::string& ootDump, const std::string& mmDump, OracleFns& oot,
                                            OracleFns& mm, bool mmStart, size_t maxAttempts = kStructuralLayoutAttempts,
                                            const ComboGenProgress* progress = nullptr,
                                            const std::string& forcedOotJson = "") {
    CheckStructuralCancellation(progress);
    if (!enabled) {
        auto seed = coordinator.GenerateAndInstall(false, masterSeed);
        coordinator.ConfigureOracles(oot, mm);
        return seed;
    }
    const auto previous = coordinator.LogicSeed();
    try {
        const auto dungeons = coordinator.Catalogue();
        coordinator.InstallLogic(nullptr);
        coordinator.ConfigureOracles(oot, mm);
        const StructureProbe probe(ootDump, mmDump, oot, mm, mmStart, progress, forcedOotJson);
        std::string missing;
        for (size_t attempt = 0; attempt < maxAttempts; ++attempt) {
            const uint64_t layoutSeed = masterSeed + static_cast<uint64_t>(attempt) * 0x9E3779B97F4A7C15ULL;
            const auto seed = GenerateSeed(dungeons, layoutSeed);
            coordinator.InstallLogic(seed, false);
            coordinator.ConfigureOracles(oot, mm);
            missing = probe.Missing(oot, mm);
            if (missing.empty()) {
                ClearStructuralAccess(coordinator);
                if (attempt)
                    std::cout << "[CrossDungeon] structural layout accepted after " << (attempt + 1) << " attempts\n";
                return seed;
            }
        }
        const bool expandedCodec = std::any_of(dungeons.begin(), dungeons.end(), [](const Dungeon& dungeon) {
            return dungeon.pool != Pool::Dungeon || dungeon.expanded;
        });
        if (expandedCodec) {
            auto repaired = GenerateConstrainedSeed(dungeons, masterSeed, [&](const nlohmann::json& candidate) {
                coordinator.InstallLogic(candidate, false);
                coordinator.ConfigureOracles(oot, mm);
                missing = probe.Missing(oot, mm);
                return missing.empty();
            });
            if (repaired) {
                ClearStructuralAccess(coordinator);
                std::cout << "[CrossDungeon] expanded layout accepted with validated cycles after " << maxAttempts
                          << " whole-map attempts\n";
                return *repaired;
            }
        }
        throw StructureExhausted("no structurally valid cross-dungeon layout after " + std::to_string(maxAttempts) +
                                 " attempts: " + missing);
    } catch (...) {
        coordinator.InstallLogic(previous, false);
        coordinator.ConfigureOracles(oot, mm);
        if (Coordinator::Enabled(previous))
            ClearStructuralAccess(coordinator);
        throw;
    }
}

// Replay is an exact-map operation: never replace an incompatible recorded layout with a new roll.
inline void ValidateSavedStructure(Coordinator& coordinator, const nlohmann::json& seed, const std::string& ootDump,
                                   const std::string& mmDump, OracleFns& oot, OracleFns& mm, bool mmStart,
                                   const std::string& forcedOotJson = "") {
    if (!Coordinator::Enabled(seed)) {
        coordinator.InstallLogic(seed);
        coordinator.ConfigureOracles(oot, mm);
        return;
    }
    const auto previous = coordinator.LogicSeed();
    try {
        coordinator.ValidateCatalogue(seed);
        coordinator.InstallLogic(nullptr);
        coordinator.ConfigureOracles(oot, mm);
        const StructureProbe probe(ootDump, mmDump, oot, mm, mmStart, nullptr, forcedOotJson);
        coordinator.InstallLogic(seed, false);
        coordinator.ConfigureOracles(oot, mm);
        const auto missing = probe.Missing(oot, mm);
        if (!missing.empty())
            throw std::invalid_argument("recorded cross-dungeon layout is structurally invalid: " + missing);
        ClearStructuralAccess(coordinator);
    } catch (...) {
        coordinator.InstallLogic(previous, false);
        coordinator.ConfigureOracles(oot, mm);
        if (Coordinator::Enabled(previous))
            ClearStructuralAccess(coordinator);
        throw;
    }
}

} // namespace ComboRando::CrossDungeon
