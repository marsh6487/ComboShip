#pragma once

#include <stdbool.h>

#ifdef __cplusplus
#include <cstdint>
#include <vector>

namespace MMMidnaAudioResources {
bool Enabled();
bool HasModel();
bool ReadClip(const char* path, std::vector<uint8_t>& bytes);
float Gain();
} // namespace MMMidnaAudioResources

extern "C" {
#endif

// Private assets always belong to MM's manager, including dormant cross-game draws.
bool MMMidnaResources_Exists(const char* path);
void* MMMidnaResources_Load(const char* path);

#ifdef __cplusplus
}
#endif
