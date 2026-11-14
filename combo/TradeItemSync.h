#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#if __has_include("FleetShipCombo/FleetComboItems.h")
#include "FleetShipCombo/FleetComboItems.h"
#else
#include "soh/FleetShipCombo/FleetComboItems.h"
#endif

// Earned-item registries remain monotone. Current copies use their own
// monotone revision: even means available, odd means consumed. Only gameplay
// consume/reacquire events advance it; old ownership is never a new pickup.
namespace ComboTrade {

template <typename Save> void WriteShared(const Save& save, nlohmann::json& sh) {
    sh["tradeAdultOwned"] = save.tradeAdultOwned;
    sh["tradeAdultRevision"] = save.tradeAdultRevision;
}

template <typename Save> void ReadShared(Save& save, const nlohmann::json& sh) {
    const auto owned = sh.find("tradeAdultOwned");
    if (owned != sh.end() && owned->is_number_integer() && *owned >= 0 && *owned <= UINT32_MAX) {
        save.tradeAdultOwned |= owned->get<uint32_t>() & ((1u << 23) - 1u);
    }
    const auto revisions = sh.find("tradeAdultRevision");
    if (revisions != sh.end() && revisions->is_array()) {
        for (size_t i = 0; i < 23 && i < revisions->size(); ++i) {
            const auto& value = (*revisions)[i];
            // Older sparse FleetNet arrays can have null gaps. Missing or
            // invalid entries leave the local generation intact.
            if (value.is_number_integer() && value >= 0 && value <= UINT32_MAX) {
                const uint32_t incoming = value.get<uint32_t>();
                if (incoming > save.tradeAdultRevision[i]) {
                    save.tradeAdultRevision[i] = incoming;
                }
            }
        }
    }
    // A consumed physical Pendant still proves its permanent perk was earned.
    if (save.tradeAdultOwned & (1u << 19)) {
        save.pendantOwned = 1;
    }
}

inline int IndexForFcItem(int fcId) {
    switch (fcId) {
        case FCI_OOT_TRADE_POCKET_EGG:
            return 0;
        case FCI_OOT_TRADE_COJIRO:
            return 2;
        case FCI_OOT_TRADE_ODD_MUSHROOM:
            return 3;
        case FCI_OOT_TRADE_ODD_POTION:
            return 4;
        case FCI_OOT_TRADE_POACHERS_SAW:
            return 5;
        case FCI_OOT_TRADE_BROKEN_SWORD:
            return 6;
        case FCI_OOT_TRADE_PRESCRIPTION:
            return 7;
        case FCI_OOT_TRADE_EYEBALL_FROG:
            return 8;
        case FCI_OOT_TRADE_EYEDROPS:
            return 9;
        case FCI_OOT_TRADE_CLAIM_CHECK:
            return 10;
        case FCI_MM_MOONS_TEAR:
            return 11;
        case FCI_MM_DEED_LAND:
            return 12;
        case FCI_MM_DEED_SWAMP:
            return 13;
        case FCI_MM_DEED_MOUNTAIN:
            return 14;
        case FCI_MM_DEED_OCEAN:
            return 15;
        case FCI_MM_ROOM_KEY:
            return 16;
        case FCI_MM_LETTER_TO_KAFEI:
            return 17;
        case FCI_MM_LETTER_TO_MAMA:
            return 18;
        case FCI_PENDANT_OF_MEMORIES:
            return 19;
        case FCI_OOT_TRADE_WEIRD_EGG:
            return 20;
        case FCI_OOT_TRADE_ZELDAS_LETTER:
            return 22;
        // The two Cuccos are native hatch results, with no FCI grant row.
        default:
            return -1;
    }
}

template <typename Save> bool AcknowledgeConsumedGrant(Save& save, int fcId) {
    const int index = IndexForFcItem(fcId);
    if (index < 0 || (save.tradeAdultRevision[index] & 1u) == 0) {
        return false;
    }
    // Retain obtained/check history, but consume an old materialization
    // deficit without replaying Item_Give and reviving the physical item.
    save.comboAppliedFc[fcId] = save.comboObtainedFc[fcId];
    return true;
}

} // namespace ComboTrade
