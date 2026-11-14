#ifndef COMBO_AUDIO_DINS_FIRE_SFX_SAMPLES_H
#define COMBO_AUDIO_DINS_FIRE_SFX_SAMPLES_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace CwDinsAudio {

enum class Cue : size_t { Charge, Contact, Release, Loop, Burnout, Count };
inline constexpr size_t kCueCount = static_cast<size_t>(Cue::Count);
inline constexpr uint32_t kSampleRate = 32000;
inline constexpr size_t kMaxOverrideSamples = kSampleRate * 10;
inline constexpr size_t kMaxOverrideFileBytes = kMaxOverrideSamples * sizeof(int16_t) + 65536;
inline constexpr std::array<const char*, kCueCount> kOverridePaths = {
    "audio/dins_fire_remake/charge.wav", "audio/dins_fire_remake/contact.wav", "audio/dins_fire_remake/release.wav",
    "audio/dins_fire_remake/loop.wav",   "audio/dins_fire_remake/burnout.wav",
};

// Built on the game thread, then published as immutable for one cast. Empty
// entries select the corresponding compiled fallback, including partial mods.
struct SampleBank {
    std::array<std::vector<int16_t>, kCueCount> clips;
};

} // namespace CwDinsAudio
#endif
