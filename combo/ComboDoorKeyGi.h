#pragma once
#include "menu/ComboItemDrawABI.h"

extern "C" int ResourceMgr_IsGiModelAvailableForGame(const char* game, const char* path);

// Visual membership follows the append-only identity catalogue, never GID_KEY_SMALL
// or the CSMC category shared by unrelated dungeon keys and key rings.
namespace ComboDoorKeyGi {
enum class Origin { None, MM, OOT };
// Vanilla/dungeon small-key shafts already run from lower left to upper right.
// Our upright authored geometry needs this local roll after the caller's Y spin.
inline constexpr int RollBinang = -0x2000;
inline constexpr float RollRadians = RollBinang * (3.14159265358979323846f / 32768.0f);
#ifdef COMBO_DOOR_KEY_HOST_MM
static inline Origin ItemOrigin(RandoItemId item) {
    switch (item) {
#define RUSTY_KEY(game, key, rg, ri, flag, name) \
    case ri:                                     \
        return Origin::game;
#include "rando/RustyDoorKeyCatalog.inc"
#undef RUSTY_KEY
        default:
            return Origin::None;
    }
}
#else
static inline Origin ItemOrigin(RandomizerGet item) {
    switch (item) {
#define RUSTY_KEY(game, key, rg, ri, flag, name) \
    case rg:                                     \
        return Origin::game;
#include "rando/RustyDoorKeyCatalog.inc"
#undef RUSTY_KEY
        default:
            return Origin::None;
    }
}
#endif
static inline const char* SelectedPath(Origin origin) {
    if (origin == Origin::None)
        return nullptr;
    const bool mm = origin == Origin::MM;
    const char* path =
        mm ? "__OTR__objects/combo_door_keys/RustyKeyDL" : "__OTR__objects/combo_door_keys/OverworldKeyDL";
    // This query loads through the registered origin owner, including its current
    // Alt selection. The active host may be the other game during a foreign draw.
    if (!ResourceMgr_IsGiModelAvailableForGame(mm ? "mm" : "oot", path))
        return nullptr;
#ifdef COMBO_BUILD
    return mm ? "__OTR__@mm:objects/combo_door_keys/RustyKeyDL" : "__OTR__@oot:objects/combo_door_keys/OverworldKeyDL";
#else
    return path;
#endif
}
static inline bool Describe(Origin origin, CwItemDrawInfo* out, bool useOps = false) {
    if (origin == Origin::None)
        return false;
    out->stateDependent = 2; // Retry missing resources and live owner Alt changes.
    const char* path = SelectedPath(origin);
    if (!path)
        return false;
    out->dlistCount = 1;
    out->xluStartIndex = -1;
    out->dlists[0] = path;
    if (useOps) {
        // The OoT receiver replays OPS. Its scale is carried inside this recipe.
        out->drawKind = CW_DRAW_KIND_OPS;
        out->scale = 0.0f;
        out->opCount = 9;
        out->ops[0] = { CW_OP_PUSH };
        out->ops[1] = { CW_OP_SCALE, 0.25f, 0.25f, 0.25f };
        out->ops[2] = { CW_OP_ROTATE_Z, static_cast<float>(RollBinang) };
        out->ops[3] = { CW_OP_SETUP_OPA };
        out->ops[4] = { CW_OP_PRIM_COLOR, 0, 0, 0, { 255, 255, 255, 255 } };
        out->ops[5] = { CW_OP_ENV_COLOR, 0, 0, 0, { 255, 255, 255, 255 } };
        out->ops[6] = { CW_OP_LOAD_MATRIX };
        out->ops[7] = { CW_OP_DLIST, 0 };
        out->ops[8] = { CW_OP_POP };
    } else {
        // The MM receiver supports SIMPLE's local pose; model roots own material state.
        out->drawKind = CW_DRAW_KIND_SIMPLE;
        out->scale = 0.25f;
        out->opCount = 1;
        out->ops[0] = { CW_OP_ROTATE_Z, static_cast<float>(RollBinang) };
    }
    return true;
}
} // namespace ComboDoorKeyGi
