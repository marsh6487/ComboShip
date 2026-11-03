#ifndef COMBO_VANILLA_FILE_H
#define COMBO_VANILLA_FILE_H

#include <cstdio>
#include <filesystem>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace ComboVanilla {
/* Replace only after a complete, flushed temporary write. On either platform,
 * failed replacement retains the previous file; never remove it to retry. */
inline bool WriteFile(const std::filesystem::path& path, const std::string& contents) {
    auto temporary = path;
    temporary += ".temp";
#ifdef _WIN32
    FILE* file = _wfopen(temporary.c_str(), L"wb");
#else
    FILE* file = std::fopen(temporary.c_str(), "wb");
#endif
    if (!file)
        return false;
    bool written = std::fwrite(contents.data(), 1, contents.size(), file) == contents.size() && std::fflush(file) == 0;
#ifdef _WIN32
    written = written && _commit(_fileno(file)) == 0;
#else
    written = written && fsync(fileno(file)) == 0;
#endif
    written = std::fclose(file) == 0 && written;
    if (!written)
        return false;
#ifdef _WIN32
    return MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return std::rename(temporary.c_str(), path.c_str()) == 0;
#endif
}
} // namespace ComboVanilla

#endif
