// Optional read-only archive-test bridge for hidden-symbol native StormLib builds.
#include <StormLib.h>
#define SOUL_ARCHIVE_EXPORT extern "C" __attribute__((visibility("default")))
SOUL_ARCHIVE_EXPORT bool Soul_SFileOpenArchive(const char* path, DWORD priority, DWORD, HANDLE* out) {
    return SFileOpenArchive(path, priority, MPQ_OPEN_READ_ONLY, out);
}
SOUL_ARCHIVE_EXPORT bool Soul_SFileOpenFileEx(HANDLE archive, const char* path, DWORD flags, HANDLE* out) {
    return SFileOpenFileEx(archive, path, flags, out);
}
SOUL_ARCHIVE_EXPORT DWORD Soul_SFileGetFileSize(HANDLE file, LPDWORD high) { return SFileGetFileSize(file, high); }
SOUL_ARCHIVE_EXPORT bool Soul_SFileReadFile(HANDLE file, void* data, DWORD size, LPDWORD read, LPOVERLAPPED overlap) {
    return SFileReadFile(file, data, size, read, overlap);
}
SOUL_ARCHIVE_EXPORT bool Soul_SFileCloseFile(HANDLE file) { return SFileCloseFile(file); }
SOUL_ARCHIVE_EXPORT bool Soul_SFileCloseArchive(HANDLE archive) { return SFileCloseArchive(archive); }
