#pragma once
#include "randomizerTypes.h"
#include "z64.h"
namespace Rando::RustyDoorKeys {
struct Entry {
    RandomizerGet item;
    RandomizerInf flag;
    const char* name;
};
inline constexpr Entry Catalog[] = {
#define RUSTY_KEY(game, key, rg, ri, flag, name) RUSTY_KEY_##game(rg, flag, name)
#define RUSTY_KEY_OOT(rg, flag, name) { rg, flag, name },
#define RUSTY_KEY_MM(rg, flag, name)
#include "../../../../combo/rando/RustyDoorKeyCatalog.inc"
#undef RUSTY_KEY_MM
#undef RUSTY_KEY_OOT
#undef RUSTY_KEY
};
inline const Entry* Find(RandomizerGet item) {
    for (const auto& key : Catalog)
        if (key.item == item)
            return &key;
    return nullptr;
}
inline int SourceItem(int scene, int id, int nativeEntrance = -1) {
#include "RustyDoorKeyActors.inc"
    return -1;
}
inline constexpr const char* CatalogVersionCVar = "gRandoSettings.RustyDoorKeyCatalogVersion";
inline constexpr uint8_t LegacyCatalog = 0;
inline constexpr uint8_t CurrentCatalog = 1;
inline constexpr bool IsExtended(uint8_t version) {
    return version >= CurrentCatalog;
}
inline constexpr bool CastleGateUsesKey(bool enabled, uint8_t version) {
    return enabled && IsExtended(version);
}
inline constexpr uint8_t GenerationVersion(bool enabled) {
    return enabled ? CurrentCatalog : LegacyCatalog;
}
// Only actual new-generation request entrypoints may call this, before SetAllToContext/Prep.
void PrepareNewGeneration();
// Actual native new generation after settings are configured, before pool/solver setup.
template <class Context> inline void StampNewGeneration(Context& ctx) {
    ctx.GetOption(RSK_RUSTY_DOOR_KEY_VERSION)
        .Set(GenerationVersion(static_cast<bool>(ctx.GetOption(RSK_LOCK_OVERWORLD_DOORS))));
}
template <class Context> inline void ResetImportedCatalog(Context& ctx) {
    ctx.GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Set(LegacyCatalog);
}
inline RandomizerGet LegacySourceItem(int scene, int index) {
    struct Source {
        int scene;
        int index;
        RandomizerGet item;
    };
    static constexpr Source sources[] = {
#define LEGACY_RUSTY_SOURCE(scene, index, key) { scene, index, key },
#include "RustyDoorKeyLegacyActors.inc"
#undef LEGACY_RUSTY_SOURCE
    };
    for (const auto& source : sources)
        if (source.scene == scene && source.index == index)
            return source.item;
    return RG_NONE;
}
inline constexpr RandomizerGet KeyForVersion(RandomizerGet item, uint8_t version) {
    if (!IsExtended(version)) {
        switch (item) {
            case RG_RUSTY_KEY_HYRULE_CASTLE:
            case RG_RUSTY_KEY_RANCH_HOUSE_ROOM:
                return RG_NONE;
            case RG_RUSTY_KEY_ADULT_POTION_SHOP_BACK:
                return RG_KAK_POTION_SHOP_KEY;
            default:
                break;
        }
    }
    return item;
}
template <class Owned>
inline bool CanUse(RandomizerGet item, bool enabled, uint8_t version, bool skeleton, Owned owned) {
    if (!enabled || skeleton)
        return true;
    const auto* key = Find(KeyForVersion(item, version));
    return !key || owned(key->flag);
}
template <class Context, class Logic> inline bool CanUseInterior(Context& ctx, Logic& logic, RandomizerGet item) {
    return !IsExtended(ctx.GetOption(RSK_RUSTY_DOOR_KEY_VERSION).Get()) || logic.HasItem(item);
}

} // namespace Rando::RustyDoorKeys
