#pragma once

#include <cstring>
#include <cstdint>
#include <string>

#define SEASONAL_OUTFITS_CVAR "gEnhancements.SeasonalOutfits"
#define SEASONAL_OUTFITS_SET_CVAR "gEnhancements.SeasonalOutfits.Set"

namespace SeasonalOutfits {
// A reflected/cutscene/remote player can draw repeatedly in one graphics
// frame. Deferred combined DL pools may rotate only once for that owner/frame.
struct DrawFrameGate {
    const void* owner = nullptr;
    uint32_t frame = 0;
    bool valid = false;
    bool Enter(const void* nextOwner, uint32_t nextFrame) {
        if (valid && owner == nextOwner && frame == nextFrame)
            return false;
        owner = nextOwner;
        frame = nextFrame;
        valid = true;
        return true;
    }
};

inline bool PrivateArchive(const std::string& file) {
    constexpr char suffix[] = ".private.o2r";
    return file.size() >= sizeof(suffix) - 1 &&
           file.compare(file.size() - (sizeof(suffix) - 1), sizeof(suffix) - 1, suffix) == 0;
}

inline bool ValidSet(const char* set) {
    if (!set || !*set)
        return false;
    size_t length = 0;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(set); *p; ++p) {
        if (++length > 63 || !((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') ||
                               *p == '_' || *p == '-'))
            return false;
    }
    return true;
}

inline std::string Skeleton(const char* set, bool adult, int season) {
    static const char* const seasons[] = { "spring", "summer", "autumn", "winter" };
    if (!ValidSet(set) || season < 0 || season >= 4)
        return {};
    return std::string("__OTR__objects/seasonal_outfits/") + set + (adult ? "/adult/" : "/child/") + seasons[season] +
           (adult ? "/gLinkAdultSkel" : "/gLinkChildSkel");
}

inline int Season(const char* path, bool adult, std::string* set = nullptr) {
    constexpr char prefix[] = "__OTR__objects/seasonal_outfits/";
    if (!path || std::strncmp(path, prefix, sizeof(prefix) - 1))
        return -1;
    const char* first = path + sizeof(prefix) - 1;
    const char* slash = std::strchr(first, '/');
    if (!slash)
        return -1;
    const std::string family(first, slash);
    if (!ValidSet(family.c_str()))
        return -1;
    for (int season = 0; season < 4; ++season)
        if (Skeleton(family.c_str(), adult, season) == path) {
            if (set)
                *set = family;
            return season;
        }
    return -1;
}

inline bool Active(bool enabled, bool hasSeasons, int season, bool remote) {
    // The saved gameplay SEASON_OFF and presentation-only overrides keep the ordinary body.
    return enabled && hasSeasons && season >= 0 && season < 4 && !remote;
}

inline int Select(int forced, int seasonal, int ordinary, bool remote) {
    return forced >= 0 ? forced : !remote && seasonal >= 0 ? seasonal : ordinary;
}
} // namespace SeasonalOutfits
