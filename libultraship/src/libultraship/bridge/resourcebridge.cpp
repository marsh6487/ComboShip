#include "libultraship/bridge/resourcebridge.h"
#include "ship/Context.h"
#include <string>
#include <algorithm>
#include <array>
#include <cstdio>
#include <deque>
#include <string_view>

#include "fast/resource/type/Texture.h"
#include "ship/resource/CrossRMRegistry.h"
#include "ship/resource/ResourceManagerScope.h"
#include "ship/resource/archive/Archive.h"
#include "ship/utils/StrHash64.h"
#include "ship/window/Window.h"

std::shared_ptr<Ship::IResource> ResourceLoad(const char* name) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(name);
}

std::shared_ptr<Ship::IResource> ResourceLoad(uint64_t crc) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->LoadResource(crc);
}

namespace {
constexpr size_t kPausePageTileCount = 15;
constexpr char kPauseTileRoute[] = "__OTR__@pause-page:";

struct PausePageAtlas {
    std::weak_ptr<Ship::ResourceManager> owner;
    std::weak_ptr<Ship::Archive> archive;
    void* const* source;
    uint64_t mountRevision;
    bool alt;
    std::array<std::string, kPausePageTileCount> paths;
    alignas(2) std::array<std::array<char, 48>, kPausePageTileCount> routes;
    std::array<void*, kPausePageTileCount> tiles{};
};

// Display lists may be replayed by frame interpolation after an archive switch.
// Keep their route strings at stable addresses while holding no extra archive owners.
std::deque<PausePageAtlas> sPausePageAtlases;

void** ResolvePausePageAtlas(const std::shared_ptr<Ship::ResourceManager>& owner, void* const* source) {
    if (!owner || !owner->GetArchiveManager()) {
        return nullptr;
    }

    auto manager = owner->GetArchiveManager();
    const uint64_t revision = manager->GetMountRevision();
    const bool altAssets = owner->IsAltAssetsEnabled();
    for (auto it = sPausePageAtlases.rbegin(); it != sPausePageAtlases.rend(); ++it) {
        if (it->source == source && it->owner.lock() == owner && it->mountRevision == revision &&
            it->alt == altAssets) {
            return it->tiles.front() ? it->tiles.data() : nullptr;
        }
    }

    auto& atlas = sPausePageAtlases.emplace_back();
    atlas.owner = owner;
    atlas.source = source;
    atlas.mountRevision = revision;
    atlas.alt = altAssets;
    for (size_t i = 0; i < kPausePageTileCount; ++i) {
        std::string_view path(static_cast<const char*>(source[i]));
        atlas.paths[i] = path.substr(path.starts_with("__OTR__") ? 7 : 0);
    }

    const auto archiveList = manager->GetArchives();
    // Alternate assets can replace the complete page. A partially localized
    // replacement would mix the title strip and the page body from different art.
    for (std::string_view prefix : { altAssets ? std::string_view("alt/") : std::string_view(), std::string_view() }) {
        for (auto it = archiveList->rbegin(); it != archiveList->rend(); ++it) {
            const auto& archive = *it;
            if (!std::all_of(atlas.paths.begin(), atlas.paths.end(), [&](const auto& path) {
                    const std::string candidate = std::string(prefix) + path;
                    return archive->HasFile(candidate) || archive->HasFile(candidate + ".meta");
                })) {
                continue;
            }
            Ship::ResourceManagerScope scope(owner);
            if (!std::all_of(atlas.paths.begin(), atlas.paths.end(), [&](const auto& path) {
                    auto resource = owner->LoadResourceProcess({ std::string(prefix) + path, 0, archive }, true);
                    return std::dynamic_pointer_cast<Fast::Texture>(resource) != nullptr;
                })) {
                continue;
            }
            atlas.archive = archive;
            const size_t atlasIndex = sPausePageAtlases.size() - 1;
            for (size_t i = 0; i < kPausePageTileCount; ++i) {
                atlas.paths[i].insert(0, prefix);
                auto& route = atlas.routes[i];
                std::snprintf(route.data(), route.size(), "%s%zu/%zu", kPauseTileRoute, atlasIndex, i);
                atlas.tiles[i] = route.data();
            }
            return atlas.tiles.data();
        }
        if (!altAssets) {
            break;
        }
    }
    return nullptr;
}
} // namespace

