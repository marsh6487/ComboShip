#pragma once

#include "ComboFaroresWind.h"
#include <string>
#include <string_view>
#ifdef COMBO_BUILD
#include "../ComboResolve.h"
#endif

namespace FaroresWind {
using FindLocationName = const char* (*)(int32_t entrance);

struct Destination {
    int game;
    int32_t entrance;
};

inline Destination SelectDestination(int hostGame, int32_t nativeEntrance, int sharedStatus,
                                     const ComboFwPoint& shared) {
    if (sharedStatus == 1 && ComboFw_PointValid(&shared))
        return { shared.game, shared.entrance };

    // Only old saves or a standalone session may use the native point. Explicitly
    // cleared or invalid shared points must never show a stale local destination.
    return { hostGame, sharedStatus < 0 ? nativeEntrance : -1 };
}

template <typename Fits>
std::string FitHeading(std::string_view name, std::string_view prefix, std::string_view suffix, Fits fits) {
    if (name.empty() || name == "Unknown")
        return {};

    std::string safeName;
    for (unsigned char character : name.substr(0, 256)) {
        // The heading goes into the native message decoder, so accept printable
        // glyphs and replace bytes that could be taken for control codes.
        safeName += character >= ' ' && character <= '~' ? static_cast<char>(character) : '?';
    }

    std::string heading(prefix);
    heading += safeName;
    heading += suffix;
    if (fits(heading))
        return heading;

    heading.assign(safeName);
    heading += suffix;
    if (fits(heading))
        return heading;

    // Prefer the full location over the prefix. Preserve the game tag even
    // when a custom scene name must be shortened.
    std::string tail("...");
    tail += suffix;
    while (!safeName.empty() && !fits(safeName + tail))
        safeName.pop_back();
    return safeName.empty() ? std::string{} : safeName + tail;
}

template <typename Fits>
std::string Heading(int hostGame, int32_t nativeEntrance, std::string_view prefix, FindLocationName localLookup,
                    Fits fits) {
    ComboFwPoint shared{};
    const int status = ComboFw_ReadSavedPointForDisplay(&shared);
    const Destination destination = SelectDestination(hostGame, nativeEntrance, status, shared);
    if (destination.entrance < 0)
        return {};

    FindLocationName lookup = localLookup;
    std::string_view suffix;
#ifdef COMBO_BUILD
    if (destination.game != hostGame) {
        const bool mm = destination.game == 1;
        lookup = reinterpret_cast<FindLocationName>(
            Combo_ResolveSym(mm ? "2ship" : "soh", mm ? "MM_GetFwLocationName" : "SOH_GetFwLocationName"));
        suffix = mm ? " (MM)" : " (OoT)";
    }
#endif
    const char* name = lookup ? lookup(destination.entrance) : nullptr;
    return name ? FitHeading(name, prefix, suffix, fits) : std::string{};
}
} // namespace FaroresWind
