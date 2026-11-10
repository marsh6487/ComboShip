#pragma once

#include "CrossDungeonEntrances.h"
#include <array>
#include <numeric>

namespace ComboRando::SoloEntrance {
using namespace CrossDungeon;

struct Policy {
    bool decoupled = false, mixed = false;
    bool dungeons = false, bosses = false, interiors = false, shops = false, grottos = false, overworld = false;
};

inline bool InMixedPool(Pool pool, const Policy& p) {
    if (!p.mixed)
        return false;
    switch (pool) {
        case Pool::Dungeon:
            return p.dungeons;
        case Pool::Boss:
            return p.bosses;
        case Pool::Interior:
        case Pool::DoorInterior:
            return p.interiors;
        case Pool::Shop:
            return p.shops;
        case Pool::Grotto:
            return p.grottos;
        case Pool::Overworld:
            return p.overworld;
    }
    return false;
}

inline int Group(Pool pool, const Policy& policy) {
    return InMixedPool(pool, policy) ? -1 : static_cast<int>(pool);
}

inline void ValidatePermutation(const std::vector<Dungeon>& catalogue, const std::vector<size_t>& permutation,
                                const Policy& policy) {
    if (permutation.size() != catalogue.size())
        throw std::invalid_argument("Solo MM entrance permutation size mismatch");
    std::set<size_t> seen;
    for (size_t i = 0; i < permutation.size(); ++i)
        if (permutation[i] >= catalogue.size() || !seen.insert(permutation[i]).second ||
            Group(catalogue[i].pool, policy) != Group(catalogue[permutation[i]].pool, policy))
            throw std::invalid_argument("Solo MM entrance permutation violates pool selection or bijection");
}

inline std::vector<size_t> Inverse(const std::vector<size_t>& permutation) {
    std::vector<size_t> result(permutation.size());
    std::set<size_t> seen;
    for (size_t i = 0; i < permutation.size(); ++i) {
        if (permutation[i] >= permutation.size() || !seen.insert(permutation[i]).second)
            throw std::invalid_argument("invalid Solo MM entrance permutation");
        result[permutation[i]] = i;
    }
    return result;
}

inline std::vector<size_t> Shuffle(const std::vector<Dungeon>& catalogue, uint64_t seed, const Policy& policy) {
    std::vector<size_t> result(catalogue.size());
    std::iota(result.begin(), result.end(), 0);
    std::map<int, std::vector<size_t>> groups;
    for (size_t i = 0; i < catalogue.size(); ++i)
        groups[Group(catalogue[i].pool, policy)].push_back(i);
    for (const auto& [group, members] : groups) {
        uint64_t stream = seed ^ (0x534f4c4f454e5452ULL + static_cast<uint64_t>(group + 1));
        for (size_t remaining = members.size(); remaining > 1; --remaining) {
            stream = stream * 6364136223846793005ULL + 1442695040888963407ULL;
            std::swap(result[members[remaining - 1]], result[members[(stream >> 33) % remaining]]);
        }
    }
    return result;
}

struct Permutations {
    std::vector<size_t> forward;
    std::vector<size_t> reverse; // physical destination's exit -> chosen native return slot
};

inline Permutations Generate(const std::vector<Dungeon>& catalogue, uint64_t seed, const Policy& policy) {
    auto forward = Shuffle(catalogue, seed, policy);
    auto reverse = policy.decoupled ? Shuffle(catalogue, seed ^ 0x52455455524e4552ULL, policy) : Inverse(forward);
    return { std::move(forward), std::move(reverse) };
}

inline std::vector<Route> BuildRoutes(const std::vector<Dungeon>& catalogue, const std::vector<size_t>& forward,
                                      const std::vector<size_t>& reverse, const Policy& policy) {
    Validate(catalogue);
    ValidatePermutation(catalogue, forward, policy);
    ValidatePermutation(catalogue, reverse, policy);
    if (!policy.decoupled && reverse != Inverse(forward))
        throw std::invalid_argument("coupled Solo MM entrance returns are not inverse ingress routes");
    for (const auto& dungeon : catalogue)
        if (dungeon.entrance.game != 1)
            throw std::invalid_argument("Solo MM layout contains another game's entrance");

    std::vector<Route> routes;
    std::set<Endpoint> bossEscapes;
    for (const auto& dungeon : catalogue)
        if (dungeon.pool == Pool::Boss)
            bossEscapes.insert(dungeon.escapeExits.begin(), dungeon.escapeExits.end());
    for (size_t sourceIndex = 0; sourceIndex < catalogue.size(); ++sourceIndex) {
        const auto& source = catalogue[sourceIndex];
        const auto& target = catalogue[forward[sourceIndex]];
        routes.push_back({ Phase::AfterNative, source.entrance, target.entrance, target.id,
                           target.pool == Pool::Boss ? std::optional<Endpoint>(target.entrance) : std::nullopt });
    }
    std::vector<Route> nonbossReturns;
    for (size_t targetIndex = 0; targetIndex < catalogue.size(); ++targetIndex) {
        const auto& target = catalogue[targetIndex];
        const auto& source = catalogue[reverse[targetIndex]];
        if (target.pool == Pool::Boss)
            continue;
        if (target.hasReverse)
            nonbossReturns.push_back({ Phase::BeforeNative, target.exit, source.returnTo, source.id,
                                       source.pool == Pool::Boss ? source.reverseCheckpoint : std::nullopt });
        for (auto escape : target.escapeExits)
            nonbossReturns.push_back({ Phase::BeforeNative, escape, source.returnTo, source.id });
    }
    for (const auto& route : nonbossReturns)
        if (!bossEscapes.count(route.from))
            routes.push_back(route);
    for (size_t targetIndex = 0; targetIndex < catalogue.size(); ++targetIndex) {
        const auto& target = catalogue[targetIndex];
        if (target.pool != Pool::Boss)
            continue;
        const auto& source = catalogue[reverse[targetIndex]];
        Endpoint destination = source.returnTo;
        auto checkpoint = source.pool == Pool::Boss ? source.warpCheckpoint : std::nullopt;
        if (source.pool == Pool::Boss) {
            destination = *source.warpReturn;
            // Odolwa's own return slot remains in the temple's prison. Other
            // boss exits compose with the selected owning-dungeon return.
            const bool internal = source.boss == 8 && source.warpCheckpoint.has_value();
            bool composed = false;
            if (!internal) {
                for (auto escape : source.escapeExits) {
                    const auto route = std::find_if(nonbossReturns.begin(), nonbossReturns.end(), [&](const Route& r) {
                        return r.phase == Phase::BeforeNative && r.from == escape;
                    });
                    if (route != nonbossReturns.end()) {
                        destination = route->to;
                        checkpoint = route->arrivalCheckpoint;
                        composed = true;
                        break;
                    }
                }
                if (!composed)
                    for (size_t i = 0; i < catalogue.size(); ++i)
                        if (catalogue[i].entrance == destination) {
                            destination = catalogue[forward[i]].entrance;
                            checkpoint = destination;
                            break;
                        }
            }
        }
        if (target.hasReverse)
            routes.push_back({ Phase::BeforeNative, target.exit, source.returnTo, source.id,
                               source.pool == Pool::Boss ? source.reverseCheckpoint : std::nullopt });
        for (auto escape : target.escapeExits)
            routes.push_back({ Phase::BeforeNative, escape, destination, source.id, checkpoint });
    }
    std::set<std::tuple<Phase, int, int32_t>> unique;
    for (const auto& route : routes)
        if (!unique.emplace(route.phase, route.from.game, route.from.index).second || route.from.game != 1 ||
            route.to.game != 1 || (route.arrivalCheckpoint && route.arrivalCheckpoint->game != 1))
            throw std::invalid_argument("ambiguous or foreign Solo MM entrance route");
    return routes;
}

inline bool PhysicalBossClears(const std::vector<Dungeon>& catalogue, const Policy& policy) {
    const bool hasBoss =
        std::any_of(catalogue.begin(), catalogue.end(), [](const Dungeon& d) { return d.pool == Pool::Boss; });
    return hasBoss && InMixedPool(Pool::Boss, policy) &&
           std::any_of(catalogue.begin(), catalogue.end(),
                       [&](const Dungeon& d) { return d.pool != Pool::Boss && InMixedPool(d.pool, policy); });
}

inline int BossSource(const std::vector<Dungeon>& catalogue, const std::vector<size_t>& forward, const Policy& policy,
                      int physicalBoss) {
    if (physicalBoss < 8 || physicalBoss > 11)
        return -1;
    if (PhysicalBossClears(catalogue, policy))
        return physicalBoss;
    for (size_t source = 0; source < catalogue.size(); ++source)
        if (catalogue[forward[source]].pool == Pool::Boss && catalogue[forward[source]].boss == physicalBoss)
            return catalogue[source].pool == Pool::Boss ? catalogue[source].boss : -1;
    return -1;
}
} // namespace ComboRando::SoloEntrance
