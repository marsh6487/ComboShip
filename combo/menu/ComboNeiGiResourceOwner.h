#pragma once
#include <cstring>

// Unqualified native resources belong to the active host. Portable foreign
// recipes acquire an explicit route at the receiver's existing resolver.
inline const char* CwNeiGiResourceOwner(const char* path) {
    if (!path)
        return nullptr;
    if (std::strncmp(path, "__OTR__@oot-gi-base:", 20) == 0)
        return "oot-gi-base";
    if (std::strncmp(path, "__OTR__@mm-gi-base:", 19) == 0)
        return "mm-gi-base";
    if (std::strncmp(path, "__OTR__@oot:", 12) == 0)
        return "oot";
    if (std::strncmp(path, "__OTR__@mm:", 11) == 0)
        return "mm";
    return nullptr;
}
