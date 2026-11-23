#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>

namespace ComboOptionalSfx {

// Each cue group has one voice. Camera and companion speech can overlap.
template <size_t ClipCount, size_t VoiceCount> class Mixer {
  public:
    using Bank = std::array<std::vector<int16_t>, ClipCount>;

    void SetBank(Bank bank) {
        std::lock_guard lock(mMutex);
        mBank = std::move(bank);
        mVoices = {};
    }

    bool Play(size_t clip, size_t voice) {
        std::lock_guard lock(mMutex);
        if (mBank[clip].empty()) {
            return false;
        }
        mVoices[voice] = { &mBank[clip], 0 };
        return true;
    }

    void Mix(int16_t* output, size_t frames, float gain) {
        std::lock_guard lock(mMutex);
        for (auto& voice : mVoices) {
            if (voice.clip == nullptr) {
                continue;
            }
            const size_t count = std::min(frames, voice.clip->size() - voice.position);
            for (size_t i = 0; i < count; ++i) {
                const int32_t sample = static_cast<int32_t>((*voice.clip)[voice.position++] * gain);
                for (size_t channel = 0; channel < 2; ++channel) {
                    const size_t index = i * 2 + channel;
                    output[index] = static_cast<int16_t>(std::clamp(output[index] + sample, -32768, 32767));
                }
            }
            if (voice.position == voice.clip->size()) {
                voice = {};
            }
        }
    }

    void Stop(size_t voice) {
        std::lock_guard lock(mMutex);
        mVoices[voice] = {};
    }

    void Reset() {
        std::lock_guard lock(mMutex);
        mVoices = {};
    }

  private:
    struct Voice {
        const std::vector<int16_t>* clip = nullptr;
        size_t position = 0;
    };

    std::mutex mMutex;
    Bank mBank;
    std::array<Voice, VoiceCount> mVoices{};
};

} // namespace ComboOptionalSfx
