#include "Rando/SoloEntrances.h"
#include "Rando/Rando.h"
#include "combo/ComboExport.h"
#include "combo/rando/CrossDungeonRuntime.h"
#include "combo/rando/CrossDungeonState.h"
#include "combo/rando/CrossEntranceBossMap.h"
#include "combo/rando/CrossEntranceMmCatalogue.h"
#include "combo/rando/SoloEntranceLayout.h"
#include "combo/rando/SoloEntranceMmOverworld.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <numeric>
#include <optional>
#include <stdexcept>

namespace Rando::SoloEntrances {
namespace {
using namespace ComboRando::CrossDungeon;
constexpr uint32_t kVersion = 2;
constexpr uint16_t kOptionMask = 0xff;
constexpr size_t kLayoutAttempts = 128;
constexpr uint64_t kStream = 0x534f4c4f4d4d4552ULL;
constexpr std::array<RandoOptionId, 8> kSettings = {
    RO_SHUFFLE_DUNGEON_ENTRANCES, RO_SHUFFLE_EXPANDED_DUNGEON_ENTRANCES,
    RO_SHUFFLE_BOSS_ENTRANCES,    RO_SHUFFLE_INTERIOR_ENTRANCES,
    RO_SHUFFLE_SHOP_ENTRANCES,    RO_SHUFFLE_DOOR_INTERIORS,
    RO_SHUFFLE_GROTTO_ENTRANCES,  RO_SHUFFLE_OVERWORLD_ENTRANCES
};
constexpr std::array<RandoOptionId, 8> kPolicySettings = { RO_DECOUPLED_ENTRANCES,    RO_MIX_ENTRANCE_POOLS,
                                                           RO_MIX_DUNGEON_ENTRANCES,  RO_MIX_BOSS_ENTRANCES,
                                                           RO_MIX_INTERIOR_ENTRANCES, RO_MIX_SHOP_ENTRANCES,
                                                           RO_MIX_GROTTO_ENTRANCES,   RO_MIX_OVERWORLD_ENTRANCES };
constexpr std::array<const char*, 8> kPolicyNames = { "decoupled",    "mixed",    "mixDungeons", "mixBosses",
                                                      "mixInteriors", "mixShops", "mixGrottos",  "mixOverworld" };
// This override is written only by the explicit headless replay API. Native
// generation, JSON preview reads and selected-slot binding never change it.
std::optional<RandoSoloEntranceSave> oracleSeed;

bool SoloMode() {
#ifdef COMBO_BUILD
    return gComboGameMode == COMBO_GAME_MM;
#else
    return false;
#endif
}

PoolOptions Options(uint16_t mask) {
    if (mask & ~kOptionMask)
        throw std::invalid_argument("unknown Solo MM entrance options");
    PoolOptions p;
    p.dungeons = mask & 1;
    p.expanded = mask & 2;
    p.bosses = mask & 4;
    p.interiors = mask & 8;
    p.shops = mask & 16;
    p.doorInteriors = mask & 32;
    p.grottos = mask & 64;
    if ((p.expanded && !p.dungeons) || (p.doorInteriors && p.interiors))
        throw std::invalid_argument("inconsistent Solo MM entrance options");
    return p;
}

uint16_t Mask(PoolOptions p, bool overworld) {
    if (p.legacyInteriors)
        throw std::invalid_argument("legacy cross-game interiors are not a Solo MM pool");
    return (p.dungeons ? 1 : 0) | (p.expanded ? 2 : 0) | (p.bosses ? 4 : 0) | (p.interiors ? 8 : 0) |
           (p.shops ? 16 : 0) | (p.doorInteriors ? 32 : 0) | (p.grottos ? 64 : 0) | (overworld ? 128 : 0);
}

ComboRando::SoloEntrance::Policy Policy(uint32_t mask) {
    if (mask & ~0xffu)
        throw std::invalid_argument("unknown Solo MM entrance policy");
    return { bool(mask & 1),  bool(mask & 2),  bool(mask & 4),  bool(mask & 8),
             bool(mask & 16), bool(mask & 32), bool(mask & 64), bool(mask & 128) };
}

nlohmann::json PolicyJson(uint32_t mask) {
    Policy(mask);
    auto result = nlohmann::json::object();
    for (size_t i = 0; i < kPolicyNames.size(); ++i)
        result[kPolicyNames[i]] = bool(mask & (1u << i));
    return result;
}

nlohmann::json PoolJson(uint16_t mask) {
    auto result = OptionsJson(Options(mask));
    result["overworld"] = bool(mask & 128);
    return result;
}

std::vector<Dungeon> Catalogue(uint16_t mask) {
    auto result = ComboCrossMmCatalogue::Catalogue(Options(mask));
    if (mask & 128)
        ComboSoloMmOverworld::Append(result);
    Validate(result);
    return result;
}

uint16_t SavedOptions(const RandoSaveInfo& save) {
    uint16_t mask = 0;
    for (size_t i = 0; i < kSettings.size(); ++i) {
        if (save.randoSaveOptions[kSettings[i]] > 1)
            throw std::invalid_argument("invalid Solo MM entrance option value");
        if (save.randoSaveOptions[kSettings[i]])
            mask |= 1u << i;
    }
    // The broad pool owns doors, and expanded dungeons depend on dungeon shuffle.
    // Disabled subordinate UI values may still be remembered for the next edit.
    if (!(mask & 1))
        mask &= ~2u;
    if (mask & 8)
        mask &= ~32u;
    return mask;
}

uint32_t SavedPolicy(const RandoSaveInfo& save) {
    uint32_t result = 0;
    for (size_t i = 0; i < kPolicySettings.size(); ++i) {
        if (save.randoSaveOptions[kPolicySettings[i]] > 1)
            throw std::invalid_argument("invalid Solo MM entrance policy value");
        result |= save.randoSaveOptions[kPolicySettings[i]] << i;
    }
    return result;
}

nlohmann::json CatalogueJson(const std::vector<Dungeon>& catalogue) {
    auto result = nlohmann::json::array();
    for (const auto& dungeon : catalogue)
        result.push_back(DungeonJson(dungeon));
    return result;
}

uint32_t Unsigned(const nlohmann::json& value, uint64_t maximum, const char* field) {
    if (!value.is_number_integer() || (!value.is_number_unsigned() && value.get<int64_t>() < 0) ||
        value.get<uint64_t>() > maximum)
        throw std::invalid_argument(std::string("invalid Solo MM entrance ") + field);
    return value.get<uint32_t>();
}

void ValidateRuntime(const nlohmann::json& seed, const std::vector<Dungeon>& catalogue,
                     const std::vector<Route>& routes, int32_t checkpoint, int32_t faroresWindCheckpoint,
                     uint16_t completed, bool bosses) {
    if (checkpoint < -1 || checkpoint > 0xffff || faroresWindCheckpoint < -1 || faroresWindCheckpoint > 0xffff ||
        (completed & ~0x0f00u) || (!bosses && completed))
        throw std::invalid_argument("invalid Solo MM entrance runtime state");
    if (checkpoint >= 0 || faroresWindCheckpoint >= 0) {
        RuntimeState state(1);
        state.InstallValidated(seed, routes, catalogue);
        for (int32_t candidate : { checkpoint, faroresWindCheckpoint })
            if (candidate >= 0 && !state.IsRecoveryEntrance(candidate))
                throw std::invalid_argument("unknown Solo MM entrance checkpoint");
    }
}

nlohmann::json EncodePod(const RandoSoloEntranceSave& saved) {
    if (saved.version == 0 && saved.count == 0)
        return nullptr;
    if (saved.version != kVersion || saved.count == 0 || saved.count > std::size(saved.permutation))
        throw std::invalid_argument("invalid Solo MM entrance saved layout");
    const auto options = Options(saved.options);
    const auto catalogue = Catalogue(saved.options);
    if (!saved.options || saved.count != catalogue.size())
        throw std::invalid_argument("stale Solo MM entrance catalogue");
    const std::vector<size_t> forward(saved.permutation, saved.permutation + saved.count);
    const std::vector<size_t> reverse(saved.reversePermutation, saved.reversePermutation + saved.count);
    const auto routes = ComboRando::SoloEntrance::BuildRoutes(catalogue, forward, reverse, Policy(saved.policy));
    nlohmann::json seed = { { "version", 4 },
                            { "enabled", true },
                            { "soloVersion", kVersion },
                            { "masterSeed", saved.seed },
                            { "options", PoolJson(saved.options) },
                            { "policy", PolicyJson(saved.policy) },
                            { "dungeons", CatalogueJson(catalogue) },
                            { "permutation", forward },
                            { "reversePermutation", reverse } };
    ValidateRuntime(seed, catalogue, routes, saved.checkpoint, saved.faroresWindCheckpoint, saved.completedBossMask,
                    options.bosses);
    seed["runtime"] = { { "checkpoint", saved.checkpoint },
                        { "faroresWindCheckpoint", saved.faroresWindCheckpoint },
                        { "completedBossMask", saved.completedBossMask } };
    return seed;
}

RandoSoloEntranceSave DecodePod(const nlohmann::json& seed) {
    RandoSoloEntranceSave next{};
    next.checkpoint = -1;
    next.faroresWindCheckpoint = -1;
    if (seed.is_null())
        return next;
    if (!seed.is_object() || Unsigned(seed.at("soloVersion"), kVersion, "version") != kVersion ||
        Unsigned(seed.at("version"), 4, "route version") != 4 || !seed.at("enabled").is_boolean() ||
        !seed.at("enabled").get<bool>())
        throw std::invalid_argument("unsupported Solo MM entrance record");
    auto pool = seed.at("options");
    if (!pool.is_object() || !pool.at("overworld").is_boolean())
        throw std::invalid_argument("invalid Solo MM overworld option");
    const bool overworld = pool.at("overworld").get<bool>();
    pool.erase("overworld");
    const auto options = ParseOptions(pool);
    next.options = Mask(options, overworld);
    Options(next.options);
    if (!next.options || seed.at("options") != PoolJson(next.options))
        throw std::invalid_argument("noncanonical Solo MM entrance options");
    const auto& policy = seed.at("policy");
    if (!policy.is_object())
        throw std::invalid_argument("invalid Solo MM entrance policy record");
    for (size_t i = 0; i < kPolicyNames.size(); ++i) {
        if (!policy.at(kPolicyNames[i]).is_boolean())
            throw std::invalid_argument("Solo MM entrance policy must be boolean");
        if (policy.at(kPolicyNames[i]).get<bool>())
            next.policy |= 1u << i;
    }
    if (policy != PolicyJson(next.policy))
        throw std::invalid_argument("noncanonical Solo MM entrance policy");
    const auto catalogue = Catalogue(next.options);
    if (seed.at("dungeons") != CatalogueJson(catalogue))
        throw std::invalid_argument("Solo MM entrance catalogue is stale or contains foreign endpoints");
    if (catalogue.empty() || catalogue.size() > std::size(next.permutation))
        throw std::invalid_argument("empty or oversized Solo MM entrance layout");
    next.version = kVersion;
    next.seed = Unsigned(seed.at("masterSeed"), UINT32_MAX, "master seed");
    next.count = static_cast<uint16_t>(catalogue.size());
    const auto& forward = seed.at("permutation");
    const auto& reverse = seed.at("reversePermutation");
    if (!forward.is_array() || !reverse.is_array() || forward.size() != next.count || reverse.size() != next.count)
        throw std::invalid_argument("invalid Solo MM entrance permutation sizes");
    for (size_t i = 0; i < next.count; ++i) {
        next.permutation[i] = static_cast<uint16_t>(Unsigned(forward[i], next.count - 1, "forward permutation"));
        next.reversePermutation[i] = static_cast<uint16_t>(Unsigned(reverse[i], next.count - 1, "reverse permutation"));
    }
    const auto routes = ComboRando::SoloEntrance::BuildRoutes(catalogue, forward.get<std::vector<size_t>>(),
                                                              reverse.get<std::vector<size_t>>(), Policy(next.policy));
    const auto runtime = seed.value("runtime", nlohmann::json::object());
    if (!runtime.is_object())
        throw std::invalid_argument("invalid Solo MM entrance runtime record");
    auto readCheckpoint = [&](const char* key) {
        if (!runtime.contains(key))
            return int32_t(-1);
        const auto& checkpoint = runtime.at(key);
        if (!checkpoint.is_number_integer() ||
            (checkpoint.is_number_unsigned() && checkpoint.get<uint64_t>() > 0xffff) ||
            checkpoint.get<int64_t>() < -1 || checkpoint.get<int64_t>() > 0xffff)
            throw std::invalid_argument("invalid Solo MM entrance checkpoint");
        return checkpoint.get<int32_t>();
    };
    next.checkpoint = readCheckpoint("checkpoint");
    next.faroresWindCheckpoint = readCheckpoint("faroresWindCheckpoint");
    next.completedBossMask = static_cast<uint16_t>(
        Unsigned(runtime.value("completedBossMask", nlohmann::json(0)), 0xffff, "boss completion mask"));
    ValidateRuntime(seed, catalogue, routes, next.checkpoint, next.faroresWindCheckpoint, next.completedBossMask,
                    options.bosses);
    return next;
}

RandoSoloEntranceSave FromPermutation(const std::vector<Dungeon>& catalogue,
                                      const ComboRando::SoloEntrance::Permutations& permutations, uint16_t options,
                                      uint32_t policy, uint32_t seed) {
    ComboRando::SoloEntrance::BuildRoutes(catalogue, permutations.forward, permutations.reverse, Policy(policy));
    if (catalogue.size() > std::size(RandoSoloEntranceSave{}.permutation))
        throw std::invalid_argument("Solo MM entrance catalogue exceeds native save capacity");
    RandoSoloEntranceSave saved{};
    saved.version = kVersion;
    saved.seed = seed;
    saved.options = options;
    saved.policy = policy;
    saved.count = static_cast<uint16_t>(catalogue.size());
    saved.checkpoint = -1;
    saved.faroresWindCheckpoint = -1;
    for (size_t i = 0; i < catalogue.size(); ++i) {
        saved.permutation[i] = static_cast<uint16_t>(permutations.forward[i]);
        saved.reversePermutation[i] = static_cast<uint16_t>(permutations.reverse[i]);
    }
    return saved;
}

struct CachedLogic {
    RandoSoloEntranceSave saved{};
    HostState state{ 1 };
    BossSourceMap bosses = EmptyBossSources();
};

const CachedLogic* CurrentLogic() {
    if (!SoloMode() || !IS_RANDO)
        return nullptr;
    const auto& saved = gSaveContext.save.shipSaveInfo.rando.soloEntrances;
    if (!saved.version && !saved.count)
        return nullptr;
    static thread_local CachedLogic cache;
    if (cache.saved.version != saved.version || cache.saved.seed != saved.seed ||
        cache.saved.options != saved.options || cache.saved.policy != saved.policy ||
        cache.saved.count != saved.count ||
        std::memcmp(cache.saved.permutation, saved.permutation, sizeof(saved.permutation)) != 0 ||
        std::memcmp(cache.saved.reversePermutation, saved.reversePermutation, sizeof(saved.reversePermutation)) != 0) {
        const auto seed = EncodePod(saved);
        HostState next(1);
        next.dungeons = Catalogue(saved.options);
        const auto forward = seed.at("permutation").get<std::vector<size_t>>();
        next.routes = ComboRando::SoloEntrance::BuildRoutes(
            next.dungeons, forward, seed.at("reversePermutation").get<std::vector<size_t>>(), Policy(saved.policy));
        auto bosses = EmptyBossSources();
        for (int boss = 8; boss < 12; ++boss)
            bosses[boss] = ComboRando::SoloEntrance::BossSource(next.dungeons, forward, Policy(saved.policy), boss);
        cache.saved = saved;
        cache.state = std::move(next);
        cache.bosses = bosses;
    }
    return &cache;
}
} // namespace

nlohmann::json Seed(const RandoSaveInfo& save) {
    return EncodePod(save.soloEntrances);
}

std::vector<Dungeon> CatalogueForSave(const RandoSaveInfo& save) {
    if (Seed(save).is_null())
        return {};
    return Catalogue(save.soloEntrances.options);
}

std::vector<Route> BuildRoutes(const RandoSaveInfo& save) {
    const auto seed = Seed(save);
    if (seed.is_null())
        return {};
    return ComboRando::SoloEntrance::BuildRoutes(
        Catalogue(save.soloEntrances.options), seed.at("permutation").get<std::vector<size_t>>(),
        seed.at("reversePermutation").get<std::vector<size_t>>(), Policy(save.soloEntrances.policy));
}

int BossSourceForSave(const RandoSaveInfo& save, int physicalBoss) {
    const auto seed = Seed(save);
    if (seed.is_null())
        return -1;
    return ComboRando::SoloEntrance::BossSource(Catalogue(save.soloEntrances.options),
                                                seed.at("permutation").get<std::vector<size_t>>(),
                                                Policy(save.soloEntrances.policy), physicalBoss);
}

bool BossesUsePhysicalClears(const RandoSaveInfo& save) {
    if (Seed(save).is_null())
        return false;
    return ComboRando::SoloEntrance::PhysicalBossClears(Catalogue(save.soloEntrances.options),
                                                        Policy(save.soloEntrances.policy));
}

void ApplySeed(RandoSaveInfo& save, const nlohmann::json& seed) {
    // Commit only after every source, destination, alias and permutation is valid.
    const auto next = DecodePod(seed);
    save.soloEntrances = next;
}

void Clear(RandoSaveInfo& save) {
    save.soloEntrances = {};
    save.soloEntrances.checkpoint = -1;
    save.soloEntrances.faroresWindCheckpoint = -1;
}

void ValidateSettings(const RandoSaveInfo& save) {
    if (!SoloMode())
        return;
    const auto mask = SavedOptions(save);
    const auto& recorded = save.soloEntrances;
    if (!recorded.version && !recorded.count) {
        if (mask)
            throw std::invalid_argument("Solo MM entrance settings require a recorded layout");
        return;
    }
    Seed(save);
    if (save.randoSaveOptions[RO_LOGIC] == RO_LOGIC_VANILLA || mask != recorded.options ||
        SavedPolicy(save) != recorded.policy || save.finalSeed != recorded.seed)
        throw std::invalid_argument("Solo MM entrance layout does not match saved settings or seed identity");
}

void Generate(RandoSaveInfo& save, uint32_t masterSeed, uint32_t attempt) {
    Clear(save);
    if (!SoloMode() || save.randoSaveOptions[RO_LOGIC] == RO_LOGIC_VANILLA)
        return;
    const auto mask = SavedOptions(save);
    if (!mask)
        return;
    const auto policy = SavedPolicy(save);
    const auto catalogue = Catalogue(mask);
    const uint64_t stream = kStream ^ masterSeed ^ (0x9e3779b97f4a7c15ULL * attempt);
    const auto permutations = ComboRando::SoloEntrance::Generate(catalogue, stream, Policy(policy));
    save.soloEntrances = FromPermutation(catalogue, permutations, mask, policy, masterSeed);
}

void SelectForLogic(RandoSaveInfo& save, const std::function<bool(const RandoSoloEntranceSave&)>& isValid) {
    if (!SoloMode() || !save.soloEntrances.version)
        return;
    const auto initial = save.soloEntrances;
    const auto catalogue = Catalogue(initial.options);
    const auto policy = Policy(initial.policy);
    uint64_t stream = kStream ^ initial.seed;
    for (size_t i = 0; i < initial.count; ++i) {
        stream = (stream ^ initial.permutation[i]) * 1099511628211ULL;
        stream = (stream ^ initial.reversePermutation[i]) * 1099511628211ULL;
    }
    auto make = [&](const ComboRando::SoloEntrance::Permutations& permutations) {
        return FromPermutation(catalogue, permutations, initial.options, initial.policy, initial.seed);
    };
    auto candidate = initial;
    for (size_t attempt = 0; attempt < kLayoutAttempts; ++attempt) {
        if (isValid(candidate)) {
            save.soloEntrances = candidate;
            return;
        }
        candidate = make(ComboRando::SoloEntrance::Generate(catalogue, stream + attempt, policy));
    }
    // Keep every selected entrance in both permutations. Compose native-validated
    // cycles when a whole-map roll cannot preserve all checks. Every callback uses
    // the original native search deadline; this does not extend the fill budget.
    const auto desired = ComboRando::SoloEntrance::Generate(catalogue, stream, policy);
    ComboRando::SoloEntrance::Permutations current;
    current.forward.resize(catalogue.size());
    std::iota(current.forward.begin(), current.forward.end(), 0);
    current.reverse = current.forward;
    if (!isValid(make(current)))
        throw std::runtime_error("Solo MM native entrance baseline cannot reach every shuffled check");
    bool changed = false;
    for (int direction = 0; direction < (policy.decoupled ? 2 : 1); ++direction) {
        const auto& goal = direction ? desired.reverse : desired.forward;
        std::vector<bool> visited(goal.size(), false);
        for (size_t first = 0; first < goal.size(); ++first) {
            if (visited[first])
                continue;
            std::vector<size_t> cycle;
            for (size_t at = first; !visited[at]; at = goal[at]) {
                visited[at] = true;
                cycle.push_back(at);
            }
            if (cycle.size() < 2)
                continue;
            auto trial = current;
            auto& permutation = direction ? trial.reverse : trial.forward;
            for (size_t member : cycle)
                permutation[member] = goal[member];
            if (!policy.decoupled)
                trial.reverse = ComboRando::SoloEntrance::Inverse(trial.forward);
            if (isValid(make(trial))) {
                current = std::move(trial);
                changed = true;
                continue;
            }
            for (size_t pair = 1; pair < cycle.size(); ++pair) {
                trial = current;
                auto& swap = direction ? trial.reverse : trial.forward;
                std::swap(swap[cycle[pair - 1]], swap[cycle[pair]]);
                if (!policy.decoupled)
                    trial.reverse = ComboRando::SoloEntrance::Inverse(trial.forward);
                if (isValid(make(trial))) {
                    current = std::move(trial);
                    changed = true;
                }
            }
        }
    }
    candidate = make(current);
    if (!changed || !isValid(candidate))
        throw std::runtime_error("Solo MM entrance shuffle found no valid layout");
    save.soloEntrances = candidate;
}

int ResolveEntrance(int index, int region) {
    // Native savewarp/Soaring roots are not overworld walking entrances.
    if (region == RR_MAX)
        return index;
    const auto* logic = CurrentLogic();
    if (!logic)
        return index;
    const int canonical = ComboSoloMmOverworld::Canonicalize(index);
    const int grotto = ComboCrossMmCatalogue::GrottoExitForRegion(static_cast<RandoRegionId>(region));
    // An unselected grotto must not fall through into an overworld route that
    // happens to share its physical scene-return index; runtime uses this same key.
    const int portal = grotto >= 0 ? grotto : canonical;
    const auto* route = logic->state.Lookup(portal, portal);
    return route ? route->to.index : index;
}

bool BossLogicEnabled() {
    const auto* logic = CurrentLogic();
    return logic && logic->state.HasPool(Pool::Boss);
}

int LogicalClear(int sourceBoss, int nativeValue) {
    const auto* logic = CurrentLogic();
    if (!logic || !logic->state.HasPool(Pool::Boss))
        return nativeValue;
    constexpr std::array<RandoEvent, 4> events = { RE_CLEARED_WOODFALL_TEMPLE, RE_CLEARED_SNOWHEAD_TEMPLE,
                                                   RE_CLEARED_GREAT_BAY_TEMPLE, RE_CLEARED_STONE_TOWER_TEMPLE };
    for (int physical = 8; physical < 12; ++physical)
        if (logic->bosses[physical] == sourceBoss)
            return RANDO_EVENTS[events[physical - 8]] != 0;
    return 0;
}

void RestoreForOracle(RandoSaveInfo& save) {
    if (!SoloMode())
        return;
    if (oracleSeed) {
        save.soloEntrances = *oracleSeed;
        if (oracleSeed->version)
            save.finalSeed = oracleSeed->seed;
        for (size_t i = 0; i < kSettings.size(); ++i)
            save.randoSaveOptions[kSettings[i]] = (oracleSeed->options >> i) & 1;
        for (size_t i = 0; i < kPolicySettings.size(); ++i)
            save.randoSaveOptions[kPolicySettings[i]] = (oracleSeed->policy >> i) & 1;
    } else {
        Clear(save);
    }
}
} // namespace Rando::SoloEntrances

#ifdef COMBO_BUILD
extern "C" COMBO_EXPORT int MM_SetSoloEntranceSeed(const char* raw) {
    if (gComboGameMode != COMBO_GAME_MM)
        return 0;
    try {
        auto next = Rando::SoloEntrances::DecodePod(nlohmann::json::parse(raw ? raw : "null"));
        Rando::SoloEntrances::oracleSeed = next;
        return 1;
    } catch (...) {
        return 0; // The previous explicit oracle input survives a rejected replay.
    }
}
#endif
