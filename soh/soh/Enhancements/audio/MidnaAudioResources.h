#pragma once

#include <cstdint>
#include <vector>

namespace MidnaAudioResources {
bool Enabled();
bool HasModel();
bool ReadClip(const char* path, std::vector<uint8_t>& bytes);
float Gain();
} // namespace MidnaAudioResources
