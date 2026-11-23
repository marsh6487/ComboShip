#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>

namespace DinSwordGi {
struct Resources {
    const char* core;
    const char* flame;
    const char* coreVertices;
    const char* flameVertices;
    const char* equipment;
};
inline constexpr Resources profiles[] = {
    { "objects/din_fire_sword/poc1/adult/CoreDL", "objects/din_fire_sword/poc1/adult/FlameDL",
      "objects/din_fire_sword/poc1/adult/CoreVertices", "objects/din_fire_sword/poc1/adult/FlameVertices",
      "objects/object_link_boy/DinSleekEquipmentPOC1_OOT_Adult/SwordDL" },
    { "objects/din_fire_sword/poc1/child/CoreDL", "objects/din_fire_sword/poc1/child/FlameDL",
      "objects/din_fire_sword/poc1/child/CoreVertices", "objects/din_fire_sword/poc1/child/FlameVertices",
      "objects/object_link_child/DinSleekEquipmentPOC1_OOT_Child/SwordDL" },
    { "objects/din_fire_sword/poc1/bgs/CoreDL", "objects/din_fire_sword/poc1/bgs/FlameDL",
      "objects/din_fire_sword/poc1/bgs/CoreVertices", "objects/din_fire_sword/poc1/bgs/FlameVertices", nullptr },
};
inline constexpr char coreTexture[] = "__OTR__objects/din_fire_sword/poc1/CoreTex";
inline constexpr char flameTexture[] = "__OTR__objects/din_fire_sword/poc1/FlameTex";

// The old Malon Fuller Frame ZSS equipment pack publishes a longsword at the
// same global Alt path as intentional GI models. Match the actual resolved
// root's vertex/texture references and command stream, independent of archive
// names and load order.
template <class Instruction> bool IsLegacyZssLongsword(const Instruction* commands, std::size_t count) {
    if (!commands || count != 535 || commands[10].words.w0 != 0x9BE4CB5B || commands[10].words.w1 != 0xACE832B1 ||
        commands[12].words.w0 != 0x889020A3 || commands[12].words.w1 != 0xE8874D84) {
        return false;
    }
    uint64_t fingerprint = 0xCBF29CE484222325;
    for (std::size_t i = 0; i < count; ++i) {
        fingerprint = (fingerprint ^ static_cast<uint32_t>(commands[i].words.w0)) * 0x100000001B3;
        fingerprint = (fingerprint ^ static_cast<uint32_t>(commands[i].words.w1)) * 0x100000001B3;
    }
    return fingerprint == 0x3E3F4BB1875AE1B5;
}

template <class Available> int SelectedProfile(const char* path, bool enabled, bool alt, Available available) {
    if (!path || !enabled || !alt)
        return 0;
    int profile = -1;
    bool progressive = false;
    if (!std::strcmp(path, "objects/din_fire_sword/progressive/adult/SwordDL")) {
        profile = 0;
        progressive = true;
    } else if (!std::strcmp(path, "objects/din_fire_sword/progressive/child/SwordDL")) {
        profile = 1;
        progressive = true;
    } else if (!std::strcmp(path, "objects/din_fire_sword/progressive/bgs/SwordDL")) {
        profile = 2;
        progressive = true;
    } else if (!std::strcmp(path, "alt/objects/object_custom_equip/gCustomMasterSwordDL"))
        profile = 0;
    else if (!std::strcmp(path, "alt/objects/object_custom_equip/gCustomKokiriSwordDL"))
        profile = 1;
    else if (!std::strcmp(path, "alt/objects/object_custom_equip/gCustomLongswordDL"))
        profile = 2;
    if (profile < 0)
        return 0;
    const auto& layers = profiles[profile];
    // Standalone custom paths can belong to another weapon mod. Require the
    // Din pack's equipment marker, matching the held sword's eligibility gate.
    if (!progressive && !available(layers.equipment ? layers.equipment : profiles[0].equipment))
        return 0;
    for (const auto* dependency :
         { layers.core, layers.flame, layers.coreVertices, layers.flameVertices, coreTexture, flameTexture })
        if (!available(dependency))
            return 0;
    return profile + 1;
}
} // namespace DinSwordGi
