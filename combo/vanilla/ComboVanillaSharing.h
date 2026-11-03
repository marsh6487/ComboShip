#ifndef COMBO_VANILLA_SHARING_H
#define COMBO_VANILLA_SHARING_H

#include <array>
#include <cstdint>
#include <utility>
#include <nlohmann/json.hpp>

namespace ComboVanilla {
enum class World { Oot = 0, Mm = 1 };
struct Receiver {
    World world = World::Oot;
    int slot = -1;
    bool firstCycleComplete = false;
    bool kokiriSword = false;
    bool masterSword = false;
    bool biggoronSword = false;
    bool hammer = false;
};

/* This list is ownership only. Native inventory/story/capacity, consumables,
 * active modes, options, equipment selection and FC progressive counters do not
 * belong to this protocol. New ownership families require an explicit row. */
#define COMBO_VANILLA_NEI_FLAGS(X) \
    X(shovelOwned)                 \
    X(dominionOwned)               \
    X(ultrashotOwned)              \
    X(pokeballOwned)               \
    X(capeOwned)                   \
    X(bombArrowsOwned)             \
    X(timeGateOwned)               \
    X(magicMirrorOwned)            \
    X(hyliasGraceOwned)            \
    X(phantomHourglassOwned)       \
    X(netEquipped)                 \
    X(bottomlessBottleMode)        \
    X(quartzOwned)                 \
    X(cojiroPartnerOwned)

inline bool SharingUnsigned(const nlohmann::json& value, uint32_t maximum, uint32_t& out) {
    if (!value.is_number_integer() || value < 0 || value > maximum)
        return false;
    out = value.get<uint32_t>();
    return true;
}

inline bool SharingFlag(const nlohmann::json& value) {
    if (value.is_boolean())
        return value.get<bool>();
    uint32_t bit = 0;
    return SharingUnsigned(value, 1, bit) && bit != 0;
}

/* Canonical OoT page-two IDs, including their full-width EXT identities. Cell
 * identity is checked as well as item identity: a corrupt value cannot migrate
 * into another native or custom inventory cell. The Roc cell is progressive in
 * display only; a Cape snapshot always represents the exact Cape, never a copy. */
inline bool SharingCell(unsigned cell, uint32_t item) {
    static constexpr std::array<uint16_t, 24> primary = { 0x9E, 0xA9, 0xAA, 0xD0, 0xAF, 0xA3, 0xA4, 0xAD,
                                                          0xB2, 0xA5, 0xB0, 0xA2, 0xA6, 0xA7, 0xA8, 0x220,
                                                          0xB1, 0xA1, 0xB5, 0xB4, 0xB7, 0xAB, 0xB3, 0x223 };
    if (cell >= primary.size())
        return false;
    if (item == primary[cell])
        return true;
    return (cell == 0 && item == 0x9F) || (cell == 7 && item == 0x224) || (cell == 15 && item == 0xA0) ||
           (cell == 17 && item == 0x221) || (cell == 20 && item == 0x222) ||
           ((cell == 22 || cell == 23) && item == 0xAC);
}

inline bool SharingIdentity(const nlohmann::json& body, int slot) {
    if (slot < 0 || slot > 2 || !body.is_object())
        return false;
    const auto version = body.find("version");
    const auto sourceSlot = body.find("slot");
    return version != body.end() && version->is_number_integer() && *version == 1 && sourceSlot != body.end() &&
           sourceSlot->is_number_integer() && *sourceSlot == slot;
}

inline nlohmann::json FilterSharing(const nlohmann::json& snapshot, const Receiver& receiver) {
    using nlohmann::json;
    json out = json::object();
    if (receiver.world == World::Mm && !receiver.firstCycleComplete)
        return out;
    if (!snapshot.is_object())
        return out;
    const auto block = snapshot.find("vanillaNei");
    if (block == snapshot.end() || !SharingIdentity(*block, receiver.slot))
        return out;
    const auto& source = *block;
#define COPY_FLAG(field)                                        \
    if (source.contains(#field) && SharingFlag(source[#field])) \
        out[#field] = 1;
    COMBO_VANILLA_NEI_FLAGS(COPY_FLAG)
#undef COPY_FLAG
    if (source.contains("clawshotOwned") && SharingFlag(source["clawshotOwned"]))
        out["clawshotOwned"] = 1;
    if (source.contains("separateRocsFeather") && SharingFlag(source["separateRocsFeather"]))
        out["separateRocsFeather"] = 1;
    if (source.contains("ritoMaskOwned") && SharingFlag(source["ritoMaskOwned"]))
        out["ritoMaskOwned"] = 1;
    for (const auto& pair : { std::pair{ "caneSkills", 0x3Fu }, std::pair{ "wandRodsOwned", 0x3Fu },
                              std::pair{ "slateRunesOwned", 0x1Fu }, std::pair{ "seasonsOwned", 0xFu },
                              std::pair{ "seasonsRodOwned", 1u }, std::pair{ "twilightUpgrade", 7u } }) {
        uint32_t value;
        if (source.contains(pair.first) && SharingUnsigned(source[pair.first], 0xFF, value))
            out[pair.first] = value & pair.second;
    }
    const bool oot = receiver.world == World::Oot;
    uint32_t equipment = 0;
    if (source.contains("extEquipOwnedBits"))
        SharingUnsigned(source["extEquipOwnedBits"], UINT32_MAX, equipment);
    uint32_t shields = 0;
    if (source.contains("shieldOwned"))
        SharingUnsigned(source["shieldOwned"], 0xFFFF, shields);
    if (oot && source.contains("mmMirrorShield") && SharingFlag(source["mmMirrorShield"]))
        shields |= 0x20;
    shields &= oot ? 0x38u : 0x18u;
    out["shieldOwned"] = shields;
    // Shield aliases are ownership in both stores; the equipment grid reads the
    // ext bit, while the native/imported shield wheel reads shieldOwned.
    equipment &= oot ? 0x0FFF0000u : (0x0FFF0000u & ~(1u << 21));
    if (shields & 0x08)
        equipment |= 1u << 19;
    if (shields & 0x10)
        equipment |= 1u << 20;
    if (shields & 0x20)
        equipment |= 1u << 21;
    out["extEquipOwnedBits"] = equipment;
    uint32_t weapons = 0;
    if (source.contains("weaponUpgrades"))
        SharingUnsigned(source["weaponUpgrades"], 0xFF, weapons);
    if (oot) {
        uint32_t sword = 0;
        if (source.contains("mmSwordTier") && SharingUnsigned(source["mmSwordTier"], 3, sword))
            weapons |= sword == 3 ? 6u : sword == 2 ? 2u : 0u;
        if (source.contains("mmGreatFairySword") && SharingFlag(source["mmGreatFairySword"]))
            weapons |= 16u;
    }
    const uint32_t allowedWeapons = !oot ? 0u
                                         : (receiver.hammer ? 1u : 0u) | (receiver.kokiriSword ? 6u : 0u) |
                                               (receiver.masterSword ? 8u : 0u) | (receiver.biggoronSword ? 16u : 0u);
    out["weaponUpgrades"] = weapons & allowedWeapons;
    if (!oot)
        out["ootWeaponUpgradesOwned"] = weapons & 0x09u; // latent entitlement, never a native base/effect

    if (oot && source.contains("pendantOwned") && SharingFlag(source["pendantOwned"]))
        out["pendantOwned"] = 1;
    const auto owned = source.find("ownedItems");
    if (owned != source.end() && owned->is_array()) {
        json cells = json::array();
        for (unsigned i = 0; i < 24; ++i) {
            uint32_t value = 0xFF;
            if (i >= owned->size() || !SharingUnsigned((*owned)[i], 0xFFFF, value) || !SharingCell(i, value))
                value = 0xFF;
            cells.push_back(value);
        }
        out["ownedItems"] = std::move(cells);
    }
    const auto masks = source.find("mmMasks");
    if (oot && masks != source.end() && masks->is_array()) {
        json cells = json::array();
        for (unsigned i = 0; i < 24; ++i) {
            uint32_t value = 0xFF;
            if (i >= masks->size() || !SharingUnsigned((*masks)[i], 0xFF, value) || value != 0xB8 + i)
                value = 0xFF;
            cells.push_back(value);
        }
        out["mmMasks"] = std::move(cells);
    }
    return out;
}

template <class Nei> nlohmann::json SharingSnapshot(const Nei& native, int slot) {
    nlohmann::json body = { { "version", 1 }, { "slot", slot } };
#define SAVE_FLAG(field) body[#field] = native.field != 0 ? 1 : 0;
    COMBO_VANILLA_NEI_FLAGS(SAVE_FLAG)
#undef SAVE_FLAG
    body["clawshotOwned"] = (native.twilightUpgrade & 1u) != 0;
    body["twilightUpgrade"] = native.twilightUpgrade & 7u;
    body["caneSkills"] = native.caneSkills;
    body["wandRodsOwned"] = native.wandRodsOwned;
    body["slateRunesOwned"] = native.slateRunesOwned;
    body["seasonsOwned"] = native.seasonsOwned;
    body["seasonsRodOwned"] = native.seasonsRodOwned;
    body["extEquipOwnedBits"] = native.extEquipOwnedBits;
    body["weaponUpgrades"] = native.weaponUpgrades;
    body["shieldOwned"] = native.shieldOwned;
    body["pendantOwned"] = native.pendantOwned != 0;
    return { { "vanillaNei", std::move(body) } };
}

enum class SharedGrant { Cane = 0, Wand = 1, Slate = 2, Season = 3, SeasonRod = 4 };

inline bool SharingCaneTypeOwned(uint8_t skills, uint8_t type) {
    // Four wheel entries, not two progressive chains. Block and Stone are
    // independently usable in Vanilla; only Somaria has a sub-selection.
    static constexpr std::array<uint8_t, 4> masks = { 0x03, 0x04, 0x18, 0x20 };
    return type < masks.size() && (skills & masks[type]) != 0;
}

/* The callback is the game's existing explicit grant API, never a progressive
 * rando pickup. Native selection is retained when still valid after the union. */
template <class Nei, class Grant>
void ApplySharingOwnership(Nei& native, const nlohmann::json& filtered, Grant&& grant) {
    const auto caneType = native.caneType;
    const bool keepCaneType = SharingCaneTypeOwned(native.caneSkills, caneType);
    const std::array<uint8_t, 2> caneSelection = { native.caneSkillSel[0], native.caneSkillSel[1] };
    const auto wandMode = native.wandMode;
    const auto slateMode = native.slateMode;
    const auto season = native.season;
#define APPLY_FLAG(field) native.field |= static_cast<uint8_t>(filtered.value(#field, 0));
    COMBO_VANILLA_NEI_FLAGS(APPLY_FLAG)
#undef APPLY_FLAG
    native.twilightUpgrade |= static_cast<uint8_t>(filtered.value("twilightUpgrade", 0));
    if (filtered.value("clawshotOwned", 0))
        native.twilightUpgrade |= 1u;
    auto bits = [&](const char* key, auto& current, unsigned count, SharedGrant kind) {
        const uint8_t incoming = static_cast<uint8_t>(filtered.value(key, 0));
        const uint8_t gained = incoming & ~current;
        for (unsigned index = 0; index < count; ++index)
            if (gained & (1u << index))
                grant(kind, index);
        current |= incoming;
    };
    if (filtered.value("seasonsRodOwned", 0) && !native.seasonsRodOwned) {
        grant(SharedGrant::SeasonRod, 0);
        native.seasonsRodOwned = 1;
    }
    bits("caneSkills", native.caneSkills, 6, SharedGrant::Cane);
    bits("wandRodsOwned", native.wandRodsOwned, 6, SharedGrant::Wand);
    bits("slateRunesOwned", native.slateRunesOwned, 5, SharedGrant::Slate);
    bits("seasonsOwned", native.seasonsOwned, 4, SharedGrant::Season);
    native.extEquipOwnedBits |= filtered.value("extEquipOwnedBits", 0u);
    native.shieldOwned |= static_cast<uint16_t>(filtered.value("shieldOwned", 0u));
    native.weaponUpgrades |= static_cast<uint8_t>(filtered.value("weaponUpgrades", 0u));
    native.pendantOwned |= static_cast<uint8_t>(filtered.value("pendantOwned", 0));
    if (keepCaneType) {
        native.caneType = caneType;
        native.caneSkillSel[0] = caneSelection[0];
        native.caneSkillSel[1] = caneSelection[1];
    }
    if (wandMode < 6 && (native.wandRodsOwned & (1u << wandMode)))
        native.wandMode = wandMode;
    if (slateMode < 5 && (native.slateRunesOwned & (1u << slateMode)))
        native.slateMode = slateMode;
    native.season = season == 4 || (season < 4 && (native.seasonsOwned & (1u << season))) ? season : 4;
}

/* Keep wheel choices local. The only ordered inventory upgrade is the exact Roc
 * Cape: the Feather cannot downgrade a Cape which was obtained first. */
template <class Nei, class Encode, class Decode>
void ApplySharingCells(Nei& native, const nlohmann::json& filtered, Encode&& encode, Decode&& decode) {
    const auto cells = filtered.find("ownedItems");
    if (cells != filtered.end()) {
        for (unsigned i = 0; i < 24; ++i) {
            const uint16_t incoming = (*cells)[i].template get<uint16_t>();
            if (incoming == 0xFF)
                continue;
            const uint16_t local = encode(native.ownedItems[i]);
            if (!SharingCell(i, local) || (i == 0 && incoming == 0x9F))
                native.ownedItems[i] = decode(incoming);
        }
    }
    const auto masks = filtered.find("mmMasks");
    if (masks != filtered.end())
        for (unsigned i = 0; i < 24; ++i)
            if ((*masks)[i] != 0xFF)
                native.ownedItems[24 + i] = (*masks)[i].template get<uint16_t>();
}

/* Unchanged identity leaves are absent from scalar and array deltas. Stamp
 * every packet at its source; the receiver must never infer a peer's slot from
 * its own live file or relabel a dormant peer's old snapshot. */
inline void StampSharingDelta(nlohmann::json& packet, int sourceSlot) {
    if (packet.is_object() && packet.value("op", nlohmann::json()) == "delta")
        packet["vanillaNei"] = { { "version", 1 }, { "slot", sourceSlot } };
}

inline bool PrepareSharingDelta(nlohmann::json& partial, const nlohmann::json& packet, int receiverSlot) {
    const auto identity = packet.find("vanillaNei");
    auto body = partial.find("vanillaNei");
    if (identity == packet.end() || !SharingIdentity(*identity, receiverSlot) || body == partial.end() ||
        !body->is_object())
        return false;
    for (const char* field : { "version", "slot" }) {
        const auto value = body->find(field);
        if (value != body->end() && (!value->is_number_integer() || *value != (*identity)[field]))
            return false;
        (*body)[field] = (*identity)[field];
    }
    return true;
}

#undef COMBO_VANILLA_NEI_FLAGS
} // namespace ComboVanilla
#endif
