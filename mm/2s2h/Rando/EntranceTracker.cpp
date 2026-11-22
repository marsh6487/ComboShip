#include "Rando/EntranceTracker.h"

#ifdef COMBO_BUILD
#include "2s2h/ShipUtils.h"
#include "Rando/EntranceTrackerDiscovery.h"
#include "Rando/EntranceTrackerNames.h"
#include "Rando/Logic/Logic.h"
#include "Rando/Rando.h"
#include "combo/ComboExport.h"
#include "combo/ComboResolve.h"
#include "combo/rando/CrossDungeonTracker.h"
#include "combo/rando/SoloEntranceMmRuntime.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <spdlog/fmt/fmt.h>
#include <string_view>
#include "combo/menu/ComboMenuSharedContext.h"

namespace Rando::EntranceTracker {
namespace {
using namespace ComboRando::CrossDungeon;

struct Row {
    std::vector<int32_t> indices;
    Pool pool;
    bool exit;
    std::string source;
    std::string destination;

    bool IsDiscovered(std::span<const uint16_t> discovered) const {
        return std::any_of(indices.begin(), indices.end(), [&](int32_t index) {
            return std::find(discovered.begin(), discovered.end(), index) != discovered.end();
        });
    }
};

std::string ArrivalName(Endpoint endpoint, const std::vector<Dungeon>& catalogue) {
    if (endpoint.game == 0) {
        const auto getName = reinterpret_cast<const char* (*)(int32_t)>(
            Combo_ResolveSym("soh", "SOH_GetEntranceTrackerDestinationLabel"));
        if (const char* name = getName ? getName(endpoint.index) : nullptr)
            return std::string("OoT · ") + name;
        return fmt::format("OoT entrance {:#06x}", endpoint.index);
    }
    if (const auto* portal = FindTrackerPortal(catalogue, endpoint)) {
        if (portal->pool == Pool::Grotto || IsInteriorPool(portal->pool)) {
            const std::string name = PortalSourceName(*portal);
            if (!name.empty())
                return endpoint == portal->entrance ? name : "Outside " + name;
        }
    }
    const auto region = Logic::Regions.find(Logic::GetRegionIdFromEntrance(endpoint.index));
    if (region != Logic::Regions.end()) {
        if (region->second.name[0] != '\0')
            return region->second.name;
        if (const std::string_view scene = Ship_GetSceneName(region->second.sceneId); scene != "Unknown")
            return std::string(scene);
    }
    return fmt::format("Entrance {:#06x}", endpoint.index);
}

std::vector<Row> BuildRows(const RuntimeState& runtime, bool showExits) {
    // A native index can be shared by an ingress and an exit. Resolve the source
    // the same way as MM's transition dispatcher, which tries BeforeNative first.
    std::map<int32_t, const Route*> routes;
    for (const auto& route : runtime.Routes()) {
        if (!showExits && route.phase == Phase::BeforeNative)
            continue;
        if (route.from.game == 1 && (route.phase == Phase::BeforeNative || !routes.contains(route.from.index)))
            routes[route.from.index] = &route;
    }
    const auto& catalogue = runtime.Catalogue();
    std::vector<Row> result;
    std::map<std::pair<Endpoint, Endpoint>, size_t> exitRows;
    for (const auto& [index, route] : routes) {
        const Dungeon* portal = nullptr;
        const bool exit = route->phase == Phase::BeforeNative;
        if (!exit) {
            portal = runtime.FindEntrance(index);
        } else {
            const auto isExit = [&](const Dungeon& entry) {
                return entry.exit == route->from || std::find(entry.escapeExits.begin(), entry.escapeExits.end(),
                                                              route->from) != entry.escapeExits.end();
            };
            auto found = std::find_if(catalogue.begin(), catalogue.end(),
                                      [&](const Dungeon& entry) { return entry.pool == Pool::Boss && isExit(entry); });
            if (found == catalogue.end())
                found = std::find_if(catalogue.begin(), catalogue.end(), isExit);
            if (found != catalogue.end())
                portal = &*found;
        }
        if (!portal)
            continue;
        if (exit) {
            // First and repeat blue warps can use different indices for the same route.
            const auto [row, inserted] = exitRows.try_emplace({ portal->entrance, route->to }, result.size());
            if (!inserted) {
                result[row->second].indices.push_back(index);
                continue;
            }
        }
        std::string source = PortalSourceName(*portal);
        if (source.empty())
            source = ArrivalName(portal->entrance, catalogue);
        if (exit)
            source += portal->pool == Pool::Boss ? " (blue warp)" : " (exit)";
        result.push_back({ { index }, portal->pool, exit, std::move(source), ArrivalName(route->to, catalogue) });
    }
    std::sort(result.begin(), result.end(), [](const Row& a, const Row& b) {
        return std::tie(a.pool, a.exit, a.source, a.indices.front()) <
               std::tie(b.pool, b.exit, b.source, b.indices.front());
    });
    return result;
}

void DrawRoutes(int expectedSlot, bool foreground) {
    if ((foreground && !gPlayState) || gSaveContext.gameMode != GAMEMODE_NORMAL || expectedSlot < 0 ||
        expectedSlot > 2 || gSaveContext.fileNum != expectedSlot) {
        ImGui::TextUnformatted("Waiting for file load...");
        return;
    }

    uint64_t revision;
    const auto& runtime = GetRuntime(revision);
    if (!IS_RANDO || !runtime.Enabled()) {
        ImGui::TextUnformatted("This file has no shuffled MM entrances.");
        return;
    }

    static uint64_t previousRevision = UINT64_MAX;
    static std::vector<Row> rows;
    static ImGuiTextFilter search;
    if (previousRevision != revision) {
        rows = BuildRows(runtime, HasDecoupledExits());
        previousRevision = revision;
    }
    if (rows.empty()) {
        ImGui::TextUnformatted("This file has no shuffled MM entrances.");
        return;
    }

    constexpr char revealCvar[] = "gRando.EntranceTracker.RevealDestinations";
    const bool allowReveal = !CVarGetInteger("gSettings.DisableChanges", 0);
    bool reveal = allowReveal && CVarGetInteger(revealCvar, 0);
    search.Draw("Search entrances");
    ImGui::BeginDisabled(!allowReveal);
    if (ImGui::Checkbox("Reveal undiscovered destinations", &reveal)) {
        CVarSetInteger(revealCvar, reveal);
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    ImGui::EndDisabled();

    constexpr const char* pools[] = { "Dungeons", "Bosses", "Interiors", "Grottos", "Shops", "Doors", "Overworld" };
    const auto discovered = Discovered(gSaveContext.save.shipSaveInfo.rando);
    ImGui::BeginChild("MM Entrance Tracker Routes");
    auto group = rows.begin();
    bool hasMatches = false;
    while (group != rows.end()) {
        const auto pool = group->pool;
        const auto end = std::find_if(group, rows.end(), [pool](const Row& row) { return row.pool != pool; });
        std::vector<std::pair<const Row*, bool>> matches;
        for (auto row = group; row != end; ++row) {
            const bool visited = row->IsDiscovered(discovered);
            if (search.PassFilter(row->source.c_str()) ||
                ((visited || reveal) && search.PassFilter(row->destination.c_str()))) {
                matches.emplace_back(&*row, visited);
            }
        }
        if (!matches.empty()) {
            hasMatches = true;
            ImGui::SetNextItemOpen(true, search.IsActive() ? ImGuiCond_Always : ImGuiCond_Once);
            if (ImGui::TreeNode(pools[static_cast<size_t>(pool)])) {
                for (const auto& [row, visited] : matches)
                    ImGui::TextWrapped("%s -> %s", row->source.c_str(),
                                       visited || reveal ? row->destination.c_str() : "???");
                ImGui::TreePop();
            }
        }
        group = end;
    }
    if (!hasMatches)
        ImGui::TextUnformatted("No matching entrances.");
    ImGui::EndChild();
}
} // namespace

void Window::DrawElement() {
    DrawRoutes(gSaveContext.fileNum, true);
}
} // namespace Rando::EntranceTracker

extern "C" COMBO_EXPORT void MM_DrawEntranceTracker(int expectedSlot, int foreground) {
    ComboMenuContext::UseSharedImGuiContext();
    Rando::EntranceTracker::DrawRoutes(expectedSlot, foreground != 0);
}

extern "C" COMBO_EXPORT const char* MM_GetEntranceTrackerDestinationLabel(int32_t entrance) {
    using namespace Rando::EntranceTracker;
    uint64_t revision;
    const auto& runtime = GetRuntime(revision);
    static std::string label;
    label = ArrivalName({ 1, entrance }, runtime.Catalogue());
    return label.c_str();
}
#endif