std::shared_ptr<Ship::IResource> ResourceLoadPausePageTile(const char* route) {
    size_t atlasIndex;
    size_t tileIndex;
    if (std::sscanf(route + sizeof(kPauseTileRoute) - 1, "%zu/%zu", &atlasIndex, &tileIndex) != 2 ||
        atlasIndex >= sPausePageAtlases.size() || tileIndex >= kPausePageTileCount) {
        return nullptr;
    }
    const auto& atlas = sPausePageAtlases[atlasIndex];
    const auto owner = atlas.owner.lock();
    const auto archive = atlas.archive.lock();
    if (!owner || !archive || !archive->IsLoaded()) {
        return nullptr;
    }
    Ship::ResourceManagerScope scope(owner);
    return owner->LoadResourceProcess({ atlas.paths[tileIndex], 0, archive }, true);
}

extern "C" {

void** ResourceGetPausePageTiles(void* const* tiles, const char* game) {
    const auto active = Ship::Context::GetRawInstance()->GetResourceManager();
    if (game) {
        const auto owner = Ship::CrossRMRegistry::Get(game);
        if (void** resolved = ResolvePausePageAtlas(owner, tiles)) {
            return resolved;
        }
    }
    return ResolvePausePageAtlas(active, tiles);
}

uint64_t ResourceGetCrcByName(const char* name) {
    return CRC64(name);
}

const char* ResourceGetNameByCrc(uint64_t crc) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->HashToCString(crc);
}

size_t ResourceGetSizeByName(const char* name) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->GetResourceSize(name);
}

size_t ResourceGetSizeByCrc(uint64_t crc) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->GetResourceSize(crc);
}

uint8_t ResourceGetIsCustomByName(const char* name) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->GetResourceIsCustom(name);
}

uint8_t ResourceGetIsCustomByCrc(uint64_t crc) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->GetResourceIsCustom(crc);
}

void* ResourceGetDataByName(const char* name) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->GetResourceRawPointer(name);
}

void* ResourceGetDataByCrc(uint64_t crc) {
    return Ship::Context::GetRawInstance()->GetResourceManager()->GetResourceRawPointer(crc);
}

uint16_t ResourceGetTexWidthByName(const char* name) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(name));

    if (res != nullptr) {
        return res->Width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t ResourceGetTexWidthByCrc(uint64_t crc) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(crc));

    if (res != nullptr) {
        return res->Width;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t ResourceGetTexHeightByName(const char* name) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(name));

    if (res != nullptr) {
        return res->Height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

uint16_t ResourceGetTexHeightByCrc(uint64_t crc) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(crc));

    if (res != nullptr) {
        return res->Height;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

size_t ResourceGetTexSizeByName(const char* name) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(name));

    if (res != nullptr) {
        return res->ImageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

size_t ResourceGetTexSizeByCrc(uint64_t crc) {
    const auto res = std::static_pointer_cast<Fast::Texture>(ResourceLoad(crc));

    if (res != nullptr) {
        return res->ImageDataSize;
    }

    SPDLOG_ERROR("Given texture path is a non-existent resource");
    return -1;
}

void ResourceGetGameVersions(uint32_t* versions, size_t versionsSize, size_t* versionsCount) {
    auto list = Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions();
    memcpy(versions, list.data(), std::min(versionsSize, list.size() * sizeof(uint32_t)));
    *versionsCount = list.size();
}

void ResourceLoadDirectoryAsync(const char* name) {
    Ship::Context::GetRawInstance()->GetResourceManager()->LoadResourcesAsync(name);
}

uint32_t ResourceHasGameVersion(uint32_t hash) {
    auto list = Ship::Context::GetRawInstance()->GetResourceManager()->GetArchiveManager()->GetGameVersions();
    return std::find(list.begin(), list.end(), hash) != list.end();
}

void ResourceLoadDirectory(const char* name) {
    Ship::Context::GetRawInstance()->GetResourceManager()->LoadResources(name);
}

void ResourceDirtyDirectory(const char* name) {
    Ship::Context::GetRawInstance()->GetResourceManager()->DirtyResources(name);
}

void ResourceDirtyByName(const char* name) {
    auto resource = ResourceLoad(name);

    if (resource != nullptr) {
        resource->Dirty();
    }
}

void ResourceDirtyByCrc(uint64_t crc) {
    auto resource = ResourceLoad(crc);

    if (resource != nullptr) {
        resource->Dirty();
    }
}

void ResourceUnloadByName(const char* name) {
    Ship::Context::GetRawInstance()->GetResourceManager()->UnloadResource(name);
}

void ResourceUnloadByCrc(uint64_t crc) {
    ResourceUnloadByName(ResourceGetNameByCrc(crc));
}

void ResourceUnloadDirectory(const char* name) {
    Ship::Context::GetRawInstance()->GetResourceManager()->UnloadResources(name);
}

uint32_t IsResourceManagerLoaded() {
    return Ship::Context::GetRawInstance()->GetResourceManager()->IsLoaded();
}
}
