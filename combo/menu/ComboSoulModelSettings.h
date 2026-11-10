#pragma once

#include <libultraship/bridge/consolevariablebridge.h>

// Rendering preferences shared by both game tabs. These are deliberately not
// seed options: changing a model never changes soul ownership or progression.
namespace ComboSoulModels {
inline constexpr char kSimplerEnemyCvar[] = "gRandoEnhancements.SimplerEnemySoulModels";
inline constexpr char kSimplerNpcCvar[] = "gRandoEnhancements.SimplerNpcSoulModels";

inline bool SimplerEnemy() {
    return CVarGetInteger(kSimplerEnemyCvar, 0) != 0;
}

inline bool SimplerNpc() {
    return CVarGetInteger(kSimplerNpcCvar, 0) != 0;
}

inline unsigned PreferenceMask() {
    return (SimplerEnemy() ? 1u : 0u) | (SimplerNpc() ? 2u : 0u);
}
} // namespace ComboSoulModels
