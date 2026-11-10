#pragma once
#include "Rando/Rando.h"
extern "C" {
#include "functions.h"
}
namespace Rando::RustyDoorKeys {
struct Entry {
    RandoItemId item;
    RandoInf flag;
    const char* name;
};
inline constexpr Entry Catalog[] = {
#define RUSTY_KEY(game, key, rg, ri, flag, name) RUSTY_KEY_##game(ri, flag, name)
#define RUSTY_KEY_MM(ri, flag, name) { ri, flag, name },
#define RUSTY_KEY_OOT(ri, flag, name)
#include "../../../combo/rando/RustyDoorKeyCatalog.inc"
#undef RUSTY_KEY_MM
#undef RUSTY_KEY_OOT
#undef RUSTY_KEY
};
inline const Entry* Find(RandoItemId item) {
    for (const auto& key : Catalog)
        if (key.item == item)
            return &key;
    return nullptr;
}
// Visual classification includes both native keys and OoT aliases. Keep native
// ownership/door logic on Find(), which deliberately contains only MM keys.
inline constexpr bool IsDoorKeyItem(RandoItemId item) {
    switch (item) {
#define RUSTY_KEY(game, key, rg, ri, flag, name) case ri:
#include "../../../combo/rando/RustyDoorKeyCatalog.inc"
#undef RUSTY_KEY
        return true;
        default:
            return false;
    }
}
inline bool CanUse(RandoItemId item) {
    const auto* key = Find(item);
    return !key || !RANDO_SAVE_OPTIONS[RO_SHUFFLE_RUSTY_KEYS] || Flags_GetRandoInf(key->flag);
}
inline bool Give(RandoItemId item) {
    const auto* key = Find(item);
    if (!key)
        return false;
    Flags_SetRandoInf(key->flag);
    return true;
}
inline bool Remove(RandoItemId item) {
    const auto* key = Find(item);
    if (!key)
        return false;
    Flags_ClearRandoInf(key->flag);
    return true;
}
inline int SourceItem(int scene, int id) {
#include "RustyDoorKeyActors.inc"
    return -1;
}
inline RandoItemId ExitItem(int source) {
    switch (source) {
#define RUSTY_ROUTE(key, outside, inside) \
    case outside:                         \
        return RI_RUSTY_KEY_##key;
#include "RustyDoorKeyRoutes.inc"
#undef RUSTY_ROUTE
        default:
            return RI_UNKNOWN;
    }
}
inline bool CanExit(int source) {
    return CanUse(ExitItem(source));
}
} // namespace Rando::RustyDoorKeys
