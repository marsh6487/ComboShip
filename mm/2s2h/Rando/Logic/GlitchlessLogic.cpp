#include "Logic.h"
#include "2s2h/Rando/Types.h"
#include "Rando/SoloEntrances.h"
#include "regs.h"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <spdlog/spdlog.h>

extern "C" {
#include "variables.h"
#include "ShipUtils.h"
uint64_t GetUnixTimestamp();
}

namespace Rando::Logic {
namespace {

// Assumed inventory is a search input, never an acquisition in either game.
// Restore the entire native state on success and on every exceptional exit.
struct FillSimulation {
    SaveContext original = gSaveContext;
    uint64_t regionTime = gCurrentRegionTime;
    bool originalSimulation = gItemGrantSimulation;
    RegEditor* registers = gRegEditor;
    RegEditor originalRegisters{};
    FillSimulation() {
        if (registers != nullptr) {
            originalRegisters = *registers;
        }
        gItemGrantSimulation = true;
    }
    void RestoreRegisters() const {
        if (registers != nullptr) {
            *registers = originalRegisters;
        }
    }
    ~FillSimulation() {
        gSaveContext = original;
        gCurrentRegionTime = regionTime;
        RestoreRegisters();
        gItemGrantSimulation = originalSimulation;
    }
};

} // namespace

void ApplyGlitchlessLogicToSaveContext(std::vector<RandoCheckId>& checkPool, std::vector<RandoItemId>& itemPool) {
    const uint64_t tick = GetUnixTimestamp();
    FillSimulation simulation;
    const std::set<RandoCheckId> shuffledChecks(checkPool.begin(), checkPool.end());
    std::map<RandoCheckId, RandoItemId> placements;
    size_t probes = 0, backtracks = 0, maximumDepth = 0;
    auto checkBudget = [&] {
        if (GetUnixTimestamp() - tick > 10000) {
            SPDLOG_ERROR("Glitchless search budget: placed={} maximum={} probes={} backtracks={}", placements.size(),
                         maximumDepth, probes, backtracks);
            throw std::runtime_error("Generation took too long, aborting");
        }
    };
    if (checkPool.size() != itemPool.size() || shuffledChecks.size() != checkPool.size()) {
        throw std::runtime_error("Glitchless item/check pools are not balanced and unique");
    }

    // Keep occurrences, including duplicate keys/progressives and health. A set
    // of item IDs loses quantities; RemoveItem cannot undo progressive grants.
    std::vector<RandoItemId> order = itemPool;
    for (size_t i = 0; i < order.size(); i++) {
        std::swap(order[i], order[Ship_Random(0, order.size())]);
    }
    std::stable_sort(order.begin(), order.end(), [](RandoItemId a, RandoItemId b) {
        return (RandoItemIdToConfinementGroup(a) >= 0) > (RandoItemIdToConfinementGroup(b) >= 0);
    });

    auto grant = [](RandoItemId item) {
        // Native UNKNOWN converts to JUNK; these three IDs have no inventory
        // behavior. Still collect their checks and keep every pool occurrence.
        if (item == RI_UNKNOWN || item == RI_NONE || item == RI_JUNK) {
            return;
        }
        const auto resolved = ConvertItem(item);
        if (resolved != RI_UNKNOWN && resolved != RI_NONE && resolved != RI_JUNK) {
            GiveItem(resolved);
        }
    };

    // Cache exact native suffix inventories once. Each entry includes every
    // occurrence in that suffix (including health/ammo), never RemoveItem undo.
    // Probes copy these states; they do not re-grant the whole assumed pool.
    std::vector<SaveContext> assumed(order.size() + 1);
    assumed.back() = simulation.original;
    for (size_t i = order.size(); i-- > 0;) {
        checkBudget();
        simulation.RestoreRegisters();
        gSaveContext = assumed[i + 1];
        gCurrentRegionTime = simulation.regionTime;
        grant(order[i]);
        assumed[i] = gSaveContext;
    }

    // Every probe rebuilds inventory, events, region times and collected checks.
    // An unfilled shuffled check still contains its vanilla placeholder in the
    // native save: it may be reachable, but must never grant that placeholder.
    auto reachable = [&](size_t assumeFrom) {
        checkBudget();
        probes++;
        simulation.RestoreRegisters();
        gSaveContext = assumed[assumeFrom];
        gCurrentRegionTime = simulation.regionTime;
        for (auto [check, item] : placements) {
            RANDO_SAVE_CHECKS[check].randoItemId = item;
            RANDO_SAVE_CHECKS[check].shuffled = true;
        }
        std::set<RandoRegionId> regions = { RR_MAX };
        std::set<RandoCheckId> checks;
        std::set<std::pair<RandoEvent, std::function<bool()>>*> events;
        auto times = InitializeRegionTimeStates(RR_MAX);
        uint64_t ownedTime = TimeLogic::GetOwnedTimeSlices();
        bool changed;
        do {
            checkBudget();
            changed = false;
            const auto previousTimes = times;
            const size_t previousRegions = regions.size();
            for (RandoRegionId region : regions) {
                FindReachableRegions(region, regions, times);
            }
            changed |= regions.size() != previousRegions;
            for (RandoRegionId region : regions) {
                SetCurrentRegionTime(times, region);
                for (auto& event : Regions[region].events) {
                    if (!events.contains(&event) && event.second()) {
                        RANDO_EVENTS[event.first]++;
                        events.insert(&event);
                        changed = true;
                    }
                }
                for (auto& [check, condition] : Regions[region].checks) {
                    if (!checks.contains(check) && condition.first()) {
                        TimeLogic::ValidateRegionTimeOwnership(region, check, times[region].timeSlices, "Glitchless");
                        checks.insert(check);
                        if (!shuffledChecks.contains(check) || placements.contains(check)) {
                            grant(RANDO_SAVE_CHECKS[check].randoItemId);
                        }
                        changed = true;
                    }
                }
            }
            // Match native time-item propagation, while also observing time-mask
            // growth when no new region/event/check was discovered this pass.
            const uint64_t newOwnedTime = TimeLogic::GetOwnedTimeSlices();
            if (newOwnedTime != ownedTime) {
                ownedTime = newOwnedTime;
                for (auto& [region, state] : times) {
                    state.timeSlices = ownedTime;
                    if (state.canStayOverTime) {
                        state.timeSlices = TimeLogic::ExpandTimeForward(ownedTime, Regions[region]);
                    }
                }
                changed = true;
            }
            for (auto& [region, state] : times) {
                auto old = previousTimes.find(region);
                changed |= old == previousTimes.end() || old->second.timeSlices != state.timeSlices;
            }
        } while (changed);
        return checks;
    };

    // Entrance candidates use this same native item/event/time fixed point and
    // deadline. Every inventory snapshot receives the selected map, so neither
    // assumed fill nor its final no-assumption replay can fall back to vanilla.
    SoloEntrances::SelectForLogic(simulation.original.save.shipSaveInfo.rando,
                                  [&](const RandoSoloEntranceSave& layout) {
                                      for (auto& context : assumed)
                                          context.save.shipSaveInfo.rando.soloEntrances = layout;
                                      const auto reached = reachable(0);
                                      return std::all_of(checkPool.begin(), checkPool.end(),
                                                         [&](RandoCheckId check) { return reached.contains(check); });
                                  });
    const auto fullInventory = reachable(0);
    for (RandoCheckId check : checkPool) {
        if (!fullInventory.contains(check)) {
            throw std::runtime_error("Native full item pool cannot reach check: " +
                                     std::string(Rando::StaticData::Checks[check].name));
        }
    }

    struct Choice {
        std::vector<RandoCheckId> checks;
        size_t next = 0;
        RandoCheckId placed = RC_UNKNOWN;
    };
    std::vector<Choice> choices;
    size_t depth = 0;
    while (depth < order.size()) {
        checkBudget();
        if (choices.size() == depth) {
            const auto accessible = reachable(depth + 1);
            std::map<int, size_t> needed, available;
            for (size_t i = depth + 1; i < order.size(); i++) {
                const int group = RandoItemIdToConfinementGroup(order[i]);
                if (group >= 0) {
                    needed[group]++;
                }
            }
            for (RandoCheckId check : checkPool) {
                if (!placements.contains(check)) {
                    available[CheckIdToConfinementGroup(check)]++;
                }
            }
            Choice choice;
            for (RandoCheckId check : checkPool) {
                if (placements.contains(check) || !accessible.contains(check) ||
                    !IsItemAllowedAtCheck(order[depth], check)) {
                    continue;
                }
                const int usedGroup = CheckIdToConfinementGroup(check);
                bool reserved = true;
                for (auto [group, count] : needed) {
                    if (count > available[group] - (group == usedGroup ? 1 : 0)) {
                        reserved = false;
                        break;
                    }
                }
                if (reserved) {
                    choice.checks.push_back(check);
                }
            }
            for (size_t i = 0; i < choice.checks.size(); i++) {
                std::swap(choice.checks[i], choice.checks[Ship_Random(0, choice.checks.size())]);
            }
            choices.push_back(std::move(choice));
        }
        auto& choice = choices[depth];
        if (choice.next == choice.checks.size()) {
            choices.pop_back();
            if (depth == 0) {
                throw std::runtime_error("No reachable native placement satisfies Glitchless confinement");
            }
            depth--;
            backtracks++;
            placements.erase(choices[depth].placed);
            choices[depth].placed = RC_UNKNOWN;
            continue;
        }
        choice.placed = choice.checks[choice.next++];
        placements.emplace(choice.placed, order[depth]);
        depth++;
        maximumDepth = std::max(maximumDepth, depth);
    }

    // No assumed items survive this replay. Require every pool check, not only
    // the goal, before atomically publishing placements into the original save.
    const auto replay = reachable(order.size());
    for (RandoCheckId check : checkPool) {
        if (!replay.contains(check)) {
            throw std::runtime_error("Native Glitchless replay cannot reach placed check: " +
                                     std::string(Rando::StaticData::Checks[check].name));
        }
    }
    for (auto [check, item] : placements) {
        auto& saved = simulation.original.save.shipSaveInfo.rando.randoSaveChecks[check];
        saved.randoItemId = item;
        saved.shuffled = true;
    }
    checkPool.clear();
    itemPool.clear();
    SPDLOG_INFO("Successfully placed all items with native Glitchless assumed fill and replay");
}

} // namespace Rando::Logic
