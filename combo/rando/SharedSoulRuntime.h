#pragma once
#include "CrossForeign.h"
#include <set>

namespace ComboRando {
// Adapter boundaries are the native saved mask, native ownership flags, save-direct
// grants and the peer's exported grant. Both runtime and fill use the same groups.
template <class Adapter> struct SharedSoulRuntime {
    static std::vector<CwSharedName> Targets(GameId game, const std::string& name, uint32_t mask) {
        for (const auto& group : BuildSharedItemGroups({}, mask)) {
            if (!group.soul)
                continue;
            for (const auto& member : group.names)
                if (member.game == game && member.name == name)
                    return group.names;
        }
        return {};
    }
    static void Grant(const std::string& name) {
        if (!Targets(Adapter::Game(), name, Adapter::Mask()).empty() && !Adapter::Owned(name))
            Adapter::GrantNative(name.c_str());
    }
    static void Receive(const std::string& name, int sourceSlot, uint32_t sourceMask) {
        // A dormant module may still hold a different valid slot after a failed
        // load. Never persist a direct receipt into that resident save.
        if (sourceSlot != Adapter::Slot() || Targets(Adapter::Game(), name, Adapter::Mask() & sourceMask).empty())
            return;
        Grant(name);
    }
    static bool OnObtained(int item) {
        const uint32_t mask = Adapter::Mask();
        if (!mask)
            return false;
        GameId sourceGame = Adapter::Game();
        std::string name = Adapter::ItemName(item);
        if (name.empty()) {
            name = Adapter::LegacyPeerName(item);
            sourceGame = Adapter::Game() == GAME_OOT ? GAME_MM : GAME_OOT;
        }
        const auto targets = Targets(sourceGame, name, mask);
        for (const auto& target : targets) {
            if (target.game == Adapter::Game()) {
                // Native MM callbacks precede the source flag. Never grant that
                // source recursively; only its merged siblings need a grant.
                if (sourceGame != target.game || target.name != name)
                    Grant(target.name);
            } else {
                Adapter::GrantPeer(target.name.c_str(), Adapter::Slot(), mask);
            }
        }
        // The native give choke has already recorded its local FC counters.
        // A handled category must not fall through to an unqualified peer grant:
        // that second path would bypass this receipt's slot/category handshake.
        return !targets.empty();
    }
    static nlohmann::json Extract() {
        std::set<std::string> owned;
        const uint32_t mask = Adapter::Mask();
        for (const auto& pair : SharedSoulPairs) {
            const char* name = Adapter::Game() == GAME_OOT ? pair.ootName : pair.mmName;
            if ((mask & pair.bit) && Adapter::Owned(name))
                owned.insert(name);
        }
        return { { "slot", Adapter::Slot() }, { "mask", mask }, { "owned", owned } };
    }
    static void Apply(const nlohmann::json& state) {
        if (!state.is_object() || !state.contains("slot") || !state["slot"].is_number_integer() ||
            state["slot"].get<int>() != Adapter::Slot() || !state.contains("mask") ||
            !state["mask"].is_number_unsigned() || !state.contains("owned") || !state["owned"].is_array())
            return;
        const uint32_t mask = Adapter::Mask() & state["mask"].get<uint32_t>();
        const GameId peer = Adapter::Game() == GAME_OOT ? GAME_MM : GAME_OOT;
        auto reconcile = [&](GameId game, const nlohmann::json& owned) {
            for (const auto& value : owned) {
                if (!value.is_string())
                    continue;
                for (const auto& target : Targets(game, value.get<std::string>(), mask))
                    if (target.game == Adapter::Game())
                        Grant(target.name);
            }
        };
        reconcile(peer, state["owned"]);
        reconcile(Adapter::Game(), Extract()["owned"]);
    }
};
} // namespace ComboRando
