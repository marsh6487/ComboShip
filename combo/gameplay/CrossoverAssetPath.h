#ifndef COMBO_CROSSOVER_ASSET_PATH_H
#define COMBO_CROSSOVER_ASSET_PATH_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Resolve a loose Crossover asset using the active game's install/app directories.
// Paths use UTF-8, including on Windows.
// The result is valid until the next call on this thread.
const char* Nei_FindCrossoverAsset(const char* name);
FILE* Nei_OpenCrossoverFile(const char* path);

#ifdef __cplusplus
}

#include <filesystem>
#include <string>
#include <string_view>

namespace Crossover {
inline std::filesystem::path Utf8Path(std::string_view path) {
    return std::filesystem::path(std::u8string(path.begin(), path.end()));
}

inline std::string FindAsset(const std::string& name, const std::string& gameDirectory, bool findOotAnimation,
                             const std::string& appDirectory, const std::string& installDirectory) {
    const std::filesystem::path roots[] = { Utf8Path(appDirectory), Utf8Path(installDirectory), "." };
    auto find = [&](const std::string& relativePath) -> std::string {
        for (const auto& root : roots) {
            auto path = root / Utf8Path(relativePath);
            std::error_code error;
            if (std::filesystem::is_regular_file(path, error)) {
                const auto utf8 = path.make_preferred().u8string();
                return { utf8.begin(), utf8.end() };
            }
        }
        return {};
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

inline FILE* OpenFile(const char* path) {
#ifdef _WIN32
    return _wfopen(Utf8Path(path).c_str(), L"rb");
#else
    return fopen(path, "rb");
#endif
}
} // namespace Crossover
#endif

#endif
