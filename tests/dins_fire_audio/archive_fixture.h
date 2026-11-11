#ifndef DINS_FIRE_AUDIO_ARCHIVE_FIXTURE_H
#define DINS_FIRE_AUDIO_ARCHIVE_FIXTURE_H
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Real LUS object types/registry; controlled ArchiveManager raw-file boundary.
namespace DinsAudioFixture {
// Fresh managers, empty mounts/counts; active owner defaults to OoT.
void Reset();
void Mount(bool mm, const std::string& path, const std::vector<uint8_t>& bytes);
void Remove(bool mm, const std::string& path);
void SetAvailable(bool mm, bool available);
void SetActive(bool mm);
// Fresh RM/ArchiveManager identity, preserving mounts; read count resets.
void ReplaceOwner(bool mm);
size_t ReadCount(bool mm);
// Resource calls assert if an audio callback has raised this thread-local flag.
void SetAudioThread(bool audioThread);
}
#endif
