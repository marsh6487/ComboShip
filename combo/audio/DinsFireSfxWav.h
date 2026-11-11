#ifndef COMBO_AUDIO_DINS_FIRE_SFX_WAV_H
#define COMBO_AUDIO_DINS_FIRE_SFX_WAV_H

#include "DinsFireSfxSamples.h"
#include <cstring>
#include <span>

namespace CwDinsAudio {

inline uint16_t WavRead16(const uint8_t* data) {
    return uint16_t(data[0]) | (uint16_t(data[1]) << 8);
}

inline uint32_t WavRead32(const uint8_t* data) {
    return uint32_t(WavRead16(data)) | (uint32_t(WavRead16(data + 2)) << 16);
}

// Optional raw WAV files, not typed AudioSample resources. Empty means invalid;
// a nonempty all-zero result is a valid authored silent override. Decode only
// on the game thread before publishing immutable storage to the mixer.
inline std::vector<int16_t> DecodeOverrideWav(std::span<const uint8_t> bytes) {
    if (bytes.size() < 12 || bytes.size() > kMaxOverrideFileBytes || std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
        std::memcmp(bytes.data() + 8, "WAVE", 4) != 0 || uint64_t(WavRead32(bytes.data() + 4)) + 8 != bytes.size()) {
        return {};
    }

    bool haveFormat = false;
    const uint8_t* pcm = nullptr;
    size_t pcmBytes = 0;
    size_t offset = 12;
    while (offset < bytes.size()) {
        if (bytes.size() - offset < 8) {
            return {};
        }
        const uint8_t* header = bytes.data() + offset;
        const size_t length = WavRead32(header + 4);
        offset += 8;
        if (length > bytes.size() - offset) {
            return {};
        }
        const uint8_t* data = bytes.data() + offset;
        if (std::memcmp(header, "fmt ", 4) == 0) {
            // PCM fmt16 and bounded extended PCM with cbSize=0 are supported;
            // WAVE_FORMAT_EXTENSIBLE, float, stereo and resampling are not.
            if (haveFormat || length < 16 || length == 17 || WavRead16(data) != 1 || WavRead16(data + 2) != 1 ||
                WavRead32(data + 4) != kSampleRate || WavRead32(data + 8) != kSampleRate * sizeof(int16_t) ||
                WavRead16(data + 12) != sizeof(int16_t) || WavRead16(data + 14) != 16 ||
                (length >= 18 && WavRead16(data + 16) != 0)) {
                return {};
            }
            haveFormat = true;
        } else if (std::memcmp(header, "data", 4) == 0) {
            if (pcm != nullptr || length == 0 || length % sizeof(int16_t) != 0 ||
                length / sizeof(int16_t) > kMaxOverrideSamples) {
                return {};
            }
            pcm = data;
            pcmBytes = length;
        }
        offset += length;
        if (length % 2 != 0) {
            if (offset == bytes.size()) {
                return {};
            }
            ++offset; // RIFF pad byte is present; its value is unspecified.
        }
    }
    if (!haveFormat || pcm == nullptr) {
        return {};
    }

    std::vector<int16_t> samples(pcmBytes / sizeof(int16_t));
    for (size_t index = 0; index < samples.size(); ++index) {
        const int32_t raw = WavRead16(pcm + index * sizeof(int16_t));
        samples[index] = static_cast<int16_t>(raw >= 32768 ? raw - 65536 : raw);
    }
    return samples;
}

} // namespace CwDinsAudio
#endif
