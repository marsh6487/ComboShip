#pragma once

#ifdef COMBO_BUILD
#include <ship/resource/ResourceManager.h>
#include <ship/resource/archive/Archive.h>
#include <ship/resource/archive/ArchiveManager.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace SOH::Unbound {
namespace BuiltinTempleSagesDetail {
inline constexpr char kIndex[] = "custom/temple_sages/builtin_index.json";

inline std::string Basename(const std::string& path) {
    const auto slash = path.find_last_of("/\\");
    return path.substr(slash == std::string::npos ? 0 : slash + 1);
}

inline bool OwnedPath(std::string_view path) {
    if (path.empty() || path.size() > 1024 || path.back() == '/') {
        return false;
    }
    for (const unsigned char value : path) {
        if (value < 32 || value == 127 || value == '\\' || value == ':' || value == '*' || value == '?') {
            return false;
        }
    }
    if (path.starts_with("alt/")) {
        path.remove_prefix(4);
    }
    if (path == kIndex ||
        !(path.starts_with("custom/temple_sages/") || path.starts_with("scenes/shared/tokinoma_scene/") ||
          path.starts_with("scenes/tokinoma/"))) {
        return false;
    }
    while (!path.empty()) {
        const auto slash = path.find('/');
        const auto part = path.substr(0, slash);
        if (part.empty() || part == "." || part == "..") {
            return false;
        }
        if (slash == std::string_view::npos) {
            break;
        }
        path.remove_prefix(slash + 1);
    }
    return true;
}

// A read-only view of exact port-owned paths. Its parent stays alive, keeps its
// original priority, and is never closed or unloaded by this view.
class Layer final : public Ship::Archive {
  public:
    Layer(std::shared_ptr<Ship::Archive> source, std::unordered_set<std::string> paths, uint32_t validationVersion)
        : Archive(source->GetPath()), source(std::move(source)), paths(std::move(paths)),
          validationVersion(validationVersion) {
    }

    bool Open() override {
        if (!source->IsLoaded()) {
            return false;
        }
        // AddArchive checks GetGameVersion even when HasGameVersion is false.
        // Borrow an admitted value without publishing another ROM identity.
        SetGameVersion(validationVersion);
        for (const auto& path : paths) {
            IndexFile(path);
        }
        return true;
    }

    bool Close() override {
        SetLoaded(false);
        return true;
    }

    bool WriteFile(const std::string&, const std::vector<uint8_t>&) override {
        return false;
    }

    std::shared_ptr<Ship::File> LoadFile(const std::string& path) override {
        return IsLoaded() && paths.contains(path) ? source->LoadFile(path) : nullptr;
    }

    std::shared_ptr<Ship::File> LoadFile(uint64_t hash) override {
        const auto names = ListFiles();
        const auto found = names->find(hash);
        return found != names->end() ? LoadFile(found->second) : nullptr;
    }

    const std::shared_ptr<Ship::Archive>& Source() const {
        return source;
    }

  private:
    const std::shared_ptr<Ship::Archive> source;
    const std::unordered_set<std::string> paths;
    const uint32_t validationVersion;
};
} // namespace BuiltinTempleSagesDetail

// Call after clean base conversion/cache eviction and before optional mods.
// Use the supplied OoT owner throughout; the active Context may belong to MM.
inline bool MountBuiltinTempleSages(const std::shared_ptr<Ship::ResourceManager>& resources) {
    using namespace BuiltinTempleSagesDetail;
    if (!resources || !resources->GetArchiveManager()) {
        SPDLOG_WARN("[BuiltinTempleSages] no OoT archive owner; chamber layer skipped");
        return false;
    }
    auto archives = resources->GetArchiveManager();
    auto mounted = archives->GetArchives();
    std::shared_ptr<Ship::Archive> source;
    for (const auto& archive : *mounted) {
        // The first normal port archive is the boot source. Never read the
        // highest-priority index or borrow a same-named archive from a mod.
        if (archive && Basename(archive->GetPath()) == "soh.o2r") {
            source = archive;
            break;
        }
    }
    if (!source) {
        SPDLOG_INFO("[BuiltinTempleSages] normal soh.o2r is absent; chamber layer skipped");
        return false;
    }
    for (const auto& archive : *mounted) {
        auto layer = std::dynamic_pointer_cast<Layer>(archive);
        if (layer && layer->Source() == source) {
            return layer->IsLoaded();
        }
    }
    try {
        auto file = source->LoadFile(kIndex);
        if (!file) {
            SPDLOG_INFO("[BuiltinTempleSages] {} has no built-in index; older port keeps its normal scene assets",
                        source->GetPath());
            return false;
        }
        if (!file->IsLoaded || !file->Buffer || file->Buffer->size() > 4 * 1024 * 1024) {
            throw std::runtime_error("unreadable or oversized index");
        }
        auto index = nlohmann::json::parse(file->Buffer->begin(), file->Buffer->end());
        if (!index.is_object() || !index.contains("version") || !index["version"].is_number_integer() ||
            index["version"] != 1 || !index.contains("resources") || !index["resources"].is_array() ||
            index["resources"].empty() || index["resources"].size() > 65536) {
            throw std::runtime_error("unsupported index schema");
        }
        std::unordered_set<std::string> paths;
        for (const auto& entry : index["resources"]) {
            if (!entry.is_string()) {
                throw std::runtime_error("non-string resource path");
            }
            auto path = entry.get<std::string>();
            if (!OwnedPath(path) || !paths.insert(path).second) {
                throw std::runtime_error("invalid or duplicate resource path: " + path);
            }
            auto data = source->LoadFile(path);
            if (!data || !data->IsLoaded || !data->Buffer) {
                throw std::runtime_error("missing port resource: " + path);
            }
        }
        // A partial JSON control set would draw the annex with vanilla actor
        // or collision state. Both asset modes must enter the same full scene.
        for (const char* prefix : { "", "alt/" }) {
            for (const char* suffix : { "scene.json", "rooms/0.json", "collision.json", "collision.bin" }) {
                // Canonical scene patches retain the clean base's story and
                // spawn controls. A partial Alt scene would hide that base.
                if (prefix[0] && std::string_view(suffix) == "scene.json") {
                    continue;
                }
                if (!paths.contains(std::string(prefix) + "scenes/tokinoma/" + suffix)) {
                    throw std::runtime_error("incomplete native JSON controls");
                }
            }
        }
        uint32_t validationVersion = source->GetGameVersion();
        if (!archives->IsGameVersionValid(validationVersion)) {
            validationVersion = std::numeric_limits<uint32_t>::max();
            for (const auto version : archives->GetGameVersions()) {
                if (archives->IsGameVersionValid(version)) {
                    validationVersion = version;
                    break;
                }
            }
            if (!archives->IsGameVersionValid(validationVersion)) {
                throw std::runtime_error("no admitted archive version for built-in layer");
            }
        }
        auto layer = std::make_shared<Layer>(source, std::move(paths), validationVersion);
        layer->Load();
        if (!archives->AddArchive(layer)) {
            throw std::runtime_error("archive manager rejected built-in layer");
        }
        SPDLOG_INFO("[BuiltinTempleSages] mounted {} from {} ({} exact resources)", layer->GetPath(), source->GetPath(),
                    layer->ListFiles()->size());
        return true;
    } catch (const std::exception& error) {
        SPDLOG_WARN("[BuiltinTempleSages] {}: {}; chamber layer skipped", source->GetPath(), error.what());
        return false;
    }
}
} // namespace SOH::Unbound
#endif
