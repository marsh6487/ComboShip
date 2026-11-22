#pragma once

#include <cstdio>
#include <cstdlib>
#include <memory>

// Item texture fixtures must stay outside the separate pause page atlas path.
inline std::shared_ptr<Ship::IResource> ResourceLoadPausePageTile(const char*) {
    std::fputs("FAIL: an item texture was routed to the pause page atlas\n", stderr);
    std::abort();
}
