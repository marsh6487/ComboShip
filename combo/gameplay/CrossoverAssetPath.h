#ifndef COMBO_CROSSOVER_ASSET_PATH_H
#define COMBO_CROSSOVER_ASSET_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

// Resolve a loose Crossover asset using the active game's install/app directories.
// The result is valid until the next call on this thread.
const char* Nei_FindCrossoverAsset(const char* name);

#ifdef __cplusplus
}

#include <filesystem>
#include <string>

namespace Crossover {
inline std::string FindAsset(const std::string& name, const std::string& gameDirectory, bool findOotAnimation,
                             const auto& locate) {
    auto find = [&](const std::string& relativePath) -> std::string {
        std::filesystem::path path = locate(relativePath);
        std::error_code error;
        return std::filesystem::is_regular_file(path, error) ? path.make_preferred().string() : std::string{};
    };
    if (std::string path = find(gameDirectory + "/" + name); !path.empty()) {
        return path;
    }
    if (gameDirectory != "nei") {
        if (std::string path = find("nei/" + name); !path.empty()) {
            return path;
        }
    }
    if (findOotAnimation && name == "pikachu_anims.bin") {
        if (std::string path = find("nei/soh/" + name); !path.empty()) {
            return path;
        }
    }
    return find(name);
}
} // namespace Crossover
#endif

#endif
